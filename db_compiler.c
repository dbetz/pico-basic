/* db_compiler.c - a simple basic compiler
 *
 * Copyright (c) 2009 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include "db_compiler.h"
#include "db_vmdebug.h"
#include "db_vm.h"

static DATA_SPACE uint8_t heap_space[4096];

DefIntrinsic(abs);
DefIntrinsic(rnd);
DefIntrinsic(left);
DefIntrinsic(right);
DefIntrinsic(mid);
DefIntrinsic(chr);
DefIntrinsic(str);
DefIntrinsic(val);
DefIntrinsic(asc);
DefIntrinsic(len);
DefIntrinsic(printStr);
DefIntrinsic(printInt);
DefIntrinsic(printTab);
DefIntrinsic(printNL);
DefIntrinsic(printFlush);

/* InitCompiler - initialize the compiler and create a parse context */
ParseContext *InitCompiler(System *sys, int maxObjects)
{
    ParseContext *c;

    /* allocate and initialize the parse context */
    if (!(c = (ParseContext *)AllocateFreeSpace(sys, sizeof(ParseContext))))
        return NULL;
    memset(c, 0, sizeof(ParseContext));
    
    /* initialize free space */
    c->sys = sys;
    c->freeMark = sys->freeNext;
    
    c->heap = InitHeap(heap_space, sizeof(heap_space), 128);
        
    /* initialize the common types */
    InitCommonType(c, integerType, TYPE_INTEGER);
    InitCommonType(c, integerArrayType, TYPE_ARRAY);
    c->integerArrayType.type.u.arrayInfo.elementType = CommonType(c, integerType);
    InitCommonType(c, byteType, TYPE_BYTE);
    InitCommonType(c, byteArrayType, TYPE_ARRAY);
    c->byteArrayType.type.u.arrayInfo.elementType = CommonType(c, byteType);
    InitCommonType(c, stringType,TYPE_STRING);
    InitCommonType(c, stringArrayType, TYPE_ARRAY);
    c->stringArrayType.type.u.arrayInfo.elementType = CommonType(c, stringType);

    /* initialize the global symbol table */
    InitSymbolTable(&c->globals);

    /* add the intrinsic functions */
    AddIntrinsic(c, "ABS",          abs,        "i=i")
    AddIntrinsic(c, "RND",          rnd,        "i=i")
    AddIntrinsic(c, "LEFT$",        left,       "s=si")
    AddIntrinsic(c, "RIGHT$",       right,      "s=si")
    AddIntrinsic(c, "MID$",         mid,        "s=sii")
    AddIntrinsic(c, "CHR$",         chr,        "s=i")
    AddIntrinsic(c, "STR$",         str,        "s=i")
    AddIntrinsic(c, "VAL",          val,        "i=s")
    AddIntrinsic(c, "ASC",          asc,        "i=s")
    AddIntrinsic(c, "LEN",          len,        "i=s")
    AddIntrinsic(c, "printStr",     printStr,   "is")
    AddIntrinsic(c, "printInt",     printInt,   "ii")
    AddIntrinsic(c, "printTab",     printTab,   "i")
    AddIntrinsic(c, "printNL",      printNL,    "i")
    AddIntrinsic(c, "printFlush",   printFlush, "i")

    /* return the new parse context */
    return c;
}

/* Compile - compile a program */
VMHANDLE Compile(ParseContext *c, int oneStatement)
{
    System *sys = c->sys;
    int prompt = VMFALSE;
    
    /* setup an error target */
    if (setjmp(c->errorTarget) != 0)
        return NULL;

    /* use the rest of the free space for the compiler heap */
    c->nextLocal = sys->freeNext;
    c->heapSize = sys->freeTop - sys->freeNext;

    /* initialize block nesting table */
    c->btop = (Block *)((char *)c->blockBuf + sizeof(c->blockBuf));
    c->bptr = c->blockBuf - 1;

    /* initialize the code staging buffer */
    c->ctop = c->codeBuf + sizeof(c->codeBuf);
    c->cptr = c->codeBuf;

    /* initialize the scanner */
    c->savedToken = 0;
    c->labels = NULL;

    /* start in the main code */
    c->codeType = CODE_TYPE_MAIN;

    /* initialize scanner */
    c->inComment = VMFALSE;
    
    /* get the next line */
    do {
        Token tkn;
        
        /* display a prompt on continuation lines for immediate mode */
        if (oneStatement && prompt)
            VM_printf("  > ");
            
        /* get the next line */
        if (!GetLine(c))
            return NULL;
            
        /* prompt on continuation lines */
        prompt = VMTRUE;
            
        /* parse the statement */
        if ((tkn = GetToken(c)) != T_EOL)
            ParseStatement(c, tkn);
            
    } while (!oneStatement || c->codeType == CODE_TYPE_FUNCTION || c->bptr >= c->blockBuf);
        
    /* end the main code with a halt */
    putcbyte(c, OP_HALT);
    
    /* write the main code */
    InitSymbolTable(&c->arguments);
    StartCode(c, "main", CODE_TYPE_MAIN);
    StoreCode(c);

    /* free up the space the compiler was consuming */
    sys->freeNext = c->freeMark;

    /* return the main code object */
    return c->code;
}

