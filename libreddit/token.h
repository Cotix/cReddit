#ifndef _REDDIT_TOKEN_H_
#define _REDDIT_TOKEN_H_

#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

#include "reddit.h"
#include "jsmn.h"


/*
 * Structure representing a block of memory and it's current size
 */
typedef struct MemoryBlock {
    char   *memory;
    size_t  size;
} MemoryBlock;

/*
 * Represents the state of a parser for parsing Reddit JSON. It holds a block
 * of memory with the JSON text, the jsmn parsed tokens, the number of tokens,
 * and the current token that is being parsed.
 */
typedef struct TokenParser {
    MemoryBlock *block;
    jsmntok_t *tokens;
    int       tokenCount;
    int       currentToken;
} TokenParser;

/*
 * A simple enum representing possible errors from the parser
 */
typedef enum TokenParserResult {
    TOKEN_PARSER_SUCCESS   = 0,
    TOKEN_PARSER_CURL_FAIL,
    TOKEN_PARSER_JSON_FAIL
} TokenParserResult;

/*
 * This is an identifier for a token contained inside of a json
 *
 * Json notation works on a system of 'key' and 'value' pairs. Ex:
 *   { "Key": "Value", "Key2": "Value2", ...
 *
 * When getting output from Reddit, key's stay largely the same every time
 * something is called. The value is then the data you're interested in. This
 * structure handles detailing how to handle a particular key value, whether it
 * should be handled by assigning the value to a pointer, or call a function
 * callback to handle it when that specific key is found.
 */
typedef struct TokenIdent {

    /* Token identifier */
    char  *name;

    enum {
        TOKEN_BOOL = 0, /* Should be of type 'unsigned int' -- See bit_mask member */
        TOKEN_INT = 1, /* Same as Bool, 'int', however can be any value */
        TOKEN_STRING = 2, /* Should be of type 'char*' */
        TOKEN_OBJECT = 3, /* For now, acts same as TOKEN_STRING */
        TOKEN_DATE = 4 /* Parse this id as a date object */
    } type;

    enum {
        /* If equal to 'TOKEN_SET', then 'value' will be cast and assigned
         * based on the 'type' to the value of the token after 'name' */
        TOKEN_SET = 0,

        /* If we have this, then the parser will run the EscCode parser on the string 'value'
         * and store the contents in 'strParse' and 'strParseWide' */
        TOKEN_SET_PARSE,

        /* If 'name' is found as a key, then 'funcCallback' will be called
         * and 'value' will be completely ignored. */
        TOKEN_CHECK_CALL
    } action;


    /* A pointer to one of the token type's to store the token's data -- See enum type */
    void *value;

    /* Some extra pointers if we have a Parsed string */
    char **parseStr;
    wchar_t **parseStrWide;

    /* If using 'TOKEN_BOOL', then if the token is 'true', *value will be cast as an
     * unsigned int, and |= with bitMask. If 'false', it will be &= ~bitMask */
    unsigned int bitMask;

    /* Simple flag -- This handles the case of 'TOKEN_STRING' in combo with 'TOKEN_SET'
     * In that case, if this is set to '1' then free() will be called on value
     * before it is overwritten to avoid a memory leak */
    bool freeFlag;

    /* A function to call when this token is found
     *
     * Parameters from left to right:
     *   Pointer to the current parser, which includes the json text and current token
     *   Pointer to idents, which is an array detailing how to parse the tokens.
     *   The extra arguments send over to parseTokens
     */
    void (*funcCallback) (TokenParser       *parser,
                           struct TokenIdent *idents,
                           va_list             args
                           );

} TokenIdent;

/*
 * Functions for handling allocations of a TokenParser
 */
TokenParser *tokenParserNew();
void         tokenParserFree(TokenParser *parser);

/*
 * Some basic functions for creating MemoryBlocks
 */
MemoryBlock *memoryBlockNew();
void memoryBlockFree(MemoryBlock *block);

char *getCopyOfToken(const char *json, jsmntok_t token);
char *trueFalseString(char *string, bool tf);

/* Functions to get the JSON from a url and run the parser over it */
TokenParserResult redditvRunParser(char *url, char *post, TokenIdent *idents, va_list args);
TokenParserResult redditRunParser(char *url, char *post, TokenIdent *idents, ...);

