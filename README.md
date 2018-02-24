cReddit
=======

CLI Reddit client written in C. Oh, crossplatform too!


How to build
============
Current required libraries are libcurl and libncursesw (Wide-character version
of libncurses). The project itself comprises a library called libreddit and a
program called creddit.

To do a normal compilation, run the two commands:
``` make ``` and
``` make install ```

Note: 'make install' will need to be run as root to install to the root
directory.


If you would like to compile libreddit on its own as a shared library, you can
run ``` make libreddit ```. The resulting shared library will be
./build/libreddit.so.  You can also install the library separately via ``` make
libreddit_install ```.  To use the library, include "reddit.h" and link against
libreddit.so.

To see other documented compilation options, read the top comments in ./Makefile.

Default Keypresses
------------------

Link screen:
*    k / UP      -- Move up one link in the list
*    j / DOWN    -- Move down one link in the list
*    K           -- Scroll up link text of open link
*    J           -- Scroll down link text of open link 
*    L           -- Get the next list of Links from Reddit
*    u           -- Update the list (Clears the list of links, and then gets a new list from Reddit)
*    l / ENTER   -- Open the selected link
*    c           -- Display the comments for the selected link
*    q           -- Close open Link, or exit program if no link is open
*    [1 - 5]     -- Switch between subreddit list types (hot, new, rising, controversial, top)

Comment screen:
*    k / UP      -- Move up one comment in the list
*    j / DOWN    -- Move down one comment in the list
*    K           -- Scroll up comment text of open comment
*    J           -- Scroll down comment text of open comment
*    PGUP        -- Move up one comment at the same depth
*    PGDN        -- Move down one comment at the same depth
*    l / ENTER   -- Open the selected comment
*    q / h       -- Close the open comment, or close the comment screen if no comment is open
