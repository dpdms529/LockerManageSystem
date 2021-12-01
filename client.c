#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#define DEFULAT_PROTOCOL 0
#define MAXLINE 100

void readLine(int, char *);

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

	while(readLine(clientfd, outmsg)) {
		scanf("%s", inmsg);
		write(clientfd, inmsg, strlen(inmsg)+1);
	}
	exit(0);
}

void readLine(int fd, char* str) 
{
	int n;
	do {
		n = read(fd, str, 1);
	} while(n > 0 && *str++ != '\0');
	return (n > 0);
}
