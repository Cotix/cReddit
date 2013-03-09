
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include "reddit.h"
#include <curl/curl.h>

int main(int argc, char *argv[])
{
	curl_global_init(CURL_GLOBAL_ALL);
	struct post threads[25];//Our array with reddit threads
	redditGetSubreddit("/r/starcraft","hot",threads);
	int i;
	for(i = 0; i != 25; ++i)
	{
		printf("%s(%s)%s -%s\n\n",threads[i].id,threads[i].votes,threads[i].title,threads[i].author);
	}

  	/* we're done with libcurl, so clean it up */ 
  	curl_global_cleanup();

  return 0;
}