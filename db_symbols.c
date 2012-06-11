/* db_symbols.c - symbol table routines
 *
 * Copyright (c) 2009 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db_compiler.h"

static VMHANDLE AddLocal1(ParseContext *c, SymbolTable *table, const char *name, VMHANDLE type, VMVALUE offset);
static VMHANDLE FindLocal1(SymbolTable *table, const char *name);

/* InitSymbolTable - initialize a symbol table */
void InitSymbolTable(SymbolTable *table)
{
    table->head = NULL;
    table->tail = NULL;
    table->count = 0;
}

/* AddGlobal - add a global symbol to the symbol table */
VMHANDLE AddGlobal(ParseContext *c, const char *name, StorageClass storageClass, VMHANDLE type)
{
    VMHANDLE symbol;
    
    /* allocate the symbol object */
    if (!(symbol = NewSymbol(c->heap, name, storageClass, type)))
        return NULL;
        
    /* add it to the symbol table */
    if (c->globals.tail == NULL)
        c->globals.head = c->globals.tail = symbol;
    else {
        Symbol *last = GetSymbolPtr(c->globals.tail);
        c->globals.tail = symbol;
        last->next = symbol;
    }
    ++c->globals.count;
    
    /* return the symbol */
    return symbol;
}

/* FindGlobal - find a symbol in the global symbol table */
VMHANDLE FindGlobal(ParseContext *c, const char *name)
{
    VMHANDLE symbol = c->globals.head;
    while (symbol) {
        Symbol *sym = GetSymbolPtr(symbol);
        if (strcasecmp(name, sym->name) == 0)
            return symbol;
        symbol = sym->next;
    }
    return NULL;
}

/* AddArgument - add a symbol to an argument symbol table */
VMHANDLE AddArgument(ParseContext *c, const char *name, VMHANDLE type, VMVALUE offset)
{
    return AddLocal1(c, &c->arguments, name, type, offset);
}

/* FindArgument - find an argument symbol in a symbol table */
VMHANDLE FindArgument(ParseContext *c, const char *name)
{
    return FindLocal1(&c->arguments, name);
}

/* AddLocal - add a symbol to a local symbol table */
VMHANDLE AddLocal(ParseContext *c, const char *name, VMHANDLE type, VMVALUE offset)
{
    return AddLocal1(c, &c->locals, name, type, offset);
}

/* FindLocal - find a local symbol in a symbol table */
VMHANDLE FindLocal(ParseContext *c, const char *name)
{
    return FindLocal1(&c->locals, name);
}

/* AddLocal1 - add a symbol to a local symbol table */
static VMHANDLE AddLocal1(ParseContext *c, SymbolTable *table, const char *name, VMHANDLE type, VMVALUE offset)
{
    VMHANDLE local;
    
    /* allocate the local symbol object */
    if (!(local = NewLocal(c->heap, name, type, offset)))
        return NULL;
        
    /* add it to the symbol table */
    if (table->tail == NULL)
        table->head = table->tail = local;
    else {
        Local *last = GetLocalPtr(table->tail);
        table->tail = local;
        last->next = local;
    }
    ++table->count;
    
    /* return the symbol */
    return local;
}

/* FindLocal1 - find a local symbol in a symbol table */
static VMHANDLE FindLocal1(SymbolTable *table, const char *name)
{
    VMHANDLE local = table->head;
    while (local) {
        Local *sym = GetLocalPtr(local);
        if (strcasecmp(name, sym->name) == 0)
            return local;
        local = sym->next;
    }
    return NULL;
}

/* IsConstant - check to see if the value of a symbol is a constant */
int IsConstant(Symbol *symbol)
{
    return symbol->storageClass == SC_CONSTANT;
}

/* DumpGlobals - dump the global symbol table */
void DumpGlobals(ParseContext *c)
{
    VMHANDLE symbol = c->globals.head;
    if (symbol) {
        VM_printf("Globals:\n");
        while (symbol) {
            Symbol *sym = GetSymbolPtr(symbol);
            VM_printf("  %s %04x\n", sym->name, sym->v.iValue);
            symbol = sym->next;
        }
    }
}

/* DumpLocals - dump a local symbol table */
void DumpLocals(ParseContext *c)
{
    VMHANDLE symbol;
    if ((symbol = c->arguments.head) != NULL) {
        VM_printf("Arguments:\n");
        while (symbol) {
            Local *sym = GetLocalPtr(symbol);
            VM_printf("  %s %d\n", sym->name, sym->offset);
            symbol = sym->next;
        }
    }
    if ((symbol = c->locals.head) != NULL) {
        VM_printf("Locals:\n");
        while (symbol) {
            Local *sym = GetLocalPtr(symbol);
            VM_printf("  %s %d\n", sym->name, sym->offset);
            symbol = sym->next;
        }
    }
}

