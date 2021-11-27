server: server.c
	gcc -Wall -Wextra -Wpedantic -g server.c -o server
clean:
	rm server
