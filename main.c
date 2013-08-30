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
    for(i = 0; i < numposts; i++)
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

void buildCommentScreen(comment *comments, int selected, int numposts)
{
    clear();
    // setup colors for currently selected post
    start_color();
    init_pair(1, COLOR_RED, COLOR_WHITE);

    int i;
    for(i = 0; i < numposts; i++)
    {
        if(i == selected) attron(COLOR_PAIR(1));

        if(comments[i].author != NULL)
            printw("%s\n", comments[i].author);

        if(comments[i].text != NULL)
            printw("%s\n", comments[i].text);

        attroff(COLOR_PAIR(1));
    }

    // draw things on the screen
    refresh();
}

void showThread(post *posts, int selected, int displayCount) {
    clear();
    // = {0} to avoid accessing unitialized memory
    comment cList[500] = {0};
    int *commentCount = malloc(sizeof(int));
    redditGetThread(posts[selected].id, cList, commentCount, displayCount);
    int cdisplayCount = displayCount;
    if (*commentCount < displayCount) {
        cdisplayCount = *commentCount;
    }
    free(commentCount);

    // Basically a copy of the code above
    start_color();
    // init_pair(1,COLOR_CYAN,COLOR_MAGENTA);

    char *ctext[cdisplayCount]; //Text buffer for each line
    int u;
    for(u = 0; u < cdisplayCount; u++)
    {
        if(cList[u].id == 0 || cList[u].text == NULL || cList[u].id == NULL || cList[u].author == NULL)
            continue;
        /*printComment(cList[u].author, cList[u].text);*/
        /*attroff(COLOR_PAIR(1));*/
    }
    int selectedComment = 0;

    refresh();
    buildCommentScreen(cList, selectedComment, cdisplayCount);

    int c;
    while(c =wgetch(stdscr))
    {
        if(c == 'q')
            break;
        switch(c)
        {
            case 'j': case KEY_DOWN:
                if (selectedComment == cdisplayCount-1) {
                    showThread(posts, selected, displayCount+25);
                } else {
                    selectedComment++;
                    refresh();
                }
                break;

            case 'k': case KEY_UP:
                if (selectedComment != 0){
                    selectedComment--;
                    refresh();
                }
                break;
        }
        buildCommentScreen(cList, selectedComment, cdisplayCount);

    }
}

void showSubreddit(char *subreddit)
{
    post posts[25];                         // array with reddit posts
    int *postCount = malloc(sizeof(int));   // number of posts

    redditGetSubreddit(subreddit, "hot", posts, postCount);

    // we will display 25 posts at max right now
    int displayCount = 25;
    if (*postCount < 25) {
        displayCount = *postCount;
    }
    free(postCount);

    char *text[displayCount];    //Text buffer for each line

    // write the post list to the screen 
    int i;
    for(i = 0; i < displayCount; i++)
    {
        if(posts[i].id == 0) // first post actually has id of 1?
            continue;

        char buffer[2048];      //Lets make a bigg ass text buffer so we got enough space

        // add the post number with some formatting
        //strcpy(buffer, posts[i].id);
        if (i < 9) sprintf(buffer, " %d:", i+1);
        else sprintf(buffer, "%d:", i+1);

        // add the votes with some janky formatting
        strcat(buffer, " [");
        char str_votes[10];
        strcpy(str_votes, posts[i].votes);
        switch (strlen(str_votes)) {
            case 3: 
                strcat(buffer, " ");
                break;
            case 2:
                strcat(buffer, "  ");
                break;
            case 1:
                strcat(buffer, "   ");
                break;
            default: break;
        }
        strcat(buffer, posts[i].votes);
        strcat(buffer, "] ");

        strcat(buffer, posts[i].title);
        strcat(buffer, " - ");
        strcat(buffer, posts[i].author);

        text[i] = (char*) malloc(strlen(buffer)); //Now lets make a small buffer that fits exacly!
        // And safely copy our data into it!
        text[i][0] = '\0';
        strncat(text[i], buffer, strlen(buffer) - 1);
    }

    int selected = 0; //Lets select the first post!
    buildScreen(text, selected, displayCount); //And print the screen!

    int c;
    /*comment cList[500];*/
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
                showThread(posts, selected, 25);
        }
        buildScreen(text, selected, displayCount); //Print the updates!!
    }

    // free text after printing
    int j;
    for (j = 0; j < displayCount; j++) {
        free(text[j]);
        // free the post list
        free(posts[j].subreddit);
        free(posts[j].author);
        free(posts[j].title);
        free(posts[j].votes);
        free(posts[j].id);
    }
}

int main(int argc, char *argv[])
{
    //Incase the user doesn't specify an argument
    if (!argv[1]) {
        printf("Please supply a subreddit to go to e.g. /r/coding\n"); //Added a \n
        exit(1);
    }

    initscr();
    raw();//We want character for character input
    keypad(stdscr,1);//Enable extra keys like arrowkeys
    noecho(); 
    curl_global_init(CURL_GLOBAL_ALL);


    showSubreddit(argv[1]);
    /* we're done with libcurl, so clean it up */ 
    curl_global_cleanup();
    endwin();
    return 0;
}

