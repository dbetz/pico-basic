#include <stdio.h>
#include "db_compiler.h"
#include "db_vm.h"

static DATA_SPACE uint8_t space[WORKSPACESIZE];

DefIntrinsic(dump);

static int TermGetLine(void *cookie, char *buf, int len, VMVALUE *pLineNumber);

int main(int argc, char *argv[])
{
    VMVALUE lineNumber = 0;
    uint8_t *freeMark;
    ObjHeap *heap;
    System *sys;
    
    VM_sysinit(argc, argv);

    sys = InitSystem(space, sizeof(space));
    sys->getLine = TermGetLine;
    sys->getLineCookie = &lineNumber;
    
    if (!(heap = InitHeap(sys, HEAPSIZE, MAXOBJECTS)))
        return 1;
        
    AddIntrinsic(heap, "DUMP",          dump,        "i")

    freeMark = sys->freeNext;
     
    for (;;) {
        VMHANDLE code;
        sys->freeNext = freeMark;
        if ((code = Compile(sys, heap, VMTRUE)) != NULL) {
            sys->freeNext = freeMark;
            Execute(sys, heap, code);
        }
    }
    
    return 0;
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

