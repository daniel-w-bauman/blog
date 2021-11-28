#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#define BLOGNAME "Blogger I hardly knew her"
#define NUM_POSTS 5
#define HTTP_HEADER "HTTP/1.1 200 OK\r\n\n"


void report(struct sockaddr_in *serverAddress);

void getPosts(int start, char **buffer);

void insertPosts(int fd, int start);

void sendBody(int fd, char *fname);

void getFirstLine(char buffer[], char result[]);

int requestType(char request[]);

void getAddress(char buffer[], char result[]);

int main(void) {
	// Socket setup: creates an endpoint for communication, returns a descriptor
	int serverSocket = socket(
		AF_INET,      // Domain: specifies protocol family
		SOCK_STREAM,  // Type: specifies communication semantics
		0             // Protocol: 0 because there is a single protocol for the specified family
	);

	// Construct local address structure
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);//inet_addr("127.0.0.1");

	// Bind socket to local address
	// bind() assigns the address specified by serverAddress to the socket
	// referred to by the file descriptor serverSocket.
	bind(
		serverSocket,                         // file descriptor referring to a socket
		(struct sockaddr *) &serverAddress,   // Address to be assigned to the socket
		sizeof serverAddress                  // Size (bytes) of the address structure
	);

	// Mark socket to listen for incoming connections
	int listening = listen(serverSocket, BACKLOG);
	if (listening < 0)
		err(1,"unable to listen");

	report(&serverAddress);     // Custom report function

	// Wait for a connection, create a connected socket if a connection is pending
	char buffer[2048] = {0};
	char firstline[2048] = {0};
	char address[2048] = {0};
	for(;;) {
		int clientSocket = accept(serverSocket, NULL, NULL);
		switch(fork()) {
		case -1:
			perror("fork");
			break;
			exit(1);
		case 0:
			read(clientSocket, buffer, SOCK_NONBLOCK);
			printf("%s\n", buffer);
			shutdown(clientSocket, SHUT_RDWR);
			close(clientSocket); // child closes connection
			exit(0);
		default:
			break;
		}
	}
	return 0;
}


void report(struct sockaddr_in *serverAddress) {
	char hostBuffer[INET6_ADDRSTRLEN];
	char serviceBuffer[NI_MAXSERV]; // defined in `<netdb.h>`
	socklen_t addr_len = sizeof(*serverAddress);
	int err = getnameinfo(
		(struct sockaddr *) serverAddress,
		addr_len,
		hostBuffer,
		sizeof(hostBuffer),
		serviceBuffer,
		sizeof(serviceBuffer),
		NI_NUMERICHOST
	);
	if (err != 0) {
		printf("It's not working!!\n");
	}
	printf("Server listening on http://%s:%d\n", hostBuffer, PORT);
}


void sendBody(int fd, char *fname) {
	dprintf(fd, "%s", HTTP_HEADER);
	FILE *htmlData = fopen(fname, "r");
	char line[100];

	if(htmlData == NULL){
		dprintf(fd, "%s\r\n\n", "HTTP/1.1 404 Not Found");
	} else {
		while(fgets(line, sizeof line, htmlData) != 0) {
			dprintf(fd, "%s", line);
		}
	}
}


void getFirstLine(char buffer[], char result[]){
	int i = 0;
	char c;
	while(((c = buffer[i]) != EOF) && (c != '\n') && (c != '\r') && (c != '\0')){
		result[i] = c;
		i++;
	}
	result[i] = '\0';
}

/*
	Categorization of requests:
	-1 (default) - error
	0 - get /
	1 - get /favicon
	2 - get /login
	3 - get* | len > 20
*/
int requestType(char request[]){
	char firstline[2048];
	getFirstLine(request, firstline);
	int category = -1;
	if(strcmp(firstline, "GET / HTTP/1.1") == 0){
		category = 0;
	} else if(strcmp(firstline, "GET /favicon.ico HTTP/1.1") == 0){
		category = 1;
	} else if(strcmp(firstline, "GET /login HTTP/1.1") == 0) {
		category = 2;
	} else {
		if((int)strlen(firstline) > 20){
			category = 3;
		} else {
			printf("First line not recognized\n");
		}
	}
	return category;
}


void getAddress(char buffer[], char result[]){
	int i = 5;
	char c;
	while(((c = buffer[i]) != '\0') && (c != ' ') && (c != '\r') && (c != EOF) && (c != '\n') && (i < (int)strlen(buffer))){
		result[i-5] = c;
		i++;
	}
	result[i-5] = '\0';
}
