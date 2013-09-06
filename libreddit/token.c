#ifndef _REDDIT_TOKEN_C_
#define _REDDIT_TOKEN_C_

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <wchar.h>

#include "global.h"
#include "token.h"
#include "cookie.h"

/*
 * Returns a pointer to valid newalized memory_block
 */
MemoryBlock *memoryBlockNew()
{
    MemoryBlock *block = rmalloc(sizeof(MemoryBlock));
    block->memory = rmalloc(1);
    block->memory[0] = 0;
    block->size   = 0;
    return block;
}

/*
 * Frees a memory_block returned by memory_block_new()
 */
void memoryBlockFree(MemoryBlock *block)
{
    free(block->memory);
    free(block);
}


TokenParser *tokenParserNew()
{
    TokenParser *parser = rmalloc(sizeof(TokenParser));
    memset(parser, 0, sizeof(TokenParser));
    parser->block = memoryBlockNew();
    return parser;
}

void tokenParserFree(TokenParser *parser)
{
    memoryBlockFree(parser->block);
    free(parser->tokens);
    free(parser);
}

jsmnerr_t tokenParserCreateTokens(TokenParser *parser)
{
    jsmn_parser jsmnParser;
    jsmnerr_t result;
    const int chunk_size = 100;

    if (parser->tokens == NULL) {
        parser->tokens = rmalloc(chunk_size * sizeof(jsmntok_t));
        parser->tokenCount = chunk_size;
    }

    memset(parser->tokens, 0, parser->tokenCount * sizeof(jsmntok_t));
    jsmn_init(&jsmnParser);

    while ((result = jsmn_parse(&jsmnParser, parser->block->memory, parser->tokens, parser->tokenCount)) == JSMN_ERROR_NOMEM) {
        parser->tokenCount += chunk_size;
        parser->tokens = rrealloc(parser->tokens, parser->tokenCount * sizeof(jsmntok_t));
        memset(parser->tokens + parser->tokenCount - chunk_size, 0, chunk_size * sizeof(jsmntok_t));
    }

    if (result != JSMN_SUCCESS)
        parser->tokenCount = 0;
    else
        parser->tokenCount = jsmnParser.toknext - 1;

    return result;
}

/*
 * Returns a allocated copy of the contents of a token.
 * Handy since it's a pretty common need.
 */
char *getCopyOfToken(const char *json, jsmntok_t token)
{
    char *copy;
    if (token.end - token.start + 1 < 1)
        return NULL;

    copy = rmalloc(token.end - token.start + 1);
    memcpy(copy, json + token.start, token.end - token.start);
    copy[token.end - token.start] = 0;
    return copy;
}

