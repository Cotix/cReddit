#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "reddit.h"
#include <ncurses.h>
#include <form.h>

#define SIZEOFELEM(x)  (sizeof(x) / sizeof(x[0]))


typedef struct {
    reddit_link_list *list;
    int displayed;
    int offset;
    int selected;
} link_screen;


reddit_state *global_state;


link_screen *link_screen_new()
{
    link_screen *screen = malloc(sizeof(link_screen));
    memset(screen, 0, sizeof(link_screen));
    screen->list = reddit_link_list_new();
    return screen;
}

void link_screen_free(link_screen *screen)
{
    reddit_link_list_free(screen->list);
    free(screen);
}


/*
   Prints a list of posts to the screen
   */
void drawScreen(link_screen *screen)
{
    int i;
    reddit_link *link = NULL;
    if (screen == NULL)
        return ;

    erase();
    // setup colors for currently selected post
    start_color();
    init_pair(1, COLOR_RED, COLOR_WHITE);

    link = screen->list->first;
    for(i = 0; i < screen->offset; i++){
        link = link->next;
    }

    for(i = 0; i < screen->displayed; i++)
    {
        if(i == screen->selected) attron(COLOR_PAIR(1));
        printw("%d. [%4d] %s - %s\n", i + screen->offset + 1, link->score, link->author, link->title);
        attroff(COLOR_PAIR(1));
        link = link->next;
    }

    // draw things on the screen
    refresh();
}

void linkScreenDown(link_screen *screen)
{
    screen->selected++;
    if (screen->selected + 1 > screen->displayed) {
        screen->selected--;
        if (screen->offset + screen->displayed + 1 < screen->list->link_count)
            screen->offset++;

    }
}

void linkScreenUp(link_screen *screen)
{
    screen->selected--;
    if (screen->selected < 0) {
        screen->selected++;
        if (screen->offset > 0)
            screen->offset--;
    }
}

/*
 * FIXME: COMMENTS DO !NOT! WORK YET
 */
#if 0
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

void buildCommentScreen(Comment *comments, int selected, int numposts)
{
    erase();
    // setup colors for currently selected post
    start_color();
    init_pair(1, COLOR_RED, COLOR_WHITE);

    int i;
    for(i = 0; i < numposts; i++)
    {
        if(i == selected) attron(COLOR_PAIR(1));

        // Make the op look different
        if (i == 0) {
            // Here pass the contents of the author's post on as a comment and format it differently if you like
            printComment(comments[i].author, comments[i].text);
            continue;
        }

        // Handle all other commenters
        if (comments[i].author != NULL && comments[i].text != NULL)
            printComment(comments[i].author, comments[i].text);
        else if (comments[i].author != NULL)
            printComment(comments[i].author, "");
        else if (comments[i].text != NULL)
            printComment("Unkown", comments[i].text);

        attroff(COLOR_PAIR(1));
    }

    // draw things on the screen
    refresh();
}

bool showThread(Post *posts, int selected, int displayCount) {
    erase();
    // = {0} sets all to NULL to avoid accessing unitialized memory
    Comment cList[500] = {0};
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
    int selectedComment = displayCount-26;
    if (selectedComment > cdisplayCount)
        selectedComment = cdisplayCount-1;
    if (selectedComment < 0)
        selectedComment++;

    refresh();
    buildCommentScreen(cList, selectedComment, cdisplayCount);

    int c;
    bool breakNow = false;
    while(c = wgetch(stdscr))
    {
        if (c == 'q' || c ==  'h' || breakNow == true)
            return true;

        switch(c)
        {
            case 'j': case KEY_DOWN:
                if (selectedComment == cdisplayCount-1) {
                    if (true == showThread(posts, selected, displayCount+25)) {
                        return true;
                    }

                } else {
                    selectedComment++;
                    /*refresh();*/
                }
                break;
            case 'k': case KEY_UP:
                if (selectedComment != 0){
                    selectedComment--;
                    /*refresh();*/
                }
                break;

            case 'l': case '\n': 
                // TODO
                // Open link/image/whateveryawant
                // if(selectedComment == 0)
                    // getUrlImageWhatevs(cList[0].text);
                break;
        }
        buildCommentScreen(cList, selectedComment, cdisplayCount);

    }
}
#endif

void showSubreddit(char *subreddit)
{
    link_screen *screen;
    screen = link_screen_new();

    screen->list->subreddit = reddit_copy_string(subreddit);
    screen->list->type = REDDIT_HOT;

    reddit_get_listing(screen->list);

    screen->displayed = 25;
    screen->offset = 0;
    screen->selected = 0;


    drawScreen(screen); //And print the screen!

    int c;
    while((c = wgetch(stdscr)))
    {
        if(c == 'q') //Lets make a break key, so i dont have to close the tab like last time :S
            break;//YEA FUCK YOU WHILE
        switch(c)
        {
            case 'k': case KEY_UP:
                linkScreenUp(screen);
                drawScreen(screen);
                break;

            case 'j': case KEY_DOWN:
                linkScreenDown(screen);
                drawScreen(screen);
                break;

            case 'l': case '\n': // Display selected thread
                //showThread(posts, selected, 25);
                drawScreen(screen);
                break;
        }
    }

    link_screen_free(screen);
}

int main(int argc, char *argv[])
{
    reddit_user_logged *user = reddit_user_logged_new();
    //Incase the user doesn't specify an argument
    if (!argv[1]) {
        printf("Please supply a subreddit to go to e.g. /r/coding\n"); //Added a \n
        exit(1);
    }

    initscr();
    raw();//We want character for character input
    keypad(stdscr,1);//Enable extra keys like arrowkeys
    noecho();

    /* Start libreddit */
    reddit_global_init();

    global_state = reddit_state_new();

    reddit_state_set(global_state);

    /* If you want to try logging in as your user
     * Replace 'username' and 'password' with the approiate fields */
    reddit_user_logged_login(user, "creddit_test_user", "password");

    showSubreddit(argv[1]);

    reddit_user_logged_free(user);
    reddit_state_free(global_state);
    reddit_global_cleanup();
    endwin();
    return 0;
}

