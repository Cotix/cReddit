cReddit
=======

CLI Reddit client written in C. Oh, crossplatform too!


How to build
============
Current required libraries are libcurl and libncurses. To build the full
project on its own, run 'make'. You can also run 'make install' to have the
executable installed into the /usr/bin folder (Or anywhere else, if you set
PREFIX).

If you would like to compile libreddit on its own as a static library, you can
run 'make libreddit'. The resulting static library will be ./build/libreddit.a.
Further compilation options can be read in the top of the Makefile

Default Keypresses
------------------

Link screen:
*    k / UP      -- Move up one link in the list
*    j / DOWN    -- Move down one link in the list
*    L           -- Get the next list of Links from Reddit
*    u           -- Update the list (Clears the list of links, and then gets a new list from Reddit)
*    l / ENTER   -- Open the selected link
*    c           -- Display the comments for the selected link
*    q           -- Close open Link, or exit program if no link is open

Comment screen:
*    k / UP      -- Move up one comment in the list
*    j / DOWN    -- Move down one comment in the list
*    l / ENTER   -- Open the selected comment
*    q / h       -- Close the open comment, or close the comment screen if no comment is open
