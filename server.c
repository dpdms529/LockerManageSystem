#include "student.h"
#include "locker.h"
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(){
	int listenfd, connfd, clientlen;
	struct sockaddr_un serverUNIXaddr, clientUNIXaddr;
	int lockerNum, pwdLen;
	struct student st;
	struct locker * locker;


	signal(SIGCHLD, SIG_IGN);
	clientlen = sizeof(clientUNIXaddr);

	listenfd = socket(AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
	serverUNIXaddr.sun_family = AF_UNIX;
	strcpy(serverUNIXaddr.sun_path, "convert");
	unlink("convert");
	bind(listenfd, &serverUNIXaddr,sizeof(serverUNIXaddr));

	printf("사물함 개수를 설정하세요 : ");
	scanf("%d",&lockerNum);
	printf("비밀번호 자릿수를 설정하세요 : ");
	scanf("%d", &pwdLen);

	locker = (struct locker *)malloc(lockerNum * sizeof(struct locker));

	listen(listenfd, 5);

	while(1){
		connfd = accept(listenfd, &clientUNIXaddr, &clientlen);
		if(fork() == 0){

		}
	}
}
