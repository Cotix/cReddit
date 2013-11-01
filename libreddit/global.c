#ifndef _REDDIT_GLOBAL_C_
#define _REDDIT_GLOBAL_C_

#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>

#include "global.h"

/*
 * The definition of the currentRedditState variable. See global.h for info
 */
RedditState *currentRedditState = NULL;

/*
 * This is the actual definition of the debugFile pointer, who's extern definition is in global.h
 */
#ifdef REDDIT_DEBUG
FILE *debugFile = NULL;
#endif


/*
 * Just a small wrapper around malloc -- It checks for NULL and if a malloc
 * fails it simply exits the program
 */
void *rmalloc(size_t bytes)
{
    void *m = malloc(bytes);
    if (m == NULL)
        exit(EXIT_FAILURE);
    return m;
}

/*
 * Similar wrapper around realloc
 */
void *rrealloc(void *old, size_t bytes)
{
    void *m = realloc(old, bytes);
    if (m == NULL)
        exit(EXIT_FAILURE);
    return m;
}

/*
 * These initalize the library and close the library
 *
 * Currently they just start and end the curl library
 */
EXPORT_SYMBOL void redditGlobalInit()
{
    curl_global_init(CURL_GLOBAL_ALL);
    DEBUG_PRINT(L"libreddit Loaded\n");
}

EXPORT_SYMBOL void redditGlobalCleanup()
{
    curl_global_cleanup();
    DEBUG_PRINT(L"libreddit Closed\n");
}

#ifdef REDDIT_DEBUG
EXPORT_SYMBOL void redditSetDebugFile(FILE *file)
{
    debugFile = file;
}
#endif

#endif
