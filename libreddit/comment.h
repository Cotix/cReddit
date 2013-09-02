#ifndef _REDDIT_COMMENT_H_
#define _REDDIT_COMMENT_H_

#include "reddit.h"
#include "link.h"
#include "jsmn.h"
#include "token.h"

reddit_comment *reddit_get_comment(token_parser *parser, reddit_comment_list *list);

#endif
