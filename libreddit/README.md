libreddit
=========

libreddit is a sub-project of cReddit, whose focus is to create a library
written in C which is compliant with the Reddit API.

Current, the library uses libcurl for handling the networking, and libjsmn
(Modified, and compiled in directly) as well as some internal tools to parse
the JSON objects.

API
---

The full exposed API is in ../include/reddit.h. This file contains all the
structures for each type of object Reddit uses, and good info on how to use
them.

Known FIXMEs and TODOs
----------------------
*   token.c:

    curl hangs if you don't have a internet connection, meaning libreddit will
    hang indefinitely if you're not connected to the internet. It is probably
    because we're using the 'curl_easy' functions. Making sure that libreddit
    doesn't wait forever on a broken connection will probably require a bit
    more advanced curl usage, but shouldn't be very hard since only one
    function has to be modified.

*   comment.c:

    The 'morechildren' API call is currently broken, needs to be updated to
    match the format of a morechildren JSON response (Which is different from a
    normal comment request reqponse).

*   subreddit.c:

    There's currently nothing in this file. Ideally, it should have a few
    functions that return info on a subreddit (like link.c, comment.c, etc...)

*   state.c:

    The redditStateSet and redditStateGet setup to keep an internal state going
    is convient and works well, but it's not thread-safe. Whether or not this
    should be a concern or a focus should be consitered.

