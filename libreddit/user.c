#ifndef _REDDIT_USER_C_
#define _REDDIT_USER_C_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "global.h"
#include "user.h"
#include "token.h"

/*
 * Allocate and reture a new 'reddit_user' struct'
 *
 * A 'reddit_user' represents any and all users
 */
reddit_user *reddit_user_new()
{
    reddit_user *log = rmalloc(sizeof(reddit_user));

    memset(log, 0, sizeof(reddit_user));
    log->comment_karma = -1;
    log->link_karma    = -1;

    return log;
}

void reddit_user_free (reddit_user *log)
{
    free(log->name);
    free(log->id);
    free(log->modhash);
    free(log);
}

/*
 * allocates and creates a new 'reddit_user_logged'
 * It's a superset of 'reddit_user', representing a logged-in user
 */
reddit_user_logged *reddit_user_logged_new()
{
    reddit_user_logged *user = rmalloc(sizeof(reddit_user_logged));

    user->user_info = reddit_user_new();
    user->stay_logged_on = false;
    user->user_state     = REDDIT_USER_OFFLINE;

    return user;
}

/*
 * Frees a 'reddit_user_logged' and everything attached to it
 */
void reddit_user_logged_free(reddit_user_logged *user)
{
    reddit_user_free(user->user_info);
    free(user);
}

reddit_user *reddit_get_user(token_parser *parser)
{
    reddit_user *user = reddit_user_new();

    token_ident ids[] = {
        ADD_TOKEN_IDENT_STRING("modhash",       user->modhash),
        ADD_TOKEN_IDENT_STRING("id",            user->id),
        ADD_TOKEN_IDENT_STRING("name",          user->name),
        ADD_TOKEN_IDENT_INT   ("link_karma",    user->link_karma),
        ADD_TOKEN_IDENT_INT   ("comment_karma", user->comment_karma),
        ADD_TOKEN_IDENT_BOOL  ("has_mail",      user->flags, REDDIT_USER_HAS_MAIL),
        ADD_TOKEN_IDENT_BOOL  ("is_friend",     user->flags, REDDIT_USER_IS_FRIEND),
        ADD_TOKEN_IDENT_BOOL  ("has_mod_mail",  user->flags, REDDIT_USER_HAS_MOD_MAIL),
        ADD_TOKEN_IDENT_BOOL  ("over_18",       user->flags, REDDIT_USER_OVER_18),
        ADD_TOKEN_IDENT_BOOL  ("is_gold",       user->flags, REDDIT_USER_IS_GOLD),
        ADD_TOKEN_IDENT_BOOL  ("is_mod",        user->flags, REDDIT_USER_IS_MOD),
        {0}
    };

    parse_tokens(parser, ids);

    return user;
}

/*
 * Standard way of handling the parser -- When we need something from the va_list
 * in a token callback, we place this macro at the top of the func definition
 *
 * It makes sure all the lists read from va_args are correct in every callback
 */
#define ARG_LIST_LOGIN \
    reddit_errno *response = va_arg(args, reddit_errno*);


DEF_TOKEN_CALLBACK(handle_error_array)
{
    ARG_LIST_LOGIN

    if (parser->tokens[parser->current_token].full_size > 0)
        *response = REDDIT_ERROR_RESPONSE;

}

DEF_TOKEN_CALLBACK(handle_cookie)
{
    char *tmp = NULL;

    READ_TOKEN_TO_TMP(parser->block->memory, tmp, parser->tokens[parser->current_token]);
    reddit_cookie_new("reddit_session", tmp);
    free(tmp);
}

/*
 * Takes a redit_user_logged and returns a reddit_login_result
 *
 * Logs in the user storred in the reddit_user_logged into Reddit
 * Also if successful adds the 'reddit_session' cookie to the global state
 *
 */
reddit_errno reddit_user_logged_login (reddit_user_logged *log, char *name, char *passwd)
{
    char login_info[4096];
    char tf[6];
    reddit_errno response = REDDIT_SUCCESS;

    token_parser_result res;

    token_ident ids[] = {
        ADD_TOKEN_IDENT_FUNC("errors", handle_error_array),
        ADD_TOKEN_IDENT_FUNC("cookie", handle_cookie),
        {0}
    };

    /* If passwd or user isn't set then just exit */
    if (!(passwd) || !(name))
        return REDDIT_ERROR_USER;

    /* Remove any session cookie we may still have -- We'll get a new one after login
     * If you want to handle multiple-users logged on, make sure to switch out the global
     * state */
    reddit_remove_cookie("reddit_session");

    sprintf(login_info, "api_type=json&rem=%s&user=%s&passwd=%s", true_false_string(tf, log->stay_logged_on), name, passwd);


    res = reddit_run_parser("http://www.reddit.com/api/login", login_info, ids, &response);

    if (res != TOKEN_PARSER_SUCCESS)
        response = REDDIT_ERROR_RESPONSE;

    if (response == REDDIT_SUCCESS)
        log->user_state = REDDIT_USER_LOGGED_ON;
    else
        log->user_state = REDDIT_USER_OFFLINE;

    return response;
}

#define ARG_LIST_USER_UPDATE \
    reddit_user **user = va_arg(args, reddit_user**);

DEF_TOKEN_CALLBACK(user_update_helper)
{
    ARG_LIST_USER_UPDATE

    int i;
    for (i = 0; idents[i].name != NULL; i++) {
        if (strcmp(idents[i].name, "kind") == 0) {
            /*
             * Check contents of the name
             */
            if (idents[i].value != NULL && strcmp(*((char**)idents[i].value), "t2") == 0) {
                if (*user != NULL)
                    reddit_user_free(*user);

                *user = reddit_get_user(parser);
            }

            break;
        }
    }
}

reddit_errno reddit_user_logged_update (reddit_user_logged *user)
{
    token_parser_result res;
    char *kind_str = NULL;

    token_ident ids[] = {
        ADD_TOKEN_IDENT_STRING("kind", kind_str),
        ADD_TOKEN_IDENT_FUNC("data", user_update_helper),
        {0}
    };

    res = reddit_run_parser("http://www.reddit.com/api/me.json", NULL, ids, &(user->user_info));
    free(kind_str);

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;

}


#endif
