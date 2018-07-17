#define _XOPEN_SOURCE_EXTENDED

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "reddit.h"
#if defined(CREDDIT_USE_NCURSESW)
#include <ncursesw/curses.h>
#include <ncursesw/form.h>
#else
#include <ncurses.h>
#include <form.h>
#endif
#include <locale.h>

#include "global.h"
#include "opt.h"


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
    unsigned int helpOpen : 1;
    int helpLineCount;
    wchar_t **helpText;
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
    int width;
    int commentOpenSize;
    unsigned int commentOpen : 1;
} CommentScreen;


RedditState *globalState;

wchar_t *linkScreenHelp[] = {
    L"Keypresses:",
    L"Link Screen:",
    L"- k / UP -- Move up one link in the list",
    L"- j / DOWN -- Move down one link in the list",
    L"- K -- Scroll up link text of open link",
    L"- J -- Scroll down link text of open link",
    L"- L -- Get the next list of Links from Reddit",
    L"- u -- Update the list (Clears the list of links, and then gets a new list from Reddit)",
    L"- l / ENTER -- Open the selected link",
    L"- c -- Display the comments for the selected link",
    L"- q -- Close open Link, or exit program if no link is open",
    L"- [1 - 5] -- Switch between subreddit list types (hot, new, rising, controversial, top)",
    L"- ? -- Opens the help(but you wouldn't be reading this if you couldn't figure that out would you?)",
    L"",
    L"Comment screen:",
    L"- k / UP -- Move up one comment in the list",
    L"- j / DOWN -- Move down one comment in the list",
    L"- l / ENTER -- Open the selected comment",
    L"- K -- Scroll up comment text of open comment",
    L"- J -- Scroll down comment text of open comment",
    L"= PGUP -- Move up one comment at the same depth",
    L"- PGDN -- Move down one comment at the same depth",
    L"- q / h -- Close the open comment, or close the comment screen if no comment is open",
    L"",
    L"To report any bugs, submit patches, etc. Please see the github page at:",
    L"http://www.github.com/Cotix/cReddit\n"
};

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

