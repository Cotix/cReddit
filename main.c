#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include "reddit.h"
#include <curl/curl.h>
#include <ncurses.h>
int main(int argc, char *argv[])
{
	initscr();
	raw();//We want character for character input
	keypad(stdscr,1);//Enable extra keys like arrowkeys
	noecho(); 
	curl_global_init(CURL_GLOBAL_ALL);
	struct post threads[25];//Our array with reddit threads
	redditGetSubreddit("/r/starcraft","hot",threads);
	int i;
	for(i = 0; i != 25; ++i)
	{
		printw("%s(%s)%s -%s\n\n",threads[i].id,threads[i].votes,threads[i].title,threads[i].author);
	}
	refresh();
	getch();
  /* we're done with libcurl, so clean it up */ 
  curl_global_cleanup();
	endwin();
  return 0;
}
