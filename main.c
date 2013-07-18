#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include "reddit.h"
#include <curl/curl.h>
#include <ncurses.h>
#include <form.h>

#define SIZEOFELEM(x)  (sizeof(x) / sizeof(x[0]))


void buildScreen(char **text, int selected, int size)
{
    clear();
    start_color();
    init_pair(1,COLOR_CYAN,COLOR_MAGENTA);
    int i = 0;
    for(i = 0; i != size; ++i)
    {
        if(i == selected)
            attron(COLOR_PAIR(1));
        printw("%s\n",text[i]);
        attroff(COLOR_PAIR(1));
    }
    refresh();
}

/*
    Prints horizontal line of dashes to screen
*/
void printHLine(int width) {
    int i;
    for (i = 0; i < width; i++) {
        printw("-");
    }
} 

/*
    Print comments separated by hline equal to width of term
*/
void printComment(char *text) {
    printHLine(COLS);
    printw("%s\n",text);
}

void showSubreddit(char *subreddit)
{
    struct post threads[25];//Our array with reddit threads
    int *postCount;
    postCount = malloc(sizeof(int));
    redditGetSubreddit(subreddit,"hot",threads,postCount);
    //Just some ncurses testing
    int i;
    int displayCount = 25;
    if (*postCount < 25) {
        displayCount=*postCount;
    }
    printw("%i\n", displayCount);
    char *text[displayCount]; //Text buffer for each line
    for(i = 0; i != displayCount; ++i)
    {
        if(threads[i].id == 0)
            continue;
        char buffer[2048]; //Lets make a bigg ass text buffer so we got enough space
        strcpy(buffer,threads[i].id);
        strcat(buffer," (");
        strcat(buffer,threads[i].votes);
        strcat(buffer,") ");
        strcat(buffer,threads[i].title);
        strcat(buffer," - ");
        strcat(buffer,threads[i].author);
        text[i] = (char*)malloc(strlen(buffer)); //Now lets make a small buffer that fits exacly!
        strcpy(text[i],buffer); //And copy our data into it!
        printw("%s\n",buffer);
        refresh();
    }

    int selected = 0; //Lets select the first post!
    buildScreen(text,selected,displayCount); //And print it!
    int c;
    struct comments cList[500];
    while(c = wgetch(stdscr))
    {
        if(c == 'q') //Lets make a break key, so i dont have to close the tab like last time :S
            break;//YEA FUCK YOU WHILE
        switch(c)
        {
            case 'k': case KEY_UP:
                if(selected != 0)
                    selected--;
                break;

            case 'j': case KEY_DOWN:
                if(selected != 24)
                    selected++;
                break;

            case '\n': // Display selected thread
                refresh();
                int *commentCount;
                commentCount = malloc(sizeof(int));
                redditGetThread(threads[selected].id,cList,commentCount);
                int cdisplayCount = 25;
                if (*commentCount < 25) {
                    cdisplayCount=*postCount;
                }
                // Basically a copy of the code above
                int u;

                clear();
                start_color();
                init_pair(1,COLOR_MAGENTA,COLOR_CYAN);

                char *ctext[cdisplayCount]; //Text buffer for each line
                for(u = 0; u != cdisplayCount; ++u)
                {
                    //printw("starting");
                    if(cList[u].id == 0 || cList[u].text == NULL || cList[u].id == NULL || cList[u].author == NULL)
                        continue;
                    char cbuffer[2048];
                    strcpy(cbuffer,cList[u].id);
                    strcat(cbuffer," ");
                    strcat(cbuffer,cList[u].author);
                    // Votes will have to be implemented with up votes minus
                    // downvotes
                    //strcat(cbuffer," (");
                    //strcat(cbuffer,cList[u].votes);
                    //strcat(cbuffer,")");
                    strcat(cbuffer," - ");
                    strcat(cbuffer,cList[u].text);
                    ctext[u] = (char*)malloc(strlen(cbuffer)); //Now lets make a small buffer that fits exacly!
                    strcpy(ctext[u],cbuffer); //And copy our data into it!
                    printComment(cbuffer);
                    attroff(COLOR_PAIR(1));
                }
                refresh();

                wgetch(stdscr);
        }
        buildScreen(text,selected,displayCount); //Print the updates!!
    }

}

int main(int argc, char *argv[])
{
    initscr();
    raw();//We want character for character input
    keypad(stdscr,1);//Enable extra keys like arrowkeys
    noecho(); 
    curl_global_init(CURL_GLOBAL_ALL);

    //Incase the user doesn't specify an argument
    if (!argv[1]) {
        curl_global_cleanup(); //Dont forget to clean up!!! My whole terminal bugged cause you forgot this :)
        endwin();
        printf("Please supply a subreddit to go to e.g. /r/coding\n"); //Added a \n
        exit(0);
    }

    showSubreddit(argv[1]);
    /* we're done with libcurl, so clean it up */ 
    curl_global_cleanup();
    endwin();
    return 0;
}
