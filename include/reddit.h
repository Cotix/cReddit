#ifndef _REDDIT_H_
#define _REDDIT_H_

#include <ctype.h>
#include <stdbool.h>

typedef enum RedditErrno {
    /* This response indicates no error happened */
    REDDIT_SUCCESS = 0,

    /* Generic Error code
     * It's use should be avoided if possible */
    REDDIT_ERROR,

    /* Issue with the text recieved from reddit
     * Probably a parsing issue with that JSON */
    REDDIT_ERROR_RESPONSE,

    /* Unable to connect to reddit.com */
    REDDIT_ERROR_NOTCON,

    /* Specefic error for logging on
     * Indicates an issue with password or username */
    REDDIT_ERROR_USER

} RedditErrno;

typedef struct RedditCookieLink {
    struct RedditCookieLink *next;
    char *name;
    char *data;
} RedditCookieLink;


typedef struct RedditState {
    RedditCookieLink *base;
} RedditState;


typedef enum RedditUserState {
    REDDIT_USER_LOGGED_ON = 0,
    REDDIT_USER_OFFLINE   = -1
} RedditUserState;

/*
 * coresponds to a 'i3' datatype
 *
 * This structure represents data on any user.
 */
typedef struct RedditUser {
    char *name;
    char *modhash;

    char *id;

    int commentKarma;
    int linkKarma;

    unsigned int flags;
} RedditUser;

#define REDDIT_USER_HAS_MAIL       1
#define REDDIT_USER_HAS_MOD_MAIL   2
#define REDDIT_USER_OVER_18        4
#define REDDIT_USER_IS_MOD         8
#define REDDIT_USER_IS_GOLD        16
#define REDDIT_USER_IS_FRIEND      32


/*
 * Structure representing a currently logged-on user
 *
 * Technically a superset of 'reddit_user', coresponds to
 * a user that is or is going to log-in.
 */
typedef struct RedditUserLogged {
    RedditUserState userState;
    bool stayLoggedOn;

    RedditUser *userInfo;
} RedditUserLogged;



/*
 * Structure representing a comment on a reddit link or post
 */
typedef struct RedditLink {
    struct RedditLink *next;

    char *title;
    char *selftext;
    char *id;
    char *permalink;
    char *author;
    char *url;

    int score;
    int downs;
    int ups;

    int numComments;
    int numReports;

    unsigned int flags;
} RedditLink;

#define REDDIT_LINK_IS_SELF       1
#define REDDIT_LINK_OVER_18       2
#define REDDIT_LINK_CLICKED       4
#define REDDIT_LINK_STICKIED      8
#define REDDIT_LINK_EDITED        16
#define REDDIT_LINK_HIDDEN        32
#define REDDIT_LINK_DISTINGUISHED 64


typedef enum RedditListType {
    REDDIT_HOT = 0,
    REDDIT_NEW = 1,
    REDDIT_RISING = 2,
    REDDIT_CONTR = 3,
    REDDIT_TOP = 4
} RedditListType;

typedef struct RedditLinkList {
    char *subreddit;
    char *modhash;
    RedditListType type;

    int linkCount;
    RedditLink *first;
    RedditLink *last;
} RedditLinkList;

typedef struct RedditComment {
    int replyCount;
    struct RedditComment **replies;

    char *id;
    char *author;
    char *parentId;
    char *body;

    int ups;
    int downs;
    int numReports;
    int totalReplyCount;
    int directChildrenCount;
    char *childrenId;
    char **directChildrenIds;

    unsigned int flags;
} RedditComment;

#define REDDIT_COMMENT_SCORE_HIDDEN  1
#define REDDIT_COMMENT_DISTINGUISHED 2
#define REDDIT_COMMENT_EDITED        4
#define REDDIT_COMMENT_NEED_TO_GET   8

typedef enum RedditCommentSortType {
    REDDIT_SORT_BEST = 0,
    REDDIT_SORT_TOP,
    REDDIT_SORT_NEW,
    REDDIT_SORT_CONTR,
    REDDIT_SORT_OLD,
    REDDIT_SORT_RANDOM
} RedditCommentSortType;

typedef struct RedditCommentList {
    RedditComment *baseComment;

    RedditLink *post;
    char *permalink;
    char *id;
} RedditCommentList;


extern void  redditCookieNew        (char *name, char *data);
extern void  redditCookieFree       (RedditCookieLink *link);
extern void  redditRemoveCookie     (char *name);
extern char *redditGetCookieString  ();

extern RedditState *redditStateNew  ();
extern void         redditStateFree (RedditState *state);
extern RedditState *redditStateGet  ();
extern void         redditStateSet  (RedditState *state);

extern RedditUser        *redditUserNew         ();
extern void               redditUserFree        (RedditUser  *log);

extern RedditUserLogged *redditUserLoggedNew    ();
extern void              redditUserLoggedFree   (RedditUserLogged *user);
extern RedditErrno       redditUserLoggedLogin  (RedditUserLogged *log, char *name, char *passwd);
extern RedditErrno       redditUserLoggedUpdate (RedditUserLogged *user);

extern RedditLink        *redditLinkNew              ();
extern void               redditLinkFree             (RedditLink *link);
extern RedditLinkList    *redditLinkListNew          ();
extern void               redditLinkListFree         (RedditLinkList *list);
extern void               redditLinkListFreeLinks    (RedditLinkList *list);

extern RedditErrno        redditGetListing           (RedditLinkList *list);


extern RedditComment       *redditCommentNew            ();
extern void                 redditCommentFree           (RedditComment *comment);
extern void                 redditCommentAddReply       (RedditComment *comment, RedditComment *reply);
extern RedditCommentList   *redditCommentListNew        ();
extern void                 redditCommentListFree       (RedditCommentList *list);
extern RedditErrno         redditGetCommentList         (RedditCommentList *list);
extern RedditErrno         redditGetCommentChildren     (RedditCommentList *list, RedditComment *parent);
extern char                *redditCopyString            (const char *string);

extern void   redditGlobalInit();
extern void   redditGlobalCleanup();

#endif
