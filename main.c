#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include "reddit.h"
#include <curl/curl.h>
#include <ncurses.h>
#include <form.h>

#define SIZEOFELEM(x)  (sizeof(x) / sizeof(x[0]))

/*
   Prints a list of posts to the screen
*/
void buildScreen(char **posts, int selected, int numposts)
{
    clear();
    // setup colors for currently selected post
    start_color();
    init_pair(1, COLOR_RED, COLOR_WHITE);

    int i;
    for(i = 0; i != numposts; i++)
    {
        if(i == selected) attron(COLOR_PAIR(1));
        printw("%s\n", posts[i]);
        attroff(COLOR_PAIR(1));
    }

    // draw things on the screen
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
void printComment(char *author, char *text) {
    printHLine(COLS);
    attron(COLOR_PAIR(1));
    printw("%s\n", author);
    attroff(COLOR_PAIR(1));
    printw("    %s\n", text);
}

void showSubreddit(char *subreddit)
{
    post posts[25];             //Our array with reddit posts
    
    int *postCount;
    postCount = malloc(sizeof(int));
    
    redditGetSubreddit(subreddit, "hot", posts, postCount);
    
    int displayCount = 25;
    if (*postCount < 25) {
        displayCount = *postCount;
    }
  
    free(postCount); // done with postCount now

    char *text[displayCount];    //Text buffer for each line
   
    // write the post list to the screen 
    int i;
    for(i = 0; i < displayCount; i++)
    {
        if(posts[i].id == 0)// because id 0 is the actual post?
            continue;
        
        char buffer[2048];      //Lets make a bigg ass text buffer so we got enough space
        //strcpy(buffer, posts[i].id);
        sprintf(buffer, "%d:", i+1);
        
        // add the votes with some janky formatting
        strcat(buffer, " [");
        //char str_votes[10];
        //strcpy(str_votes, posts[i].votes);
        //int votes = atoi(str_votes);
        //if (votes < 10) strcat(buffer, " ");
        //else strcat(buffer, "  ");
        // switch (strlen(str_votes)) {
        //     case 3: 
        //         strcat(buffer, " ");
        //         break;
        //     case 2:
        //         strcat(buffer, "  ");
        //         break;
        //     case 1:
        //         strcat(buffer, "   ");
        //         break;
        //     deafult: break;
        // }
        //if (votes < 1000) strcat(buffer, " ");
        // if (votes < 1000) {
        //     strcat(buffer, " ");
        //     if (votes < 100) {
        //         strcat(buffer, " ");
        //         if (votes < 10) strcat(buffer, " ");
        //     }
        // }
        strcat(buffer, posts[i].votes);
        strcat(buffer, "] ");
        
        strcat(buffer, posts[i].title);
        strcat(buffer, " - ");
        strcat(buffer, posts[i].author);

        text[i] = (char*) malloc(strlen(buffer)); //Now lets make a small buffer that fits exacly!
        strcpy(text[i], buffer); //And copy our data into it!
        
        // free the post list
        free(posts[i].subreddit);
        free(posts[i].author);
        free(posts[i].title);
        free(posts[i].votes);
        free(posts[i].id);
    }

    int selected = 0; //Lets select the first post!
    buildScreen(text, selected, displayCount); //And print it!

    // free text after printing
    int j;
    for (j = 0; j < displayCount; j++) {
        free(text[j]);
    } 

    int c;
    comment cList[500];
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

            case 'l': case '\n': // Display selected thread
                refresh();
                int *commentCount;
                commentCount = malloc(sizeof(int));
                redditGetThread(posts[selected].id, cList, commentCount);
                int cdisplayCount = 25;
                if (*commentCount < 25) {
                    cdisplayCount=*postCount;
                }
                // Basically a copy of the code above
                int u;

                clear();
                start_color();
                // init_pair(1,COLOR_CYAN,COLOR_MAGENTA);

                char *ctext[cdisplayCount]; //Text buffer for each line
                for(u = 0; u != cdisplayCount; ++u)
                {
                    //printw("starting");
                    if(cList[u].id == 0 || cList[u].text == NULL || cList[u].id == NULL || cList[u].author == NULL)
                        continue;
                    char cbuffer[2048];
                    printComment(cList[u].author, cList[u].text);
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
