#include "reddit.h"
#include "ncurses.h"

int startsWith(char *pre, char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

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
    strcpy(newString, pre);
    strcat(newString, str);
    return newString;
}

void redditGetSubreddit(char * sub, char * sorting, Post * postList, int * postCount)
{
    CURL *curl_handle;
    MemoryStruct chunk;
    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;
    curl_handle = curl_easy_init();
    if (!startsWith("/r/",sub))
        sub = prepend("/r/",sub);

    //GET request
    int url_size = REDDIT_URL_BASE_LENGTH + strlen(sub) + strlen(sorting);
    char url[url_size];
    strcpy(url, "http://reddit.com");
    strcat(url, sub);
    strcat(url, "/");
    strcat(url, sorting);
    strcat(url, ".json");
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "cReddit/0.0.1 opensource mainline by /u/blacksmid"); //Our user-agent!

    //Lets request
    curl_easy_perform(curl_handle);

    //and cleanup
    curl_easy_cleanup(curl_handle);

    const char *js;
    jsmnerr_t r;
    jsmn_parser p;

    size_t numtokens = 25000;
    jsmntok_t t[numtokens];

    js = chunk.memory;
    jsmn_init(&p);

    if((r = jsmn_parse(&p, js, t, numtokens)) != JSMN_SUCCESS)
    {
        printf("JSON error.");
        exit(-1);
    }

    int i =0;
    char buffer[2048];

    int atPost = 0;

    for(i = 0; i < numtokens; i++)
    {
        if(t[i].start == -1)
            continue;

        if(t[i].type == JSMN_OBJECT)
        {
            i+=t[i].size;
            continue;
        }
        int tokenLength = (t[i].end) - (t[i].start);
        if(tokenLength > 2040 || t[i].start >= t[i].end)
            continue;

        memcpy(buffer, &chunk.memory[t[i].start], tokenLength);
        buffer[tokenLength] = 0;

        if(strcmp("id", buffer) == 0)
        {
            i++;
            tokenLength = (t[i].end) - (t[i].start);
            postList[atPost].id = malloc(tokenLength + 1);
            memcpy(postList[atPost].id, &chunk.memory[t[i].start], tokenLength);
            postList[atPost].id[tokenLength] = 0;
        }
        else if(strcmp("title",buffer) == 0)
        {
            i++;
            tokenLength = (t[i].end) - (t[i].start);
            postList[atPost].title = malloc(tokenLength + 1);
            memcpy(postList[atPost].title, &chunk.memory[t[i].start], tokenLength);
            postList[atPost].title[tokenLength] = 0;
        }
        else if(strcmp("score",buffer) == 0)
        {
            i++;
            tokenLength = (t[i].end) - (t[i].start);
            postList[atPost].votes = malloc(tokenLength + 1);
            memcpy(postList[atPost].votes, &chunk.memory[t[i].start], tokenLength);
            postList[atPost].votes[tokenLength] = 0;
        }
        else if(strcmp("author",buffer) == 0)
        {
            i++;
            tokenLength = (t[i].end) - (t[i].start) + 1;
            postList[atPost].author = malloc(tokenLength + 1);
            memcpy(postList[atPost].author, &chunk.memory[t[i].start], tokenLength);
            postList[atPost].author[tokenLength] = 0;
        }
        else if(strcmp("subreddit",buffer) == 0)
        {
            i++;
            tokenLength = (t[i].end) - (t[i].start);
            postList[atPost].subreddit = malloc(tokenLength + 1);
            memcpy(postList[atPost].subreddit, &chunk.memory[t[i].start], tokenLength);
            postList[atPost].subreddit[tokenLength] = 0;
        }
        else if(strcmp("distinguished",buffer) == 0)
        {
            i++;
            atPost++;
            if(atPost == 25)
                break;
        }
    }
    *postCount = atPost;
    if(chunk.memory)
        free(chunk.memory);
}

void redditGetThread(char * postid, Comment * commentList, int * commentCount, int maxComments)
{
    CURL *curl_handle;
    MemoryStruct chunk;
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
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
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
    for(i = 0; i < 2500; i++)
    {
        if(t[i].start == -1)
            continue;

        if(t[i].type == JSMN_OBJECT)
        {
            i+=t[i].size;
            continue;
        }
        int tokenLength = (t[i].end)-(t[i].start);
        if(tokenLength > 2040 || t[i].start >= t[i].end)
            continue;

        memcpy(buffer, &chunk.memory[t[i].start], tokenLength);
        buffer[tokenLength] = 0;
        refresh();

        if(strcmp("id", buffer) == 0)
        {
            i++;
            tokenLength = (t[i].end) - (t[i].start);
            commentList[atPost].id = malloc(tokenLength + 1);
            memcpy(commentList[atPost].id, &chunk.memory[t[i].start], tokenLength);
            commentList[atPost].id[tokenLength] = 0;
        }
        else if(strcmp("author",buffer) == 0)
        {
            i++;
            tokenLength = (t[i].end) - (t[i].start);
            commentList[atPost].author = malloc(tokenLength + 1);
            memcpy(commentList[atPost].author, &chunk.memory[t[i].start], tokenLength);
            commentList[atPost].author[tokenLength] = 0;
        }
        else if(strcmp("body",buffer) == 0)
        {
            i++;
            tokenLength = (t[i].end) - (t[i].start);
            commentList[atPost].text = malloc(tokenLength + 1);
            memcpy(commentList[atPost].text, &chunk.memory[t[i].start], tokenLength);
            commentList[atPost].text[tokenLength] = 0;
        }
        else if(strcmp("ups",buffer) == 0)
        {
            i++;
            atPost++;
            if(atPost == maxComments)
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

char *askForSubreddit() {
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

