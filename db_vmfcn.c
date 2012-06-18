/* db_vmfcn.c - intrinsic functions
 *
 * Copyright (c) 2009 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include <ctype.h>
#include "db_vm.h"

/* local functions */
static VMVALUE GetStringVal(uint8_t *str, int len);

/* fcn_abs - ABS(n): return the absolute value of a number */
void fcn_abs(Interpreter *i)
{
    i->sp[1] = abs(i->sp[1]);
}

/* fcn_rnd - RND(n): return a random number between 0 and n-1 */
void fcn_rnd(Interpreter *i)
{
    i->sp[1] = rand() % i->sp[1];
}

/* fcn_left - LEFT$(str$, n): return the leftmost n characters of a string */
void fcn_left(Interpreter *i)
{
    uint8_t *str;
    size_t len;
    VMVALUE n;
    str = GetStringPtr((VMHANDLE)i->sp[1]);
    len = GetHeapObjSize((VMHANDLE)i->sp[1]);
    n = i->sp[2];
    if (n > len)
        n = len;
    i->sp[2] = (VMVALUE)StoreByteVector(i->heap, ObjTypeString, str, n);
}

/* fcn_right - RIGHT$(str$, n): return the rightmost n characters of a string */
void fcn_right(Interpreter *i)
{
    uint8_t *str;
    size_t len;
    VMVALUE n;
    str = GetStringPtr((VMHANDLE)i->sp[1]);
    len = GetHeapObjSize((VMHANDLE)i->sp[1]);
    n = i->sp[2];
    if (n > len)
        n = len;
    i->sp[2] = (VMVALUE)StoreByteVector(i->heap, ObjTypeString, str + len - n, n);
    Drop(i, 2);
}

/* fcn_mid - MID$(str$, start, n): return n characters from the middle of a string */
void fcn_mid(Interpreter *i)
{
    VMVALUE start, n;
    uint8_t *str;
    size_t len;
    str = GetStringPtr((VMHANDLE)i->sp[1]);
    len = GetHeapObjSize((VMHANDLE)i->sp[1]);
    start = i->sp[2];
    n = i->sp[3];
    if (start < 0 || start >= len)
        Abort(i, str_subscript_err, start + 1);
    if (start + n > len)
        n = len - start;
    i->sp[3] = (VMVALUE)StoreByteVector(i->heap, ObjTypeString, str + start, n);
}

/* fcn_chr - CHR$(n): return a one character string with the specified character code */
void fcn_chr(Interpreter *i)
{
    uint8_t buf[1];
    buf[0] = (char)i->sp[1];
    i->sp[1] = (VMVALUE)StoreByteVector(i->heap, ObjTypeString, buf, 1);
}

/* fcn_str - STR$(n): return n converted to a string */
void fcn_str(Interpreter *i)
{
    char buf[32];
    snprintf(buf, sizeof(buf), str_value_fmt, i->sp[1]);
    i->sp[1] = (VMVALUE)StoreByteVector(i->heap, ObjTypeString, (uint8_t *)buf, strlen(buf));
}

/* fcn_val - VAL(str$): return the numeric value of a string */
void fcn_val(Interpreter *i)
{
    uint8_t *str;
    size_t len;
    str = GetStringPtr((VMHANDLE)i->sp[1]);
    len = GetHeapObjSize((VMHANDLE)i->sp[1]);
    i->sp[1] = GetStringVal(str, len);
}

/* fcn_asc - ASC(str$): return the character code of the first character of a string */
void fcn_asc(Interpreter *i)
{
    uint8_t *str;
    size_t len;
    str = GetStringPtr((VMHANDLE)i->sp[1]);
    len = GetHeapObjSize((VMHANDLE)i->sp[1]);
    i->sp[1] = len > 0 ? *str : 0;
}

/* fcn_len - LEN(str$): return length of a string */
void fcn_len(Interpreter *i)
{
    i->sp[1] = GetHeapObjSize((VMHANDLE)i->sp[1]);
}

/* GetStringVal - get the numeric value of a string */
static VMVALUE GetStringVal(uint8_t *str, int len)
{
    VMVALUE val = 0;
    int radix = 'd';
    int sign = 1;
    int ch;

    /* handle non-empty strings */
    if (--len >= 0) {

        /* check for a sign or a radix indicator */
        switch (ch = *str++) {
        case '-':
            sign = -1;
            break;
        case '+':
            /* sign is 1 by default */
            break;
        case '0':
            if (--len < 0)
                return 0;
            switch (ch = *str++) {
            case 'b':
            case 'B':
                radix = 'b';
                break;
            case 'x':
            case 'X':
                radix = 'x';
                break;
            default:
                ++len;
                --str;
                radix = 'o';
                break;
            }
            break;
        default:
            if (!isdigit(ch))
                return 0;
            val = ch - '0';
            break;
        }


        /* get the number */
        switch (radix) {
        case 'b':
            while (--len >= 0) {
                ch = *str++;
                if (ch == '0' || ch == '1')
                    val = val * 2 + ch - '0';
                else if (ch != '_')
                    break;
            }
            break;
        case 'd':
            while (--len >= 0) {
                ch = *str++;
                if (isdigit(ch))
                    val = val * 10 + ch - '0';
                else if (ch != '_')
                    break;
            }
            break;
        case 'x':
            while (--len >= 0) {
                ch = *str++;
                if (isxdigit(ch)) {
                    int lowerch = tolower(ch);
                    val = val * 16 + (isdigit(lowerch) ? lowerch - '0' : lowerch - 'a' + 16);
                }
                else if (ch != '_')
                    break;
            }
            break;
        case 'o':
            while (--len >= 0) {
                ch = *str++;
                if (ch >= '0' && ch <= '7')
                    val = val * 8 + ch - '0';
                else if (ch != '_')
                    break;
            }
            break;
        }
    }
    
    /* return the numeric value */
    return sign * val;
}

/* fcn_printStr - printStr(n): print a string */
void fcn_printStr(Interpreter *i)
{
    VMHANDLE string;
    uint8_t *str;
    size_t size;
    string = i->hsp[0];
    str = GetByteVectorBase(string);
    size = GetHeapObjSize(string);
    while (size > 0) {
        VM_putchar(*str++);
        --size;
    }
    DropH(i, 1);
}

/* fcn_printInt - printInt(n): print an integer */
void fcn_printInt(Interpreter *i)
{
    VM_printf(str_value_fmt, i->sp[0]);
    Drop(i, 1);
}

/* fcn_printTab - printTab(): print a tab */
void fcn_printTab(Interpreter *i)
{
    VM_putchar('\t');
}

/* fcn_printNL - printNL(): print a newline */
void fcn_printNL(Interpreter *i)
{
    VM_putchar('\n');
}

/* fcn_printFlush - printFlush(): flush the output buffer */
void fcn_printFlush(Interpreter *i)
{
    VM_flush();
}
