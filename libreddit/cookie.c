#ifndef _REDDIT_COOKIE_C_
#define _REDDIT_COOKIE_C_

#include <stdlib.h>
#include <string.h>

#include "cookie.h"

#include "global.h"


/*
 * Adds a new cookie to current_reddit_state
 */
void reddit_cookie_new(char *name, char *data)
{

    reddit_cookie_link *link = rmalloc(sizeof(reddit_cookie_link));
    //reddit_cookie_link *node;

    link->name = rmalloc(strlen(name) + 1);
    link->data = rmalloc(strlen(data) + 1);

    strcpy(link->name, name);
    strcpy(link->data, data);

    link->next = NULL;

    /*
     * If for whatever reason we don't have an allocated reddit-state yet
     * Make a new one
     */
    if (current_reddit_state == NULL)
        current_reddit_state = reddit_state_new();

    /* Attach the link */
    if (current_reddit_state->end != NULL) {
        current_reddit_state->end->next = link;
        current_reddit_state->end = link; /* Replace our current end */
    } else {
        current_reddit_state->base = link;
        current_reddit_state->end = link;
    }
}

/*
 * Free's a cookie link -- Both reddit_state_free and reddit_remove_cookie call this
 * function. You should probably use one of those for what you need.
 *
 * This function does no clean-up on the linked-list as a whole, just frees a single link
 */
void reddit_cookie_free(reddit_cookie_link *link)
{
    free(link->name);
    free(link->data);
    free(link);
}

/*
 * Removes a cookie with the given name from current_reddit_state
 */
void reddit_remove_cookie(char *name)
{
    reddit_cookie_link *prev = NULL, *node;

    /* Incase no state has yet been set */
    if (current_reddit_state == NULL) return ;

    for(node = current_reddit_state->base; node != NULL; prev = node, node = node->next) {
        if (strcmp(node->name, name) == 0) {
            /* Found the node */
            if (prev != NULL) {
                prev->next = node->next;
            } else {
                current_reddit_state->base = node->next;
            }
            reddit_cookie_free(node);
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
char *reddit_get_cookie_string()
{
    char *cookie_str = NULL;
    int current_length = 0;
    reddit_cookie_link *node;

    if (current_reddit_state == NULL) return NULL;

    for(node = current_reddit_state->base; node != NULL; node = node->next) {
        int add_size = strlen(node->name) + strlen(node->data) + 2;
        current_length += add_size;
        cookie_str = rrealloc(cookie_str, current_length);
        if (node != current_reddit_state->base) {
            strcat(cookie_str, "&");
        } else {
            cookie_str[0] = 0;
        }
        strcat(cookie_str, node->name);
        strcat(cookie_str, "=");
        strcat(cookie_str, node->data);
    }

    return cookie_str;
}

#endif
