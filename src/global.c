
#include <stdio.h>
#include <stdlib.h>

#include "global.h"

FILE *debugFile;

static char *valloc_sprintf (const char *format, va_list lst)
{
    size_t size = vsnprintf(NULL, 0, format, lst) + 1;
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