/* Runs a setup TokenParser. redditRunParser calls this. Normally it's
 * only used in callbacks when a new object is going to be parsed */
void vparseTokens (TokenParser *parser, TokenIdent *identifiers, va_list args);
void parseTokens  (TokenParser *parser, TokenIdent *identifiers, ...);

/*
 * Small #define macro to allocate space for a token and then read the data from
 * a token into the temporary string.
 */
#define READ_TOKEN_TO_TMP(json, tmp, token)                         \
    do {                                                            \
        tmp = rrealloc(tmp, token.end - token.start + 1);           \
        memcpy(tmp, json + token.start, token.end - token.start);   \
        tmp[token.end - token.start] = 0;                           \
    } while (0)

/* Quick macros for testing jsmntok_t tokens */
#define TOKEN_EQUALS(token, text)    (strcmp(token, text) == 0)
#define TOKEN_IS_TRUE(json, token)   ((toupper(json[token.start])) == 'T')
#define TOKEN_IS_FALSE(json, token)  ((toupper(json[token.start])) == 'F')

/* These next macros take a token and read it in as a specific type
 * of JSON object, either a string, number of boolean */
#define READ_TOKEN_AS_STRING(string, json, token)     \
    do {                                              \
        string = getCopyOfToken(json, token);      \
    } while (0)

#define READ_TOKEN_AS_NUMBER(number, json, token, tmp)   \
    do {                                                 \
        READ_TOKEN_TO_TMP(json, tmp, token);             \
        number = atoi(tmp);                              \
    } while (0)

#define READ_TOKEN_AS_BOOL(boolean, json, token)         \
    do {                                                 \
        boolean = TOKEN_IS_TRUE(json, token);            \
    } while (0)

/*
 * A basic prototype for a callback function for the token parser
 */
#define DEF_TOKEN_CALLBACK(name) \
    static void name (TokenParser *parser, TokenIdent *idents, va_list args)

/*
 * This macro is a quick way to create token_ident entries which simply set
 * a variable in a pointer to a struct that has the same name as it's key
 * (This is the most common use case)
 */
#define ADD_TOKEN_IDENT_STRING(key_name, member) \
    {.name = key_name,                           \
     .type = TOKEN_STRING,                       \
     .action = TOKEN_SET,                        \
     .value = &(member),                         \
     .freeFlag = 1}

/*
 * Same as above ADD_TOKEN_IDENT_STRING, but for integers instead of strings
 */
#define ADD_TOKEN_IDENT_INT(key_name, member) \
    {.name = key_name,                        \
     .type = TOKEN_INT,                       \
     .action = TOKEN_SET,                     \
     .value = &(member)}

/*
 * A specific token for getting the date from the 'created_utc' JSON API object
 */
#define ADD_TOKEN_IDENT_DATE(key_name, member) \
    {.name = key_name,                        \
     .type = TOKEN_DATE,                       \
     .action = TOKEN_SET,                     \
     .value = &(member)}

/*
 * Similar macro for bool's. The difference here is it needs the bit_mask
 * value to use
 */
#define ADD_TOKEN_IDENT_BOOL(key_name, member, mask) \
    {.name = key_name,                               \
     .type = TOKEN_BOOL,                             \
     .action = TOKEN_SET,                            \
     .value = &(member),                             \
     .bitMask = mask}


#define ADD_TOKEN_IDENT_STRPARSE(key_name, unparsed, parsed, wideparsed) \
    {.name = key_name,                                                   \
     .type = TOKEN_STRING,                                               \
     .action = TOKEN_SET_PARSE,                                          \
     .value = &(unparsed),                                               \
     .parseStr = &(parsed),                                              \
     .parseStrWide = &(wideparsed),                                      \
     .freeFlag = 1}
/*
 * Again, another similar macro. This time, it creates a callback instead
 */
#define ADD_TOKEN_IDENT_FUNC(key_name, func) \
    {.name = key_name,                       \
     .type = TOKEN_OBJECT,                   \
     .action = TOKEN_CHECK_CALL,             \
     .funcCallback = &(func)}


#endif
