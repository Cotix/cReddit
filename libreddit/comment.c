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
reddit_comment *reddit_comment_new ()
{
    reddit_comment *comment;
    comment = rmalloc(sizeof(reddit_comment));
    memset(comment, 0, sizeof(reddit_comment));
    return comment;
}

/*
 * Recurrisivly free's all replys on a reddit_comment
 */
void reddit_comment_free_replies (reddit_comment *comment)
{
    int i;
    for (i = 0; i < comment->reply_count; i++)
        reddit_comment_free(comment->replies[i]);

    free(comment->replies);
}

/*
 * Frees all of a comment as well as all of it's replies
 */
void reddit_comment_free (reddit_comment *comment)
{
    int i;
    free(comment->id);
    free(comment->author);
    free(comment->parent_id);
    free(comment->body);
    for (i = 0; i < comment->direct_children_count; i++)
        free(comment->direct_children_ids[i]);

    free(comment->direct_children_ids);
    free(comment->children_id);
    reddit_comment_free_replies(comment);
    free(comment);
}

/*
 * Chains a comment as a reply on another comment.
 */
void reddit_comment_add_reply (reddit_comment *comment, reddit_comment *reply)
{
    comment->reply_count++;
    comment->replies = rrealloc(comment->replies, (comment->reply_count) * sizeof(reddit_comment));
    comment->replies[comment->reply_count - 1] = reply;

}

reddit_comment_list *reddit_comment_list_new ()
{
    reddit_comment_list *list = rmalloc(sizeof(reddit_comment_list));
    memset(list, 0, sizeof(reddit_comment_list));
    return list;
}

void reddit_comment_list_free (reddit_comment_list *list)
{
    reddit_comment_free(list->base_comment);
    reddit_link_free(list->post);
    free(list->perm_link);
    free(list->id);
    free(list);
}

#define ARG_COMMENT_LISTING \
    reddit_comment_list *list = va_arg(args, reddit_comment_list*); \
    reddit_comment *comment = va_arg(args, reddit_comment*);

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

    comment->direct_children_count = parser->tokens[parser->current_token].full_size;
    comment->direct_children_ids = rmalloc(comment->direct_children_count * sizeof(char*));

    for (i = 0; i < comment->direct_children_count; i++) {
        parser->current_token++;
        comment->direct_children_ids[i] = get_copy_of_token(parser->block->memory, parser->tokens[parser->current_token]);
    }
}

DEF_TOKEN_CALLBACK(get_comment_list_helper)
{
    ARG_COMMENT_LISTING
    reddit_comment *reply = NULL;

    token_ident more_ids[] = {
        ADD_TOKEN_IDENT_INT   ("count",    comment->total_reply_count),
        ADD_TOKEN_IDENT_STRING("id",       comment->children_id),
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

    token_ident ids[] = {
        ADD_TOKEN_IDENT_STRING("kind", kind_str),
        ADD_TOKEN_IDENT_FUNC  ("data", get_comment_list_helper),
        {0}
    };

    parse_tokens(parser, ids, list, comment);
    free(kind_str);
}


reddit_comment *reddit_get_comment(token_parser *parser, reddit_comment_list *list)
{
    reddit_comment *comment = reddit_comment_new();

    token_ident ids[] = {
        ADD_TOKEN_IDENT_FUNC  ("replies",       get_comment_replies),
        ADD_TOKEN_IDENT_STRING("author",        comment->author),
        ADD_TOKEN_IDENT_STRING("body",          comment->body),
        ADD_TOKEN_IDENT_STRING("contentText",   comment->body),
        ADD_TOKEN_IDENT_STRING("id",            comment->id),
        ADD_TOKEN_IDENT_INT   ("ups",           comment->ups),
        ADD_TOKEN_IDENT_INT   ("downs",         comment->downs),
        ADD_TOKEN_IDENT_INT   ("num_reports",   comment->num_reports),
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
reddit_errno reddit_get_comment_list (reddit_comment_list *list)
{
    char full_link[2048], *kind_str = NULL;
    token_parser_result res;

    token_ident ids[] = {
        ADD_TOKEN_IDENT_STRING("id",   list->id),
        ADD_TOKEN_IDENT_STRING("kind", kind_str),
        ADD_TOKEN_IDENT_FUNC  ("data", get_comment_list_helper),
        {0}
    };

    strcpy(full_link, "http://www.reddit.com");
    strcat(full_link, list->perm_link);
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
reddit_errno reddit_get_comment_children (reddit_comment_list *list, reddit_comment *parent)
{
    token_parser_result res;
    char post_text[4096], *kind_str = NULL;
    int i, children;

    token_ident ids[] = {
        ADD_TOKEN_IDENT_STRING("kind", kind_str),
        ADD_TOKEN_IDENT_FUNC  ("data", get_comment_list_helper),
        {0}
    };

    strcpy(post_text, "api_type=json&children=");
    if (parent->total_reply_count > 20)
        children = 20;
    else
        children = parent->total_reply_count;

    if (children == 0)
        return REDDIT_ERROR;

    strcat(post_text, parent->direct_children_ids[0]);
    for (i = 1; i < children; i++) {
        strcat(post_text, ",");
        strcat(post_text, parent->direct_children_ids[i]);
    }

    strcat(post_text, "&link_id=t3_");
    strcat(post_text, list->post->id);

    printf("Post text: %s\n", post_text);

    res = reddit_run_parser("http://www.reddit.com/api/morechildren", post_text, ids, list, parent);


    printf("Got here!\n");
    parent->total_reply_count -= children;

    free(kind_str);

    if (res == TOKEN_PARSER_SUCCESS)
        return REDDIT_SUCCESS;
    else
        return REDDIT_ERROR_RESPONSE;
}

#endif
