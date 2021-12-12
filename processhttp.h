#ifndef PROCESSHTTP
#define PROCESSHTTP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h> // for getnameinfo()
#include <signal.h>
#include <err.h>

// Usual socket headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <dirent.h>

#include <arpa/inet.h>

#define SIZE 1024
#define BACKLOG 10  // Passed to listen()
#define PORT 3000
#define BLOGNAME "Blog"
#define NUM_POSTS 5
#define HTTP_HEADER "HTTP/1.1 200 OK\r\n\n"

void report(struct sockaddr_in *serverAddress);

void sendBody(int fd, char *fname);

void getFirstLine(char *buffer, char *result);

int requestType(char *request);

void getAddress(char *buffer, char *result);

void getPostBody(char *buffer, char *result);

void processUpload(char *buffer, char *title, char *text);

char from_hex(char ch);

char to_hex(char code);

char *url_encode(char *str);

char *url_decode(char *str);

void writePost(char *title, char *text);

void cleanPostName(char *postName, char *result);

void updateIndex();

void removeBlog(char *address);

void getPosts(char *buffer);

#endif
