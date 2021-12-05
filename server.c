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

void writeInfo(int, char*, char*, int);

int main(int argc, char *argv[]){
	int fd, listenfd, connfd, clientlen;
	struct sockaddr_un serverUNIXaddr, clientUNIXaddr;
	int lockerNum, bigNum, pwdLen;
	struct student record;
	struct locker * locker;
	int id,menu;
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
	for(int i = 0;i<lockerNum;i++){
		locker[i].id = i;
		if(i<bigNum){
			locker[i].isBig = 0;
			locker[i].cap = 5;
		}else{
			locker[i].isBig = 1;
			locker[i].cap = 10;
		}
		locker[i].wrongCnt = 0;
		locker[i].islock = 0;
		locker[i].lockTime = 0;
	}

	listen(listenfd, 5);

	while(1){
		connfd = accept(listenfd, &clientUNIXaddr, &clientlen);
		if(fork() == 0){
			if((fd = open(argv[1],O_RDWR|O_CREAT|O_TRUNC,0640))==-1){
				perror(argv[1]);
				exit(2);
			}
			writeInfo(connfd, "학번 : ", inmsg, 1);
			id = atoi(inmsg);
			if(id < START_ID){
				printf("wrong id");
				exit(2);
			}
			lseek(fd,(long)(id-START_ID)*sizeof(record),SEEK_SET);
			if((read(fd,&record,sizeof(record))<0)||(record.id==0)){
				record.id = id;
				writeInfo(connfd, "이름 : ", name, 1);
				strcpy(record.name, name);
				lseek(fd,-sizeof(record),SEEK_CUR);
				write(fd,&record,sizeof(record));
			}
			printf("%s님이 로그인했습니다\n",name);
			while(1){
				sprintf(outmsg, "%s\n -----메뉴-----\n 1. 사물함 신청\n 2. 내 사물함 보기\n 3. 종료\n",name);
				writeInfo(connfd, outmsg, inmsg, 1);
				menu = atoi(inmsg);
				if(menu == 1){
					writeInfo(connfd,"사물함 신청\n",inmsg, 0);
					for(int i = 0;i<lockerNum;i++){
						sprintf(outmsg,"id : %d, is Big : %d, cap : %d\n", locker[i].id, locker[i].isBig, locker[i].cap);
						writeInfo(connfd, outmsg, inmsg,0);
					}
				}else if (menu == 2){
					printf("내 사물함 보기\n");
				}else if(menu == 3){
					printf("%s님이 로그아웃했습니다\n", name);
					exit(0);
				}
			}

		}else close(connfd);
	}
}

void writeInfo(int connfd, char* outmsg, char* inmsg, int re){
	if(re){
		write(connfd, "?",2);
		read(connfd, inmsg, MAXLINE);
		write(connfd, outmsg, strlen(outmsg) + 1);
		read(connfd, inmsg, MAXLINE);
	}else{
		write(connfd, outmsg, strlen(outmsg) + 1);
	}
}

