#ifndef _REDDIT_COMMENT_C_
#define _REDDIT_COMMENT_C_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "global.h"
#include "comment.h"
#include "token.h"

/*
 * Creates a new reddit_comment
 */
RedditComment *redditCommentNew ()
{
    RedditComment *comment;
    comment = rmalloc(sizeof(RedditComment));
    memset(comment, 0, sizeof(RedditComment));
    return comment;
}

/*
 * Recurrisivly free's all replys on a reddit_comment
 */
void redditCommentFreeReplies (RedditComment *comment)
{
    int i;
    for (i = 0; i < comment->replyCount; i++)
        redditCommentFree(comment->replies[i]);

    free(comment->replies);
}

/*
 * Frees all of a comment as well as all of it's replies
 */
void redditCommentFree (RedditComment *comment)
{
    int i;
    free(comment->id);
    free(comment->author);
    free(comment->parentId);
    free(comment->body);
    free(comment->wbody);
    for (i = 0; i < comment->directChildrenCount; i++)
        free(comment->directChildrenIds[i]);

    free(comment->directChildrenIds);
    free(comment->childrenId);
    redditCommentFreeReplies(comment);
    free(comment);
}

/*
 * Chains a comment as a reply on another comment.
 */
void redditCommentAddReply (RedditComment *comment, RedditComment *reply)
{
    comment->replyCount++;
    comment->replies = rrealloc(comment->replies, (comment->replyCount) * sizeof(RedditComment));
    comment->replies[comment->replyCount - 1] = reply;

}

RedditCommentList *redditCommentListNew ()
{
    RedditCommentList *list = rmalloc(sizeof(RedditCommentList));
    memset(list, 0, sizeof(RedditCommentList));
    return list;
}

void redditCommentListFree (RedditCommentList *list)
{
    redditCommentFree(list->baseComment);
    redditLinkFree(list->post);
    free(list->permalink);
    free(list->id);
    free(list);
}

#define ARG_COMMENT_LISTING \
    RedditCommentList *list = va_arg(args, RedditCommentList*); \
    RedditComment *comment = va_arg(args, RedditComment*);

/*
 * Not really sure there is a good way to avoid the fact that to parse the array
 * requires directly going over the tokens (Which you don't really want to have to do,
 * it's best to let the token_parser handle the actual parsing of the tokens).
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


RedditComment *redditGetComment(TokenParser *parser, RedditCommentList *list)
{
    RedditComment *comment = redditCommentNew();

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_FUNC  ("replies",       getCommentReplies),
        ADD_TOKEN_IDENT_STRING("author",        comment->author),
        ADD_TOKEN_IDENT_STRING("body",          comment->body),
        ADD_TOKEN_IDENT_STRING("contentText",   comment->body),
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

    /*
    tmp = comment->body;
    comment->body = redditParseEscCodes(tmp);
    comment->wbody = redditParseEscCodesWide(tmp);
    free(tmp);
    */

    return comment;
}

/*
 * TODO: implement sorting via the 'reddit_comment_sort_type' enum setting in a
 * reddit_comment_list
 */
RedditErrno redditGetCommentList (RedditCommentList *list)
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

    printf("Link: %s\n", fullLink);

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
 */
RedditErrno redditGetCommentChildren (RedditCommentList *list, RedditComment *parent)
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
