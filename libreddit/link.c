#ifndef _REDDIT_LINK_C_
#define _REDDIT_LINK_C_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "global.h"
#include "link.h"
#include "token.h"
#include "jsmn.h"

/*
 * Allocates an empty RedditLink structure
 */
EXPORT_SYMBOL RedditLink *redditLinkNew ()
{
    RedditLink *link = rmalloc(sizeof(RedditLink));

    /* Just initialize everything as NULL or zero */
    memset(link, 0, sizeof(RedditLink));

    return link;
}

/*
 * Frees a single RedditLink structure
 * Note: Doesn't free the RedditLink at 'link->next'
 */
EXPORT_SYMBOL void redditLinkFree (RedditLink *link)
{
    if (link == NULL)
        return ;

    free(link->selftext);
    free(link->title);

    free(link->selftextEsc);
    free(link->titleEsc);

    free(link->wselftextEsc);
    free(link->wtitleEsc);

    free(link->id);
    free(link->permalink);
    free(link->author);
    free(link->url);

    free(link);
}

/*
 * Allocates an empty RedditLinkList structure
 */
EXPORT_SYMBOL RedditLinkList *redditLinkListNew()
{
    RedditLinkList *list = rmalloc(sizeof(RedditLinkList));
    memset(list, 0, sizeof(RedditLinkList));
    return list;
}

EXPORT_SYMBOL void redditLinkListFreeLinks (RedditLinkList *list)
{
    int i;
    if (list == NULL)
        return ;
    for (i = 0; i < list->linkCount; i++)
        redditLinkFree(list->links[i]);
    free(list->links);
    list->links = NULL;
    list->linkCount = 0;
}

/*
 * Fress a RedditLinkList as well as free's all RedditLink structures attached.
 */
EXPORT_SYMBOL void redditLinkListFree (RedditLinkList *list)
{
    if (list == NULL)
        return ;
    redditLinkListFreeLinks(list);
    free(list->subreddit);
    free(list->modhash);
    free(list->afterId);
    free(list);
}

/*
 * Adds a RedditLink onto a RedditLinkList
 */
EXPORT_SYMBOL void redditLinkListAddLink (RedditLinkList *list, RedditLink *link)
{
    list->linkCount++;
    list->links = rrealloc(list->links, list->linkCount * sizeof(RedditLink*));
    list->links[list->linkCount - 1] = link;
}

/* Macro for the varidic arguments on parseTokens */
#define ARG_LINK_GET_LINK \
    RedditLink *link = va_arg(args, RedditLink*);

/*
 * Details how to parse a RedditLink. Doesn't do much more then allocate a new
 * RedditLink and map it's members to key strings.
 */
RedditLink *redditGetLink(TokenParser *parser)
{
    RedditLink *link = redditLinkNew();

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRPARSE("selftext",      link->selftext, link->selftextEsc, link->wselftextEsc),
        ADD_TOKEN_IDENT_STRPARSE("title",         link->title,    link->titleEsc,    link->wtitleEsc),
        ADD_TOKEN_IDENT_STRING  ("id",            link->id),
        ADD_TOKEN_IDENT_STRING  ("permalink",     link->permalink),
        ADD_TOKEN_IDENT_STRING  ("author",        link->author),
        ADD_TOKEN_IDENT_STRING  ("url",           link->url),
        ADD_TOKEN_IDENT_INT     ("score",         link->score),
        ADD_TOKEN_IDENT_INT     ("downs",         link->downs),
        ADD_TOKEN_IDENT_INT     ("ups",           link->ups),
        ADD_TOKEN_IDENT_INT     ("num_comments",  link->numComments),
        ADD_TOKEN_IDENT_INT     ("num_reports",   link->numReports),
        ADD_TOKEN_IDENT_BOOL    ("is_self",       link->flags, REDDIT_LINK_IS_SELF),
        ADD_TOKEN_IDENT_BOOL    ("over_18",       link->flags, REDDIT_LINK_OVER_18),
        ADD_TOKEN_IDENT_BOOL    ("clicked",       link->flags, REDDIT_LINK_CLICKED),
        ADD_TOKEN_IDENT_BOOL    ("stickied",      link->flags, REDDIT_LINK_STICKIED),
        ADD_TOKEN_IDENT_BOOL    ("edited",        link->flags, REDDIT_LINK_EDITED),
        ADD_TOKEN_IDENT_BOOL    ("hidden",        link->flags, REDDIT_LINK_HIDDEN),
        ADD_TOKEN_IDENT_BOOL    ("distinguished", link->flags, REDDIT_LINK_DISTINGUISHED),
        ADD_TOKEN_IDENT_DATE    ("created_utc",   link->created_utc),
        {0}
    };

    parseTokens(parser, ids, link);

    return link;
}

/* varidic function arguments for a RedditLinkList */
#define ARG_LIST_GET_LISTING \
    RedditLinkList *list = va_arg(args, RedditLinkList*);

/*
 * When we encounter a 't3', which is a new RedditLink, we parse it with
 * redditGetLink, and then add it to our RedditLinkList
 */
DEF_TOKEN_CALLBACK(getListingHelper)
{
    ARG_LIST_GET_LISTING


    /* Search for the 'kind' ident, and check if it was set to 't3' */
    int i;
    for (i = 0; idents[i].name != NULL; i++)
        if (strcmp(idents[i].name, "kind") == 0)
            if (idents[i].value != NULL && strcmp(*((char**)idents[i].value), "t3") == 0)
                redditLinkListAddLink(list, redditGetLink(parser));

}

/*
 * Gets the contents of a subreddit and adds them to 'list' -- If list already has elements in it
 * it will ask reddit to give us the next links in the list
 *
 * 'subreddit' should be in the form '/r/subreddit' or empty to indicate 'front'
 */
EXPORT_SYMBOL RedditErrno redditGetListing (RedditLinkList *list)
{
    char subred[1024], *kindStr = NULL;
    TokenParserResult res;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("modhash", list->modhash),
        ADD_TOKEN_IDENT_STRING("kind",    kindStr),
        ADD_TOKEN_IDENT_FUNC  ("data",    getListingHelper),
        ADD_TOKEN_IDENT_STRING("after",   list->afterId),
        {0}
    };

    strcpy(subred, REDDIT_URL);
    if (list->subreddit != NULL)
        strcat(subred, list->subreddit);

    if (list->type == REDDIT_NEW)
        strcat(subred, REDDIT_SUB_NEW);
    else if (list->type == REDDIT_RISING)
        strcat(subred, REDDIT_SUB_RISING);
    else if (list->type == REDDIT_CONTR)
        strcat(subred, REDDIT_SUB_CONTROVERSIAL);
    else if (list->type == REDDIT_TOP)
        strcat(subred, REDDIT_SUB_TOP);

    strcat(subred, REDDIT_JSON);

    if (list->linkCount > 0)
        sprintf(subred + strlen(subred), "?after=%s", list->afterId);

    res = redditRunParser(subred, NULL, ids, list);

    free(kindStr);

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;
}


#endif
