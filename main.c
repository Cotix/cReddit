#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include "reddit.h"
#include <curl/curl.h>
#include <ncurses.h>
#include <form.h>

void buildScreen(char **text, int selected, int size)
{
	clear();
	start_color();
	init_pair(1,COLOR_RED,COLOR_YELLOW);
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

void showSubreddit(char *subreddit)
{
	struct post threads[25];
	if(!startsWith("/r/", subreddit))
		return;
	redditGetSubreddit(subreddit,"hot",threads);
	
	int i;
	char *text[25];
	for(i = 0; i != 25; ++i)
	{
		if(threads[i].id == 0)
			continue;
		int textlen = strlen(threads[i].id) + strlen(threads[i].votes) + strlen(threads[i].title) + strlen(threads[i].author) + 6;
		text[i] = malloc(textlen);
		//strcpy(text[i],threads[i].id);// do we need this?
		strcpy(text[i],"(");
		strcat(text[i],threads[i].votes);
		strcat(text[i],")\t");
		strcat(text[i],threads[i].title);
		strcat(text[i]," - ");
		strcat(text[i],threads[i].author);
		printw("%s\n",text[i]);
		refresh();
	}
		
	int selected = 0;
	buildScreen(text,selected,25);
	int c;
	struct comments cList[500];
	while(c = wgetch(stdscr))
	{
		if(c == 'q')
			break;//YEA FUCK YOU WHILE, TAKE THAT BITCH
		switch(c)
		{
		case KEY_UP:
			if(selected != 0)
				selected--;
			break;
		case KEY_DOWN:
			if(selected != 24)
				selected++;
			break;
		case '\n':
			printw("Lets gogogo\n");
			refresh();
			redditGetThread(threads[selected].id,cList);
			wgetch(stdscr);
			break;
		default:
			break;
		}
    	buildScreen(text,selected,25);
    }

}

int main(int argc, char *argv[])
{
	initscr();
	raw();
	keypad(stdscr,1);
	noecho(); 
	curl_global_init(CURL_GLOBAL_ALL);
	
	if(argc == 2) {
		showSubreddit(argv[1]);
	} else {
		showSubreddit(ask_for_subreddit());
	}
 
	cleanup();
  	return 0;
}
