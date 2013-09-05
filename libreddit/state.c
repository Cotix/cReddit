#ifndef _REDDIT_STATE_C_
#define _REDDIT_STATE_C_

#include <stdlib.h>

#include "state.h"
/*
 * Include reddit library globals
 */
#include "global.h"



/*
 * Allocate a new reddit_state and set any needed values to defaults
 */
RedditState *redditStateNew()
{
    RedditState *state;
    state = rmalloc(sizeof(RedditState));
    state->base = NULL;

    return state;
}

/*
 * Frees a reddit_state returned by reddit_state_new()
 * Also frees anything attached to the reddit_state, such as the cookie linked-list
 */
void redditStateFree(RedditState *state)
{

    /* First free the linked-list of cookies */
    RedditCookieLink *tmp;
    RedditCookieLink *node;
    for(node = state->base; node != NULL; node = tmp) {
        tmp = node->next;
        redditCookieFree(node);
    }

    /* Free the actual state */
    free(state);
}

/*
 * Returns the current reddit_state the library is using
 */
RedditState *redditStateGet()
{
    return currentRedditState;
}

/*
 * Sets the current reddit_state that the library will use
 */
void redditStateSet(RedditState *state)
{
    currentRedditState = state;
}


#endif
