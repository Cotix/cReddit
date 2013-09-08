#define _XOPEN_SOURCE_EXTENDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "reddit.h"
#include <ncurses.h>
#include <form.h>
#include <locale.h>

#define SIZEOFELEM(x)  (sizeof(x) / sizeof(x[0]))

typedef struct {
    RedditLinkList *list;
    int displayed;
    int offset;
    int selected;
} LinkScreen;

typedef struct {
    RedditCommentList *list;

} CommentStr;


RedditState *globalState;


LinkScreen *linkScreenNew()
{
    LinkScreen *screen = malloc(sizeof(LinkScreen));
    memset(screen, 0, sizeof(LinkScreen));
    screen->list = redditLinkListNew();
    return screen;
}

void linkScreenFree(LinkScreen *screen)
{
    redditLinkListFree(screen->list);
    free(screen);
}


/*
   Prints a list of posts to the screen
   */
void drawScreen(LinkScreen *screen)
{
    int i, screenLines;
    wchar_t buffer[COLS + 1];
    wchar_t format[2048];

    if (screen == NULL)
        return ;

    erase();
    // setup colors for currently selected post


    screenLines = screen->offset + screen->displayed + 1;

    attron(COLOR_PAIR(1));

    for(i = screen->offset; i < screenLines; i++) {
        size_t bufLen;
        if(i == screen->selected) {
            attroff(COLOR_PAIR(1));
            attron(COLOR_PAIR(2));
        } else {
            attron(COLOR_PAIR(1));
        }
        swprintf(buffer, COLS + 1, L"%d. [%4d] %20s - ", i + 1, screen->list->links[i]->score, screen->list->links[i]->author); //, screen->list->links[i]->wtitle);
        bufLen = wcslen(buffer);
        swprintf(format, 2048, L"%%-%dls\n", COLS - bufLen);
        swprintf(buffer + bufLen, COLS - bufLen + 1, format, screen->list->links[i]->wtitle);

        addwstr(buffer);
        if (i == screen->selected)
            attroff(COLOR_PAIR(2));
    }

    // draw things on the screen
    touchwin(stdscr);
    refresh();
}

void linkScreenDown(LinkScreen *screen)
{
    screen->selected++;
    if (screen->selected > screen->offset + screen->displayed) {
        if (screen->offset + screen->displayed + 1 < screen->list->linkCount)
            screen->offset++;
        else
            screen->selected--;

    }
}

void linkScreenUp(LinkScreen *screen)
{
    screen->selected--;
    if (screen->selected < 0)
        screen->selected++;
    if (screen->selected < screen->offset)
        screen->offset--;
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
    LinkScreen *screen;
    screen = linkScreenNew();

    screen->list->subreddit = redditCopyString(subreddit);
    screen->list->type = REDDIT_HOT;

    redditGetListing(screen->list);

    if (screen->list->linkCount < LINES - 1)
        screen->displayed = screen->list->linkCount - 1;
    else
        screen->displayed = LINES - 1;

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

    linkScreenFree(screen);
}

int main(int argc, char *argv[])
{
    RedditUserLogged *user = redditUserLoggedNew();
    //Incase the user doesn't specify an argument
    if (!argv[1]) {
        wprintf(L"Please supply a subreddit to go to e.g. /r/coding\n"); //Added a \n
        exit(1);
    }

    setlocale(LC_CTYPE, "");


    initscr();
    raw();//We want character for character input
    keypad(stdscr,1);//Enable extra keys like arrowkeys
    noecho();
    start_color();
    use_default_colors();
    init_pair(1, -1, -1);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);

    /* Start libreddit */
    redditGlobalInit();

    globalState = redditStateNew();

    redditStateSet(globalState);

    /* If you want to try logging in as your user
     * Replace 'username' and 'password' with the approiate fields */
    //redditUserLoggedLogin(user, "", "");

    showSubreddit(argv[1]);

    redditUserLoggedFree(user);
    redditStateFree(globalState);
    redditGlobalCleanup();
    endwin();
    return 0;
}

