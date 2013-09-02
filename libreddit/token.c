#ifndef _REDDIT_TOKEN_C_
#define _REDDIT_TOKEN_C_

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "global.h"
#include "token.h"
#include "cookie.h"

/*
 * Returns a pointer to valid newalized memory_block
 */
memory_block *memory_block_new()
{
    memory_block *block = rmalloc(sizeof(memory_block));
    block->memory = rmalloc(1);
    block->memory[0] = 0;
    block->size   = 0;
    return block;
}

/*
 * Frees a memory_block returned by memory_block_new()
 */
void memory_block_free(memory_block *block)
{
    free(block->memory);
    free(block);
}


token_parser *token_parser_new()
{
    token_parser *parser = rmalloc(sizeof(token_parser));
    memset(parser, 0, sizeof(token_parser));
    parser->block = memory_block_new();
    return parser;
}

void token_parser_free(token_parser *parser)
{
    memory_block_free(parser->block);
    free(parser->tokens);
    free(parser);
}

jsmnerr_t token_parser_create_tokens(token_parser *parser)
{
    jsmn_parser jsmn_parser;
    jsmnerr_t result;
    const int chunk_size = 100;

    if (parser->tokens == NULL) {
        parser->tokens = rmalloc(chunk_size * sizeof(jsmntok_t));
        parser->token_count = chunk_size;
    }

    memset(parser->tokens, 0, parser->token_count * sizeof(jsmntok_t));
    jsmn_init(&jsmn_parser);

    while ((result = jsmn_parse(&jsmn_parser, parser->block->memory, parser->tokens, parser->token_count)) == JSMN_ERROR_NOMEM) {
        parser->token_count += chunk_size;
        parser->tokens = rrealloc(parser->tokens, parser->token_count * sizeof(jsmntok_t));
        memset(parser->tokens + parser->token_count - chunk_size, 0, chunk_size * sizeof(jsmntok_t));
    }

    if (result != JSMN_SUCCESS)
        parser->token_count = 0;
    else
        parser->token_count = jsmn_parser.toknext - 1;

    return result;
}

/*
 * Returns a allocated copy of the contents of a token.
 * Handy since it's a pretty common need.
 */
char *get_copy_of_token(const char *json, jsmntok_t token)
{
    char *copy;
    if (token.end - token.start + 1 < 1)
        return NULL;

    copy = rmalloc(token.end - token.start + 1);
    memcpy(copy, json + token.start, token.end - token.start);
    copy[token.end - token.start] = 0;
    return copy;
}

size_t write_to_parser(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    token_parser *parser = (token_parser*)userp;

    parser->block->memory = rrealloc(parser->block->memory, parser->block->size + realsize + 1);

    memcpy(parser->block->memory + parser->block->size, contents, realsize);
    parser->block->size += realsize;
    parser->block->memory[parser->block->size] = 0;

    return realsize;
}

/*
 * Returns either 'True' or 'False' in string form of a bool value
 *
 * The return value is always the exact same pointer of 'string'
 * Note -- 'string' should be already allocated to atleast a length of 6
 */
char *true_false_string(char *string, bool tf)
{
    if (tf) {
        strcpy(string, "true");
    } else {
        strcpy(string, "false");
    }
    return string;
}

char *reddit_copy_string(const char *string)
{
    char *new_str = rmalloc(strlen(string) + 1);
    strcpy(new_str, string);
    return new_str;
}

#define CALL_TOKEN_FUNC(func, parser, idents, args)              \
    do {                                                         \
        va_list args_copy;                                       \
        va_copy(args_copy, args);                                \
        (func) (parser, idents, args_copy);                      \
        va_end(args_copy);                                       \
    } while (0)

