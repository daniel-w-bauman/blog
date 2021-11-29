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

typedef struct {
	char *username;
	char *password;
	int token;
} User;

void report(struct sockaddr_in *serverAddress);

void sendBody(int fd, char *fname);

void getFirstLine(char *buffer, char *result);

int requestType(char *request);

void getAddress(char *buffer, char *result);

void getCreds(char *buffer, char *username, char *password);

void ctrlcHandler(int signaln);

int login(User user);

void getPostBody(char *buffer, char *result);

void processUpload(char *buffer, char *title, char *text);

char from_hex(char ch);

char to_hex(char code);

char *url_encode(char *str);

char *url_decode(char *str);

void writePost(char *title, char *text);

void cleanPostName(char *postName, char *result);

void updateIndex();

int serverSocket;

User users[] = {{"daniel", "danielpass", -1}, {"charlie", "charliepass", -1}};

int tokenN = 0;

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
				sendBody(clientSocket, address);
				free(address);
				free(firstline);
			} else if(rtype == 2){ // POST /signin
				char *postBody = malloc(2048);
				getPostBody(buffer, postBody);
				char *username = malloc(strlen(postBody));
				char *password = malloc(strlen(postBody));
				getCreds(postBody, username, password);
				int token = login((User){username, password, -1});
				if(token == -1){
					fprintf(stderr, "login unsuccessful for %s\n", username);
				} else {
					fprintf(stderr, "login successful for %s, token is %d\n", username, token);
				}
				sendBody(clientSocket, "login.html");
				free(password);
				free(username);
				free(postBody);
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
	char line[100];
	char filename[1024] = "docs/";
	strcat(filename, fname);
	FILE *htmlData = fopen(filename, "r");
	if(htmlData == NULL){
		dprintf(fd, "%s\r\n\n", "HTTP/1.1 404 Not Found");
	} else {
		while(fgets(line, sizeof line, htmlData) != 0) {
			dprintf(fd, "%s", line);
		}
		fclose(htmlData);
	}
}


void getFirstLine(char *buffer, char *result){
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
	1 - get /something
	2 - post /signin
	3 - post /upload
*/
int requestType(char *request){
	char firstline[2048];
	getFirstLine(request, firstline);
	int category = -1;
	if(strcmp(firstline, "GET / HTTP/1.1") == 0){
		category = 0;
	} else if(strcmp(firstline, "POST /signin HTTP/1.1") == 0){
		category = 2;
	} else if(strcmp(firstline, "POST /upload HTTP/1.1") == 0){
		category = 3;
	} else {
		char buffer[4];
		buffer[0] = request[0];
		buffer[1] = request[1];
		buffer[2] = request[2];
		buffer[3] = '\0';
		if(strcmp(buffer, "GET") == 0){
			category = 1;
		}
	}
	return category;
}


void getAddress(char *buffer, char *result){
	int i = 5;
	char c;
	while(((c = buffer[i]) != '\0') && (c != ' ') && (c != '\r') && (c != EOF) && (c != '\n') && (i < (int)strlen(buffer))){
		result[i-5] = c;
		i++;
	}
	result[i-5] = '\0';
}


void getCreds(char *buffer, char *username, char *password){
	char c;
	int state = 0;
	while((c = *buffer++) != '\0'){
		if(state == 0){
			if(c == '='){
				state = 1;
			}
		} else if(state == 1){
			if(c == '&'){
				state = 2;
			} else {
				*username++ = c;
			}
		} else if(state == 2){
			if(c == '='){
				state = 3;
			}
		} else if(state == 3){
			*password++ = c;
		}
	}
	*username++ = '\0';
	*password++ = '\0';
}


void ctrlcHandler(int signaln){
	(void)signaln;
	shutdown(serverSocket, SHUT_RDWR);
	close(serverSocket);
	fprintf(stdout, "shutting down server.\n");
	exit(1);
}


int login(User user){
	int token = -1;
	for(size_t i = 0; i < (sizeof users / sizeof *users); i++){
		if(strcmp(user.username, users[i].username) == 0){
			if(strcmp(user.password, users[i].password) == 0){
				token = tokenN;
				tokenN++;
				users[i].token = token;
			}
		}
	}
	return token;
}


void getPostBody(char *buffer, char *result){
	int state = 0;
	int i = 0;
	int j = 0;
	char c;
	while(((c = buffer[i]) != '\0') && (c != EOF) && (i < 2048)){
		i++;
		if(buffer[i] == '\r' && state == 0)
			state = 1;
		else if(buffer[i] == '\n' && state == 1)
			state = 2;
		else if(buffer[i] == '\r' && state == 2)
			state = 3;
		else if(buffer[i] == '\n' && state == 3)
			state = 4;
		else if(state == 4){
			//fprintf(stderr, "Found newline\n");
			result[j++] = buffer[i];
		}	else {
			state = 0;
		}
	}
	result[j] = '\0';
}


void processUpload(char *buffer, char *title, char *text){
	char c;
	int i = 0;
	int j = 0;
	int state = 0;
	while(((c = buffer[i]) != '\0') && (c != EOF)){
		i++;
		if(c == '=' && state == 0){
			state = 1;
		} else if(state == 1){
			if(c == '&'){
				state = 2;
				title[j] = '\0';
				j = 0;
			} else {
				title[j] = c;
				j++;
			}
		} else if(state == 2){
			if(c == '='){
				state = 3;
			}
		} else if(state == 3){
			text[j] = c;
			j++;
		}
	}
	text[j] = '\0';
}


char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}


char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}


char *url_encode(char *str) {
  char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;

  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
      *pbuf++ = *pstr;
    else if (*pstr == ' ')
      *pbuf++ = '+';
    else
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}


char *url_decode(char *str) {
  char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') {
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}


void writePost(char *title, char *text){
	FILE *template = fopen("PreTemplate.html", "r");
	if(template == NULL){
		fprintf(stderr, "file can\'t open:\n%s\n", "PreTemplate.html");
		return;
	}
	char *templateText = malloc(2048);
	char c;
	int i = 0;
	while((c = fgetc(template)) != EOF){
		templateText[i++] = c;
	}
	templateText[i] = '\0';
	fclose(template);
	char *sanitizedTitle = malloc(strlen(title)+1);
	cleanPostName(title, sanitizedTitle);
	char *fname = malloc(strlen(title)+18);
	char *folder = "docs/posts/";
	strcpy(fname, folder);
	strcat(fname, sanitizedTitle);
	free(sanitizedTitle);
	char *ending = ".html";
	strcat(fname, ending);
	FILE* postF = fopen(fname, "w");
	if(postF == NULL){
		fprintf(stderr, "file can\'t open:\n%s\n", fname);
		return;
	}
	i = 0;
	while((c = templateText[i]) != '\0'){
		fputc(c, postF);
		i++;
	}
	free(templateText);
	fprintf(postF, "\t\t<h1>%s</h1>\n", title);
	fprintf(postF, "\t\t<p>");
	while(((c = *text++) != EOF) && (c != '\0')){
		if(c == '\n')
			fprintf(postF, "</p>\n\t\t<p>");
		else
			fputc(c, postF);
	}
	fprintf(postF, "</p>\n\t</body>\n</html>");
	fclose(postF);
	free(fname);
}

void cleanPostName(char *postName, char *result){
	char c;
	int i = 0;
	while((c = postName[i]) != '\0'){
		if(c == ' ')
			result[i] = '-';
		else
			result[i] = c;
		i++;
	}
	result[i] = '\0';
}

void updateIndex(){
	system("python addposts.py");
}
