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
    int screenLineCount;
    wchar_t **screenLines;
} LinkScreen;

typedef struct {
    wchar_t *text;
    RedditComment *comment;
    unsigned int folded : 1;
    int foldCount;
    int indentCount;
} CommentLine;

typedef struct {
    RedditCommentList *list;
    int lineCount;
    int allocLineCount;
    CommentLine **lines;
    int displayed;
    int offset;
    int selected;
    int commentOpenSize;
    unsigned int commentOpen : 1;
} CommentScreen;


RedditState *globalState;


LinkScreen *linkScreenNew()
{
    LinkScreen *screen = malloc(sizeof(LinkScreen));
    memset(screen, 0, sizeof(LinkScreen));
    return screen;
}

void linkScreenFree(LinkScreen *screen)
{
    int i;
    for (i = 0; i < screen->screenLineCount; i++)
        free(screen->screenLines[i]);

    free(screen->screenLines);
    free(screen);
}

CommentLine *commentLineNew()
{
    CommentLine *line = malloc(sizeof(CommentLine));
    memset(line, 0, sizeof(CommentLine));
    return line;
}

void commentLineFree(CommentLine *line)
{
    free(line->text);
    free(line);
}

CommentScreen *commentScreenNew()
{
    CommentScreen *screen = malloc(sizeof(CommentScreen));
    memset(screen, 0, sizeof(CommentScreen));
    return screen;
}

void commentScreenFree(CommentScreen *screen)
{
    int i;
    for (i = 0; i < screen->lineCount; i++)
        commentLineFree(screen->lines[i]);

    free(screen->lines);
    free(screen);
}

void commentScreenAddLine(CommentScreen *screen, CommentLine *line)
{
    screen->lineCount++;
    if (screen->lineCount >= screen->allocLineCount) {
        screen->allocLineCount+=100;
        screen->lines = realloc(screen->lines, sizeof(CommentLine**) * screen->allocLineCount);
    }
    screen->lines[screen->lineCount - 1] = line;
}

wchar_t *createCommentLine(RedditComment *comment, int width, int indent)
{
    wchar_t *text = malloc(sizeof(wchar_t) * (width+1));
    int i, ilen = indent * 3, bodylen, texlen;

    bodylen = wcslen(comment->wbody);
    memset(text, 32, sizeof(wchar_t) * (width));
    text[width] = (wchar_t)0;

    swprintf(text + ilen, width + 1 - ilen, L"%s > ", comment->author);

    texlen = wcslen(text);
    for (i = 0; i <= width - texlen - 1; i++)
        if (i <= bodylen - 1 && comment->wbody[i] != L'\n')
            text[i + texlen] = comment->wbody[i];
        else
            text[i + texlen] = (wchar_t)32;

    return text;
}

int getCommentScreenRecurse(CommentScreen *screen, RedditComment *comment, int width, int indent)
{
    int i;
    RedditComment *current;
    CommentLine *line;
    int indentCur = indent + 1;
    int nested = 0;
    for (i = 0; i < comment->replyCount; i++) {
        current = comment->replies[i];
        line = commentLineNew();
        line->text = createCommentLine(current, width, indentCur);
        line->indentCount = indentCur;
        line->comment = current;
        commentScreenAddLine(screen, line);
        if (current->replyCount) {
            line->foldCount = getCommentScreenRecurse(screen, current, width, indentCur);
            nested += line->foldCount;
        }
        nested++;
    }
    return nested;
}

CommentScreen *getCommentScreenFromCommentList(RedditCommentList *list, int width)
{
    CommentScreen *screen;
    CommentLine *endComment;
    if (list == NULL)
        return NULL;

    screen = commentScreenNew();

    getCommentScreenRecurse(screen, list->baseComment, width, -1);

    endComment = commentLineNew();
    endComment->text = malloc(sizeof(wchar_t) * (width + 1));

    swprintf(endComment->text, width+ 1, L"More Replies (%d)", list->baseComment->totalReplyCount);

    commentScreenAddLine(screen, endComment);
    return screen;
}

