DIR="./build"
if [ ! -d "$DIR" ]; then
  echo "Creating 'build' directory..."
  mkdir ./build
fi
gcc -g -o ./build/cReddit `curl-config --cflags` main.c reddit.c jsmn.c -lncurses `curl-config --libs`
