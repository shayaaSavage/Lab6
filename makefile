CC=gcc
CFLAGS=-I
all: dComp client server 
dComp : 
	@echo "compiling..."
	$(CC) -c -fPIC ./lib/mylib.c    -o ./lib/mylib.o
	$(CC) -shared ./lib/mylib.o -o ./lib/libDin.so
	@echo "__you need to add: LD_LIBRARY_PATH=$(pwd)/lib ./yourProg"
client : 
	@echo "Build client..."
	$(CC) client.c  -Llib -lDin -o ./client
server : 
	@echo "Build server..."
	$(CC) -pthread server.c  -Llib -lDin -o ./server
clear :
	rm client server ./lib/libDin.so ./lib/mylib.o
