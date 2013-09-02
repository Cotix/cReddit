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
reddit_state *reddit_state_new()
{
    reddit_state *state;
    state = rmalloc(sizeof(reddit_state));

    state->base = NULL;
    state->end  = NULL;

    return state;
}

/*
 * Frees a reddit_state returned by reddit_state_new()
 * Also frees anything attached to the reddit_state, such as the cookie linked-list
 */
void reddit_state_free(reddit_state *state)
{

    /* First free the linked-list of cookies */
    reddit_cookie_link *tmp;
    reddit_cookie_link *node;
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
reddit_state *reddit_state_get()
{
    return current_reddit_state;
}

/*
 * Sets the current reddit_state that the library will use
 */
void reddit_state_set(reddit_state *state)
{
    current_reddit_state = state;
}


#endif
