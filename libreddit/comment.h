#ifndef _REDDIT_COMMENT_H_
#define _REDDIT_COMMENT_H_

#include "reddit.h"
#include "link.h"
#include "jsmn.h"
#include "token.h"

RedditComment *reddit_get_comment(TokenParser *parser, RedditCommentList *list);

#endif
