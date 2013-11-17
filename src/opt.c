
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>

#include "global.h"
#include "opt.h"

void optAddOptions (optParser *parser, optOption *options, int optionCount)
{
    const int blockSize = 100;
    int opt = parser->optCount, i;
    parser->optCount += optionCount;

    /* Make sure to leave one empty arg */
    if (parser->optCount + 1 >= parser->optAllocCount) {
        do {
            parser->optAllocCount += blockSize;
            parser->options = realloc(parser->options, parser->optAllocCount * sizeof(optOption*));
            memset(parser->options, 0, blockSize * sizeof(optOption*));
        } while (parser->optCount + 1 >= parser->optAllocCount);
    }

    for (i = 0; i < optionCount; i++)
        parser->options[opt + i] = options + i;
}

void optClearParser (optParser *parser)
{
    free(parser->options);
}

static optResponse optReadString (optParser *parser, optOption *option)
{
    parser->curopt++;
    strcpy(option->svalue, parser->argv[parser->curopt]);
    return OPT_CONTINUE;
}

static optResponse optReadInt (optParser *parser, optOption *option)
{
    parser->curopt++;
    option->ivalue = atoi(parser->argv[parser->curopt]);
    return OPT_CONTINUE;
}

static optResponse optProcessOption (optParser *parser, optOption *option)
{
    option->isSet = 1;
    switch (option->arg) {
    case OPT_INT:
        return optReadInt(parser, option);

    case OPT_STRING:
        return optReadString(parser, option);

    default:
        break;
    }
    return OPT_CONTINUE;
}

static optResponse optCheckOpts (optParser *parser)
{
    optOption **option = parser->options;
    char c = parser->argv[parser->curopt][parser->offset];

    for (; *option != NULL; option++)
        if ((*option)->opt_short == c)
            return optProcessOption(parser, *option);

    if (*option == NULL)
        return OPT_UNKNOWN;

    return OPT_CONTINUE;
}

static optResponse optParseChars (optParser *parser)
{
    int len;
    optResponse res = OPT_CONTINUE;

    len = strlen(parser->argv[parser->curopt]);

    for (parser->offset = 1; parser->offset < len; parser->offset++) {
        res = optCheckOpts(parser);
        if (res != OPT_CONTINUE)
            break;
    }
    return res;
}

static optResponse optParseLong (optParser *parser)
{
    optOption **option = parser->options;
    char str[OPT_LONG_MAX];

    /* add 2 so we don't copy the '--' at the beginning */
    strcpy(str, parser->argv[parser->curopt] + 2);

    for (; *option != NULL; option++)
        if (strcmp((*option)->opt_long, str) == 0)
            return optProcessOption(parser, *option);

    if (*option == NULL)
        return OPT_UNKNOWN;

    return OPT_CONTINUE;
}

optResponse optRunParser (optParser *parser)
{
    optResponse res = OPT_CONTINUE;

    if (parser->options == NULL)
        return OPT_SUCCESS;

    parser->curopt++;

    while ((parser->curopt != parser->argc) && res == OPT_CONTINUE) {
        if ((parser->argv)[parser->curopt][0] == '-') {
            if (parser->argv[parser->curopt][1] == '-')
                res = optParseLong(parser);
            else
                res = optParseChars(parser);

            parser->curopt++;
        } else {
            res = OPT_UNUSED;
        }
    }
    if (parser->curopt == parser->argc && res == OPT_CONTINUE)
        res = OPT_SUCCESS;

    return res;
}

