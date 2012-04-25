#include <stdio.h>
#include "db_compiler.h"
#include "db_vm.h"

static DATA_SPACE uint8_t space[HEAPSIZE];

DefIntrinsic(dump);

static void repl(System *sys);

int main(int argc, char *argv[])
{
    System *sys;
    
    VM_sysinit(argc, argv);

    sys = InitSystem(space, sizeof(space));

    repl(sys);
    
    return 0;
}

static int TermGetLine(void *cookie, char *buf, int len, VMVALUE *pLineNumber)
{
    VMVALUE *pLine = (VMVALUE *)cookie;
    *pLineNumber = ++(*pLine);
    return fgets(buf, len, stdin) != NULL;
}

static void repl(System *sys)
{
    ParseContext *c;
    sys->freeNext = sys->freeSpace;
    if (!(c = InitCompiler(sys, MAXOBJECTS)))
        VM_printf("insufficient memory\n");
    else {
        AddIntrinsic(c, "DUMP",          dump,        "i")
        for (;;) {
            VMVALUE lineNumber = 0;
            VMHANDLE code;
            c->getLine = TermGetLine;
            c->getLineCookie = &lineNumber;
            if ((code = Compile(c, VMTRUE)) != NULL) {
                Interpreter *i = (Interpreter *)sys->freeNext;
                size_t stackSize = (sys->freeTop - sys->freeNext - sizeof(Interpreter)) / sizeof(VMVALUE);
                if (stackSize <= 0)
                    VM_printf("insufficient memory\n");
                else {
                    InitInterpreter(i, c->heap, stackSize);
                    Execute(i, code);
                }
            }
        }
    }
}

void fcn_dump(Interpreter *i)
{
    DumpHeap(i->heap);
}

