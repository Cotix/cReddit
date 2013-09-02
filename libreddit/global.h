#ifndef _REDDIT_GLOBAL_H_
#define _REDDIT_GLOBAL_H_

#include "state.h"
#include "token.h"

#define CREDDIT_USERAGENT "cReddit/0.0.1"

/*
 * the below variable is a library global state for reddit (Mostly holds session cookies)
 *
 * When the library is used, you first need to use reddit_state_new() and then
 * reddit_state_set() to initalize the library with a new state. You can switch
 * to a different state at any time via reddit_state_set() and/or get the current
 * state via reddit_state_get.
 *
 * This data is needed on about every function call, storing it here instead of
 * requiring it every call it much easier.
 *
 */
extern reddit_state *current_reddit_state;

void *rmalloc (size_t bytes);
void *rrealloc (void *old, size_t bytes);

#endif
