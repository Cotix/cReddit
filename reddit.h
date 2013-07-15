#ifndef __REDDIT_H_
#define __REDDIT_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include <curl/curl.h>

/* REDDIT_URL_BASE_LENGTH captures the base URL, the trailing '/' after a
 * subreddit and the ".json" suffix. Excluding the subreddit and sorting,
 * it looks like the following: http://reddit.com/.json
 * This is currently defined to be 23 bytes, we add 1 more for '\0'
 * termination. */
#define REDDIT_URL_BASE_LENGTH 24

struct MemoryStruct {
  	char *memory;
  	size_t size;
};

struct post {
	char * title;
	char * votes;
	char * id;
	char * author;
	char * subreddit;
};

struct comments {
	char * text;
	char * id;
	char * author;
	char * votes;
};

void redditGetThread(char * postid, struct comments * commentList, int * commentCount);
void redditGetSubreddit(char * sub, char * sorting, struct post * postList, int * postCount);
char *ask_for_subreddit();
void showSubreddit(char *subreddit);
void cleanup();
int startsWith(char *pre, char *str);

#endif 
