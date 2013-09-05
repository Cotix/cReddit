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

typedef struct TokenParser {
    MemoryBlock *block;
    jsmntok_t *tokens;
    int       tokenCount;
    int       currentToken;
} TokenParser;

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
 * When getting output from Reddit, key's stay largly the same every time
 * something is called. The value is then the data you're interested in. This
 * structure handles detailing how to handle a paticular key value, whether it
 * should be handled by assigning the value to a pointer, check if the contained
 * pointer equals the current value and then call a function callback if so, or
 * call a function callback to handle it when that specefic key is found
 *
 * The function callback get's sent a large list of data, but it's pretty simple,
 * It recieves:
 *   A char* to the full json text being parsed
 *
 */
typedef struct TokenIdent {

    /* Token identifier */
    char  *name;

    enum {
        TOKEN_BOOL = 0, /* Should be of type 'unsigned int' -- See bit_mask member */
        TOKEN_INT = 1, /* Same as Bool, 'int', however can be any value */
        TOKEN_STRING = 2, /* Should be of type 'char*' */
        TOKEN_OBJECT = 3 /* For now, acts same as TOKEN_STRING */
    } type;

    enum {
        /* If equal to 'TOKEN_SET', then 'var_ptr' will be cast and assigned
         * based on the 'token_type' to the value of the token after 'token_name' */
        TOKEN_SET = 0,

        /* If 'token_name' is found as a key, then 'token_func' will be called
         * and 'var_ptr' will be completely ignored. */
        TOKEN_CHECK_CALL
    } action;


    /* A pointer to one of the token type's to store the token's data -- See enum type */
    void *value;

    /* If using 'TOKEN_BOOL', then if the token is 'true', *value will be cast as an
     * unsigned int, and |= with bit_mask. If 'false', it will be &= ~bit_mask */
    unsigned int bitMask;

    /* Simple flag -- This handles the case fo 'TOKEN_STRING' in combo with 'TOKEN_SET'
     * In that case, if this is set to '1' then free() will be called on var_ptr
     * before it is overwritten to avoid a memory leak */
    bool freeFlag;

    /* A function to call when this token is found
     *
     * Parameters from left to right:
     *   Full json character data
     *   Array of all the parsed tokens
     *   Pointer to an int containing the current token in the array of tokens
     *     Note: The pointer is incremented one token past where 'token_name' is located
     *     in the array
     *   A temporary buffer (Which is already malloced and can be realloced as much as wanted)
     *   The extra arguments send over to parse_tokens */
    void (*funcCallback) (TokenParser       *parser,
                           struct TokenIdent *idents,
                           va_list             args
                           );

} TokenIdent;

TokenParser *tokenParserNew();
void          tokenParserFree(TokenParser *parser);

char *getCopyOfToken(const char *json, jsmntok_t token);
MemoryBlock *memoryBlockNew();
void memoryBlockFree(MemoryBlock *block);
char *trueFalseString(char *string, bool tf);

TokenParserResult redditvRunParser(char *url, char *post, TokenIdent *idents, va_list args);
TokenParserResult redditRunParser(char *url, char *post, TokenIdent *idents, ...);

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

#define TOKEN_EQUALS(token, text)    (strcmp(token, text) == 0)
#define TOKEN_IS_TRUE(json, token)   ((toupper(json[token.start])) == 'T')
#define TOKEN_IS_FALSE(json, token)  ((toupper(json[token.start])) == 'F')

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
 * This macro is a quick way to create token_ident entrys which simply set
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
 * Similar macro for bool's. The difference here is it needs the bit_mask
 * value to use
 */
#define ADD_TOKEN_IDENT_BOOL(key_name, member, mask) \
    {.name = key_name,                               \
     .type = TOKEN_BOOL,                             \
     .action = TOKEN_SET,                            \
     .value = &(member),                             \
     .bitMask = mask}

/*
 * Again, another similar macro. This time, it creates a callback instead
 */
#define ADD_TOKEN_IDENT_FUNC(key_name, func) \
    {.name = key_name,                       \
     .type = TOKEN_OBJECT,                   \
     .action = TOKEN_CHECK_CALL,             \
     .funcCallback = &(func)}


#endif
