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
 * These define the Reddit API links used by the library
 */
#define REDDIT_URL            "http://www.reddit.com"
#define REDDIT_JSON           ".json"
#define REDDIT_API REDDIT_URL "/api"

#define REDDIT_SUB_NEW           "/new"
#define REDDIT_SUB_RISING        "/rising"
#define REDDIT_SUB_CONTROVERSIAL "/controversial"
#define REDDIT_SUB_TOP           "/top"

#define REDDIT_API_MORECHILDREN REDDIT_API "/morechildren" REDDIT_JSON
#define REDDIT_API_LOGIN        REDDIT_API "/login"        REDDIT_JSON
#define REDDIT_API_ME           REDDIT_API "/me"           REDDIT_JSON

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
 * the UserAgent) (LIBREDDIT_VERSION is defined by the Makefile)
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
