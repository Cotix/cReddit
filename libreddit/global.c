#ifndef _REDDIT_GLOBAL_C_
#define _REDDIT_GLOBAL_C_

#include <stdlib.h>
#include <curl/curl.h>

#include "global.h"

RedditState *currentRedditState = NULL;

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

void redditGlobalInit()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

void redditGlobalCleanup()
{
    curl_global_cleanup();
}

#endif
