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
 * Recursively free's all replys on a RedditComment
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

static void redditCommentFreeChildren (RedditComment *comment)
{
    int i;
    if (comment == NULL || comment->directChildrenIds == NULL)
        return ;

    for (i = 0; i < comment->directChildrenCount; i++)
        free(comment->directChildrenIds[i]);

    free(comment->directChildrenIds);
    comment->directChildrenIds = NULL;
}

/*
 * Frees all of a RedditComment as well as all of it's replies
 */
EXPORT_SYMBOL void redditCommentFree (RedditComment *comment)
{
    if (comment == NULL)
        return ;

    free(comment->body);
    free(comment->bodyEsc);
    free(comment->wbodyEsc);
    free(comment->id);
    free(comment->author);
    free(comment->parentId);
    free(comment->linkId);
    redditCommentFreeChildren(comment);
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
    RedditComment* ptr = comment;
    while (ptr != NULL) {
        ptr->totalReplyCount += reply->totalReplyCount + 1;
        ptr = ptr->parent;
    }
    comment->replies = rrealloc(comment->replies, (comment->replyCount) * sizeof(RedditComment*));
    comment->replies[comment->replyCount - 1] = reply;
    reply->parent = comment;
}

/*
 * Finds a comment in the list of comments who's id matches the parentId we're looking for
 * Note: Only checks linear parents from directParent. Could be improved to check every reply in the true
 * But that could be heavy for larger trees.
 */
static RedditComment *redditCommentFindParent (RedditComment *base, char *parentId)
{
    char pid[8];
    RedditComment *current;

    int stackSize = 100;
    RedditComment **stack = rmalloc(sizeof(RedditComment*) * stackSize);;
    int top = 0;

    /* We might have, Ex. 't1_idvalu', we just want 'idvalu' from that */
    if (parentId[2] == '_')
        strcpy(pid, parentId + 3);
    else
        strcpy(pid, parentId);

    stack[0] = base;
    top = 0;
    /* Keep looping and decrementing our stack index until it's under zero */
    for(; top >= 0; top--) {

        /* Grab the RedditComment on the top of the stack */
        current = stack[top];

        /* If it's the right one, jump to the cleanup code */
        if (strcmp(pid, current->id) == 0)
            goto cleanup;

        /* If it's not and it has some replies to it, add all the replies to
         * the stack and keep going */
        if (current->replyCount > 0) {
            if (top + current->replyCount >= stackSize) {
                stackSize += current->replyCount;
                stack = realloc(stack, sizeof(RedditComment*) * stackSize);
            }
            /* Copy the contents of the replies array right onto the stack. */
            memcpy(stack + top, current->replies, current->replyCount * sizeof(RedditComment*));
            top += current->replyCount;
        }
    }

    current = NULL;

cleanup:;
    free(stack);
    return current;
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

static void redditCommentAddMore (TokenParser *parser, RedditComment *parent)
{
    int i;

    redditCommentFreeChildren(parent);

    parent->directChildrenCount = parser->tokens[parser->currentToken].full_size;
    parent->directChildrenIds = rmalloc(parent->directChildrenCount * sizeof(char*));

    for (i = 0; i < parent->directChildrenCount; i++) {
        parser->currentToken++;
        parent->directChildrenIds[i] = getCopyOfToken(parser->block->memory, parser->tokens[parser->currentToken]);
    }
}

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

    redditCommentAddMore(parser, comment);
}

