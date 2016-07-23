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
 * Returns a pointer to valid new MemoryBlock
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
 * Frees a MemoryBlock returned by memoryBlockNew()
 */
void memoryBlockFree(MemoryBlock *block)
{
    if (block == NULL)
        return ;
    free(block->memory);
    free(block);
}

/*
 * Returns an empty allocated TokenParser
 */
TokenParser *tokenParserNew()
{
    TokenParser *parser = rmalloc(sizeof(TokenParser));
    memset(parser, 0, sizeof(TokenParser));
    parser->block = memoryBlockNew();
    return parser;
}

/*
 * Frees a TokenParser as well as it's memory block
 */
void tokenParserFree(TokenParser *parser)
{
    if (parser == NULL)
        return ;
    memoryBlockFree(parser->block);
    free(parser->tokens);
    free(parser);
}
/*
 * This function takes a TokenParser with an already full MemoryBlock
 * and runs jsmn over it to tokenize the JSON. Because jsmn doesn't do
 * any allocation on it's own, this function keeps looping over jsmn_parse
 * while it returns out of memory errors and then allocates more memory
 * and runs it again
 *
 * It returns the state jsmn_parse returned.
 */
static jsmnerr_t tokenParserCreateTokens(TokenParser *parser)
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

/*
 * Callback used by curl. The userp is a pointer to a TokenParser. This
 * callback stores the JSON text that curl got back into the TokenParser. It
 * reallocates the size of the memory buffer as more memory is needed for the
 * JSON.
 */
static size_t writeToParser(void *contents, size_t size, size_t nmemb, void *userp)
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
 * Note -- 'string' should be already allocated to at least a length of 6
 */
char *trueFalseString(char *string, bool tf)
{
    if (tf)
        strcpy(string, "true");
    else
        strcpy(string, "false");

    return string;
}

/*
 * Takes in a string (const or not) and returns an allocated version of it. Since
 * all the structures the library uses assumes allocated strings, this function
 * allows literals to be used easily by wrapping them with this function.
 */
EXPORT_SYMBOL char *redditCopyString(const char *string)
{
    char *newStr = rmalloc(strlen(string) + 1);
    strcpy(newStr, string);
    return newStr;
}

/*
 * Small macro for calling a function callback It creates a copy of the va_list,
 * runs the callback with that copy, and then ends the list.
 */
#define CALL_TOKEN_FUNC(func, parser, idents, args)             \
    do {                                                        \
        va_list argsCopy;                                       \
        va_copy(argsCopy, args);                                \
        (func) (parser, idents, argsCopy);                      \
        va_end(argsCopy);                                       \
    } while (0)

/*
 * This function handles the bulk of the token parser work. It 'performs' the action
 * specified by a TokenIdent.
 *
 * When called, it checks if the current token has an equal name to the current
 * TokenIdent. If it does, then it checks the tokens action and calls the
 * callback on a TOKEN_CHECK_CALL, else it checks for TOKEN_SET and does proper
 * casting and assignments in those cases depending on the type of token.
 */
