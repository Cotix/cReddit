#ifndef _REDDIT_COMMENT_C_
#define _REDDIT_COMMENT_C_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "global.h"
#include "comment.h"
#include "token.h"

/*
 * Creates a new redditComment
 */
EXPORT_SYMBOL RedditComment *redditCommentNew ()
{
    RedditComment *comment;
    comment = rmalloc(sizeof(RedditComment));
    memset(comment, 0, sizeof(RedditComment));
    return comment;
}

/*
 * Recurrisivly free's all replys on a RedditComment
 */
EXPORT_SYMBOL void redditCommentFreeReplies (RedditComment *comment)
{
    int i;
    if (comment == NULL)
        return ;
    for (i = 0; i < comment->replyCount; i++)
        redditCommentFree(comment->replies[i]);

    free(comment->replies);
}

/*
 * Frees all of a RedditComment as well as all of it's replies
 */
EXPORT_SYMBOL void redditCommentFree (RedditComment *comment)
{
    int i;
    if (comment == NULL)
        return ;

    free(comment->body);
    free(comment->bodyEsc);
    free(comment->wbodyEsc);
    free(comment->id);
    free(comment->author);
    free(comment->parentId);
    for (i = 0; i < comment->directChildrenCount; i++)
        free(comment->directChildrenIds[i]);

    free(comment->directChildrenIds);
    free(comment->childrenId);
    redditCommentFreeReplies(comment);
    free(comment);
}

/*
 * Chains a RedditComment as a reply on another RedditComment.
 */
EXPORT_SYMBOL void redditCommentAddReply (RedditComment *comment, RedditComment *reply)
{
    comment->replyCount++;
    comment->replies = rrealloc(comment->replies, (comment->replyCount) * sizeof(RedditComment));
    comment->replies[comment->replyCount - 1] = reply;
    reply->parent = comment;
}

/*
 * Creates a new blank list of comments
 */
EXPORT_SYMBOL RedditCommentList *redditCommentListNew ()
{
    RedditCommentList *list = rmalloc(sizeof(RedditCommentList));
    memset(list, 0, sizeof(RedditCommentList));
    return list;
}

/*
 * Frees that list of comments
 */
EXPORT_SYMBOL void redditCommentListFree (RedditCommentList *list)
{
    if (list == NULL)
        return ;
    redditCommentFree(list->baseComment);
    redditLinkFree(list->post);
    free(list->permalink);
    free(list->id);
    free(list);
}

/* Simple macro which expands to to the definitions of the varidic arguments
 * set to parseTokens */
#define ARG_COMMENT_LISTING \
    RedditCommentList *list = va_arg(args, RedditCommentList*); \
    RedditComment *comment = va_arg(args, RedditComment*);

/*
 * Not really sure there is a good way to avoid the fact that to parse the array
 * requires directly going over the tokens (Which you don't really want to have to do,
 * it's best to let the TokenParser handle the actual parsing of the tokens).
 *
 * Either way, this function simply loops over an array of id's for comments which are
 * attached to the current comment, but not displayed, and adds a copy of that string
 * to an array of ids in the comment
 */
DEF_TOKEN_CALLBACK(getCommentRepliesMore)
{
    ARG_COMMENT_LISTING
    (void)list; /* Make the compiler shut-up */
    int i;

    comment->directChildrenCount = parser->tokens[parser->currentToken].full_size;
    comment->directChildrenIds = rmalloc(comment->directChildrenCount * sizeof(char*));

    for (i = 0; i < comment->directChildrenCount; i++) {
        parser->currentToken++;
        comment->directChildrenIds[i] = getCopyOfToken(parser->block->memory, parser->tokens[parser->currentToken]);
    }
}

/*
 * This callback is sort of a 'dispatch' which simply deligates what to do to the relevant functions
 *
 * In the event of a 't3' datatype, we have a RedditLink and call redditGetLink to parse it
 * A 't1' is a comment, so we use redditGetComment, and then redditCommentAddReply
 * A 'more' type means that it's list of comment 'stubs' for a morechildren API call.
 */
DEF_TOKEN_CALLBACK(getCommentListHelper)
{
    ARG_COMMENT_LISTING
    RedditComment *reply = NULL;

    TokenIdent more_ids[] = {
        ADD_TOKEN_IDENT_INT   ("count",    comment->totalReplyCount),
        ADD_TOKEN_IDENT_STRING("id",       comment->childrenId),
        ADD_TOKEN_IDENT_FUNC  ("children", getCommentRepliesMore),
        {0}
    };

    int i;
    for (i = 0; idents[i].name != NULL; i++) {
        if (strcmp(idents[i].name, "kind") == 0) {
            if (idents[i].value != NULL) {
                if (strcmp(*((char**)idents[i].value), "t3") == 0) {
                    /* This is the reddit_link data -- free the old and
                     * grab the new link data */
                    free(list->post);
                    list->post = redditGetLink(parser);
                } else if (strcmp(*((char**)idents[i].value), "t1") == 0) {
                    /* A reply to 'comment' -- Send the 'data' field along to be parsed
                     * and add the result as a reply to the current comment */
                    reply = redditGetComment(parser, list);
                    redditCommentAddReply(comment, reply);
                } else if (strcmp(*((char**)idents[i].value), "more") == 0) {
                    parseTokens(parser, more_ids, list, comment);
                }
            } else {
                break;
            }
        }
    }
}