/*
 * This function renders a single line on a link screen into that LinkScreen's
 * line buffer. If this is called on an already rendered line, it will be
 * rendered again.
 *
 * 'line' should be zero-based.
 * 'width' is one-based, should be the width to render the line at.
 */
void linkScreenRenderLine (LinkScreen *screen, int line, int width)
{
    size_t tmp, title;
    size_t offset;
    if (screen->screenLineCount <= line) {
        screen->screenLineCount = line + 1;
        screen->screenLines = realloc(screen->screenLines, (line + 1) * sizeof(wchar_t*));
        screen->screenLines[line] = 0;
    }

    screen->screenLines[line] = realloc(screen->screenLines[line], (width + 1) * sizeof(wchar_t));

    swprintf(screen->screenLines[line], width + 1, L"%d. [%4d] %20s - ", line + 1, screen->list->links[line]->score, screen->list->links[line]->author);

    offset = wcslen(screen->screenLines[line]);
    title = wcslen(screen->list->links[line]->wtitle);
    for (tmp = 0; tmp <= width - offset; tmp++)
        if (tmp >= title)
            screen->screenLines[line][tmp + offset] = (wchar_t)32;
        else
            screen->screenLines[line][tmp + offset] = screen->list->links[line]->wtitle[tmp];

    screen->screenLines[line][width] = (wchar_t)0;
}

/*
 * Prints a list of posts to the screen
 */
