if [ ! -d "./build"]; then
  mkdir ./build
fi
gcc -g -o ./build/cReddit `curl-config --cflags` main.c reddit.c jsmn.c -lncurses `curl-config --libs`
