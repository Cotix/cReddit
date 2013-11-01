#!/bin/bash

# This small file allows creddit to be run without having to reinstall it.
# it just uses LD_PRELOAD to replace any globally-installed libreddit.so with
# the local version. (See Makefile for more info on Debug mode)

# Also, if debugging is set, then creddit is run inside of valgrind so memory
# issues can be checked for.

if [ "y" == "$REDDIT_DEBUG" ]; then
    LD_PRELOAD=./build/libreddit.so valgrind --leak-check=yes ./build/creddit $@
else
    LD_PRELOAD=./build/libreddit.so ./build/creddit $@
fi
