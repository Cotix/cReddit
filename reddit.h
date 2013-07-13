#ifndef __REDDIT_H_
#define __REDDIT_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include <curl/curl.h>
 
#define REDDIT_URL_BASE_LENGTH 21 
<<<<<<< HEAD
#define SIZEOFELEM(x)  (sizeof(x) / sizeof(x[0]))

=======
 
>>>>>>> 4f71652bef00b52b08fea1eff463ca00cfcc7686
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

<<<<<<< HEAD
void redditGetThread(char * postid, struct comments * commentList, int * commentCount);
void redditGetSubreddit(char * sub, char * sorting, struct post * postList, int * postCount);
=======
void redditGetThread(char * postid, struct comments * commentList);
void redditGetSubreddit(char * sub, char * sorting, struct post * postList);
>>>>>>> 4f71652bef00b52b08fea1eff463ca00cfcc7686
char *ask_for_subreddit();
void showSubreddit(char *subreddit);
void cleanup();
int startsWith(char *pre, char *str);

#endif 