/*
 * Simple callback, which is called when we encounter a "replies" field.
 * It simply calls another callback everytime a 'data' key is found, which
 * may or may not refer to a reply or a RedditLink.
 */
DEF_TOKEN_CALLBACK(getCommentReplies)
{
    ARG_COMMENT_LISTING
    char *kindStr = NULL;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("kind", kindStr),
        ADD_TOKEN_IDENT_FUNC  ("data", getCommentListHelper),
        {0}
    };

    parseTokens(parser, ids, list, comment);
    free(kindStr);
}

DEF_TOKEN_CALLBACK(handleBodyText)
{
    ARG_COMMENT_LISTING
    (void)list;
    int len;

    free(comment->body);
    free(comment->bodyEsc);
    free(comment->wbodyEsc);

    comment->body = getCopyOfToken(parser->block->memory, parser->tokens[parser->currentToken]);

    len = strlen(comment->body);

    comment->bodyEsc  = redditParseEscCodes(comment->body, len);
    comment->wbodyEsc = redditParseEscCodesWide(comment->body, len);
}

/*
 * This code parses a JSON object to pull out a RedditComment. The 'getCommentReplies'
 * callback makes this all work, as it handles the case where 'replies' contains a
 * number of more RedditComment structures. That callback calls redditGetComment on
 * those objects in a recursive fashion to parse them.
 *
 * Beyond that, the list of TokenIdent's is fairly self-explainatory, it simply maps
 * the variables of a RedditComment to it's key in the JSON.
 */
RedditComment *redditGetComment(TokenParser *parser, RedditCommentList *list)
{
    RedditComment *comment = redditCommentNew();

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_FUNC  ("replies",       getCommentReplies),
        ADD_TOKEN_IDENT_STRING("author",        comment->author),
        ADD_TOKEN_IDENT_FUNC  ("body",          handleBodyText),
        ADD_TOKEN_IDENT_FUNC  ("contentText",   handleBodyText),
        ADD_TOKEN_IDENT_STRING("id",            comment->id),
        ADD_TOKEN_IDENT_INT   ("ups",           comment->ups),
        ADD_TOKEN_IDENT_INT   ("downs",         comment->downs),
        ADD_TOKEN_IDENT_INT   ("num_reports",   comment->numReports),
        ADD_TOKEN_IDENT_BOOL  ("edited",        comment->flags, REDDIT_COMMENT_EDITED),
        ADD_TOKEN_IDENT_BOOL  ("score_hidden",  comment->flags, REDDIT_COMMENT_SCORE_HIDDEN),
        ADD_TOKEN_IDENT_BOOL  ("distinguished", comment->flags, REDDIT_COMMENT_DISTINGUISHED),
        {0}
    };

    parseTokens(parser, ids, list, comment);

    return comment;
}

/*
 * TODO: implement sorting via the 'RedditCommentSortType' enum setting in a
 * RedditCommentList
 *
 * This function calls Reddit to get the list of comments on a link, and then stores them
 * in a RedditCommentList.
 */
EXPORT_SYMBOL RedditErrno redditGetCommentList (RedditCommentList *list)
{
    char fullLink[2048], *kindStr = NULL;
    TokenParserResult res;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("id",   list->id),
        ADD_TOKEN_IDENT_STRING("kind", kindStr),
        ADD_TOKEN_IDENT_FUNC  ("data", getCommentListHelper),
        {0}
    };

    strcpy(fullLink, "http://www.reddit.com");
    strcat(fullLink, list->permalink);
    strcat(fullLink, "/.json");

    if (list->baseComment == NULL)
        list->baseComment = redditCommentNew();

    res = redditRunParser(fullLink, NULL, ids, list, list->baseComment);

    free(kindStr);

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;
}

/*
 * This function makes a 'morechildren' API call and retrieves all the
 * children assosiated with 'parent'
 *
 * FIXME: Doesn't yet work correctly, it needs separate callbacks, the format
 * is slightly different then the getCommentListHelper callback is expecting.
 */
EXPORT_SYMBOL RedditErrno redditGetCommentChildren (RedditCommentList *list, RedditComment *parent)
{
    TokenParserResult res;
    char postText[4096], *kindStr = NULL;
    int i, children;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("kind", kindStr),
        ADD_TOKEN_IDENT_FUNC  ("data", getCommentListHelper),
        {0}
    };

    strcpy(postText, "api_type=json&children=");
    if (parent->totalReplyCount > 20)
        children = 20;
    else
        children = parent->totalReplyCount;

    if (children == 0)
        return REDDIT_ERROR;

    strcat(postText, parent->directChildrenIds[0]);
    for (i = 1; i < children; i++) {
        strcat(postText, ",");
        strcat(postText, parent->directChildrenIds[i]);
    }

    strcat(postText, "&link_id=t3_");
    strcat(postText, list->post->id);

    printf("Post text: %s\n", postText);

    res = redditRunParser("http://www.reddit.com/api/morechildren", postText, ids, list, parent);


    printf("Got here!\n");
    parent->totalReplyCount -= children;

    free(kindStr);

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;
}

#endif
