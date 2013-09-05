#ifndef _REDDIT_COMMENT_H_
#define _REDDIT_COMMENT_H_

#include "reddit.h"
#include "link.h"
#include "jsmn.h"
#include "token.h"

RedditComment *redditGetComment(TokenParser *parser, RedditCommentList *list);

#endif
