#ifndef _REDDIT_STATE_C_
#define _REDDIT_STATE_C_

#include <stdlib.h>

#include "state.h"
/*
 * Include reddit library globals
 */
#include "global.h"



/*
 * Allocate a new RedditState and set any needed values to defaults
 */
EXPORT_SYMBOL RedditState *redditStateNew()
{
    RedditState *state;
    state = rmalloc(sizeof(RedditState));
    state->base = NULL;
    state->userAgent = NULL;

    return state;
}

/*
 * Frees a RedditState returned by redditStateNew()
 * Also frees anything attached to the RedditState, such as the cookie linked-list
 */
EXPORT_SYMBOL void redditStateFree(RedditState *state)
{
    if (state == NULL)
        return ;
    /* First free the linked-list of cookies */
    RedditCookieLink *tmp;
    RedditCookieLink *node;
    for(node = state->base; node != NULL; node = tmp) {
        tmp = node->next;
        redditCookieFree(node);
    }

    free(state->userAgent);

    /* Free the actual state */
    free(state);
}

/*
 * Returns the current RedditState the library is using
 */
EXPORT_SYMBOL RedditState *redditStateGet()
{
    return currentRedditState;
}

/*
 * Sets the current RedditState that the library will use
 */
EXPORT_SYMBOL void redditStateSet(RedditState *state)
{
    currentRedditState = state;
}


#endif
