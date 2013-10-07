#ifndef _REDDIT_COOKIE_C_
#define _REDDIT_COOKIE_C_

#include <stdlib.h>
#include <string.h>

#include "cookie.h"

#include "global.h"


/*
 * Adds a new cookie to currentRedditState
 */
EXPORT_SYMBOL void redditCookieNew(char *name, char *data)
{
    RedditCookieLink *link = rmalloc(sizeof(RedditCookieLink));

    link->name = rmalloc(strlen(name) + 1);
    link->data = rmalloc(strlen(data) + 1);

    strcpy(link->name, name);
    strcpy(link->data, data);

    /*
     * If for whatever reason we don't have an allocated reddit-state yet
     * Make a new one
     */
    if (currentRedditState == NULL)
        currentRedditState = redditStateNew();

    /* Attach the link */
    link->next = currentRedditState->base;
    currentRedditState->base = link;
}

/*
 * Free's a cookie link -- Both redditStateFree and redditRemoveCookie call this
 * function. You should probably use one of those for what you need.
 *
 * This function does no clean-up on the linked-list as a whole, just frees a single link
 */
EXPORT_SYMBOL void redditCookieFree(RedditCookieLink *link)
{
    if (link == NULL)
        return ;
    free(link->name);
    free(link->data);
    free(link);
}

/*
 * Removes a cookie with the given name from currentRedditState
 */
EXPORT_SYMBOL void redditRemoveCookie(char *name)
{
    RedditCookieLink *prev = NULL, *node;

    /* Incase no state has yet been set */
    if (currentRedditState == NULL) return ;

    for(node = currentRedditState->base; node != NULL; prev = node, node = node->next) {
        if (strcmp(node->name, name) == 0) {
            /* Found the node */
            if (prev != NULL)
                prev->next = node->next;
            else
                currentRedditState->base = node->next;

            redditCookieFree(node);
            break;
        }
    }
}

/*
 * Returns a string holding the current cookies (To send with curl)
 *
 * The returned pointer will be NULL if there are currently no cookies
 * The returned pointer should be freed if not NULL
 */
EXPORT_SYMBOL char *redditGetCookieString()
{
    char *cookieStr = NULL;
    int currentLength = 0;
    RedditCookieLink *node;

    if (currentRedditState == NULL) return NULL;

    for(node = currentRedditState->base; node != NULL; node = node->next) {
        int add_size = strlen(node->name) + strlen(node->data) + 2;
        currentLength += add_size;
        cookieStr = rrealloc(cookieStr, currentLength);
        if (node != currentRedditState->base)
            strcat(cookieStr, "&");
        else
            cookieStr[0] = 0;

        strcat(cookieStr, node->name);
        strcat(cookieStr, "=");
        strcat(cookieStr, node->data);
    }

    return cookieStr;
}

#endif
