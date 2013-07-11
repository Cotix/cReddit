#ifndef __REDDIT_H_
#define __REDDIT_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include <curl/curl.h>
 
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

void redditGetThread(char * postid, struct comments * commentList);
void redditGetSubreddit(char * sub, char * sorting, struct post * postList);
char *ask_for_subreddit();
void showSubreddit(char *subreddit);
void cleanup();
int startsWith(char *pre, char *str);

#endif 
