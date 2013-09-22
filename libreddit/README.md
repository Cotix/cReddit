libreddit
=========

libreddit is a sub-project of cReddit, whose focus is to create a library
written in C which is compliant with the Reddit API.

Current, the library uses libcurl for handling the networking, and libjsmn
(Modified, and compiled in directly) as well as some internal tooks to parse
the JSON objects.

API
---

The full exposed API is in ../include/reddit.h. This file contains all the
structures for each type of object Reddit uses, and good info on how to use
them.

Known FIXMEs and TODOs
----------------------
    libreddit.mk:
        Currently, symbols which are used across multiple files but aren't
        suppose to be exposed for the API of libreddit are still in the symbol
        table in libreddit.a, which could possibly cause unexpected symbol
        colisions.
    comment.c:
        The 'morechildren' API call is currently broken, needs to be updated to
        match the format of a morechildren JSON response (Which is different
        from a normal comment request reqponse).
    subreddit.c:
        There's currently nothing in this file. Ideally, it should have a few
        functions that return info on a subreddit (like link.c, comment.c,
        etc...)
    state.c:
        The redditStateSet and redditStateGet setup to keep an internal state
        going is convient and works well, but it's not thread-safe. Whether 
        or not this should be a concern or a focus should be consitered.

