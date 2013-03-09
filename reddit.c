#include "reddit.h"

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}
 
void redditGetSubreddit(char * sub, char * sorting, struct post * postList)
{
	CURL *curl_handle;
  	struct MemoryStruct chunk;
 
  	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  	chunk.size = 0;   
  	curl_handle = curl_easy_init();
 
 	//GET request 
 	char url[256];
 	strcpy(url,"http://reddit.com");
 	strcat(url,sub);
 	strcat(url,"/");
 	strcat(url,sorting);
 	strcat(url,".json");
  	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
 	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
  	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
  	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "cReddit/0.0.1 opensource mainline by /u/blacksmid"); //Our user-agent!
 
	//Lets request
  	curl_easy_perform(curl_handle);

  	//and cleanup
  	curl_easy_cleanup(curl_handle);

 	const char *js;
    int r;
    jsmn_parser p;
    jsmntok_t t[2500];
    js = chunk.memory;
    jsmn_init(&p);
    r = jsmn_parse(&p, js, t, 2500);
    int i =0;
    int atPost = 0;
    for(i = 0; i != 2500; ++i)
    {
    	if(t[i].start == -1) break;
    	if(t[i].type == 1) {i+=t[i].size; continue;}
    	if(t[i].start >= t[i].end || t[i].end-t[i].start > 2040) continue;
    	char buffer[1024*2];
    	memcpy(buffer,&chunk.memory[t[i].start],t[i].end-t[i].start);
    	buffer[t[i].end-t[i].start] = 0;

    	if(strcmp("subreddit",buffer) == 0 && atPost < 25)
    	{
    		postList[atPost].subreddit = malloc(sizeof(char)*128);
    		postList[atPost].votes = malloc(sizeof(char)*32);
    		postList[atPost].id = malloc(sizeof(char)*8);
    		postList[atPost].title = malloc(sizeof(char)*512);
    		postList[atPost].author = malloc(sizeof(char)*64);
    		i += 1;
    		memcpy(postList[atPost].subreddit,&chunk.memory[t[i].start],t[i].end-t[i].start);
    		postList[atPost].subreddit[t[i].end-t[i].start] = 0;
    		i += 10;
    		memcpy(postList[atPost].id,&chunk.memory[t[i].start],t[i].end-t[i].start);
    		postList[atPost].id[t[i].end-t[i].start] = 0;
    		i += 4;
    		memcpy(postList[atPost].title,&chunk.memory[t[i].start],t[i].end-t[i].start);
    		postList[atPost].title[t[i].end-t[i].start] = 0;
    		//printf("%s %s %s\n",postList[atPost].subreddit,postList[atPost].id,postList[atPost].title);
    		//Relative possition of score and author isn't constant. So lets search the score variable
    		for(i = i; i != 2500; ++i)
    		{
    			if(t[i].start == -1) break;
    			if(t[i].start > chunk.size) break;
    			char buffer2[1024*2];
    			memcpy(buffer2,&chunk.memory[t[i].start],t[i].end-t[i].start);
    			buffer2[t[i].end-t[i].start] = 0;
    			if(strcmp("score",buffer2) == 0) break;
    		}
    		i++;
    		memcpy(postList[atPost].votes,&chunk.memory[t[i].start],t[i].end-t[i].start);
    		postList[atPost].votes[t[i].end-t[i].start] = 0;
    		i += 34;
    		memcpy(postList[atPost].author,&chunk.memory[t[i].start],t[i].end-t[i].start);
    		postList[atPost].author[t[i].end-t[i].start] = 0;
    		
    		atPost++;
    	}
    }
  	if(chunk.memory)
    	free(chunk.memory);
 
}