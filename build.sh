gcc -g -o cReddit `curl-config --cflags` main.c reddit.c jsmn.c -lncurses `curl-config --libs`