/* StartCode - start a function under construction */
void StartCode(ParseContext *c, char *name, CodeType type)
{
    /* all functions must precede or follow the main code */
    if (type != CODE_TYPE_MAIN && c->cptr > c->codeBuf)
        ParseError(c, "functions must precede or follow the main code", NULL);

    /* don't allow nested functions (for now anyway) */
    if (type != CODE_TYPE_MAIN && c->codeType != CODE_TYPE_MAIN)
        ParseError(c, "nested functions are not supported", NULL);

    /* initialize the code object under construction */
    c->codeName = name;
    c->argumentCount = c->handleArgumentCount = 0;
    InitSymbolTable(&c->locals);
    c->code = NewCode(c->heap, 0);
    c->localOffset = -F_SIZE - 1;
    c->handleLocalOffset = 1;
    c->codeType = type;
    
    /* write the code prolog */
    if (type != CODE_TYPE_MAIN) {
        putcbyte(c, OP_RESERVE);
        putcbyte(c, 0);
        putcbyte(c, 0);
    }
}

/* StoreCode - store the function or method under construction */
void StoreCode(ParseContext *c)
{
    int codeSize;

    /* check for unterminated blocks */
    switch (CurrentBlockType(c)) {
    case BLOCK_IF:
    case BLOCK_ELSE:
        ParseError(c, "expecting END IF", NULL);
    case BLOCK_FOR:
        ParseError(c, "expecting NEXT", NULL);
    case BLOCK_DO:
        ParseError(c, "expecting LOOP", NULL);
    case BLOCK_NONE:
        break;
    }

    /* fixup the RESERVE instruction at the start of the code */
    if (c->codeType != CODE_TYPE_MAIN) {
        c->codeBuf[1] = (-F_SIZE - 1) - c->localOffset;
        c->codeBuf[2] = c->handleLocalOffset - 1;
        if (c->returnFixups == codeaddr(c) - sizeof(VMVALUE)) {
            c->returnFixups = rd_cword(c, c->returnFixups);
            c->cptr -= sizeof(VMVALUE) + 1;
        }
        fixupbranch(c, c->returnFixups, codeaddr(c));
        if (c->codeType == CODE_TYPE_FUNCTION)
            putcbyte(c, OP_RETURN); // BUG: need to check for OP_RETURNH
        else
            putcbyte(c, OP_RETURNV);
        putcbyte(c, c->argumentCount);
        putcbyte(c, c->handleArgumentCount);
    }

    /* make sure all referenced labels were defined */
    CheckLabels(c);

    /* determine the code size */
    codeSize = (int)(c->cptr - c->codeBuf);

#if 1
    VM_printf("%s:\n", c->codeName);
    DecodeFunction(0, c->codeBuf, codeSize);
    DumpLocals(c);
    VM_putchar('\n');
#endif

    /* store the vector object */
    StoreByteVectorData(c->heap, c->code, c->codeBuf, codeSize);

    /* empty the local heap */
    c->nextLocal = c->sys->freeNext;
    InitSymbolTable(&c->arguments);
    InitSymbolTable(&c->locals);
    c->labels = NULL;

    /* reset to compile the next code */
    c->codeType = CODE_TYPE_MAIN;
    c->cptr = c->codeBuf;
}

/* AddIntrinsic1 - add an intrinsic function to the global symbol table */
void AddIntrinsic1(ParseContext *c, char *name, char *types, VMHANDLE handler)
{
    int argumentCount, handleArgumentCount;
    VMHANDLE symbol, type, argType;
    Type *typ;
    
    /* make the function type */
    type = NewType(c->heap, TYPE_FUNCTION);
    typ = GetTypePtr(type);
    InitSymbolTable(&c->arguments);
    
    /* set the return type */
    switch (*types++) {
    case 'i':
        typ->u.functionInfo.returnType = CommonType(c, integerType);
        break;
    case 's':
        typ->u.functionInfo.returnType = CommonType(c, stringType);
        break;
    default:
        Fatal(c, "unknown return type");
        break;
    }
    
    /* initialize the argument counts */
    argumentCount = handleArgumentCount = 0;
    
    /* add the argument types */
    if (*types++ == '=') {
        while (*types) {
            switch (*types++) {
            case 'i':
                argType = CommonType(c, integerType);
                ++argumentCount;
                break;
            case 's':
                argType = CommonType(c, stringType);
                ++handleArgumentCount;
                break;
            default:
                Fatal(c, "unknown argument type");
                break;
            }
            AddArgument(c, "", argType, 0);
        }
    }

    /* add a global symbol for the intrinsic function */
    symbol = AddGlobal(c, name, SC_CONSTANT, type);
    GetSymbolPtr(symbol)->v.hValue = handler;

    /* store the argument symbol table */
    typ = GetTypePtr(type);
    typ->u.functionInfo.arguments = c->arguments;
}

/* LocalAlloc - allocate memory from the local heap */
void *LocalAlloc(ParseContext *c, size_t size)
{
    System *sys = c->sys;
    void *p;
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (c->nextLocal + size > sys->freeTop)
        Fatal(c, "insufficient memory");
    p = c->nextLocal;
    c->nextLocal += size;
    return p;
}

/* VM_printf - formatted print */
void VM_printf(const char *fmt, ...)
{
    char buf[100], *p = buf;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
    va_end(ap);
}

/* Fatal - report a fatal error and exit */
void Fatal(ParseContext *c, const char *fmt, ...)
{
    char buf[100], *p = buf;
    va_list ap;
    va_start(ap, fmt);
    VM_printf("error: ");
    vsnprintf(buf, sizeof(buf), fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
    VM_putchar('\n');
    va_end(ap);
    longjmp(c->errorTarget, 1);
}
