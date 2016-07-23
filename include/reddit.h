#ifndef _REDDIT_H_
#define _REDDIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <stdbool.h>
#include <wchar.h>
#include <time.h>
#ifdef REDDIT_DEBUG
# include <stdio.h>
#endif

/*
 * This enum represents all possible errors from libreddit. Every function that
 * returns an error number returns a RedditErrno set to one of these options
 * (Which can be expanded upon as needed)
 */
typedef enum RedditErrno {
    /* This response indicates no error happened */
    REDDIT_SUCCESS = 0,

    /* Generic Error code
     * It's use should be avoided if possible */
    REDDIT_ERROR,

    /* Issue with the text received from reddit
     * Probably a parsing issue with that JSON */
    REDDIT_ERROR_RESPONSE,

    /* Unable to connect to reddit.com */
    REDDIT_ERROR_NOTCON,

    /* Specific error for logging on
     * Indicates an issue with password or username */
    REDDIT_ERROR_USER

} RedditErrno;

/*
 * This structure holds data on a cookie received from Reddit.  In general,
 * these are allocated and stored automatically in the current global state, so
 * allocating them directly isn't necessary. The only real use it has
 * externally is reading the name and data from a cookie in the current state
 * so you can save it for later use. Cookies should be added to the current
 * global state by a separate API call, and removed by another separate API
 * call, so allocating them directly shouldn't be necessary
 */
typedef struct RedditCookieLink {
    struct RedditCookieLink *next;
    char *name;
    char *data;
} RedditCookieLink;


/*
 * This structure represents the current state of the library, or more specifically
 * of the Reddit Session.
 *
 * Currently, all it does is contain a linked-list of cookies being used in the session
 */
typedef struct RedditState {
    RedditCookieLink *base;
    char *userAgent;
} RedditState;

/*
 * This is a simple enum for the current state of a logged user, representing
 * whether or not they are currently logged-on
 */
typedef enum RedditUserState {
    REDDIT_USER_LOGGED_ON = 0,
    REDDIT_USER_OFFLINE   = -1
} RedditUserState;

/*
 * This structure represents data on any user, Ex. a logged-in user, a user who
 * posted a comment, a user who posted a link, etc.
 */
typedef struct RedditUser {
    char *name; /* Username / reddit handle */
    char *modhash; /* modhash returned in conjunction with the userdata */

    char *id; /* The user's unique Base-36 ID */

    int commentKarma; /* User's current comment karma count */
    int linkKarma; /* User's current link karma count */

    unsigned int flags; /* used as a bitfield for the define's below */
} RedditUser;

/*
 * bitfield values for the RedditUser flags value. | (or) these values together
 * with flags to set them, and & (and) against the ! (not) of this value to
 * unset them.  Use & (and) against the define and flags to see if it is set
 */
#define REDDIT_USER_HAS_MAIL       1
#define REDDIT_USER_HAS_MOD_MAIL   2
#define REDDIT_USER_OVER_18        4
#define REDDIT_USER_IS_MOD         8
#define REDDIT_USER_IS_GOLD        16
#define REDDIT_USER_IS_FRIEND      32


/*
 * A superset of 'reddit_user', corresponds to a user that is or is going to
 * log-in. Includes a full RedditUser as well as extra info related to
 * logged-in state
 */
typedef struct RedditUserLogged {
    RedditUserState userState;
    bool stayLoggedOn; /* If this is set when logged-in, the API will request
                        * a persistent cookie, which you can then save and use
                        * again later-on */

    RedditUser *userInfo;
} RedditUserLogged;



/*
 * Structure representing a comment on a reddit link or selfpost
 *
 * The members themselves related directly to what they store. The structure
 * itself wroks exactly like RedditUser as far as the 'flags' variable goes.
 */
