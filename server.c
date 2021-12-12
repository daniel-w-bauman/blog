#include "processhttp.h"

int serverSocket;

void ctrlcHandler(int signaln);

int main(void) {
	signal(SIGINT, ctrlcHandler);

	// Socket setup: creates an endpoint for communication, returns a descriptor
	serverSocket = socket(
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
	int flag = 1;
  if (-1 == setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) {
      err(1, "setsockopt fail");
  }
	int ret = bind(
		serverSocket,                         // file descriptor referring to a socket
		(struct sockaddr *) &serverAddress,   // Address to be assigned to the socket
		sizeof serverAddress                  // Size (bytes) of the address structure
	);

	if(ret < 0){
		err(1, "unable to bind");
	}

	// Mark socket to listen for incoming connections
	int listening = listen(serverSocket, BACKLOG);
	if (listening < 0)
		err(1,"unable to listen");

	report(&serverAddress);     // Custom report function

	// Wait for a connection, create a connected socket if a connection is pending
	for(;;) {
		int clientSocket = accept(serverSocket, NULL, NULL);
		int pid = fork();
		if(pid == -1){
			perror("fork");
			break;
			exit(1);
		} else if(pid == 0){ // child process
			char buffer[2048] = {0};
			read(clientSocket, buffer, SOCK_NONBLOCK);
			int rtype = requestType(buffer);
			if(rtype == 0){ // GET /
				sendBody(clientSocket, "index.html");
			} else if(rtype == 1){ // GET /something
				char *firstline = malloc(2048);
				getFirstLine(buffer, firstline);
				char *address = malloc(strlen(firstline));
				getAddress(firstline, address);
				removeBlog(address); // in case link is from github
				sendBody(clientSocket, address);
				free(address);
				free(firstline);
			} else if(rtype == 2) { // GET / posts /
				char posts[2048] = {0};
				getPosts(posts);
//				dprintf(clientSocket, "%s", HTTP_HEADER);
				dprintf(clientSocket, "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
				dprintf(clientSocket, "%s\r\n", posts);
			} else if(rtype == 3){ // POST /upload
				char *postBody = malloc(2048);
				getPostBody(buffer, postBody);
				char *title = malloc(strlen(postBody));
				char *text = malloc(strlen(postBody));
				processUpload(postBody, title, text);
				char *decoded_title = url_decode(title);
				char *decoded_text = url_decode(text);
				writePost(decoded_title, decoded_text);
				updateIndex();
				sendBody(clientSocket, "post.html");
				free(decoded_text);
				free(decoded_title);
				free(title);
				free(text);
				free(postBody);
			} else { // unrecognized request
				printf("Sending 404\n");
				dprintf(clientSocket, "%s\r\n\n", "HTTP/1.1 404 Not Found");
			}
			shutdown(clientSocket, SHUT_RDWR);
			close(clientSocket); // child closes connection
			exit(0);
		}
	}
	return 0;
}


void ctrlcHandler(int signaln){
	(void)signaln;
	shutdown(serverSocket, SHUT_RDWR);
	close(serverSocket);
	fprintf(stdout, "shutting down server.\n");
	exit(1);
}
