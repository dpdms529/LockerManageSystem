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
#define MAXLINE 160

void writeInfo(int, char*, char*, int);

int main(int argc, char *argv[]){
	int fd, listenfd, connfd, clientlen;
	struct sockaddr_un serverUNIXaddr, clientUNIXaddr;
	int lockerNum, bigNum, pwdLen;
	struct student record;
	struct locker * locker;
	int id,menu,lockerId;
	char outmsg[MAXLINE], inmsg[MAXLINE];

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
		locker[i].id = i+1;
		if(i<lockerNum-bigNum){
			locker[i].isBig = 0;
			locker[i].cap = 5;
		}else{
			locker[i].isBig = 1;
			locker[i].cap = 10;
		}
		locker[i].pwd = (char*)malloc(pwdLen*sizeof(char));
		locker[i].wrongCnt = 0;
		locker[i].islock = 0;
		locker[i].lockTime = 0;
	}

	listen(listenfd, 5);

	while(1){
		connfd = accept(listenfd, &clientUNIXaddr, &clientlen);
		if(fork() == 0){
			if((fd = open(argv[1], O_RDWR|O_CREAT, 0640)) == -1){
                        	perror(argv[1]);
                        	exit(2);
			}
			writeInfo(connfd, "학번 : ", inmsg, 1);
			id = atoi(inmsg);
			if(id < START_ID){
				printf("wrong id");
				exit(2);
			}
			printf("id : %d\n", id);
			lseek(fd,(id-START_ID)*sizeof(record),SEEK_SET);
			int n = read(fd, &record, sizeof(record));
			printf("n : %d\n", n);
			if(n<=0||record.id==0){
				printf("norecord\n");
				record.id = id;
				writeInfo(connfd, "이름 : ", inmsg, 1);
				strncpy(record.name, inmsg, 20);
				lseek(fd,(record.id-START_ID)*sizeof(record),SEEK_SET);
				write(fd,&record,sizeof(record));
			}else{
				printf("record : %d\t %s\n", record.id,record.name);
			}
			printf("%s님이 로그인했습니다\n",record.name);
			while(1){
				sprintf(outmsg, "-----메뉴-----\n 1. 사물함 신청\n 2. 사물함 관리\n 3. 종료\n");
				writeInfo(connfd, outmsg, inmsg, 1);
				menu = atoi(inmsg);
				if(menu == 1){
					writeInfo(connfd,"사물함 신청\n",inmsg, 0);
					for(int i = 0;i<lockerNum;i++){
						sprintf(outmsg,"id : %d, is Big : %d, cap : %d\n", locker[i].id, locker[i].isBig, locker[i].cap);
						writeInfo(connfd, outmsg, inmsg,0);
					}
					writeInfo(connfd, "사물함 선택 : ", inmsg, 1);
					lockerId = atoi(inmsg) - 1;
					do{
						writeInfo(connfd, "비밀번호 입력 : ", inmsg, 1);
						if(strlen(inmsg)!=pwdLen){
						       	sprintf(outmsg, "비밀번호는 %d자리여야 합니다.\n",pwdLen);
							writeInfo(connfd, outmsg, inmsg, 0);
						}

					}while(strlen(inmsg)!=pwdLen);
					strncpy(locker[lockerId].pwd, inmsg,pwdLen);
					record.myLocker = locker[lockerId];
					lseek(fd,-sizeof(record),SEEK_CUR);
					write(fd,&record,sizeof(record));
					printf("%d\t %s\t %d\t %s\n",record.id, record.name, record.myLocker.id, record.myLocker.pwd);
				}else if (menu == 2){
					printf("내 사물함 보기\n");
					writeInfo(connfd, "비밀번호 : ", inmsg, 1);
					if(strcmp(record.myLocker.pwd, inmsg)==0){
						lockerInquiry(connfd, pwdLen, &record);

					}else{
						writeInfo(connfd, "비밀번호가 맞지않습니다.\n", inmsg, 0);
					}
				}else if(menu == 3){
					printf("%s님이 로그아웃했습니다\n", record.name);
					close(fd);
					exit(0);
				}
			}

		}else {
			close(connfd);
		}
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


void lockerInquiry(int connfd, int pwdLen, struct student *record) {
	int menu;
	char outmsg[MAXLINE], inmsg[MAXLINE];
	sprintf(outmsg, "-----사물함 관리-----\n1. 내 사물함 보기\n2. 물건 넣기\n3. 물건 빼기\n4. 사물함 반납\n5. 사물함 양도\n6. 비밀번호 변경\n");
	writeInfo(connfd, outmsg, inmsg, 1);
	menu = atoi(inmsg);
	if(menu == 1) {
		printf("내 사물함 보기\n");
		sprintf(outmsg, "사물함 번호 : %d\t 남은 공간 : %d\n", record->myLocker.id, record->myLocker.cap);
		writeInfo(connfd, outmsg, inmsg, 0);
	} else if (menu == 2) { // insert mulgun
		printf("물건 넣기\n");
		int amount, temp;
		writeInfo(connfd, "넣을 물건 개수 입력: ", inmsg, 1);
		amount = atoi(inmsg);
		temp = record->myLocker.cap - amount;
		if(temp < 0) {
			sprintf(outmsg, "용량이 부족합니다. 남은 공간: %d\n", record->myLocker.cap);
			writeInfo(connfd, outmsg, inmsg, 0);
		} else {
			record->myLocker.cap = temp;
			sprintf(outmsg, "물건 넣기 성공. 남은 공간: %d\n", record->myLocker.cap);
			writeInfo(connfd, outmsg, inmsg, 0);
		}
	} else if (menu == 3) { // mulgun out
		printf("물건 빼기\n");
		int amount, temp, originCap;
		writeInfo(connfd, "뺼 물건 개수 입력: ", inmsg, 1);
		amount = atoi(inmsg);
		temp = amount + record->myLocker.cap;
		if(record->myLocker.isBig) { //big locker
			originCap = 10;
		} else {
			originCap = 5;
		}
		if(temp > originCap) {
			sprintf(outmsg, "물건을 %d개 뺼 수 없습니다. 현재 물건 개수: %d\n", amount, originCap-record->myLocker.cap);
			writeInfo(connfd, outmsg, inmsg, 0);
		} else {
			record->myLocker.cap = temp;
			sprintf(outmsg, "물건 %d개 빼기 성공. 남은 용량: %d\n", amount, record->myLocker.cap);
			writeInfo(connfd, outmsg, inmsg, 0);
		}
	} else if(menu == 4) {
		printf("사물함 반납\n");
		int response;
		writeInfo(connfd, "반납하시겠습니까? (예 : 1, 아니오 : 2)\n", inmsg, 1);
		response = atoi(inmsg);
		if(response == 1) {
//			record.myLocker.pwd = NULL;
//			record.myLocker.cap = 0;
//			record.myLocker.wrongCnt = 0;
//			record.myLocker.lockTime = 0;
//			record.myLocker = NULL;
			writeInfo(connfd, "반납되었습니다.\n", inmsg, 0);
		} else {
			writeInfo(connfd, "반납 취소되었습니다.\n", inmsg, 0);
		}
	} else if (menu == 5) { // transfer
		printf("사물함 양도\n");
	} else if (menu == 6) { // change pwd
		printf("비밀번호 변경\n");
		writeInfo(connfd, "현재 비밀번호 입력: ", inmsg, 1);
		if(strcmp(record->myLocker.pwd, inmsg)==0) {
			do{
				writeInfo(connfd, "새 비밀번호 입력: ", inmsg, 1);
				if(strlen(inmsg) != pwdLen) {
					sprintf(outmsg, "비밀번호는 %d자리여야 합니다.\n",pwdLen);
					writeInfo(connfd, outmsg, inmsg, 0);
				}
			} while(strlen(inmsg)!=pwdLen);
			strncpy(record->myLocker.pwd, inmsg, pwdLen);
			writeInfo(connfd, "비밀번호가 변경되었습니다.\n", inmsg, 0);
		} else {
			writeInfo(connfd, "비밀번호가 틀립니다.\n", inmsg, 0);
		}
	}
}
