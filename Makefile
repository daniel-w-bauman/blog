all: server

server: server.o processhttp.o
	gcc server.o processhttp.o -o server

server.o: server.c processhttp.h
	gcc -c -Wall -Wextra -Wpedantic -g server.c -o server.o

processhttp.o: processhttp.c processhttp.h
	gcc -c -Wall -Wextra -Wpedantic -g processhttp.c -o processhttp.o

clean:
	rm server.o processhttp.o server
