#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "student.h"
#define DEFAULT_PROTOCOL 0
#define MAXLINE 100

void getInfo(int, char*, char*);

int main(int argc, char *argv[]){
	int fd, listenfd, connfd, clientlen;
	struct sockaddr_un serverUNIXaddr, clientUNIXaddr;
	int lockerNum, bigNum, pwdLen;
	struct student record;
	struct locker * locker;
	int id;
	char outmsg[MAXLINE], inmsg[MAXLINE], name[20];

	signal(SIGCHLD, SIG_IGN);
	clientlen = sizeof(clientUNIXaddr);

	listenfd = socket(AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
	serverUNIXaddr.sun_family = AF_UNIX;
	strcpy(serverUNIXaddr.sun_path, "convert");
	unlink("convert");
	bind(listenfd, &serverUNIXaddr,sizeof(serverUNIXaddr));

	printf("사물함 개수를 설정하세요 : ");
	scanf("%d",&lockerNum);
	printf("대형 사물함 개수를 설정하세요 : ");
	scanf("%d",&bigNum);
	printf("비밀번호 자릿수를 설정하세요 : ");
	scanf("%d", &pwdLen);
	printf("사물함 관리 시스템 시작\n");

	locker = (struct locker *)malloc(lockerNum * sizeof(struct locker));

	listen(listenfd, 5);

	while(1){
		connfd = accept(listenfd, &clientUNIXaddr, &clientlen);
		if(fork() == 0){
			if((fd = open(argv[1],O_RDWR|O_CREAT|O_TRUNC,0640))==-1){
				perror(argv[1]);
				exit(2);
			}
			getInfo(connfd, "학번 : ", inmsg);
			id = atoi(inmsg);
			if(id < START_ID){
				printf("wrong id");
				exit(2);
			}
			lseek(fd,(long)(id-START_ID)*sizeof(record),SEEK_SET);
			if((read(fd,&record,sizeof(record))<0)||(record.id==0)){
				record.id = id;
				getInfo(connfd, "이름 : ", name);
				strcpy(record.name, name);
				lseek(fd,-sizeof(record),SEEK_CUR);
				write(fd,&record,sizeof(record));
			}
			printf("%s님이 로그인했습니다\n");
			sprintf(outmsg, "안녕하세요 %s님!\n",name);
			write(connfd, outmsg, strlen(outmsg)+1);
			exit(0);
		}else close(connfd);
	}
}

void getInfo(int connfd, char* outmsg, char* inmsg){
	write(connfd, outmsg, strlen(outmsg)+1);
	read(connfd, inmsg, MAXLINE);
}