typedef struct RedditLink {
    struct RedditLink *next;

    char *id;
    char *permalink;
    char *author;
    char *url;

    /* These members are versions of the strings in different formats.
     * The normal versions are unparsed.
     * The 'Esc' versions are parsed for Reddit Esc codes.
     * The 'w' versions are parsed for Esc codes as well as support unicode characters */
    char *title;
    char *selftext;

    char *titleEsc;
    char *selftextEsc;

    wchar_t *wtitleEsc;
    wchar_t *wselftextEsc;

    int score;
    int downs;
    int ups;

    int numComments;
    int numReports;

    char *created_utc; // Date that this comment was created in UTC

    unsigned int flags;
    unsigned int advance;
} RedditLink;

#define REDDIT_LINK_IS_SELF       1
#define REDDIT_LINK_OVER_18       2
#define REDDIT_LINK_CLICKED       4
#define REDDIT_LINK_STICKIED      8
#define REDDIT_LINK_EDITED        16
#define REDDIT_LINK_HIDDEN        32
#define REDDIT_LINK_DISTINGUISHED 64

/*
 * This enum represents the sorting to be requested when getting a list
 * of links from Reddit.
 */
typedef enum RedditListType {
    REDDIT_HOT = 0,
    REDDIT_NEW = 1,
    REDDIT_RISING = 2,
    REDDIT_CONTR = 3,
    REDDIT_TOP = 4
} RedditListType;

/*
 * This structure represents a full list of RedditLink's. The big reason it's here
 * is because you can use the API call for getting a list of RedditLink's from a subreddit
 * more then once on the same list to get more Links.
 *
 * 'subreddit' should be a string with the name of the subreddit, Ex. '/','/r/linux',etc.
 */
typedef struct RedditLinkList {
    char *subreddit;
    char *modhash;
    RedditListType type;

    int linkCount;
    RedditLink **links;
    char *afterId;
} RedditLinkList;

/*
 * Similar to RedditUser in structure, but it's use is a bit more complex:
 *
 * Reddit has two ways of informing about comments. Specifically, in any
 * comment listing from Reddit, you'll have directly displayed comments and
 * hidden comments.  'hidden comments' are simply the 'click to see more
 * replies' on the webpage which aren't displayed by default unless you click
 * to see them. A RedditComment can have any number of actual, full comment's
 * in replies, and also any number of 'stub' children which aren't displayed
 * but can be requested via the 'morechildren' API call.
 *
 * The replies array is simply an array of pointers to RedditComment structures
 * of the full replies to this current comment. 'replyCount' tells you how many
 * direct replies we have in full to this comment, and is also the list of
 * comments that should be displayed.
 *
 * 'totalReplyCount' represents the total number of hidden replies to this
 * comment.  This includes any replies to our comment, any replies to replies
 * to our comment, and so on.
 *
 * The 'directChildrenCount' is both the number of char*'s in
 * 'directChildrenIds' and the number of direct replies to our current comment.
 * The array 'directChildrenIds' is a string array of the Base-36 id's of the
 * comments, for use in the 'morechildren' API call.
 */
typedef struct RedditComment {
    int replyCount;
    struct RedditComment **replies;
    struct RedditComment *parent;

    char *id; /* Doesn't include 't1_' */
    char *author;
    char *parentId;
    char *linkId;

    /* See 'RedditLink' for explanation of different strings */
    char *body;
    char *bodyEsc;
    wchar_t *wbodyEsc;


    int ups;
    int downs;
    int numReports;
    int totalReplyCount;
    int directChildrenCount;
    char *childrenId;
    char **directChildrenIds;

    char *created_utc; // Date that this comment was created in UTC

    unsigned int flags;
    int advance;
} RedditComment;

#define REDDIT_COMMENT_SCORE_HIDDEN  1
#define REDDIT_COMMENT_DISTINGUISHED 2
#define REDDIT_COMMENT_EDITED        4
#define REDDIT_COMMENT_NEED_TO_GET   8

/*
 * The type of sorting that should be used when getting the list of comments
 */
typedef enum RedditCommentSortType {
    REDDIT_SORT_BEST = 0,
    REDDIT_SORT_TOP,
    REDDIT_SORT_NEW,
    REDDIT_SORT_CONTR,
    REDDIT_SORT_OLD,
    REDDIT_SORT_RANDOM
} RedditCommentSortType;

