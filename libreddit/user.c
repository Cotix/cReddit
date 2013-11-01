#ifndef _REDDIT_USER_C_
#define _REDDIT_USER_C_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "global.h"
#include "user.h"
#include "token.h"

/*
 * Allocate and return a new 'RedditUser' struct'
 *
 * A 'RedditUser' represents any and all users
 */
EXPORT_SYMBOL RedditUser *redditUserNew()
{
    RedditUser *log = rmalloc(sizeof(RedditUser));

    memset(log, 0, sizeof(RedditUser));
    log->commentKarma = -1;
    log->linkKarma    = -1;

    return log;
}

/*
 * Frees a RedditUser and any data it has
 */
EXPORT_SYMBOL void redditUserFree (RedditUser *log)
{
    if (log == NULL)
        return ;
    free(log->name);
    free(log->id);
    free(log->modhash);
    free(log);
}

/*
 * allocates and creates a new 'RedditUserLogged'
 * It's a superset of 'RedditUser', representing a logged-in user
 */
EXPORT_SYMBOL RedditUserLogged *redditUserLoggedNew()
{
    RedditUserLogged *user = rmalloc(sizeof(RedditUserLogged));

    user->userInfo     = redditUserNew();
    user->stayLoggedOn = false;
    user->userState    = REDDIT_USER_OFFLINE;

    return user;
}

/*
 * Frees a 'RedditUserLogged' and everything attached to it
 */
EXPORT_SYMBOL void redditUserLoggedFree(RedditUserLogged *user)
{
    if (user == NULL)
        return ;
    redditUserFree(user->userInfo);
    free(user);
}

/*
 * This setups idents to parse a RedditUser JSON object. It doesn't use any
 * callbacks, but simply maps key strings to members on the RedditUser object
 */
RedditUser *redditGetUser(TokenParser *parser)
{
    RedditUser *user = redditUserNew();

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("modhash",       user->modhash),
        ADD_TOKEN_IDENT_STRING("id",            user->id),
        ADD_TOKEN_IDENT_STRING("name",          user->name),
        ADD_TOKEN_IDENT_INT   ("link_karma",    user->linkKarma),
        ADD_TOKEN_IDENT_INT   ("comment_karma", user->commentKarma),
        ADD_TOKEN_IDENT_BOOL  ("has_mail",      user->flags, REDDIT_USER_HAS_MAIL),
        ADD_TOKEN_IDENT_BOOL  ("is_friend",     user->flags, REDDIT_USER_IS_FRIEND),
        ADD_TOKEN_IDENT_BOOL  ("has_mod_mail",  user->flags, REDDIT_USER_HAS_MOD_MAIL),
        ADD_TOKEN_IDENT_BOOL  ("over_18",       user->flags, REDDIT_USER_OVER_18),
        ADD_TOKEN_IDENT_BOOL  ("is_gold",       user->flags, REDDIT_USER_IS_GOLD),
        ADD_TOKEN_IDENT_BOOL  ("is_mod",        user->flags, REDDIT_USER_IS_MOD),
        {0}
    };

    parseTokens(parser, ids);

    return user;
}

/*
 * Standard way of handling the parser -- When we need something from the va_list
 * in a token callback, we place this macro at the top of the func definition
 *
 * It makes sure all the lists read from va_args are correct in every callback
 */
#define ARG_LIST_LOGIN \
    RedditErrno *response = va_arg(args, RedditErrno*);

/*
 * Checks for errors in the array array, and sets our result to return an error
 * if there is any errors.
 */
DEF_TOKEN_CALLBACK(handleErrorArray)
{
    ARG_LIST_LOGIN

    if (parser->tokens[parser->currentToken].full_size > 0)
        *response = REDDIT_ERROR_RESPONSE;

}

/*
 * If we get a session cookie, then add it to the currentRedditState via redditCookieNew
 */
DEF_TOKEN_CALLBACK(handleCookie)
{
    char *tmp = NULL;

    READ_TOKEN_TO_TMP(parser->block->memory, tmp, parser->tokens[parser->currentToken]);
    redditCookieNew("reddit_session", tmp);
    free(tmp);
}

/*
 * Takes a RedditUserLogged and returns a RedditErrno
 *
 * Logs in the user stored in the RedditUserLogged into Reddit
 * Also if successful adds the 'reddit_session' cookie to the global state
 *
 */
EXPORT_SYMBOL RedditErrno redditUserLoggedLogin (RedditUserLogged *log, char *name, char *passwd)
{
    char loginInfo[4096];
    char tf[6];
    RedditErrno response = REDDIT_SUCCESS;

    TokenParserResult res;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_FUNC("errors", handleErrorArray),
        ADD_TOKEN_IDENT_FUNC("cookie", handleCookie),
        {0}
    };

    /* If passwd or user isn't set then just exit */
    if (!(passwd) || !(name))
        return REDDIT_ERROR_USER;

    /* Remove any session cookie we may still have -- We'll get a new one after login
     * If you want to handle multiple-users logged on, make sure to switch out the global
     * state */
    redditRemoveCookie("reddit_session");

    sprintf(loginInfo, "api_type=json&rem=%s&user=%s&passwd=%s", trueFalseString(tf, log->stayLoggedOn), name, passwd);


    res = redditRunParser(REDDIT_API_LOGIN, loginInfo, ids, &response);

    if (res != TOKEN_PARSER_SUCCESS)
        response = REDDIT_ERROR_RESPONSE;

    if (response == REDDIT_SUCCESS)
        log->userState = REDDIT_USER_LOGGED_ON;
    else
        log->userState = REDDIT_USER_OFFLINE;

    return response;
}

/* varidic arguments list for parsing a user login */
#define ARG_LIST_USER_UPDATE \
    RedditUser **user = va_arg(args, RedditUser**);

/*
 * This callback handles a 'data' parameter and checks for a 't2', which
 * corresponds to a RedditUser JSON object.
 */
DEF_TOKEN_CALLBACK(userUpdateHelper)
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
                    redditUserFree(*user);

                *user = redditGetUser(parser);
            }

            break;
        }
    }
}

/*
 * This function updates the data in a RedditUserLogged. It leaves the user
 * logged in, and gets a new copy of the user->userInfo RedditUser
 */
EXPORT_SYMBOL RedditErrno redditUserLoggedUpdate (RedditUserLogged *user)
{
    TokenParserResult res;
    char *kindStr = NULL;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("kind", kindStr),
        ADD_TOKEN_IDENT_FUNC("data", userUpdateHelper),
        {0}
    };

    res = redditRunParser(REDDIT_API_ME, NULL, ids, &(user->userInfo));
    free(kindStr);

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;

}


#endif
