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
RedditState *reddit_state_new()
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
void reddit_state_free(RedditState *state)
{

    /* First free the linked-list of cookies */
    RedditCookieLink *tmp;
    RedditCookieLink *node;
    for(node = state->base; node != NULL; node = tmp) {
        tmp = node->next;
        reddit_cookie_free(node);
    }

    /* Free the actual state */
    free(state);
}

/*
 * Returns the current reddit_state the library is using
 */
RedditState *reddit_state_get()
{
    return current_reddit_state;
}

/*
 * Sets the current reddit_state that the library will use
 */
void reddit_state_set(RedditState *state)
{
    current_reddit_state = state;
}


#endif
