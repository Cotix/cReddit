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
RedditComment *reddit_comment_new ()
{
    RedditComment *comment;
    comment = rmalloc(sizeof(RedditComment));
    memset(comment, 0, sizeof(RedditComment));
    return comment;
}

/*
 * Recurrisivly free's all replys on a reddit_comment
 */
void reddit_comment_free_replies (RedditComment *comment)
{
    int i;
    for (i = 0; i < comment->replyCount; i++)
        reddit_comment_free(comment->replies[i]);

    free(comment->replies);
}

/*
 * Frees all of a comment as well as all of it's replies
 */
void reddit_comment_free (RedditComment *comment)
{
    int i;
    free(comment->id);
    free(comment->author);
    free(comment->parentId);
    free(comment->body);
    for (i = 0; i < comment->directChildrenCount; i++)
        free(comment->directChildrenIds[i]);

    free(comment->directChildrenIds);
    free(comment->childrenId);
    reddit_comment_free_replies(comment);
    free(comment);
}

/*
 * Chains a comment as a reply on another comment.
 */
void reddit_comment_add_reply (RedditComment *comment, RedditComment *reply)
{
    comment->replyCount++;
    comment->replies = rrealloc(comment->replies, (comment->replyCount) * sizeof(RedditComment));
    comment->replies[comment->replyCount - 1] = reply;

}

RedditCommentList *reddit_comment_list_new ()
{
    RedditCommentList *list = rmalloc(sizeof(RedditCommentList));
    memset(list, 0, sizeof(RedditCommentList));
    return list;
}

void reddit_comment_list_free (RedditCommentList *list)
{
    reddit_comment_free(list->base_comment);
    reddit_link_free(list->post);
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
DEF_TOKEN_CALLBACK(get_comment_replies_more)
{
    ARG_COMMENT_LISTING
    (void)list; /* Make the compiler shut-up */
    int i;

    comment->directChildrenCount = parser->tokens[parser->currentToken].full_size;
    comment->directChildrenIds = rmalloc(comment->directChildrenCount * sizeof(char*));

    for (i = 0; i < comment->directChildrenCount; i++) {
        parser->currentToken++;
        comment->directChildrenIds[i] = get_copy_of_token(parser->block->memory, parser->tokens[parser->currentToken]);
    }
}

DEF_TOKEN_CALLBACK(get_comment_list_helper)
{
    ARG_COMMENT_LISTING
    RedditComment *reply = NULL;

    TokenIdent more_ids[] = {
        ADD_TOKEN_IDENT_INT   ("count",    comment->totalReplyCount),
        ADD_TOKEN_IDENT_STRING("id",       comment->childrenId),
        ADD_TOKEN_IDENT_FUNC  ("children", get_comment_replies_more),
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
                    list->post = reddit_get_link(parser);
                } else if (strcmp(*((char**)idents[i].value), "t1") == 0) {
                    /* A reply to 'comment' -- Send the 'data' field along to be parsed
                     * and add the result as a reply to the current comment */
                    reply = reddit_get_comment(parser, list);
                    reddit_comment_add_reply(comment, reply);
                } else if (strcmp(*((char**)idents[i].value), "more") == 0) {
                    parse_tokens(parser, more_ids, list, comment);
                }
            } else {
                break;
            }
        }
    }
}

DEF_TOKEN_CALLBACK(get_comment_replies)
{
    ARG_COMMENT_LISTING
    char *kind_str = NULL;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("kind", kind_str),
        ADD_TOKEN_IDENT_FUNC  ("data", get_comment_list_helper),
        {0}
    };

    parse_tokens(parser, ids, list, comment);
    free(kind_str);
}


RedditComment *reddit_get_comment(TokenParser *parser, RedditCommentList *list)
{
    RedditComment *comment = reddit_comment_new();

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_FUNC  ("replies",       get_comment_replies),
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

    parse_tokens(parser, ids, list, comment);

    return comment;
}

/* 
 * TODO: implement sorting via the 'reddit_comment_sort_type' enum setting in a
 * reddit_comment_list
 */
RedditErrno reddit_get_comment_list (RedditCommentList *list)
{
    char full_link[2048], *kind_str = NULL;
    TokenParserResult res;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("id",   list->id),
        ADD_TOKEN_IDENT_STRING("kind", kind_str),
        ADD_TOKEN_IDENT_FUNC  ("data", get_comment_list_helper),
        {0}
    };

    strcpy(full_link, "http://www.reddit.com");
    strcat(full_link, list->permalink);
    strcat(full_link, "/.json");

    printf("Link: %s\n", full_link);

    if (list->base_comment == NULL)
        list->base_comment = reddit_comment_new();

    res = reddit_run_parser(full_link, NULL, ids, list, list->base_comment);

    free(kind_str);

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;
}

/*
 * This function makes a 'morechildren' API call and retrieves all the
 * children assosiated with 'parent'
 */
RedditErrno reddit_get_comment_children (RedditCommentList *list, RedditComment *parent)
{
    TokenParserResult res;
    char post_text[4096], *kind_str = NULL;
    int i, children;

    TokenIdent ids[] = {
        ADD_TOKEN_IDENT_STRING("kind", kind_str),
        ADD_TOKEN_IDENT_FUNC  ("data", get_comment_list_helper),
        {0}
    };

    strcpy(post_text, "api_type=json&children=");
    if (parent->totalReplyCount > 20)
        children = 20;
    else
        children = parent->totalReplyCount;

    if (children == 0)
        return REDDIT_ERROR;

    strcat(post_text, parent->directChildrenIds[0]);
    for (i = 1; i < children; i++) {
        strcat(post_text, ",");
        strcat(post_text, parent->directChildrenIds[i]);
    }

    strcat(post_text, "&link_id=t3_");
    strcat(post_text, list->post->id);

    printf("Post text: %s\n", post_text);

    res = reddit_run_parser("http://www.reddit.com/api/morechildren", post_text, ids, list, parent);


    printf("Got here!\n");
    parent->totalReplyCount -= children;

    free(kind_str);

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;
}

#endif
