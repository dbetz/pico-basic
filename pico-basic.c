#include <stdio.h>
#include "db_compiler.h"
#include "db_vm.h"

static DATA_SPACE uint8_t space[WORKSPACESIZE];

DefIntrinsic(dump);
static void repl(System *sys, ObjHeap *heap);

static int TermGetLine(void *cookie, char *buf, int len, VMVALUE *pLineNumber);

int main(int argc, char *argv[])
{
    VMVALUE lineNumber = 0;
    ObjHeap *heap;
    System *sys;
    
    VM_sysinit(argc, argv);

    sys = InitSystem(space, sizeof(space));
    sys->getLine = TermGetLine;
    sys->getLineCookie = &lineNumber;
    
    if (!(heap = InitHeap(sys, HEAPSIZE, MAXOBJECTS)))
        return 1;
        
    repl(sys, heap);
    
    return 0;
}

static void repl(System *sys, ObjHeap *heap)
{
    uint8_t *freeMark = sys->freeNext;

    AddIntrinsic(heap, "DUMP",          dump,        "i")

    for (;;) {
        VMHANDLE code;
        sys->freeNext = freeMark;
        if ((code = Compile(sys, heap, VMTRUE)) != NULL) {
            Interpreter *i = (Interpreter *)sys->freeNext;
            size_t stackSize = (sys->freeTop - freeMark - sizeof(Interpreter)) / sizeof(VMVALUE);
            if (stackSize <= 0)
                VM_printf("insufficient memory\n");
            else {
                InitInterpreter(i, heap, stackSize);
                Execute(i, code);
            }
        }
    }
}

static int TermGetLine(void *cookie, char *buf, int len, VMVALUE *pLineNumber)
{
    VMVALUE *pLine = (VMVALUE *)cookie;
    *pLineNumber = ++(*pLine);
    return fgets(buf, len, stdin) != NULL;
}

void fcn_dump(Interpreter *i)
{
    DumpHeap(i->heap);
}

