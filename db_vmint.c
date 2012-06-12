/* db_vmint.c - bytecode interpreter for a simple virtual machine
 *
 * Copyright (c) 2009-2012 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "db_vm.h"

/* prototypes for local functions */
static void StartCode(Interpreter *i, VMHANDLE object);
static void PopFrame(Interpreter *i);
static void StringCat(Interpreter *i);

/* InitInterpreter - initialize the interpreter */
uint8_t *InitInterpreter(Interpreter *i, ObjHeap *heap, size_t stackSize)
{
    i->heap = heap;
    i->stack = (VMVALUE *)((uint8_t *)i + sizeof(Interpreter));
    i->stackTop = i->stack + stackSize;
    return (uint8_t *)i->stackTop;
}

/* Execute - execute the main code */
int Execute(Interpreter *i, VMHANDLE main)
{
    VMVALUE tmp, tmp2, ind;
    VMHANDLE obj, htmp;
    int8_t tmpb;

    /* initialize */
    i->code = main;
    i->cbase = i->pc = GetCodePtr(main);
    i->sp = i->fp = i->stackTop;
    i->hsp = i->hfp = (VMHANDLE *)i->stack - 1;

    if (setjmp(i->errorTarget))
        return VMFALSE;

    for (;;) {
#if 1
        ShowStack(i);
        DecodeInstruction(0, 0, i->pc);
#endif
        switch (VMCODEBYTE(i->pc++)) {
        case OP_HALT:
            return VMTRUE;
        case OP_BRT:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            if (Pop(i))
                i->pc += tmp;
            break;
        case OP_BRTSC:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            if (*i->sp)
                i->pc += tmp;
            else
                Drop(i, 1);
            break;
        case OP_BRF:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            if (!Pop(i))
                i->pc += tmp;
            break;
        case OP_BRFSC:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            if (!*i->sp)
                i->pc += tmp;
            else
                Drop(i, 1);
            break;
        case OP_BR:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            i->pc += tmp;
            break;
        case OP_NOT:
            *i->sp = (*i->sp ? VMFALSE : VMTRUE);
            break;
        case OP_NEG:
            *i->sp = -*i->sp;
            break;
        case OP_ADD:
            tmp = Pop(i);
            *i->sp += tmp;
            break;
        case OP_SUB:
            tmp = Pop(i);
            *i->sp -= tmp;
            break;
        case OP_MUL:
            tmp = Pop(i);
            *i->sp *= tmp;
            break;
        case OP_DIV:
            tmp = Pop(i);
            *i->sp = (tmp == 0 ? 0 : *i->sp / tmp);
            break;
        case OP_REM:
            tmp = Pop(i);
            *i->sp = (tmp == 0 ? 0 : *i->sp % tmp);
            break;
        case OP_CAT:
            StringCat(i);
            break;
        case OP_BNOT:
            *i->sp = ~*i->sp;
            break;
        case OP_BAND:
            tmp = Pop(i);
            *i->sp &= tmp;
            break;
        case OP_BOR:
            tmp = Pop(i);
            *i->sp |= tmp;
            break;
        case OP_BXOR:
            tmp = Pop(i);
            *i->sp ^= tmp;
            break;
        case OP_SHL:
            tmp = Pop(i);
            *i->sp <<= tmp;
            break;
        case OP_SHR:
            tmp = Pop(i);
            *i->sp >>= tmp;
            break;
        case OP_LT:
            tmp = Pop(i);
            *i->sp = (*i->sp < tmp ? VMTRUE : VMFALSE);
            break;
        case OP_LE:
            tmp = Pop(i);
            *i->sp = (*i->sp <= tmp ? VMTRUE : VMFALSE);
            break;
        case OP_EQ:
            tmp = Pop(i);
            *i->sp = (*i->sp == tmp ? VMTRUE : VMFALSE);
            break;
        case OP_NE:
            tmp = Pop(i);
            *i->sp = (*i->sp != tmp ? VMTRUE : VMFALSE);
            break;
        case OP_GE:
            tmp = Pop(i);
            *i->sp = (*i->sp >= tmp ? VMTRUE : VMFALSE);
            break;
        case OP_GT:
            tmp = Pop(i);
            *i->sp = (*i->sp > tmp ? VMTRUE : VMFALSE);
            break;
        case OP_LIT:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            CPush(i, tmp);
            break;
        case OP_GREF:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            obj = (VMHANDLE)tmp;
            CPush(i, GetSymbolPtr(obj)->v.iValue);
            break;
        case OP_GSET:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            obj = (VMHANDLE)tmp;
            GetSymbolPtr(obj)->v.iValue = Pop(i);
            break;
        case OP_LREF:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            CPush(i, i->fp[(int)tmpb]);
            break;
        case OP_LSET:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            i->fp[(int)tmpb] = Pop(i);
            break;
        case OP_VREF:
            ind = *i->sp;
            obj = *i->hsp;
            if (ind < 0 || ind >= GetHeapObjSize(obj))
                Abort(i, str_array_subscript_err, ind);
            *i->sp = GetIntegerVectorBase(obj)[ind];
            DropH(i, 1);
            break;
        case OP_VSET:
            tmp2 = Pop(i);
            ind = Pop(i);
            obj = *i->hsp;
            if (ind < 0 || ind >= GetHeapObjSize(obj))
                Abort(i, str_array_subscript_err, ind);
            GetIntegerVectorBase(obj)[ind] = tmp2;
            DropH(i, 1);
            break;
       case OP_LITH:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            CPushH(i, (VMHANDLE)tmp);
            break;
        case OP_GREFH:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            CPushH(i, GetSymbolPtr((VMHANDLE)tmp)->v.hValue);
            break;
        case OP_GSETH:
            get_VMVALUE(tmp, VMCODEBYTE(i->pc++));
            GetSymbolPtr((VMHANDLE)tmp)->v.hValue = PopH(i);
            break;
        case OP_LREFH:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            CPushH(i, i->hfp[(int)tmpb]);
            break;
        case OP_LSETH:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            i->hfp[(int)tmpb] = PopH(i);
            break;
        case OP_VREFH:
            ind = Pop(i);
            obj = *i->hsp;
            if (ind < 0 || ind >= GetHeapObjSize(obj))
                Abort(i, str_array_subscript_err, ind);
            *i->hsp = GetStringVectorBase(obj)[ind];
            break;
        case OP_VSETH:
            htmp = PopH(i);
            ind = Pop(i);
            obj = *i->hsp;
            if (ind < 0 || ind >= GetHeapObjSize(obj))
                Abort(i, str_array_subscript_err, ind);
            GetStringVectorBase(obj)[ind] = htmp;
            DropH(i, 1);
            break;
        case OP_RESERVE:
            tmp = VMCODEBYTE(i->pc++);
            tmp2 = VMCODEBYTE(i->pc++);
            Reserve(i, tmp);
            ReserveH(i, tmp2);
            break;
         case OP_CALL:
            StartCode(i, *i->hsp);
            break;
        case OP_RETURN:
            tmp = *i->sp;
            PopFrame(i);
            Push(i, tmp);
            break;
        case OP_RETURNH:
            htmp = *i->hsp;
            PopFrame(i);
            PushH(i, htmp);
            break;
        case OP_RETURNV:
            PopFrame(i);
            break;
        case OP_DROP:
            Drop(i, 1);
            break;
        default:
            Abort(i, str_opcode_err, VMCODEBYTE(i->pc - 1));
            break;
        }
    }
}

