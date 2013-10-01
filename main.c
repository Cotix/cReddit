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
    int allocLineCount;
    wchar_t **screenLines;
    int linkOpenSize;
    unsigned int linkOpen : 1;
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
    if (screen == NULL)
        return ;
    for (i = 0; i < screen->allocLineCount; i++)
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
    if (line == NULL)
        return ;
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
    if (screen == NULL)
        return ;
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
    if (list == NULL)
        return NULL;

    screen = commentScreenNew();

    getCommentScreenRecurse(screen, list->baseComment, width, -1);

    return screen;
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

    //erase();

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

    bufSize = sizeof(wchar_t) * (COLS + 1);
    bufLen = COLS;
    tmpbuf = malloc(bufSize);
    memset(tmpbuf, 0, bufSize);

    if (screen->commentOpen) {
        RedditComment *current;
        lastLine = screenLines - screen->offset;

        for (i = 0; i < bufLen; i++)
            tmpbuf[i] = L' ';

        for (i = lastLine; i < lastLine + screen->commentOpenSize; i++)
            mvaddwstr(i, 0, tmpbuf);

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
    } else if (screenLines < screen->displayed + screen->offset + 1) {
        for (i = 0; i < bufLen; i++)
            tmpbuf[i] = L' ';

        for (i = screenLines; i < screen->displayed + screen->offset + 1; i++)
            mvaddwstr(i, 0, tmpbuf);
    }

    free(tmpbuf);
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
    if (err != REDDIT_SUCCESS || list->baseComment->replyCount == 0)
        goto cleanup;

    screen = getCommentScreenFromCommentList(list, COLS);

    screen->offset = 0;
    screen->selected = 0;
    screen->displayed = LINES - 1;
    screen->commentOpenSize = (screen->displayed / 5) * 4;
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
    if (screen->allocLineCount <= line) {
        screen->allocLineCount = line + 100;
        screen->screenLines = realloc(screen->screenLines, screen->allocLineCount * sizeof(wchar_t*));
        memset(screen->screenLines + screen->allocLineCount - 100, 0, sizeof(wchar_t*) * 100);
    }

    if (screen->screenLineCount < line)
        screen->screenLineCount = line;

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
    wchar_t *tmpbuf;
    int bufSize, lastLine, bufLen;

    if (screen == NULL)
        return ;

    //erase();

    screenLines = screen->offset + screen->displayed + 1;
    if (screenLines > screen->list->linkCount)
        screenLines = screen->list->linkCount;

    attron(COLOR_PAIR(1));

    for(i = screen->offset; i < screenLines; i++) {
        if(i == screen->selected)
            attron(COLOR_PAIR(2));

        if (screen->screenLineCount <= i)
            linkScreenRenderLine(screen, i, COLS);

        mvaddwstr(i - screen->offset, 0, screen->screenLines[i]);

        if (i == screen->selected)
            attron(COLOR_PAIR(1));
    }

    bufSize = sizeof(wchar_t) * (COLS + 1);
    bufLen = COLS;
    tmpbuf = malloc(bufSize);
    memset(tmpbuf, 0, bufSize);

    if (screen->linkOpen) {
        RedditLink *current;
        lastLine = screenLines - screen->offset;

        for (i = 0; i < bufLen; i++)
            tmpbuf[i] = L' ';

        for (i = lastLine; i < lastLine + screen->linkOpenSize; i++)
            mvaddwstr(i, 0, tmpbuf);

        for (i = 0; i < bufLen; i++)
            tmpbuf[i] = L'-';

        tmpbuf[bufLen] = (wchar_t)0;
        attron(COLOR_PAIR(2));
        mvaddwstr(lastLine, 0, tmpbuf);

        attron(COLOR_PAIR(1));
        if (screen->list->linkCount >= screen->selected) {
            current = screen->list->links[screen->selected];
            if (current != NULL) {
                swprintf(tmpbuf, bufLen, L"%s - %d Score / %d Up / %d Down / %d Comments\nTitle: ", current->author, current->score, current->ups, current->downs, current->numComments);
                mvaddwstr(lastLine + 1, 0, tmpbuf);
                addwstr(current->wtitle);
                addch('\n');
                swprintf(tmpbuf, bufLen, L"-------\n");
                addwstr(tmpbuf);

                if (current->flags & REDDIT_LINK_IS_SELF)
                    addwstr(current->wselftext);
                else
                    addstr(current->url);
            }
        }
    } else if (screenLines < screen->displayed + screen->offset + 1) {
        for (i = 0; i < bufLen; i++)
            tmpbuf[i] = L' ';

        for (i = screenLines; i < screen->displayed + screen->offset + 1; i++)
            mvaddwstr(i, 0, tmpbuf);
    }

    free(tmpbuf);

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
    } else if (screen->selected + 1 > screen->list->linkCount) {
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

void linkScreenOpenLink(LinkScreen *screen)
{
    if (!screen->linkOpen) {
        screen->linkOpen = 1;
        screen->displayed -= screen->linkOpenSize - 1;
        if (screen->selected > screen->displayed + screen->offset)
            screen->offset = screen->selected - screen->displayed;
    }
}

void linkScreenCloseLink(LinkScreen *screen)
{
    if (screen->linkOpen) {
        screen->linkOpen = 0;
        screen->displayed += screen->linkOpenSize - 1;
    }
}

void linkScreenToggleLink(LinkScreen *screen)
{
    if (screen->linkOpen)
        linkScreenCloseLink(screen);
    else
        linkScreenOpenLink(screen);
}

void showSubreddit(const char *subreddit)
{
    LinkScreen *screen;
    screen = linkScreenNew();

    screen->list = redditLinkListNew();
    screen->list->subreddit = redditCopyString(subreddit);
    screen->list->type = REDDIT_HOT;

    redditGetListing(screen->list);

    screen->displayed = LINES - 1;
    screen->linkOpenSize = (screen->displayed / 5) * 4;

    screen->offset = 0;
    screen->selected = 0;


    drawScreen(screen); //And print the screen!

    int c;
    while((c = wgetch(stdscr))) {
        switch(c) {
            case 'k': case KEY_UP:
                linkScreenUp(screen);
                drawScreen(screen);
                break;

            case 'j': case KEY_DOWN:
                linkScreenDown(screen);
                drawScreen(screen);
                break;
            case 'q':
                if (screen->linkOpen) {
                    linkScreenCloseLink(screen);
                    drawScreen(screen);
                } else {
                    goto cleanup;
                }
                break;
            case 'u':
                redditLinkListFreeLinks(screen->list);
                redditGetListing(screen->list);
                screen->offset = 0;
                screen->selected = 0;
                drawScreen(screen);
                break;
            case 'l': case '\n': case KEY_ENTER:
                linkScreenToggleLink(screen);
                drawScreen(screen);
                break;
            case 'L':
                redditGetListing(screen->list);
                drawScreen(screen);
                break;
            case 'c':
                showThread(screen->list->links[screen->selected]);
                drawScreen(screen);
                break;
        }
    }

cleanup:;
    redditLinkListFree(screen->list);
    linkScreenFree(screen);
}

int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre), lenstr = strlen(str);
    return (lenstr < lenpre) ? 0 : strncmp(pre, str, lenpre) == 0;
}