void drawScreen(LinkScreen *screen)
{
    int i, screenLines;

    if (screen == NULL)
        return ;

    erase();
    // setup colors for currently selected post


    screenLines = screen->offset + screen->displayed + 1;

    attron(COLOR_PAIR(1));

    for(i = screen->offset; i < screenLines; i++) {
        if(i == screen->selected)
            attron(COLOR_PAIR(2));

        if (screen->screenLineCount <= i)
            linkScreenRenderLine(screen, i, COLS);

        mvaddwstr(i - screen->offset, 0, screen->screenLines[i]);

        if (i == screen->selected)
            attroff(COLOR_PAIR(2));
    }

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

void commentScreenDown(CommentScreen *screen)
{
    screen->selected++;
    if (screen->selected > screen->offset + screen->displayed) {
        if (screen->offset + screen->displayed + 1 < screen->lineCount)
            screen->offset++;
        else
            screen->selected--;
    } else if (screen->selected > screen->lineCount - 1) {
        screen->selected--;
    }
}

void commentScreenUp(CommentScreen *screen)
{
    screen->selected--;
    if (screen->selected < 0)
        screen->selected++;
    if (screen->selected < screen->offset)
        screen->offset--;
}

void commentScreenDisplay(CommentScreen *screen)
{
    int i, screenLines;
    wchar_t *tmpbuf;
    int bufSize, lastLine, bufLen;

    if (screen == NULL)
        return ;

    erase();

    screenLines = screen->offset + screen->displayed + 1;

    if (screenLines > screen->lineCount)
        screenLines = screen->lineCount;
    attron(COLOR_PAIR(1));

    for(i = screen->offset; i < screenLines; i++) {
        if(i == screen->selected)
            attron(COLOR_PAIR(2));

        if (screen->lines[i]->text != NULL)
            mvaddwstr(i - screen->offset, 0, screen->lines[i]->text);

        if (i == screen->selected)
            attron(COLOR_PAIR(1));
    }

    if (screen->commentOpen) {
        RedditComment *current;
        lastLine = screenLines - screen->offset;
        bufSize = sizeof(wchar_t) * (COLS + 1);
        bufLen = COLS;
        tmpbuf = malloc(bufSize);
        memset(tmpbuf, 0, bufSize);

        for (i = 0; i < bufLen; i++)
            tmpbuf[i] = L'-';

        tmpbuf[bufLen] = (wchar_t)0;
        attron(COLOR_PAIR(2));
        mvaddwstr(lastLine, 0, tmpbuf);

        attron(COLOR_PAIR(1));
        if (screen->lineCount >= screen->selected) {
            current = screen->lines[screen->selected]->comment;
            if (current != NULL) {
                swprintf(tmpbuf, bufLen, L"%s - %d Up / %d Down", current->author, current->ups, current->downs);
                mvaddwstr(lastLine + 1, 0, tmpbuf);
                swprintf(tmpbuf, bufLen, L"-------");
                mvaddwstr(lastLine + 2, 0, tmpbuf);

                mvaddwstr(lastLine + 3, 0, current->wbody);
            }
        }
        free(tmpbuf);
    }

    refresh();
}

void commentScreenOpenComment(CommentScreen *screen)
{
    if (!screen->commentOpen) {
        screen->commentOpen = 1;
        screen->displayed -= screen->commentOpenSize - 1;
        if (screen->selected > screen->displayed + screen->offset)
            screen->offset = screen->selected - screen->displayed;
    }
}

void commentScreenCloseComment(CommentScreen *screen)
{
    if (screen->commentOpen) {
        screen->commentOpen = 0;
        screen->displayed += screen->commentOpenSize - 1;
    }
}

void commentScreenToggleComment(CommentScreen *screen)
{
    if (screen->commentOpen)
        commentScreenCloseComment(screen);
    else
        commentScreenOpenComment(screen);
}

void showThread(RedditLink *link)
{
    CommentScreen *screen = NULL;
    RedditCommentList *list = NULL;
    RedditErrno err;

    if (link == NULL)
        return ;

    list = redditCommentListNew();
    list->permalink = redditCopyString(link->permalink);

    err = redditGetCommentList(list);
    if (err != REDDIT_SUCCESS)
        goto cleanup;

    screen = getCommentScreenFromCommentList(list, COLS);

    screen->offset = 0;
    screen->selected = 0;
    screen->displayed = LINES - 1;
    screen->commentOpenSize = LINES / 3 * 2;
    commentScreenDisplay(screen);
    int c;
    while((c = wgetch(stdscr))) {
        switch(c) {
            case 'j': case KEY_DOWN:
                commentScreenDown(screen);
                commentScreenDisplay(screen);
                break;
            case 'k': case KEY_UP:
                commentScreenUp(screen);
                commentScreenDisplay(screen);
                break;

            case 'l': case '\n': case KEY_ENTER:
                commentScreenToggleComment(screen);
                commentScreenDisplay(screen);
                break;
            case 'q': case 'h':
                if (screen->commentOpen) {
                    commentScreenCloseComment(screen);
                    commentScreenDisplay(screen);
                } else {
                    goto cleanup;
                }
                break;
        }
    }

cleanup:;
    redditCommentListFree(list);
    commentScreenFree(screen);
    return ;
}

void showSubreddit(char *subreddit)
{
    LinkScreen *screen;
    screen = linkScreenNew();

    screen->list = redditLinkListNew();
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
    while((c = wgetch(stdscr))) {
        if(c == 'q') //Lets make a break key, so i dont have to close the tab like last time :S
            break;//YEA FUCK YOU WHILE
        switch(c) {
            case 'k': case KEY_UP:
                linkScreenUp(screen);
                drawScreen(screen);
                break;

            case 'j': case KEY_DOWN:
                linkScreenDown(screen);
                drawScreen(screen);
                break;

            case 'l': case '\n': // Display selected thread
                showThread(screen->list->links[screen->selected]);
                drawScreen(screen);
                break;
        }
    }

    redditLinkListFree(screen->list);
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
    //redditUserLoggedLogin(user, "username", "password");

    showSubreddit(argv[1]);

    redditUserLoggedFree(user);
    redditStateFree(globalState);
    redditGlobalCleanup();
    endwin();
    return 0;
}

