#ifndef _SRC_GLOBAL_H_
#define _SRC_GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef REDDIT_DEBUG
  extern FILE *debugFile;
# define DEBUG_START(file, filename) \
    do { \
        file = fopen(filename, "w"); \
        redditSetDebugFile(file); \
    } while (0)

# define DEBUG_END(file) fclose(file)
#else
# define DEBUG_START(file, filename) do { ; } while (0)
# define DEBUG_END(file) do { ; } while (0)
#endif

#define DEBUG_FILE     debugFile
#define DEBUG_MODULE   "creddit"
#define DEBUG_FILENAME "/tmp/reddit_errors.log"
#include "debug.h"

char *alloc_sprintf (const char *format, ...);
const wchar_t *reverse_wcsnchr(const wchar_t *start, const size_t len, const wchar_t c);

#endif