size_t writeToParser(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    TokenParser *parser = (TokenParser*)userp;

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
char *trueFalseString(char *string, bool tf)
{
    if (tf) {
        strcpy(string, "true");
    } else {
        strcpy(string, "false");
    }
    return string;
}

char *redditCopyString(const char *string)
{
    char *newStr = rmalloc(strlen(string) + 1);
    strcpy(newStr, string);
    return newStr;
}

#define CALL_TOKEN_FUNC(func, parser, idents, args)              \
    do {                                                         \
        va_list argsCopy;                                       \
        va_copy(argsCopy, args);                                \
        (func) (parser, idents, argsCopy);                      \
        va_end(argsCopy);                                       \
    } while (0)

int performIdentAction(TokenParser *p, TokenIdent *identifiers, int i, va_list args)
{
    char *tmp = NULL;
    int performedAction = 0;

    READ_TOKEN_TO_TMP(p->block->memory, tmp, p->tokens[p->currentToken]);

    if (TOKEN_EQUALS(tmp, identifiers[i].name)) {
        switch(identifiers[i].action) {
        case TOKEN_CHECK_CALL:
            (p->currentToken)++;
            CALL_TOKEN_FUNC(identifiers[i].funcCallback, p, identifiers, args);
            performedAction = 1;
            break;

        case TOKEN_SET:
            (p->currentToken)++;
            READ_TOKEN_TO_TMP(p->block->memory, tmp, p->tokens[p->currentToken]);
            switch(identifiers[i].type) {
            case TOKEN_BOOL:
                if (TOKEN_IS_TRUE(p->block->memory, p->tokens[p->currentToken]))
                    *((unsigned int *)identifiers[i].value) |= identifiers[i].bitMask;
                else
                    *((unsigned int *)identifiers[i].value) &= ~identifiers[i].bitMask;

                break;
            case TOKEN_INT:
                *((int*)(identifiers[i].value)) = atoi(tmp);
                break;
            case TOKEN_STRING:
            case TOKEN_OBJECT:
                if (identifiers[i].freeFlag)
                    free(*((char**)identifiers[i].value));

                *((char**)identifiers[i].value) = getCopyOfToken(p->block->memory, p->tokens[p->currentToken]);
                break;
            }
            performedAction = 1;

            break;
        }
    }

    free(tmp);
    return performedAction;
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
void vparseTokens (TokenParser *p, TokenIdent *identifiers, va_list args)
{
    int tokenCount = 0, i;
    int identCount = 0;

    while (identifiers[identCount].name != NULL) {
        identCount++;
    }

    if (identCount == 0)
        return ;

    tokenCount = p->tokens[p->currentToken].full_size;
    if (tokenCount == 0)
        return ;

    tokenCount += p->currentToken;

    for (; p->currentToken < tokenCount; (p->currentToken)++) {
        if (p->tokens[p->currentToken].type == JSMN_OBJECT || p->tokens[p->currentToken].type == JSMN_ARRAY)
            continue;

        /* We only need to check key values */
        if (!p->tokens[p->currentToken].is_key)
            continue;

        for (i = 0; i < identCount; i++)
            if (performIdentAction(p, identifiers, i, args))
                break;

    }

    return ;
}

void parseTokens (TokenParser *parser, TokenIdent *identifiers, ...)
{
    va_list args;
    va_start(args, identifiers);
    vparseTokens (parser, identifiers, args);
    va_end(args);
}


TokenParserResult redditvRunParser(char *url, char *post, TokenIdent *idents, va_list args)
{
    TokenParser *parser = tokenParserNew();
    char *cookieStr = NULL;
    CURL *redditHandle = curl_easy_init();
    jsmnerr_t jsmnResult;

    TokenParserResult result = TOKEN_PARSER_SUCCESS;

    curl_easy_setopt(redditHandle, CURLOPT_URL, url);
    curl_easy_setopt(redditHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(redditHandle, CURLOPT_WRITEFUNCTION, writeToParser);
    curl_easy_setopt(redditHandle, CURLOPT_WRITEDATA, (void *)parser);

    cookieStr = redditGetCookieString();
    if (cookieStr != NULL)
        curl_easy_setopt(redditHandle, CURLOPT_COOKIE, cookieStr);

    if (post != NULL) {
        curl_easy_setopt(redditHandle, CURLOPT_POST, 1);
        curl_easy_setopt(redditHandle, CURLOPT_POSTFIELDS, (void *)post);
    }

    curl_easy_setopt(redditHandle, CURLOPT_USERAGENT, CREDDIT_USERAGENT);

    curl_easy_perform(redditHandle);
    curl_easy_cleanup(redditHandle);

    if (parser->block->size <= 0 || parser->block->memory == NULL) {
        result = TOKEN_PARSER_CURL_FAIL;
        goto cleanup;
    }

    if (strcmp(url, "http://www.reddit.com/api/morechildren") == 0) {
        printf("Stuff: %s\n", parser->block->memory);
    }

    jsmnResult = tokenParserCreateTokens(parser);

    if (jsmnResult != JSMN_SUCCESS) {
        result = TOKEN_PARSER_JSON_FAIL;
        goto cleanup;
    }

    vparseTokens(parser, idents, args);

    result = TOKEN_PARSER_SUCCESS;



cleanup:;

    free(cookieStr);
    tokenParserFree(parser);

    return result;
}

TokenParserResult redditRunParser(char *url, char *post, TokenIdent *idents, ...)
{
    TokenParserResult result;
    va_list args;
    va_start(args, idents);
    result = redditvRunParser(url, post, idents, args);
    va_end(args);
    return result;
}



char *redditParseEscCodes (const char *text)
{
    int i, len = strlen(text), offset = 0;
    char *new_text = rmalloc(len + 1);

    if (text == NULL)
        return NULL;

    for (i = 0; i < len; i++) {
        switch(text[i]) {
        case '\\':
            i++;
            switch(text[i]) {
            case 'n':
                new_text[offset] = '\n';
                offset++;
                break;
            case 'u':
                i+=4;
                break;
            }
            break;
        default:
            new_text[offset] = text[i];
            offset++;
        }
    }
    new_text[offset] = '\0';
    return new_text;
}

wchar_t *redditParseEscCodesWide (const char *text)
{
    int i, k, len = strlen(text), offset = 0;
    wchar_t *new_text = rmalloc((len + 1) * sizeof(wchar_t));
    wchar_t temp;

    if (text == NULL)
        return NULL;

    for (i = 0; i < len; i++) {
        switch(text[i]) {
        case L'\\':
            i++;
            switch(text[i]) {
            case 'n':
                new_text[offset] = L'\n';
                offset++;
                break;
            case 'u':
                temp = L'\0';
                for (k = 3; k >= 0; k--) {
                    i++;
                    /* This weird piece of code converts a hex character (0-9 and a-f) into
                     * it's decimal equivelent, and then shits it over the correct number of
                     * bits coresponding to it's position in the wchar_t */
                    temp |= (((text[i] < 58)? text[i] - 48: ((text[i] & 0x0F) + 9))) << (k << 2);
                }
                new_text[offset] = temp;
                offset++;
                break;
            }
            break;
        default:
            new_text[offset] = btowc(text[i]);
            offset++;
        }
    }
    new_text[offset] = L'\0';
    return new_text;
}


#endif