int performIdentAction(TokenParser *p, TokenIdent *identifiers, int i, va_list args)
{
    char *tmp = NULL;
    int performedAction = 0, len;

    time_t created_time_t;
    struct tm *localtime_internal_data;
    struct tm *created_struct_tm;
    char *formatted_time_string;

    READ_TOKEN_TO_TMP(p->block->memory, tmp, p->tokens[p->currentToken]);

    if (TOKEN_EQUALS(tmp, identifiers[i].name)) {
        switch(identifiers[i].action) {
        case TOKEN_CHECK_CALL:
            (p->currentToken)++;
            CALL_TOKEN_FUNC(identifiers[i].funcCallback, p, identifiers, args);
            performedAction = 1;
            break;

        case TOKEN_SET_PARSE:
            (p->currentToken)++;
            if (identifiers[i].freeFlag) {
                free(*((char**)identifiers[i].value));
                free(*(identifiers[i].parseStrWide));
                free(*(identifiers[i].parseStr));
            }

            *((char**)identifiers[i].value) = getCopyOfToken(p->block->memory, p->tokens[p->currentToken]);

            len = p->tokens[p->currentToken].end - p->tokens[p->currentToken].start;

            *(identifiers[i].parseStr)     = redditParseEscCodes    (*((char**)identifiers[i].value), len);
            *(identifiers[i].parseStrWide) = redditParseEscCodesWide(*((char**)identifiers[i].value), len);

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
            case TOKEN_DATE:
                // Process the tmp char* string as a date
                created_time_t = atol(tmp);
                created_struct_tm = (struct tm *)(rmalloc(sizeof(struct tm)));
                formatted_time_string = (char *)(rmalloc(CREATE_DATE_FORMAT_BYTE_COUNT));
                
                // Convert created_time_t (seconds since epoch) to 
                // the struct tm object of localtime_internal_data. After we get
                // the value, copy the internal data (static data)to a newly created
                // struct tm object.
                localtime_internal_data = localtime(&created_time_t);
                if (localtime_internal_data != NULL)
                {
                    memcpy(created_struct_tm, localtime_internal_data, sizeof(struct tm));

                    // Use strftime to convert the struct tm to a formatted string. The
                    // format for the string is specified by CREATE_DATE_FORMAT
                    if (0 != strftime(formatted_time_string, CREATE_DATE_FORMAT_BYTE_COUNT, 
                                CREATE_DATE_FORMAT, created_struct_tm))
                    {
                        *((char**)identifiers[i].value) = formatted_time_string;
                    }
                }

                // Free only created_struct_tm, don't free formatted_time_string
                // because that data is now pointed to by identifiers[i].value
                free(created_struct_tm);
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
 * function callback if necessary
 *
 * When called, this function will assume p->tokens[p->currentToken] points to
 * the start of an object, and will return if this is not the case
 *
 * The intention is that the 'p->currentToken' count will be incremented to the object
 * you want to parse when calling this, and the parser will loop over every token inside
 * of that object.
 */
void vparseTokens (TokenParser *p, TokenIdent *identifiers, va_list args)
{
    int tokenCount = 0, i;
    int identCount = 0;

    while (identifiers[identCount].name != NULL)
        identCount++;

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
            if (performIdentAction(p, identifiers, i, args)) {
                (p->currentToken)--;
                break;
            }

    }

    return ;
}

/*
 * A varidic version of vparseTokens. It is simply a wrapper around
 * vparseTokens which turns the arguments into a va_list, calls vparseTokens
 * with it, and then end's the list after the function exists.
 */
void parseTokens (TokenParser *parser, TokenIdent *identifiers, ...)
{
    va_list args;
    va_start(args, identifiers);
    vparseTokens (parser, identifiers, args);
    va_end(args);
}

/*
 * This function controls the actual parsing, by calling curl to get the JSON,
 * creating the jsmn tokens, and then calling the parser.
 *
 * 'url' is the url of the JSON you want. Ex. www.reddit.com/.json
 * 'post' is any text that should be sent in a POST request. If you want to
 *        do a GET, set this to NULL.
 * 'idents' is the list of token identifiers to use when running the parser
 * 'args' is any extra arguments to be passed on to the parser. (Normally used
 *        by callbacks)
 */
TokenParserResult redditvRunParser(char *url, char *post, TokenIdent *idents, va_list args)
{
    /* Initalize various pieces that are needed to get and parse the JSON */
    TokenParser *parser = tokenParserNew();
    char *cookieStr = NULL;
    CURL *redditHandle = curl_easy_init();
    jsmnerr_t jsmnResult;
    char fullUseragent[1024];

    DEBUG_PRINT(L"Grabbing %s\n", url);
    if (post)
        DEBUG_PRINT(L"Post: %s\n", post);

    /* Set default response */
    TokenParserResult result = TOKEN_PARSER_SUCCESS;

    /* Sets up curl to get 'url' and use writeToParser as the callback writer */
    curl_easy_setopt(redditHandle, CURLOPT_URL, url);
    curl_easy_setopt(redditHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(redditHandle, CURLOPT_WRITEFUNCTION, writeToParser);
    curl_easy_setopt(redditHandle, CURLOPT_WRITEDATA, (void *)parser);

    /* This gets and sets any cookies, if any are currently in the global state */
    cookieStr = redditGetCookieString();
    if (cookieStr != NULL)
        curl_easy_setopt(redditHandle, CURLOPT_COOKIE, cookieStr);

    /* If we're doing a POST, then this sets curl to use POST and tells it what
     * text to use */
    if (post != NULL) {
        curl_easy_setopt(redditHandle, CURLOPT_POST, 1);
        curl_easy_setopt(redditHandle, CURLOPT_POSTFIELDS, (void *)post);
    }

    /* Set the user agent, A combo of libreddit's useragent and the program
     * using libreddit's useragent */
    strcpy(fullUseragent, LIBREDDIT_USERAGENT);
    strcat(fullUseragent, currentRedditState->userAgent);
    curl_easy_setopt(redditHandle, CURLOPT_USERAGENT, fullUseragent);

    /* Run curl, which will run the callback and store our text in the parser
     * then cleanup */
    curl_easy_perform(redditHandle);
    curl_easy_cleanup(redditHandle);

    /* If we didn't get any memory back for whatever reason, set our
     * result to an error and jump to cleanup code. */
    if (parser->block->size <= 0 || parser->block->memory == NULL) {
        result = TOKEN_PARSER_CURL_FAIL;
        goto cleanup;
    }

    /* Create our tokens */
    jsmnResult = tokenParserCreateTokens(parser);

    if (jsmnResult != JSMN_SUCCESS) {
        result = TOKEN_PARSER_JSON_FAIL;
        goto cleanup;
    }

    /* Run the parser over our tokens using the idents */
    vparseTokens(parser, idents, args);

    /* We're done, set the response and then goto cleanup code */
    result = TOKEN_PARSER_SUCCESS;



    /* Simply frees any allocated memory used in the function */
cleanup:;

    free(cookieStr);
    tokenParserFree(parser);

    return result;
}

/*
 * This his a varidic wrapper around redditvRunParser. It takes a variable
 * number of arguments and creates a va_list out of them to call
 * redditvRunParser with.
 */
TokenParserResult redditRunParser(char *url, char *post, TokenIdent *idents, ...)
{
    TokenParserResult result;
    va_list args;
    va_start(args, idents);
    result = redditvRunParser(url, post, idents, args);
    va_end(args);
    return result;
}

/*
 * This is a simple structure used by the EscCodeParser. It's simply here to
 * make passing the data around easy and faster then passing a ton of pointers
 * separately.
 */
struct charParser {
    const char *text;
    int cur;
    int offset;
    int wide;
    void *new_text;
};

/*
 * This function parses a single character in a piece of text and handles any
 * escaped characters.
 */
static void parseChar (struct charParser *p)
{
    int k;
    wchar_t temp;
    switch(p->text[p->cur]) {
    case '\\':
        p->cur++;
        switch(p->text[p->cur]) {
        case 'n':
            if (p->wide)
                ((wchar_t*)(p->new_text))[p->offset] = L'\n';
            else
                ((char*)(p->new_text))[p->offset] = '\n';
            p->offset++;
            break;

        case 'u':
            if (p->wide) {
                temp = L'\0';
                for (k = 3; k >= 0; k--) {
                    p->cur++;
                    /* This weird piece of code converts a hex character (0-9 and a-f) into
                     * it's decimal equivalent, and then shits it over the correct number of
                     * bits corresponding to it's position in the wchar_t */
                    temp |= (((p->text[p->cur] < 58)? p->text[p->cur] - 48: ((p->text[p->cur] & 0x0F) + 9))) << (k << 2);
                }
                ((wchar_t*)(p->new_text))[p->offset] = temp;
                p->offset++;
            } else {
                p->cur+=4;
            }
            break;

        case '\"':
            if (p->wide)
                ((wchar_t*)(p->new_text))[p->offset] = L'\"';
            else
                ((char*)(p->new_text))[p->offset] = '\"';
            p->offset++;
            break;

        }
        break;
    default:
        if (p->wide)
            ((wchar_t*)(p->new_text))[p->offset] = btowc(p->text[p->cur]);
        else
            ((char*)(p->new_text))[p->offset] = p->text[p->cur];
        p->offset++;
        break;
    }
}

/*
 * This code implements a generic version of the 'redditParseEscCodes'
 * functions which works with both wchar_t and char strings. To use wchar_t set
 * 'wide' to 1, else set it to 0 for plain char strings.
 */
static void *redditParseEscCodesGeneric (const void *text, int len, int wide)
{
    struct charParser parser;

    if (len == 0)
        return NULL;

    parser.wide = wide;
    parser.text = text;
    parser.cur = 0;
    parser.offset = 0;

    if (wide)
        parser.new_text = rmalloc((len + 1) * sizeof(wchar_t));
    else
        parser.new_text = rmalloc((len + 1) * sizeof(char));

    for (parser.cur = 0; parser.cur < len; parser.cur++)
        parseChar(&parser);

    if (wide)
        ((wchar_t*)(parser.new_text))[parser.offset] = L'\0';
    else
        ((char*)(parser.new_text))[parser.offset] = '\0';

    return parser.new_text;
}

/*
 * Takes a string of json and the token containing the string data, and
 * parses out any '\' escape characters, such as '\n' and '\u'. This version
 * returns a straight char* version, meaning unicode characters are discarded.
 */
EXPORT_SYMBOL char *redditParseEscCodes (const char *text, int len)
{
    return redditParseEscCodesGeneric(text, len, 0);
}

/*
 * This is another version of the above redditParseEscCodes. The difference is
 * that this returns a whcar_t, so unicode characters are encoded correctly
 */
EXPORT_SYMBOL wchar_t *redditParseEscCodesWide (const char *text, int len)
{
    return redditParseEscCodesGeneric(text, len, 1);
}


#endif
