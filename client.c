#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#define DEFULAT_PROTOCOL 0
#define MAXLINE 100

int readLine(int, char *);
void writeInfo(int, char*, char*);

int main() {
	int clientfd, result;
	char inmsg[MAXLINE], outmsg[MAXLINE];
	struct sockaddr_un serverUNIXaddr;

	clientfd = socket(AF_UNIX, SOCK_STREAM, DEFULAT_PROTOCOL);
	serverUNIXaddr.sun_family = AF_UNIX;
	strcpy(serverUNIXaddr.sun_path, "convert");

	do {
		result = connect(clientfd, &serverUNIXaddr, sizeof(serverUNIXaddr));
		if (result == -1) sleep(1);
	} while (result == -1);

	while(readLine(clientfd, inmsg)) {
		writeInfo(clientfd, inmsg, outmsg);
	}
	exit(0);
}

int readLine(int fd, char* str) 
{
	int n;
	do {
		n = read(fd, str, 1);
	} while(n > 0 && *str++ != '\0');
	return (n > 0);
}

void writeInfo(int clientfd, char* inmsg, char* outmsg){
	if(strncmp(inmsg,"?",1)==0){
		write(clientfd, "?", 2);
		readLine(clientfd, inmsg);
		printf(inmsg);
		scanf("%s", outmsg);
		write(clientfd, outmsg, strlen(outmsg)+1);
	}else{
		printf(inmsg);
	}
}
