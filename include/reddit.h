#ifndef _REDDIT_H_
#define _REDDIT_H_

#include <ctype.h>
#include <stdbool.h>

typedef enum reddit_errno {
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

} reddit_errno;

typedef struct reddit_cookie_link {
    struct reddit_cookie_link *next;
    char *name;
    char *data;
} reddit_cookie_link;


typedef struct reddit_state {
    reddit_cookie_link *base;
} reddit_state;


typedef enum reddit_user_state {
    REDDIT_USER_LOGGED_ON = 0,
    REDDIT_USER_OFFLINE   = -1
} reddit_user_state;

/*
 * coresponds to a 'i3' datatype
 *
 * This structure represents data on any user.
 */
typedef struct reddit_user {
    char *name;
    char *modhash;

    char *id;

    int comment_karma;
    int link_karma;

    unsigned int flags;
} reddit_user;

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
typedef struct reddit_user_logged {
    reddit_user_state user_state;
    bool stay_logged_on;

    reddit_user *user_info;
} reddit_user_logged;



/*
 * Structure representing a comment on a reddit link or post
 */
typedef struct reddit_link {
    struct reddit_link *next;

    char *title;
    char *selftext;
    char *id;
    char *perm_link;
    char *author;
    char *url;

    int score;
    int downs;
    int ups;

    int num_comments;
    int num_reports;

    unsigned int flags;
} reddit_link;

#define REDDIT_LINK_IS_SELF       1
#define REDDIT_LINK_OVER_18       2
#define REDDIT_LINK_CLICKED       4
#define REDDIT_LINK_STICKIED      8
#define REDDIT_LINK_EDITED        16
#define REDDIT_LINK_HIDDEN        32
#define REDDIT_LINK_DISTINGUISHED 64


typedef enum reddit_list_type {
    REDDIT_HOT = 0,
    REDDIT_NEW = 1,
    REDDIT_RISING = 2,
    REDDIT_CONTR = 3,
    REDDIT_TOP = 4
} reddit_list_type;

typedef struct reddit_link_list {
    char *subreddit;
    char *modhash;
    reddit_list_type type;

    int link_count;
    reddit_link *first;
    reddit_link *last;
} reddit_link_list;

typedef struct reddit_comment {
    int reply_count;
    struct reddit_comment **replies;

    char *id;
    char *author;
    char *parent_id;
    char *body;

    int ups;
    int downs;
    int num_reports;
    int total_reply_count;
    int direct_children_count;
    char *children_id;
    char **direct_children_ids;

    unsigned int flags;
} reddit_comment;

#define REDDIT_COMMENT_SCORE_HIDDEN  1
#define REDDIT_COMMENT_DISTINGUISHED 2
#define REDDIT_COMMENT_EDITED        4
#define REDDIT_COMMENT_NEED_TO_GET   8

typedef enum reddit_comment_sort_type {
    REDDIT_SORT_BEST = 0,
    REDDIT_SORT_TOP,
    REDDIT_SORT_NEW,
    REDDIT_SORT_CONTR,
    REDDIT_SORT_OLD,
    REDDIT_SORT_RANDOM
} reddit_comment_sort_type;

typedef struct reddit_comment_list {
    reddit_comment *base_comment;

    reddit_link *post;
    char *perm_link;
    char *id;
} reddit_comment_list;


extern void  reddit_cookie_new        (char *name, char *data);
extern void  reddit_cookie_free       (reddit_cookie_link *link);
extern void  reddit_remove_cookie     (char *name);
extern char *reddit_get_cookie_string ();

extern reddit_state *reddit_state_new  ();
extern void          reddit_state_free (reddit_state *state);
extern reddit_state *reddit_state_get  ();
extern void          reddit_state_set  (reddit_state *state);

extern reddit_user        *reddit_user_new           ();
extern void                reddit_user_free          (reddit_user  *log);

extern reddit_user_logged *reddit_user_logged_new    ();
extern void                reddit_user_logged_free   (reddit_user_logged *user);
extern reddit_errno        reddit_user_logged_login  (reddit_user_logged *log, char *name, char *passwd);
extern reddit_errno        reddit_user_logged_update (reddit_user_logged *user);

extern reddit_link         *reddit_link_new                ();
extern void                 reddit_link_free               (reddit_link *link);
extern reddit_link_list    *reddit_link_list_new           ();
extern void                 reddit_link_list_free          (reddit_link_list *list);
extern void                 reddit_link_list_free_links    (reddit_link_list *list);

extern reddit_errno         reddit_get_listing             (reddit_link_list *list);


extern reddit_comment      *reddit_comment_new             ();
extern void                 reddit_comment_free            (reddit_comment *comment);
extern void                 reddit_comment_add_reply       (reddit_comment *comment, reddit_comment *reply);
extern reddit_comment_list *reddit_comment_list_new        ();
extern void                 reddit_comment_list_free       (reddit_comment_list *list);
extern reddit_errno         reddit_get_comment_list        (reddit_comment_list *list);
extern reddit_errno         reddit_get_comment_children    (reddit_comment_list *list, reddit_comment *parent);
extern char                *reddit_copy_string(const char *string);

extern void   reddit_global_init();
extern void   reddit_global_cleanup();

#endif
