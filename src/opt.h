#ifndef _SRC_OPT_H_
#define _SRC_OPT_H_
/* This file defines the interface to the command-line argument parser. The
 * parser itself runs through the list of sent options and parses them for '-'
 * single char flags, and '--' long flags, as well as values such as ints or
 * strings to collect as a parameter
 *
 * In the context of this file, 'flags' are the same thing as 'options'.*/

typedef struct optParser optParser;
typedef struct optOption optOption;

typedef enum optResponse optResponse;
typedef enum optArgument optArgument;

/* This structure holds all the data for a used parser. 'argc' and 'argv'
 * should be set to the values sent to main on the program's start.
 *
 * 'curopt' is the current string in argv being parsed, 'offset' is the
 *   location in argv we're currently parsing
 *
 * options is the array of flags to check for. In general you shouldn't mess
 *   with this directly but instead use optAddOptions to do it for you.
 *   -- You should note, options holds pointers to the option values added via
 *   optAddOptions, so the original optOption's are modified and not copies */
struct optParser {
    int argc;
    char **argv;
    int curopt, offset;

    int optCount, optAllocCount;
    optOption **options;
};

/* Returned by the optRunParser function:
 *
 * OPT_SUCCESS means that we're done
 * OPT_UNKNOWN means that the parser encountered an option which had no
 *   coresponding entry in the lost of options.
 * OPT_UNUSED means that the parser had a line of text which didn't have any
 *   flags and wasn't an argument to a separate flag
 * OPT_CONTINUE is largely internal to the library, indicating that there is
 *   still more arguments to parse
 * */
enum optResponse {
    OPT_SUCCESS,
    OPT_UNKNOWN,
    OPT_UNUSED,
    OPT_CONTINUE
};

/* Flag that represents what, if any, extra arguments should be included after
 * an option */
enum optArgument {
    OPT_NONE = 0,
    OPT_INT,
    OPT_STRING
};

/* Sane Constants for the library */
#define OPT_STRLEN     50
#define OPT_LONG_MAX   30
#define OPT_HELP_MAX   400

/* This structure represents one 'option', or 'flag' given on the command-line
 *
 * opt_long is the long name of a flag. Such as "--help" corresponds to a
 *   opt_long of "help"
 *
 * opt_short is the single char name of a flag, such as "-h" corresponds to 'h'
 *
 * optArgument is an indication of what type of argument to expect after the
 *   flag, if any
 *
 * isSet is a flag value set by the optRunParser if the flag is found on the
 *   command-line
 *
 * The union is a union over structures for each type of argument. */
struct optOption {
    char opt_long[OPT_LONG_MAX + 1];
    char opt_short;
    optArgument arg;
    char helpText[OPT_HELP_MAX + 1];

    unsigned int isSet :1;
    int ivalue;
    char svalue[OPT_STRLEN + 1];
};

/* These macros are useful for easily creating an array entry for an option. It
 * is recommended to use macros to create an array of options rather then
 * initialize the parts directly. */
#define OPT(lng, shrt, helptxt) \
    { .opt_long = lng, \
      .opt_short = shrt, \
      .helpText = helptxt, \
      .isSet = 0, \
      .arg = OPT_NONE }

#define OPT_STRING(lng, shrt, helptxt, defstr) \
    { .opt_long = lng, \
      .opt_short = shrt, \
      .helpText = helptxt, \
      .isSet = 0, \
      .arg = OPT_STRING, \
      .svalue = defstr }

#define OPT_INT(lng, shrt, helptxt, defint) \
    { .opt_long = lng, \
      .opt_short = shrt, \
      .helpText = helptxt, \
      .isSet = 0, \
      .arg = OPT_INT, \
      .ivalue = defint }

/* optAddOptions should be used first -- Send it the parser and a list of
 * options, and it will handle registering them into the parser. Note the
 * optOption's sent to optAddOptions are the ones that are going to be
 * modified, no copies are made, so they have to be kept in memory for the
 * duration of the argument parsing, or you'll segfault.
 *
 * optRunParser simply runs the parser with it's current option set, returning
 * a optResponse either when it's done, or when it encounters something wrong
 * that the calling function would want to know about. optRunParser can be
 * called again and again in succession with the same optParser until it
 * returns OPT_SUCCESS to parse the full set of arguments
 *
 * optClearParser frees any memory being used to hold pointers to the options,
 * making it safe to toss the parser. Tossing the parser doesn't do anything to
 * the registered optOption's, they need to be freed separately if needed. */
extern optResponse optRunParser   (optParser *parser);
extern void        optAddOptions  (optParser *parser, optOption *options, int optionCount);
extern void        optClearParser (optParser *parser);

#endif