/*
 * This callback is sort of a 'dispatch' which simply delegates what to do to the relevant functions
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
                    redditLinkFree(list->post);
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
        ADD_TOKEN_IDENT_FUNC    ("replies",       getCommentReplies),
        ADD_TOKEN_IDENT_STRING  ("author",        comment->author),
        ADD_TOKEN_IDENT_STRPARSE("body",          comment->body, comment->bodyEsc, comment->wbodyEsc),
        ADD_TOKEN_IDENT_STRPARSE("contentText",   comment->body, comment->bodyEsc, comment->wbodyEsc),
        ADD_TOKEN_IDENT_STRING  ("id",            comment->id),
        ADD_TOKEN_IDENT_STRING  ("link_id",       comment->linkId),
        ADD_TOKEN_IDENT_STRING  ("parent_id",     comment->parentId),
        ADD_TOKEN_IDENT_INT     ("ups",           comment->ups),
        ADD_TOKEN_IDENT_INT     ("downs",         comment->downs),
        ADD_TOKEN_IDENT_INT     ("num_reports",   comment->numReports),
        ADD_TOKEN_IDENT_BOOL    ("edited",        comment->flags, REDDIT_COMMENT_EDITED),
        ADD_TOKEN_IDENT_BOOL    ("score_hidden",  comment->flags, REDDIT_COMMENT_SCORE_HIDDEN),
        ADD_TOKEN_IDENT_BOOL    ("distinguished", comment->flags, REDDIT_COMMENT_DISTINGUISHED),
        ADD_TOKEN_IDENT_DATE    ("created_utc",   comment->created_utc),
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

    strcpy(fullLink, REDDIT_URL);
    strcat(fullLink, list->permalink);
    strcat(fullLink, REDDIT_JSON);

    if (list->baseComment == NULL)
        list->baseComment = redditCommentNew();

    res = redditRunParser(fullLink, NULL, ids, list, list->baseComment);

    free(kindStr);

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;
}

/* Small structure for holding data on a 'more' object from the morechildren call */
struct MoreChildren {
    char *parent;
    int  count;
};

DEF_TOKEN_CALLBACK(readMore)
{
    RedditComment *base        = va_arg(args, RedditComment*);
    struct MoreChildren *child = va_arg(args, struct MoreChildren*);


    RedditComment *parent = redditCommentFindParent(base, child->parent);

    if (parent == NULL) {
        /* If we didn't find parent, we just advance past the array */
        parser->currentToken += parser->tokens[parser->currentToken].full_size;
    } else {
        redditCommentAddMore(parser, parent);
        parent->totalReplyCount = child->count;
    }

}

#define ARG_MORECHILDREN \
    RedditCommentList *list = va_arg(args, RedditCommentList*); \
    RedditComment *parent = va_arg(args, RedditComment*);

DEF_TOKEN_CALLBACK(readChild)
{
    ARG_MORECHILDREN

    struct MoreChildren child = {0};
    RedditComment *comment, *foundParent;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("parent_id",child.parent),
        ADD_TOKEN_IDENT_INT   ("count",    child.count),
        ADD_TOKEN_IDENT_FUNC  ("children", readMore),
        {0}
    };

    /* Note: Requires 'kind' to be the first key in the ids array */
    if (strcmp(*((char**)idents[0].value), "more") == 0) {
        parseTokens(parser, ids, parent, &child);
        free(child.parent);
    } else {
        comment = redditGetComment(parser, list);

        foundParent = redditCommentFindParent(parent, comment->parentId);
        if (foundParent == NULL)
            free(foundParent);
        else
            redditCommentAddReply(foundParent, comment);
    }
}

DEF_TOKEN_CALLBACK(getMoreChildren)
{
    ARG_MORECHILDREN
    char *kind = NULL;
    int arry_size = parser->tokens[parser->currentToken].size;
    int i = 0;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("kind", kind), /* Note -- Keep this key first in
                                                 the array, or change the
                                                 number in readChild */
        ADD_TOKEN_IDENT_FUNC("data", readChild),
        {0}
    };

    parser->currentToken++;
    for (; i < arry_size; i++) {
        int nextObj = parser->currentToken + parser->tokens[parser->currentToken].full_size + 1;
        parseTokens(parser, ids, list, parent);
        free(kind);
        kind = NULL;
        parser->currentToken = nextObj;
    }
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
    char postText[4096];
    int i, endCount, children;
    int preCheck;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_FUNC  ("things", getMoreChildren),
        {0}
    };

    sprintf(postText, "api_type=json&link_id=t3_%s&children=", list->post->id);
    if (parent->directChildrenCount > 20)
        children = 20;
    else
        children = parent->directChildrenCount;

    if (children == 0)
        return REDDIT_ERROR;

    strcat(postText, parent->directChildrenIds[0]);
    for (i = 1; i < children; i++) {
        strcat(postText, ",");
        strcat(postText, parent->directChildrenIds[i]);
    }

    preCheck = parent->totalReplyCount;

    res = redditRunParser(REDDIT_API_MORECHILDREN, postText, ids, list, parent);

    if (preCheck == parent->totalReplyCount)
        parent->totalReplyCount -= children;

    endCount = parent->directChildrenCount - children;
    for (i = parent->directChildrenCount - 1; i >= endCount; i--)
        free(parent->directChildrenIds[i]);

    parent->directChildrenCount = endCount;

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;
}

#endif
