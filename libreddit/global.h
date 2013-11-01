#ifndef _REDDIT_GLOBAL_H_
#define _REDDIT_GLOBAL_H_

#include <stdlib.h>
#include <stdio.h>

#include "state.h"
#include "token.h"

#ifdef REDDIT_DEBUG
extern FILE* debugFile;
# define DEBUG_MODULE "libreddit"
# define DEBUG_FILE   debugFile
#endif

#include "debug.h"

/*
 * Current version of libreddit
 */
#define LIBREDDIT_VERSION 0.0.1

/*
 * This macro is used to export a symbol outside of the library. We compile with
 * -fvisibility=hidden, so functions are hidden in the .so by default. Using
 *  this macro will make them usable outside of the library.
 *
 *  Note: This is a GCC specefic add-on
 */
#define EXPORT_SYMBOL __attribute__((visibility("default")))

/*
 * This is the UserAgent string appended on by libreddit (At the beginning of
 * the UserAgent)
 */
#define QQ(macro) #macro
#define Q(macro) QQ(macro)
#define LIBREDDIT_USERAGENT "libreddit/" Q(LIBREDDIT_VERSION) " (http://github.com/Cotix/cReddit) "

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

void *rmalloc (size_t bytes);
void *rrealloc (void *old, size_t bytes);

#endif
