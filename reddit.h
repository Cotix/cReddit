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

typedef struct {
  	char *memory;
  	size_t size;
} MemoryStruct;

typedef struct {
	char * title;
	char * votes;
	char * id;
	char * author;
	char * subreddit;
} Post;

typedef struct {
	char * text;
	char * id;
	char * author;
	char * votes;
} Comment;

void redditGetThread(char * postid, Comment * commentList, int * commentCount);
void redditGetSubreddit(char * sub, char * sorting, Post * postList, int * postCount);
char *askForSubreddit();
void showSubreddit(char *subreddit);
void cleanup();
int startsWith(char *pre, char *str);

#endif 
