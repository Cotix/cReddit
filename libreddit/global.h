#ifndef _REDDIT_GLOBAL_H_
#define _REDDIT_GLOBAL_H_

#include <stdlib.h>
#include <stdio.h>

#include "state.h"
#include "token.h"

#define CREDDIT_USERAGENT "cReddit/0.0.1"

/*
 * This macro is used to export a symbol outside of the library. We compile with
 * -fvisibility=hidden, so functions are hidden in the .so by default. Using
 *  this macro will make them usable outside of the library.
 *
 *  Note: This is a GCC specefic add-on
 */
#define EXPORT_SYMBOL __attribute__((visibility("default")))


/*
 * the below variable is a library global state for reddit (Mostly holds session cookies)
 *
 * When the library is used, you first need to use redditStateNew() and then
 * redditStateSet() to initalize the library with a new state. You can switch
 * to a different state at any time via redditStateGet() and/or get the current
 * state via redditStateGet.
 *
 * This data is needed on about every function call, storing it here instead of
 * requiring it every call it much easier.
 *
 */
extern RedditState *currentRedditState;

/*
 * This adds debug macros and a debug FILE* pointer in the event that debugging is turned on
 * If you compile with -DREDDIT_DEBUG, then debugging will be used
 *
 * The 'debugFile' FILE* is the destination of all the debug messages.
 * DEBUG_FILE is the actual filename of where to send files to
 * DEBUG_PRINT is a macro that expands into a wide-character print to this file
 */
#ifdef REDDIT_DEBUG
  extern FILE *debugFile;
# define DEBUG_FILE "/tmp/reddit_error.log"
# define DEBUG_PRINT(...) fwprintf(debugFile, __VA_ARGS__)
#else
# define DEBUG_PRINT(...) do { ; } while (0)
#endif


void *rmalloc (size_t bytes);
void *rrealloc (void *old, size_t bytes);

#endif