static void StartCode(Interpreter *i, VMHANDLE object)
{
    VMVALUE tmp, tmp2;

    if (!object)
        Abort(i, str_not_code_object_err, object);
        
    switch (GetHeapObjType(object)) {
    case ObjTypeCode:
        tmp = (VMVALUE)(i->fp - i->stack);
        tmp2 = (VMVALUE)(i->hfp - (VMHANDLE *)i->stack);
        i->hfp = i->hsp;
        CPushH(i, i->code);
        i->fp = i->sp;
        Reserve(i, F_SIZE);
        i->fp[F_FP] = tmp;
        i->fp[F_HFP] = tmp2;
        i->fp[F_PC] = (VMVALUE)(i->pc - i->cbase);
        i->code = object;
        i->cbase = i->pc = GetCodePtr(object);
        break;
    case ObjTypeIntrinsic:
        (*GetIntrinsicHandler(object))(i);
        break;
    default:
        Abort(i, str_not_code_object_err, object);
        break;
    }
}

static void PopFrame(Interpreter *i)
{
    int argumentCount = VMCODEBYTE(i->pc++);
    int handleArgumentCount = VMCODEBYTE(i->pc++);
    i->code = i->hfp[HF_CODE];
    i->hsp = i->hfp;
    DropH(i, handleArgumentCount);
    i->cbase = GetCodePtr(i->code);
    i->pc = i->cbase + i->fp[F_PC];
    i->hfp = (VMHANDLE *)i->stack + i->fp[F_HFP];
    i->sp = i->fp;
    i->fp = i->stack + i->fp[F_FP];
    Drop(i, argumentCount);
}

static void StringCat(Interpreter *i)
{
    VMHANDLE hStr2 = PopH(i);
    VMHANDLE hStr1 = *i->hsp;
    uint8_t *str1, *str2, *str;
    size_t len1, len2;

    /* get the first string pointer and length */
    str1 = GetStringPtr(hStr1);
    len1 = GetHeapObjSize(hStr1);

    /* get the second string pointer and length */
    str2 = GetStringPtr(hStr2);
    len2 = GetHeapObjSize(hStr2);

    /* allocate the result string */
    *i->hsp = NewString(i->heap, len1 + len2);
    str = GetStringPtr((VMHANDLE)*i->sp);

    /* copy the source strings into the result string */
    memcpy(str, str1, len1);
    memcpy(str + len1, str2, len2);
}

void ShowStack(Interpreter *i)
{
    VMHANDLE *hp;
    VMVALUE *p;
    
    for (hp = (VMHANDLE *)i->stack; hp <= i->hsp; ++hp) {
        if (hp == i->hfp)
            VM_printf(" <hfp>");
        VM_printf(" %08x", (VMUVALUE)*hp);
    }
    
    VM_printf(" ---");
    
    for (p = i->sp; p < i->stackTop; ++p) {
        if (p == i->fp)
            VM_printf(" <fp>");
        VM_printf(" %d", *p);
    }
    
    VM_putchar('\n');
}

void StackOverflow(Interpreter *i)
{
    Abort(i, "stack overflow");
}

void Abort(Interpreter *i, const char *fmt, ...)
{
    char buf[100], *p = buf;
    va_list ap;
    va_start(ap, fmt);
    VM_printf(str_abort_prefix);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
    VM_putchar('\n');
    va_end(ap);
    if (i)
        longjmp(i->errorTarget, 1);
    else
        exit(1);
}