void linkScreenSetup(LinkScreen *screen, const char *subreddit, const RedditListType listType)
{
    screen->list = redditLinkListNew();
    screen->list->subreddit = redditCopyString(subreddit);
    screen->list->type = listType;

    redditGetListing(screen->list);

    screen->displayed = LINES - 1;
    screen->linkOpenSize = (screen->displayed / 5) * 4;

    screen->offset = 0;
    screen->selected = 0;

    /* Assign help-screen text */
    screen->helpText = linkScreenHelp;
    screen->helpLineCount = sizeof(linkScreenHelp)/sizeof(linkScreenHelp[0]);
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

void commentScreenFreeLines (CommentScreen *screen)
{
    int i;
    if (screen == NULL)
        return ;
    for (i = 0; i < screen->lineCount; i++)
        commentLineFree(screen->lines[i]);
    free(screen->lines);
    screen->lines = NULL;
    screen->lineCount = 0;
    screen->allocLineCount = 0;
}

void commentScreenFree(CommentScreen *screen)
{
    if (screen == NULL)
        return ;
    commentScreenFreeLines(screen);

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

    bodylen = wcslen(comment->wbodyEsc);
    memset(text, 32, sizeof(wchar_t) * (width));
    text[width] = (wchar_t)0;

    if (comment->directChildrenCount > 0)
        swprintf(text + ilen, width + 1 - ilen, L"%s (%d hidden) > ", comment->author, comment->totalReplyCount);
    else
        swprintf(text + ilen, width + 1 - ilen, L"%s > ", comment->author);

    texlen = wcslen(text);
    for (i = 0; i <= width - texlen - 1; i++)
        if (i <= bodylen - 1 && comment->wbodyEsc[i] != L'\n')
            text[i + texlen] = comment->wbodyEsc[i];
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

void commentScreenRenderLines (CommentScreen *screen)
{
    if (screen->lines)
        commentScreenFreeLines(screen);

    getCommentScreenRecurse(screen, screen->list->baseComment, screen->width, -1);
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

void commentScreenLevelDown(CommentScreen *screen) 
{
    int newPosition = screen->selected;
    int indentCount = screen->lines[screen->selected]->indentCount;
    while (++newPosition < screen->lineCount) {
        if (screen->lines[newPosition]->indentCount < indentCount)
            return;
        if (screen->lines[newPosition]->indentCount == indentCount) {
            screen->offset += newPosition - screen->selected;
            screen->selected = newPosition;
            return;
        }
    }
}

void commentScreenLevelUp(CommentScreen *screen)
{
    int newPosition = screen->selected;
    int indentCount = screen->lines[screen->selected]->indentCount;
    while (--newPosition >= 0) {
        if (screen->lines[newPosition]->indentCount < indentCount)
            return;
        if (screen->lines[newPosition]->indentCount == indentCount) {
            screen->offset += newPosition - screen->selected;
            if (screen->offset < 0) screen->offset = 0;
            screen->selected = newPosition;
            return;
        }
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

void commentScreenCommentScrollDown(CommentScreen *screen)
{
    RedditComment *current = screen->lines[screen->selected]->comment;
    wchar_t *currentTextPointer = &(current->wbodyEsc[current->advance]);
    
    wchar_t *foundNewLineString = wcschr(currentTextPointer, L'\n');

    // We found a \n char, so advance to it if it's before end-of-screen
    if (foundNewLineString != NULL) 
    {
        unsigned int distanceToNewLine = foundNewLineString - currentTextPointer + 1;
        if (distanceToNewLine < screen->width)
        {
            current->advance += distanceToNewLine;
            return;
        }
    }

    if (wcslen(currentTextPointer) > screen->width)
        current->advance += screen->width;
}

void commentScreenCommentScrollUp(CommentScreen *screen)
{
    RedditComment *current = screen->lines[screen->selected]->comment;

    if (current->advance == 0)
        return;

    wchar_t *currentTextPointer = &(current->wbodyEsc[current->advance - 1]);
    const wchar_t *foundNewLineString = reverse_wcsnchr(currentTextPointer - 1, current->advance - 1, L'\n');

    unsigned int distanceToScroll = 0;
    unsigned int distanceToNewLine = 0;

    if (foundNewLineString != NULL)
        distanceToNewLine = currentTextPointer - foundNewLineString;
                            /*wcslen(foundNewLineString);*/
    else
        distanceToNewLine = current->advance;

    distanceToScroll = (distanceToNewLine % screen->width == 0) ?
                        screen->width :
                        distanceToNewLine % screen->width;

    current->advance -= distanceToScroll;
}

void commentScreenDisplay(CommentScreen *screen)
{
    int i, screenLines;
    wchar_t *tmpbuf;
    int bufSize, lastLine, bufLen;

    if (screen == NULL)
        return ;

    bufSize = sizeof(wchar_t) * (COLS + 1);
    bufLen = COLS;
    tmpbuf = malloc(bufSize);
    memset(tmpbuf, 0, bufSize);

    for (i = 0; i < bufLen; i++)
        tmpbuf[i] = L' ';

    screenLines = screen->offset + screen->displayed + 1;

    for (i = 0; i < screen->displayed + screen->offset + 1 + screen->commentOpenSize; i++)
        mvaddwstr(i, 0, tmpbuf);

    attron(COLOR_PAIR(1));

    for(i = screen->offset; i < screenLines; i++) {
        if(i == screen->selected)
            attron(COLOR_PAIR(2));

        if (i < screen->lineCount && screen->lines[i]->text != NULL)
            mvaddwstr(i - screen->offset, 0, screen->lines[i]->text);
        else
            mvaddwstr(i - screen->offset, 0, tmpbuf);

        if (i == screen->selected)
            attron(COLOR_PAIR(1));
    }

    if (screen->commentOpen) {
        RedditComment *current;
        lastLine = screenLines - screen->offset;

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
                swprintf(tmpbuf, bufLen, L"%s - %d Score - %s", current->author, current->ups, current->created_utc);
                mvaddwstr(lastLine + 1, 0, tmpbuf);
                swprintf(tmpbuf, bufLen, L"-------");
                mvaddwstr(lastLine + 2, 0, tmpbuf);

                mvaddwstr(lastLine + 3, 0, &(current->wbodyEsc[current->advance] ));
            }
        }
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

    /*screen = getCommentScreenFromCommentList(list, COLS); */
    screen = commentScreenNew();

    screen->offset = 0;
    screen->selected = 0;
    screen->displayed = LINES - 1;
    screen->commentOpenSize = (screen->displayed / 5) * 4;
    screen->list = list;
    screen->width = COLS;

    commentScreenRenderLines(screen);
    commentScreenDisplay(screen);
    int c;
    while((c = wgetch(stdscr))) {
        switch(c) {
            case 'j': case KEY_DOWN:
                commentScreenDown(screen);
                break;
            case 'k': case KEY_UP:
                commentScreenUp(screen);
                break;
                
            case KEY_NPAGE:
                commentScreenLevelDown(screen);
                break;
            case KEY_PPAGE:
                commentScreenLevelUp(screen);
                break;

            case 'J':
                commentScreenCommentScrollDown(screen);
                break;
            case 'K':
                commentScreenCommentScrollUp(screen);
                break;

            case 'l': case '\n': case KEY_ENTER:
                commentScreenToggleComment(screen);
                break;
            case 'm':
                redditGetCommentChildren(screen->list, screen->lines[screen->selected]->comment);
                commentScreenRenderLines(screen);
                break;

            case 'q': case 'h':
                if (screen->commentOpen) {
                    commentScreenCloseComment(screen);
                } else {
                    goto cleanup;
                }
                break;
        }
        commentScreenDisplay(screen);
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

    swprintf(screen->screenLines[line], width + 1, L"%2d. [%4d] %20s - ", line + 1, screen->list->links[line]->score, screen->list->links[line]->author);

    offset = wcslen(screen->screenLines[line]);
    title = wcslen(screen->list->links[line]->wtitleEsc);
    for (tmp = 0; tmp <= width - offset; tmp++)
        if (tmp >= title)
            screen->screenLines[line][tmp + offset] = (wchar_t)32;
        else
            screen->screenLines[line][tmp + offset] = screen->list->links[line]->wtitleEsc[tmp];

    screen->screenLines[line][width] = (wchar_t)0;
}

void linkScreenSetupSplit (LinkScreen *screen, wchar_t *tmpbuf, int bufLen, int lastLine)
{
    int i;

    for (i = 0; i < bufLen; i++)
        tmpbuf[i] = L'-';

    tmpbuf[bufLen] = (wchar_t)0;
    attron(COLOR_PAIR(2));
    mvaddwstr(lastLine, 0, tmpbuf);
}

void linkScreenRenderLinkText (LinkScreen *screen, wchar_t *tmpbuf, int bufLen, int screenLines)
{
    RedditLink *current;
    int lastLine = screenLines - screen->offset;

    linkScreenSetupSplit(screen, tmpbuf, bufLen, lastLine);

    attron(COLOR_PAIR(1));
    if (screen->list->linkCount >= screen->selected) {
        current = screen->list->links[screen->selected];
        if (current != NULL) {
            swprintf(tmpbuf, bufLen, L"%s - %d Score / %d Comments / %s \nTitle: ", current->author, current->score, current->numComments, current->created_utc);
            mvaddwstr(lastLine + 1, 0, tmpbuf);
            addwstr(current->wtitleEsc);
            addch('\n');
            swprintf(tmpbuf, bufLen, L"-------\n");
            addwstr(tmpbuf);

            if (current->flags & REDDIT_LINK_IS_SELF)
                addwstr(&(current->wselftextEsc[current->advance]));
            else
                addstr(current->url);
        }
    }
}

void linkScreenRenderHelpText (LinkScreen *screen, wchar_t *tmpbuf, int bufLen, int screenLines)
{
    int i;
    int lastLine = screenLines - screen->offset;

    linkScreenSetupSplit(screen, tmpbuf, bufLen, lastLine);

    attron(COLOR_PAIR(1));

    for(i = 0; i < screen->helpLineCount; i++)
        mvaddwstr(lastLine + 1 + i, 0, screen->helpText[i]);

}

/*
 * Prints a list of posts to the screen
 */
void drawScreen(LinkScreen *screen)
{
    int i, screenLines;
    wchar_t *tmpbuf;
    int bufSize, bufLen;

    if (screen == NULL)
        return ;

    bufSize = sizeof(wchar_t) * (COLS + 1);
    bufLen = COLS;
    tmpbuf = malloc(bufSize);
    memset(tmpbuf, 0, bufSize);

    for (i = 0; i < bufLen; i++)
        tmpbuf[i] = L' ';

    screenLines = screen->offset + screen->displayed + 1;

    for (i = 0; i < screen->displayed + screen->offset + 1 + screen->linkOpenSize; i++)
        mvaddwstr(i, 0, tmpbuf);

    attron(COLOR_PAIR(1));

    for(i = screen->offset; i < screenLines; i++) {
        if(i == screen->selected)
            attron(COLOR_PAIR(2));

        if (i < screen->list->linkCount) {
            if (screen->screenLineCount <= i)
                linkScreenRenderLine(screen, i, COLS);

            mvaddwstr(i - screen->offset, 0, screen->screenLines[i]);
        } else {
            mvaddwstr(i - screen->offset, 0, tmpbuf);
        }

        if (i == screen->selected)
            attron(COLOR_PAIR(1));
    }



    if (screen->linkOpen) {
        if (screen->helpOpen == 0)
            linkScreenRenderLinkText(screen, tmpbuf, bufLen, screenLines);
        else
            linkScreenRenderHelpText(screen, tmpbuf, bufLen, screenLines);
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

void linkScreenTextScrollDown(LinkScreen *screen)
{
    RedditLink *current = screen->list->links[screen->selected];

    if (current->wselftextEsc == NULL)
        return;

    wchar_t *currentTextPointer = &(current->wselftextEsc[current->advance]);
    wchar_t *foundNewLineString = wcschr(currentTextPointer, '\n');
    int screenWidth = COLS;

    // We found a \n char, so advance to it if it's before end-of-screen
    if (foundNewLineString != NULL) 
    {
        unsigned int distanceToNewLine = foundNewLineString - currentTextPointer + 1;
        if (distanceToNewLine < screenWidth)
        {
            // Step past the newline char
            current->advance += distanceToNewLine;
            return;
        }
    }

    if (wcslen(currentTextPointer) > screenWidth)
        current->advance += screenWidth;
}

void linkScreenTextScrollUp(LinkScreen *screen)
{
    RedditLink *current = screen->list->links[screen->selected];
    int screenWidth = COLS;

    if (current->advance == 0)
        return;

    wchar_t *currentTextPointer = &(current->wselftextEsc[current->advance - 1]);
    const wchar_t *foundNewLineString = reverse_wcsnchr(currentTextPointer - 1, current->advance - 1, L'\n');

    unsigned int distanceToScroll = 0;
    unsigned int distanceToNewLine = 0;

    if (foundNewLineString != NULL)
        distanceToNewLine = currentTextPointer - foundNewLineString;
                            /*wcslen(foundNewLineString);*/
    else
        distanceToNewLine = current->advance;

    distanceToScroll = (distanceToNewLine % screenWidth == 0) ?
                        screenWidth :
                        distanceToNewLine % screenWidth;

    current->advance -= distanceToScroll;
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

void linkScreenOpenHelp(LinkScreen *screen)
{
    screen->helpOpen = 1;
}

void linkScreenCloseHelp(LinkScreen *screen)
{
    screen->helpOpen = 0;
}

void linkScreenToggleHelp(LinkScreen *screen)
{
    if (screen->helpOpen)
        linkScreenCloseHelp(screen);
    else
        linkScreenOpenHelp(screen);
}

void showSubreddit(const char *subreddit)
{
    LinkScreen *screen;
    RedditListType listType = REDDIT_HOT; // Default RedditListType for screen is 'hot'

    DEBUG_PRINT(L"Loading Subreddit %s\n", subreddit);
    screen = linkScreenNew();

    linkScreenSetup(screen, subreddit, listType);

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
            case 'K':
                linkScreenTextScrollUp(screen);
                drawScreen(screen);
                break;
            case 'J':
                linkScreenTextScrollDown(screen);
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
                if (screen->helpOpen)
                    linkScreenCloseHelp(screen);
                else
                    linkScreenToggleLink(screen);
                drawScreen(screen);
                break;
            case 'o':
                if (fork() == 0) {
                    char* const argv[]= {"xdg-open", 
                                        screen->list->links[screen->selected]->url,
                                        NULL};
                    execvp("xdg-open", argv);
                }

                redditLinkListFree(screen->list);
                linkScreenFree(screen);

                screen = linkScreenNew();
                linkScreenSetup(screen, subreddit, listType);

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
            case '?':
                linkScreenToggleHelp(screen);
                if (screen->helpOpen)
                    linkScreenOpenLink(screen);
                drawScreen(screen);
                break;
            case '1':
                if (screen->list->type != REDDIT_HOT)
                {
                    listType = REDDIT_HOT;
                    redditLinkListFree(screen->list);
                    linkScreenFree(screen);

                    screen = linkScreenNew();
                    linkScreenSetup(screen, subreddit, listType);

                    drawScreen(screen);
                }
                break;
            case '2':
                if (screen->list->type != REDDIT_NEW)
                {
                    listType = REDDIT_NEW;
                    redditLinkListFree(screen->list);
                    linkScreenFree(screen);

                    screen = linkScreenNew();
                    linkScreenSetup(screen, subreddit, listType);

                    drawScreen(screen);
                }
                break;
            case '3':
                if (screen->list->type != REDDIT_RISING)
                {
                    listType = REDDIT_RISING;
                    redditLinkListFree(screen->list);
                    linkScreenFree(screen);

                    screen = linkScreenNew();
                    linkScreenSetup(screen, subreddit, listType);

                    drawScreen(screen);
                }
                break;
            case '4':
                if (screen->list->type != REDDIT_CONTR)
                {
                    listType = REDDIT_CONTR;
                    redditLinkListFree(screen->list);
                    linkScreenFree(screen);

                    screen = linkScreenNew();
                    linkScreenSetup(screen, subreddit, listType);

                    drawScreen(screen);
                }
                break;
            case '5':
                if (screen->list->type != REDDIT_TOP)
                {
                    listType = REDDIT_TOP;
                    redditLinkListFree(screen->list);
                    linkScreenFree(screen);

                    screen = linkScreenNew();
                    linkScreenSetup(screen, subreddit, listType);

                    drawScreen(screen);
                }
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
    memmove(str + lenpre, str, lenstr + 1);
    /* Copy pre into the new space */
    memmove(str, pre, lenpre);
}

#define MOPT_SUBREDDIT 0
#define MOPT_USERNAME  1
#define MOPT_PASSWORD  2
#define MOPT_HELP      3
#define MOPT_ARG_COUNT 4

optOption mainOptions[MOPT_ARG_COUNT] = {
    OPT_STRING("subreddit", 's', "The name of a subreddit you want to open", ""),
    OPT_STRING("username",  'u', "A Reddit username to login as",            ""),
    OPT_STRING("password",  'p', "Password for the provided username",       ""),
    OPT       ("help",      'h', "Display command-line arguments help-text")
};

char *getPassword()
{
    const int MAX_PASSWD_SIZE = 200;
    char *password = malloc(MAX_PASSWD_SIZE);

    clear();
    mvprintw(0, 0, "Password: ");
    refresh();

    getnstr(password, MAX_PASSWD_SIZE);
    password[MAX_PASSWD_SIZE - 1] = '\0';
    return password;
}

void handleArguments (optParser *parser)
{
    optResponse res;
    int unusedCount = 0;
    int optUnused[3] = { MOPT_SUBREDDIT, MOPT_USERNAME, MOPT_PASSWORD };

    do {
       res = optRunParser(parser);
       if (res == OPT_UNUSED) {
           if (unusedCount < (sizeof(optUnused)/sizeof(optUnused[0]))) {
               int optC = optUnused[unusedCount];
               strcpy(mainOptions[optC].svalue, parser->argv[parser->curopt]);
               mainOptions[optC].isSet = 1;
           }

           unusedCount++;
       }

    } while (res != OPT_SUCCESS);
}

void displayCmd (optOption *option)
{
    printf("   ");
    if (option->opt_long[0] != '\0')
        printf("--%-10s ", option->opt_long);
    else
        printf("             ");

    if (option->opt_short != '\0')
        printf("-%c ", option->opt_short);
    else
        printf("   ");

    switch(option->arg) {
    case OPT_STRING:
        printf("[String] ");
        break;
    case OPT_INT:
        printf("[Int]    ");
        break;
    case OPT_NONE:
        printf("         ");
        break;
    }

    printf(" : %s\n", option->helpText);
}

void displayHelp (optParser *parser)
{
    optOption **option;

    printf("Usage: %s [flags] [subreddit] [username] [password] \n\n", parser->argv[0]);
    printf("Below is a list of all the flags reconized by creddit.\n");
    printf(" [String] --> argument takes a string\n");
    printf(" [Int]    --> argument takes an integer value\n");

    printf("\n");
    for (option = parser->options; *option != NULL; option++)
        displayCmd(*option);

    printf("\nYou will be prompted for a password if you don't include one\n");

    printf("\nTo report any bugs, submit patches, etc. Please see the github page at:\n");
    printf("http://www.github.com/Cotix/cReddit\n");
}

int main(int argc, char *argv[])
{
    RedditUserLogged *user = NULL;
    char *subreddit = NULL;
    char *password = NULL, *username = NULL;
    optParser parser;

    DEBUG_START(DEBUG_FILE, DEBUG_FILENAME);

    memset(&parser, 0, sizeof(optParser));

    parser.argc = argc;
    parser.argv = argv;

    optAddOptions (&parser, mainOptions, MOPT_ARG_COUNT);

    handleArguments(&parser);

    if (mainOptions[MOPT_HELP].isSet) {
        displayHelp(&parser);
        return 0;
    }

    optClearParser(&parser);

    setlocale(LC_CTYPE, "");


    initscr();
    raw();//We want character for character input
    keypad(stdscr,1);//Enable extra keys like arrowkeys
    noecho();
    start_color();
    use_default_colors();
    init_pair(1, -1, -1);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);

    DEBUG_PRINT(L"Starting...\n");

    /* Start libreddit */
    redditGlobalInit();

    globalState = redditStateNew();

    globalState->userAgent = redditCopyString("cReddit/0.0.1");

    redditStateSet(globalState);

    if (mainOptions[MOPT_USERNAME].isSet) {
        username = mainOptions[MOPT_USERNAME].svalue;
        if (!mainOptions[MOPT_PASSWORD].isSet)
            password = getPassword();
        else
            password = mainOptions[MOPT_PASSWORD].svalue;

        user = redditUserLoggedNew();
        redditUserLoggedLogin(user, username, password);

        /* Don't want to leave that important Reddit password in memory */
        memset(password, 0, strlen(password));
        if (!mainOptions[MOPT_PASSWORD].isSet)
            free(password);
    }
    if (mainOptions[MOPT_SUBREDDIT].isSet) {
        subreddit = mainOptions[MOPT_SUBREDDIT].svalue;
        if (!startsWith("/r/", subreddit) && strcmp("/", subreddit) != 0)
            prepend("/r/", subreddit);

    } else {
        subreddit = "/";
    }
    showSubreddit(subreddit);

    redditUserLoggedFree(user);
    redditStateFree(globalState);
    redditGlobalCleanup();
    endwin();

    DEBUG_END(DEBUG_FILE);
    return 0;
}


