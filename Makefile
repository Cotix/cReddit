CC=gcc
FLAGS=-g -lncurses `curl-config --cflags` `curl-config --libs`

all: main.c reddit.c jsmn.c
	mkdir -p build
	$(CC) -o ./build/cReddit $(FLAGS) main.c reddit.c jsmn.c

clean:
	rm -rf build