/*
 * A structure representing a full list of comments to a link/post.
 *
 * The 'baseComment' is a RedditComment structure that has no data except replies.
 * The replies to baseComment are the top-level comments to the link/post.
 */
typedef struct RedditCommentList {
    RedditComment *baseComment;

    RedditLink *post;
    char *permalink;
    char *id;
} RedditCommentList;


/*
 * calls to create and free a cookie
 */
extern void  redditCookieNew  (char *name, char *data);
extern void  redditCookieFree (RedditCookieLink *link);

/* This removes a cookie which has the name 'name' from the global state */
extern void  redditRemoveCookie (char *name);

/* This returns a formatted string of all the cookies in the current global
 * state.  Ex. 'reddit_session=205858&cookie2=stuff' */
extern char *redditGetCookieString  ();

/* These functions are for creating, free, and changing the global state in the
 * library */
extern RedditState *redditStateNew  ();
extern void         redditStateFree (RedditState *state);
extern RedditState *redditStateGet  ();
extern void         redditStateSet  (RedditState *state);

/* These two functions create a blank user and free an existing user */
extern RedditUser *redditUserNew  ();
extern void        redditUserFree (RedditUser  *log);

/* Creates a new logged user as well as creates a new RedditUser in the logged
 * user structure */
extern RedditUserLogged *redditUserLoggedNew  ();
extern void              redditUserLoggedFree (RedditUserLogged *user);

/* This logs a logged user in via 'name' and 'passwd' The status is returned
 * via the RedditErrno and the RedditUserState enum in a RedditUserLogged */
extern RedditErrno redditUserLoggedLogin  (RedditUserLogged *log, char *name, char *passwd);
extern RedditErrno redditUserLoggedUpdate (RedditUserLogged *user);

/* functions to create a new blank link and free it */
extern RedditLink *redditLinkNew  ();
extern void        redditLinkFree (RedditLink *link);

/* Functions to Create a new list of Links, free that list, and just free the
 * links creating the list */
extern RedditLinkList *redditLinkListNew       ();
extern void            redditLinkListFree      (RedditLinkList *list);
extern void            redditLinkListFreeLinks (RedditLinkList *list);

/* Returns a list of Links for a subreddit, using the settings in 'list' */
extern RedditErrno redditGetListing (RedditLinkList *list);

/* Create a new blank comment, free a comment, and add a comment structure as a
 * reply to a comment: NOTE: redditCommentAddReply doesn't add the comment as a reply
 * on Reddit.com. */
extern RedditComment *redditCommentNew      ();
extern void           redditCommentFree     (RedditComment *comment);
extern void           redditCommentAddReply (RedditComment *comment, RedditComment *reply);

/* Create and free a list of comments */
extern RedditCommentList *redditCommentListNew  ();
extern void               redditCommentListFree (RedditCommentList *list);

/* Call Reddit to get a list of comments */
extern RedditErrno redditGetCommentList (RedditCommentList *list);

/* Call the morechildren API to get children of 'parent' */
extern RedditErrno redditGetCommentChildren (RedditCommentList *list, RedditComment *parent);

/* simply returns an allocated copy of a string. */
extern char *redditCopyString (const char *string);

/* These functions will returned a copy of 'text' parsed for Reddit's esc codes,
 * Ex. '\n', '\uxxxx', etc... */
extern char    *redditParseEscCodes     (const char *text, int len);
extern wchar_t *redditParseEscCodesWide (const char *text, int len);

/* Global init and clean-up functions. These should be called at the start and
 * end of a program, respectively. */
extern void redditGlobalInit();
extern void redditGlobalCleanup();

#ifdef REDDIT_DEBUG
extern void redditSetDebugFile(FILE *file);
#endif

/* Format is YYYY-MM-DD-HH:MM:SS */
#define CREATE_DATE_FORMAT "\%F-\%T"
#define CREATE_DATE_FORMAT_BYTE_COUNT 32

#ifdef __cplusplus
}
#endif

#endif
