
#include <stdio.h>
#include <stdlib.h>

#include "global.h"

FILE *debugFile;

static char *valloc_sprintf (const char *format, va_list lst)
{
    va_list cpy;
    va_copy(cpy, lst);

    size_t size = vsnprintf(NULL, 0, format, cpy) + 1;
    char *buf = malloc(size);

    if (!buf)
        return NULL;

    buf[0] = '\0';

    vsnprintf(buf, size, format, lst);
    return buf;
}

char *alloc_sprintf (const char *format, ...)
{
    char *ret;
    va_list lst;

    va_start(lst, format);
    ret = valloc_sprintf(format, lst);
    va_end(lst);

    return ret;
}

// start is point in the wchar_t string to begin searching at.
// len is the number of chars to search back
// c is the char to search for.
// Steps past the starting point before searching
const wchar_t *reverse_wcsnchr(const wchar_t *start, const size_t len, const wchar_t c)
{
    const wchar_t *end = start - len;

    for (; start != end && *start != c; start--);

    return (start != end) ? start : NULL;
}