int perform_ident_action(token_parser *p, token_ident *identifiers, int i, va_list args)
{
    char *tmp = NULL;
    int performed_action = 0;

    READ_TOKEN_TO_TMP(p->block->memory, tmp, p->tokens[p->current_token]);

    if (TOKEN_EQUALS(tmp, identifiers[i].name)) {
        switch(identifiers[i].action) {
        case TOKEN_CHECK_CALL:
            (p->current_token)++;
            CALL_TOKEN_FUNC(identifiers[i].func_callback, p, identifiers, args);
            performed_action = 1;
            break;

        case TOKEN_SET:
            (p->current_token)++;
            READ_TOKEN_TO_TMP(p->block->memory, tmp, p->tokens[p->current_token]);
            switch(identifiers[i].type) {
            case TOKEN_BOOL:
                if (TOKEN_IS_TRUE(p->block->memory, p->tokens[p->current_token]))
                    *((unsigned int *)identifiers[i].value) |= identifiers[i].bit_mask;
                else
                    *((unsigned int *)identifiers[i].value) &= ~identifiers[i].bit_mask;

                break;
            case TOKEN_INT:
                *((int*)(identifiers[i].value)) = atoi(tmp);
                break;
            case TOKEN_STRING:
            case TOKEN_OBJECT:
                if (identifiers[i].free_flag)
                    free(*((char**)identifiers[i].value));

                *((char**)identifiers[i].value) = get_copy_of_token(p->block->memory, p->tokens[p->current_token]);
                break;
            }
            performed_action = 1;

            break;
        }
    }

    free(tmp);
    return performed_action;
}

/*
 * This function parses a json object, looking for tokens and calling the proper
 * function callback if nessisary
 *
 * When called, this function will assume tokens[0] points to the start of an object, and
 * will return if this is not the case
 *
 * The intention is that the 'tokens' pointer will be incremented to the object you want to
 * parse when calling this
 */
void vparse_tokens (token_parser *p, token_ident *identifiers, va_list args)
{
    int token_count = 0, i;
    int ident_count = 0;

    while (identifiers[ident_count].name != NULL) {
        ident_count++;
    }

    if (ident_count == 0)
        return ;

    token_count = p->tokens[p->current_token].full_size;
    if (token_count == 0)
        return ;

    token_count += p->current_token;

    for (; p->current_token < token_count; (p->current_token)++) {
        if (p->tokens[p->current_token].type == JSMN_OBJECT || p->tokens[p->current_token].type == JSMN_ARRAY)
            continue;

        /* We only need to check key values */
        if (!p->tokens[p->current_token].is_key)
            continue;

        for (i = 0; i < ident_count; i++)
            if (perform_ident_action(p, identifiers, i, args))
                break;

    }

    return ;
}

void parse_tokens (token_parser *parser, token_ident *identifiers, ...)
{
    va_list args;
    va_start(args, identifiers);
    vparse_tokens (parser, identifiers, args);
    va_end(args);
}


token_parser_result redditv_run_parser(char *url, char *post, token_ident *idents, va_list args)
{
    token_parser *parser = token_parser_new();
    char *cookie_str = NULL;
    CURL *reddit_handle = curl_easy_init();
    jsmnerr_t jsmn_result;

    token_parser_result result = TOKEN_PARSER_SUCCESS;

    curl_easy_setopt(reddit_handle, CURLOPT_URL, url);
    curl_easy_setopt(reddit_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(reddit_handle, CURLOPT_WRITEFUNCTION, write_to_parser);
    curl_easy_setopt(reddit_handle, CURLOPT_WRITEDATA, (void *)parser);

    cookie_str = reddit_get_cookie_string();
    if (cookie_str != NULL)
        curl_easy_setopt(reddit_handle, CURLOPT_COOKIE, cookie_str);

    if (post != NULL) {
        curl_easy_setopt(reddit_handle, CURLOPT_POST, 1);
        curl_easy_setopt(reddit_handle, CURLOPT_POSTFIELDS, (void *)post);
    }

    curl_easy_setopt(reddit_handle, CURLOPT_USERAGENT, CREDDIT_USERAGENT);

    curl_easy_perform(reddit_handle);
    curl_easy_cleanup(reddit_handle);

    if (parser->block->size <= 0 || parser->block->memory == NULL) {
        result = TOKEN_PARSER_CURL_FAIL;
        goto cleanup;
    }

    if (strcmp(url, "http://www.reddit.com/api/morechildren") == 0) {
        printf("Stuff: %s\n", parser->block->memory);
    }

    jsmn_result = token_parser_create_tokens(parser);

    if (jsmn_result != JSMN_SUCCESS) {
        result = TOKEN_PARSER_JSON_FAIL;
        goto cleanup;
    }

    vparse_tokens(parser, idents, args);

    result = TOKEN_PARSER_SUCCESS;



cleanup:;

    free(cookie_str);
    token_parser_free(parser);

    return result;
}

token_parser_result reddit_run_parser(char *url, char *post, token_ident *idents, ...)
{
    token_parser_result result;
    va_list args;
    va_start(args, idents);
    result = redditv_run_parser(url, post, idents, args);
    va_end(args);
    return result;
}



#endif