/*
 * Prepends 'pre' onto the front of the string buffer 'str'
 * Make sure 'str' is large enough to fit 'pre' on it.
 */
void prepend(const char *pre, char *str)
{
    size_t lenpre = strlen(pre), lenstr = strlen(str);

    /* Move the Strings memory forward */
    memcpy(str + lenpre, str, lenstr + 1);
    /* Copy pre into the new space */
    memcpy(str, pre, lenpre);
}

int main(int argc, char *argv[])
{
    RedditUserLogged *user = NULL;
    char *subreddit = NULL;

    if (argc > 1) {
        subreddit = argv[1];
        /* Display a simple help screen */
        if (!startsWith("/r/", subreddit) && strcmp("/", subreddit) != 0) {
            subreddit = malloc((strlen(argv[1]) + 4) * sizeof(char));
            strcpy(subreddit, argv[1]);
            prepend("/r/", subreddit);
        }

        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "help") == 0) {
            printf("Usage: %s [subreddit] [username] [password]\n", argv[0]);
            printf("\n");
            printf(" subreddit -- The name of a subreddit you want to view, Ex. '/r/coding', '/r/linux'\n");
            printf(" username  -- The username of a user you want to login as.\n");
            printf(" password  -- The password for the username, if one was given\n");
            printf("\n");
            printf("To report any bugs, submit patches, etc. Please see the github page at:\n");
            printf("http://www.github.com/Cotix/cReddit\n");
            return 0;
        }
    } else {
        subreddit = "/";
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

    if (argc == 4) {
        user = redditUserLoggedNew();
        /* If you want to try logging in as your user
         * Replace 'username' and 'password' with the approiate fields */
        redditUserLoggedLogin(user, argv[2], argv[3]);

    }
    showSubreddit(subreddit);

    redditUserLoggedFree(user);
    redditStateFree(globalState);
    redditGlobalCleanup();
    endwin();
    return 0;
}

