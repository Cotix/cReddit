#include "reddit.h"
#include "ncurses.h"

int startsWith(char *pre, char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
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

char* prepend(char *pre, char *str) 
{
    char* newString = malloc(sizeof(char) * strlen(pre) + sizeof(char) * strlen(str));
    strcpy(newString,pre);
    strcat(newString,str);
    return newString;
} 

void redditGetSubreddit(char * sub, char * sorting, struct post * postList, int * postCount)
{
    CURL *curl_handle;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;   
    curl_handle = curl_easy_init();
    if(!startsWith("/r/",sub)){
        sub = prepend("/r/",sub);
    }   
    //GET request 
    int url_size = REDDIT_URL_BASE_LENGTH + strlen(sub) + strlen(sorting);
    char url[url_size];
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

    size_t numtokens = 25000;
    jsmntok_t t[numtokens];

    js = chunk.memory;
    jsmn_init(&p);
    r = jsmn_parse(&p, js, t, numtokens);
    
    int i =0;
    char buffer[2048];
    
    int atPost = 0;

    for(i = 0; i < numtokens; ++i)
    {
        if(t[i].start == -1) continue;
        if(t[i].type == JSMN_OBJECT ) {i+=t[i].size;  continue;}
        if(t[i].start >= t[i].end || t[i].end-t[i].start > 2040) continue;
        memcpy(buffer,&chunk.memory[t[i].start],t[i].end-t[i].start);
        buffer[t[i].end-t[i].start] = 0;

        if(strcmp("id",buffer) == 0)
        {
            i++;
            char *tmp = malloc(t[i].end-t[i].start+1);
            memcpy(tmp,&chunk.memory[t[i].start],t[i].end-t[i].start);
            postList[atPost].id = malloc(t[i].end-t[i].start+1);
            tmp[t[i].end-t[i].start] = 0;
            strcpy(postList[atPost].id,tmp);
            free(tmp);
        }		
        if(strcmp("title",buffer) == 0)
        {
            i++;
            char *tmp = malloc(t[i].end-t[i].start+1);
            memcpy(tmp,&chunk.memory[t[i].start],t[i].end-t[i].start);
            postList[atPost].title = malloc(t[i].end-t[i].start+1);
            tmp[t[i].end-t[i].start] = 0;
            strcpy(postList[atPost].title,tmp);
            free(tmp);
        }
        if(strcmp("score",buffer) == 0)
        {
            i++;
            char *tmp = malloc(t[i].end-t[i].start+1);
            memcpy(tmp,&chunk.memory[t[i].start],t[i].end-t[i].start);
            postList[atPost].votes = malloc(t[i].end-t[i].start+1);
            tmp[t[i].end-t[i].start] = 0;
            strcpy(postList[atPost].votes,tmp);
            free(tmp);
        }
        if(strcmp("author",buffer) == 0)
        {
            i++;
            char *tmp = malloc(t[i].end-t[i].start+1);
            memcpy(tmp,&chunk.memory[t[i].start],t[i].end-t[i].start);
            postList[atPost].author = malloc(t[i].end-t[i].start+1);
            tmp[t[i].end-t[i].start] = 0;
            strcpy(postList[atPost].author,tmp);
            atPost++;
            free(tmp);
            if(atPost == 25)
                break;
        }
        if(strcmp("subreddit",buffer) == 0)
        {
            i++;
            char *tmp = malloc(t[i].end-t[i].start+1);
            memcpy(tmp,&chunk.memory[t[i].start],t[i].end-t[i].start);
            tmp[t[i].end-t[i].start] = 0;
            postList[atPost].subreddit = malloc(t[i].end-t[i].start+1);
            strcpy(postList[atPost].subreddit,tmp);
            free(tmp);
        }

    }
    *postCount = atPost;
    if(chunk.memory)
        free(chunk.memory);
}
void redditGetThread(char * postid, struct comments * commentList, int * commentCount)
{
    CURL *curl_handle;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
    chunk.size = 0;   
    curl_handle = curl_easy_init();

    //GET request 
    char url[256];
    strcpy(url,"http://reddit.com/comments/");
    strcat(url,postid);
    strcat(url,"/GET.json");
    printw("Getting:%s\n",url);
    //getch();
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
    char buffer[2048];

    int atPost = 0;
    for(i = 0; i < 2500; ++i)
    {
        if(t[i].start == -1) continue;
        if(t[i].type == 1 ) {i+=t[i].size;  continue;}
        if(t[i].start >= t[i].end || t[i].end-t[i].start > 2040) continue;
        memcpy(buffer,&chunk.memory[t[i].start],t[i].end-t[i].start);
        buffer[t[i].end-t[i].start] = 0;
        refresh();

        if(strcmp("id",buffer) == 0)
        {
            i++;
            char *tmp = malloc(t[i].end-t[i].start+1);
            memcpy(tmp,&chunk.memory[t[i].start],t[i].end-t[i].start);
            commentList[atPost].id = malloc(t[i].end-t[i].start+1);
            tmp[t[i].end-t[i].start] = 0;
            strcpy(commentList[atPost].id,tmp);
            free(tmp);
        }		
        /* Measured in ups and down votes, TODO
           if(strcmp("score",buffer) == 0)
           {
           i++;
           char *tmp = malloc(t[i].end-t[i].start+1);
           memcpy(tmp,&chunk.memory[t[i].start],t[i].end-t[i].start);
           commentList[atPost].votes = malloc(t[i].end-t[i].start+1);
           tmp[t[i].end-t[i].start] = 0;
           strcpy(commentList[atPost].votes,tmp);
           free(tmp);
           }*/
        if(strcmp("author",buffer) == 0)
        {
            i++;
            char *tmp = malloc(t[i].end-t[i].start+1);
            memcpy(tmp,&chunk.memory[t[i].start],t[i].end-t[i].start);
            commentList[atPost].author = malloc(t[i].end-t[i].start+1);
            tmp[t[i].end-t[i].start] = 0;
            strcpy(commentList[atPost].author,tmp);
            free(tmp);
        }
        if(strcmp("body",buffer) == 0)
        {
            i++;
            char *tmp = malloc(t[i].end-t[i].start+1);
            memcpy(tmp,&chunk.memory[t[i].start],t[i].end-t[i].start);
            tmp[t[i].end-t[i].start] = 0;
            commentList[atPost].text = malloc(t[i].end-t[i].start+1);
            strcpy(commentList[atPost].text,tmp);
            atPost++;
            free(tmp);
            if(atPost == 25)
                break;
        } 
    }
    *commentCount = atPost;
    if(chunk.memory)
        free(chunk.memory);

}

void cleanup()
{
	curl_global_cleanup();
	endwin();
}

char *ask_for_subreddit() {
	clear();
	mvprintw(10, 6, "Subreddit: ");
	int ch, i = 0;
	char *buffer = malloc(sizeof(int) * 128);
	while((ch = getch()) != '\n') {
		if(i == 127) {
			break;
		}
		if(ch == KEY_BACKSPACE) {
			delch();
		} else if(ch == KEY_F(10)) {
			cleanup();
			exit(0);
		} else {
			buffer[i++] = ch;
			addch(ch);
		}
	}
	return buffer;
}

