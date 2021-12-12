#include "processhttp.h"



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
	1 - get /something else
	2 - get /posts
	3 - post /upload
*/
int requestType(char *request){
	char firstline[2048];
	getFirstLine(request, firstline);
	int category = -1;
	if(strcmp(firstline, "GET / HTTP/1.1") == 0){
		category = 0;
	} else if(strcmp(firstline, "GET /blog/ HTTP/1.1") == 0){
		category = 0;
	} else if(strcmp(firstline, "GET /posts/ HTTP/1.1") == 0){
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
		} else {
			fprintf(stderr, "Invalid request: %s\n", firstline);
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
	fprintf(postF, "<div>\n");
	fprintf(postF, "\t<h1>%s</h1>\n", title);
	fprintf(postF, "\t<p>");
	char c;
	while(((c = *text++) != EOF) && (c != '\0')){
		if(c == '\n')
			fprintf(postF, "</p>\n\t<p>");
		else
			fputc(c, postF);
	}
	fprintf(postF, "</p>\n</div>");
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


// If address begins with blog/ (github pages address), remove that part of the address
void removeBlog(char *address){
	if(strlen(address) < 5){
		return;
	}
	char *firstFive = malloc(6);
	for(int i = 0; i < 5; i++){
		firstFive[i] = address[i];
	}
	firstFive[5] = '\0';
	if(strcmp(firstFive, "blog/") == 0){
		for(size_t i = 5; i < strlen(address); i++){
			address[i-5] = address[i];
		}
		address[strlen(address)-5] = '\0';
	}
	free(firstFive);
}


void getPosts(char *buffer) {
	struct dirent *dp;
	DIR *dir = opendir("docs/posts/");

	// Unable to open directory stream
	if (!dir)
		return;

	while((dp = readdir(dir)) != NULL){
		if(strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0){
			strcat(buffer, dp->d_name);
			strcat(buffer, " ");
		}
	}

	// Close directory stream
	closedir(dir);
}
