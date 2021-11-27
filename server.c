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


void getPosts(int start, char **buffer) {
	struct dirent *dp;
	DIR *dir = opendir("posts/");

	// Unable to open directory stream
	if (!dir)
		return;

	for(int i=0; (dp = readdir(dir)) != NULL; ){
		if(strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0){
			if((i >= start) && (i < start+NUM_POSTS)){
				*buffer++ = strdup(dp->d_name);
			}
			i++;
		}
	}

	// Close directory stream
	closedir(dir);
}

void insertPosts(int fd, int start) {
	char *fileNames[NUM_POSTS];
	getPosts(start, fileNames);
	char line[100];
	for(int i = 0; i < NUM_POSTS; ++i) {
		char filename[100];
		size_t ret = snprintf(filename, sizeof filename, "posts/%s", fileNames[i]);
		if(ret >= sizeof filename)
			fprintf(stderr, "uh oh, stinky! too much filename");
		FILE *postData = fopen(filename, "r");
		dprintf(fd, "<h1>Post %d</h1>\n", i);
		while(fgets(line, sizeof line, postData))
			dprintf(fd, "<p>%s</p>\n", line);
		fclose(postData);
	}
	for(int i = 0; i < 5; i++)
		free(fileNames[i]);
}

void sendBody(int fd, int start) {
	FILE *htmlData = fopen("index.html", "r");
	char line[100];
	while(fgets(line, sizeof line, htmlData) != 0) {
		if(strcmp(line, "		<title>Blog</title>\n") == 0){
			dprintf(fd, "<title>%s</title>\n", BLOGNAME);
		} else if(strcmp(line, "		<h1>Blog</h1>\n") == 0){
			dprintf(fd, "<h1>%s</h1>\n", BLOGNAME);
			insertPosts(fd, start);
		} else {
			dprintf(fd, "%s", line);
		}
	}
}


void getFirstLine(char buffer[], char result[]){
	for(char c; ((c = *buffer++) != EOF) && (c != '\n'); *result++ = c);
}


int main(void) {

	signal(SIGCHLD, SIG_IGN);

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
	while(1) {
		int clientSocket = accept(serverSocket, NULL, NULL);
		char buffer[2048] = {0};
		read(clientSocket, buffer, SOCK_NONBLOCK);
		char firstLine[2048] = {0};
		getFirstLine(buffer, firstLine);
		if(strcmp(firstLine, "GET /favicon.ico HTTP/1.1\n")){
			printf("Ignore favicon\n");
		} else {
			dprintf(clientSocket, "%s", HTTP_HEADER);
			sendBody(clientSocket, 0);
			close(clientSocket);
		}
		//send(clientSocket, HTTP_HEADER, sizeof HTTP_HEADER, 0);
		/*switch(fork()) {
		case -1:
			perror("fork");
			break;
		case 0:
			printf("Case 1\n");
			close(serverSocket);
			dprintf(clientSocket, "%s", HTTP_HEADER);
			sendBody(clientSocket, 0);
			close(clientSocket);
			break;
		default:
			printf("Case 2\n");
			close(clientSocket);
			break;
		}*/
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
