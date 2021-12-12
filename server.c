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
#include "locker.h"
#define DEFAULT_PROTOCOL 0
#define MAXLINE 200

void writeInfo(int, char*, char*, int);
void alarmHandler();
int lockerInquiry(int, int,  int, struct locker*);
void update(int, int, int, struct student*, struct locker*);
int readLine(int, char*);
int clientPid;

int main(int argc, char *argv[]){
	int fd, lockfd, listenfd, connfd, clientlen;
	struct sockaddr_un serverUNIXaddr, clientUNIXaddr;
	int lockerNum, bigNum, pwdLen;
	struct student record;
	struct locker * locker;
	int id,menu,lockerId,n;
	char outmsg[MAXLINE], inmsg[MAXLINE];

	signal(SIGCHLD, SIG_IGN);
	signal(SIGALRM, alarmHandler);
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
	while(bigNum>lockerNum || bigNum<0){
		printf("0이상 %d이하로 설정해주세요 : ", lockerNum);
		scanf("%d",&bigNum);
	}
	printf("비밀번호 자릿수를 설정하세요 : ");
	scanf("%d", &pwdLen);
	printf("사물함 관리 시스템 시작\n");

	if((fd = open("stdb", O_CREAT|O_TRUNC, 0640))==-1){
		perror("stdb");
		exit(2);
	}
	close(fd);
	if((lockfd = open("lcdb", O_RDWR|O_CREAT, 0640)) == -1){
		perror("lcdb");
		exit(2);
	}

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
		locker[i].pwd[0] = '\0';
		locker[i].wrongCnt = 0;
		locker[i].islock = 0;
		locker[i].lockTime = 0;
		lseek(lockfd,i*sizeof(struct locker),SEEK_SET);
		write(lockfd,&locker[i],sizeof(struct locker));
	}
	close(lockfd);

	listen(listenfd, 5);

	while(1){
		connfd = accept(listenfd, &clientUNIXaddr, &clientlen);
		if(fork() == 0){
			char pidmsg[6];
			read(connfd, pidmsg, MAXLINE);
			clientPid = atoi(pidmsg);
			printf("clientPid: %d\n",clientPid);
			if((fd = open("stdb", O_RDWR)) == -1){
                        	perror("stdb");
                        	exit(2);
			}
			if((lockfd = open("lcdb",O_RDWR)) == -1){
				perror("lcdb");
				exit(2);
			}
			
			writeInfo(connfd, "학번 : ", inmsg, 1);
			id = atoi(inmsg);
			if(id < START_ID){
				printf("wrong id");
				exit(2);
			}
			printf("\n로그인! 학번 : %d\n", id);
			lseek(fd,(id-START_ID)*sizeof(record),SEEK_SET);
			n = read(fd, &record, sizeof(record));
			if(n<=0||record.id==0){
				printf("회원가입\n");
				record.id = id;
				writeInfo(connfd, "이름 : ", inmsg, 1);
				strncpy(record.name, inmsg, 20);
				record.lockerId = -1;
				lseek(fd,(record.id-START_ID)*sizeof(record),SEEK_SET);
				write(fd,&record,sizeof(record));
			}else{
				printf("%s 회원\n", record.name);
			}
			printf("로그인한 회원 정보\n");
			printf("학번 : %d\t 이름 : %s\t 사물함 번호 : %d\n", record.id, record.name, record.lockerId);
			sprintf(outmsg,"\n%s님 안녕하세요!\n", record.name);
			writeInfo(connfd,outmsg,inmsg,0);
			while(1){
				writeInfo(connfd, "\n-----메뉴-----\n 1. 사물함 신청\n 2. 내 사물함 보기\n 3. 종료\n", inmsg, 0);
				writeInfo(connfd, "\n메뉴 선택 : ", inmsg, 1);
				menu = atoi(inmsg);
				if(menu == 1){
					update(fd, lockfd, lockerNum, &record, locker);
					if(record.lockerId != -1) {
						writeInfo(connfd, "이미 신청한 사물함이 있습니다.\n", inmsg, 0);
					} else {
						writeInfo(connfd,"신청 가능한 사물함 목록\n",inmsg, 0);
						for(int i = 0;i<lockerNum;i++){
							if(strlen(locker[i].pwd) == 0) {
								sprintf(outmsg,"사물함 번호 : %d\t 대형여부 : %d\t 용량 : %d\n", locker[i].id, locker[i].isBig, locker[i].cap);
								writeInfo(connfd, outmsg, inmsg,0);
							}
						}
					}
					writeInfo(connfd, "\n사물함 선택 : ", inmsg, 1);
					lockerId = atoi(inmsg) - 1;
					if(-1>=lockerId || lockerId >= lockerNum) {
						writeInfo(connfd, "선택할 수 없는 번호입니다.\n", inmsg, 0);
					} else {
						do{
							writeInfo(connfd, "비밀번호 입력 : ", inmsg, 1);
							if(strlen(inmsg)!=pwdLen){
								sprintf(outmsg, "비밀번호는 %d자리여야 합니다.\n",pwdLen);
								writeInfo(connfd, outmsg, inmsg, 0);
							}
							
						}while(strlen(inmsg)!=pwdLen);
						strncpy(locker[lockerId].pwd, inmsg,pwdLen);
						lseek(lockfd, lockerId*sizeof(struct locker),SEEK_SET);
						write(lockfd, &locker[lockerId], sizeof(struct locker));
						record.lockerId = lockerId;
						lseek(fd,-sizeof(record),SEEK_CUR);
						write(fd,&record,sizeof(record));
						printf("%d\t %s\t %d\t %d\t %s\n",record.id, record.name, record.lockerId, locker[record.lockerId].id, locker[record.lockerId].pwd);
					}
				}else if (menu == 2){
					update(fd, lockfd, lockerNum, &record, locker);
					if(record.lockerId == -1) {
						writeInfo(connfd, "신청한 사물함이 없습니다.\n", inmsg, 0);
					} else {
							writeInfo(connfd, "비밀번호 : ", inmsg, 1);
							if(strcmp(locker[record.lockerId].pwd, inmsg)==0){
				                        	n = lockerInquiry(connfd, fd, pwdLen, &locker[record.lockerId]);
								lseek(lockfd, record.lockerId*sizeof(struct locker), SEEK_SET);
                        					write(lockfd, &locker[record.lockerId], sizeof(struct locker));
                        					if(n){
                            						record.lockerId = -1;
                            						lseek(fd,(record.id-START_ID)*sizeof(record),SEEK_SET);
                           						write(fd,&record,sizeof(record));
                            						printf("반납! 학번 : %d\t 사물함 번호 : %d\n", record.id, record.lockerId);
                        					}							
                    					}else{
                        					writeInfo(connfd, "비밀번호가 틀렸습니다.\n", inmsg, 0);
								if(++locker[record.lockerId].wrongCnt >= 3) {
									printf("enter lock\n");
//									writeInfo(connfd,"비밀번호 오류 횟수가 초과되었습니다. 10초간 로그인이 제한됩니다.\n", inmsg, 0);
									//kill(clientPid, SIGTSTP);
									alarm(10);
									int cnt = 10;
									while(cnt!=0){
										sprintf(outmsg,"wait : %d초\n",cnt);
										writeInfo(connfd,outmsg,inmsg,0);
										sleep(1);
										cnt--;
									}
									locker[record.lockerId].wrongCnt = 0;
								}

								lseek(lockfd, record.lockerId*sizeof(struct locker), SEEK_SET);
								write(lockfd, &locker[record.lockerId], sizeof(struct locker));
                    					}
					}
				}else if(menu == 3){
					printf("\n%s님이 로그아웃했습니다\n", record.name);
					close(lockfd);
					close(fd);
					exit(0);
				}
			}

		}else {
			close(connfd);
		}
	}
}

int readLine(int fd, char* str)
{
        int n;
        do {
                n = read(fd, str, 1);
        } while(n > 0 && *str++ != '\0');
        return (n > 0);
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

void alarmHandler()
{
	printf("\n제한 해제\n");

}


int lockerInquiry(int connfd, int fd, int pwdLen, struct locker *locker) {
	int menu,amount,temp,originCap,response;
	char outmsg[MAXLINE], inmsg[MAXLINE];
	struct student record;
	while(1) {
		writeInfo(connfd, "\n-----사물함 관리-----\n1. 내 사물함 보기\n2. 물건 넣기\n3. 물건 빼기\n4. 사물함 반납\n5. 사물함 양도\n6. 비밀번호 변경\n7. 이전으로\n", inmsg, 0);
		writeInfo(connfd, "\n메뉴 선택 : ", inmsg, 1);
		menu = atoi(inmsg);
		if(menu == 1) {
			printf("\n%d : 내 사물함 보기\n", locker->id);
			sprintf(outmsg, "사물함 번호 : %d\t 남은 공간 : %d\n", locker->id, locker->cap);
			writeInfo(connfd, outmsg, inmsg, 0);
		} else if (menu == 2) { // insert mulgun
			printf("\n%d : 물건 넣기\n", locker->id);
			writeInfo(connfd, "넣을 물건 개수 입력: ", inmsg, 1);
			amount = atoi(inmsg);
			temp = locker->cap - amount;
			if(temp < 0) {
				sprintf(outmsg, "용량이 부족합니다. 남은 공간: %d\n", locker->cap);
				writeInfo(connfd, outmsg, inmsg, 0);
			} else {
				locker->cap = temp;
				sprintf(outmsg, "물건 넣기 성공. 남은 공간: %d\n", locker->cap);
				writeInfo(connfd, outmsg, inmsg, 0);
			}
		} else if (menu == 3) {
			printf("\n%d : 물건 빼기\n", locker->id);
			writeInfo(connfd, "뺼 물건 개수 입력: ", inmsg, 1);
			amount = atoi(inmsg);
			temp = amount + locker->cap;
			if(locker->isBig) { //big locker
				originCap = 10;
			} else {
				originCap = 5;
			}
			if(temp > originCap) {
				sprintf(outmsg, "물건을 %d개 뺼 수 없습니다. 현재 물건 개수: %d\n", amount, originCap-locker->cap);
				writeInfo(connfd, outmsg, inmsg, 0);
			} else {
				locker->cap = temp;
				sprintf(outmsg, "물건 %d개 빼기 성공. 남은 용량: %d\n", amount, locker->cap);
				writeInfo(connfd, outmsg, inmsg, 0);
			}
		} else if(menu == 4) {
			printf("\n%d : 사물함 반납\n",locker->id);
			writeInfo(connfd, "반납하시겠습니까? (예 : 1, 아니오 : 2)\n", inmsg, 1);
			response = atoi(inmsg);
			if(response == 1) {
				locker->pwd[0] = '\0';
				if(locker->isBig == 1) {
					locker->cap = 10;
				} else {
					locker->cap = 5;
				}
				locker->wrongCnt = 0;
				writeInfo(connfd, "반납되었습니다.\n", inmsg, 0);
				return 1;
			} else {
				writeInfo(connfd, "반납 취소되었습니다.\n", inmsg, 0);
			}
		} else if (menu == 5) {
			printf("\n%d : 사물함 양도\n", locker->id);
			lseek(fd,0,SEEK_SET);
			int cnt = 0;
			int n;
			writeInfo(connfd, "양도가능한 학생 명단\n", inmsg, 0);
			while(1){
				n=read(fd, &record, sizeof(record));
				if(n>0){
					if(record.id !=0 && record.lockerId == -1){
						sprintf(outmsg, "학번 : %d\t 이름 :  %s\n", record.id, record.name);
						writeInfo(connfd, outmsg, inmsg, 0);
						cnt++;
					}
				}else break;
			}
			if(cnt>0){
				writeInfo(connfd, "\n양도할 학생의 학번 입력: ", inmsg, 1);
				response = atoi(inmsg);
				lseek(fd,(response-START_ID)*sizeof(record),SEEK_SET);
				read(fd, &record, sizeof(record));
				record.lockerId = locker->id - 1;
				printf("양도! 학번 : %d 사물함 번호 : %d\n",record.id, record.lockerId);
				lseek(fd,-sizeof(record),SEEK_CUR);
				write(fd, &record, sizeof(record));
				return 1;
			}else{
				writeInfo(connfd,"양도가능한 학생이 없습니다.\n",inmsg,0);
			}
		} else if (menu == 6) { // change pwd
			printf("\n%d : 비밀번호 변경\n", locker->id);
			writeInfo(connfd, "현재 비밀번호 입력: ", inmsg, 1);
			if(strcmp(locker->pwd, inmsg)==0) {
				do{
					writeInfo(connfd, "비밀번호 입력 : ", inmsg, 1);
					if(strlen(inmsg)!=pwdLen){
					       	sprintf(outmsg, "비밀번호는 %d자리여야 합니다.\n",pwdLen);
						writeInfo(connfd, outmsg, inmsg, 0);
					}

				}while(strlen(inmsg)!=pwdLen);
				strncpy(locker->pwd, inmsg, pwdLen);
				writeInfo(connfd, "비밀번호가 변경되었습니다.\n", inmsg, 0);
				return 0;
			} else {
				writeInfo(connfd, "비밀번호가 틀립니다.\n", inmsg, 0);
			}
		} else if(menu == 7) {
			return 0;
		}
	}
}


void update(int fd, int lockfd, int lockerNum, struct student* record, struct locker* locker){
	int n;

	lseek(fd, (record->id-START_ID)*sizeof(*record), SEEK_SET);
        read(fd, record, sizeof(*record));

	printf("\n-----업데이트-----\n");
	printf(" 학번 : %d\t 이름 : %s\t 사물함 번호 : %d\n", record->id, record->name, record->lockerId);

        for(int i = 0;i<lockerNum;i++){
		lseek(lockfd,i*sizeof(struct locker),SEEK_SET);
                n = read(lockfd, (locker+i), sizeof(struct locker));
                printf("사물함 번호 : %d\t 사물함 비밀번호 : %s\t 사물함 용량 : %d\n", (locker+i)->id, (locker+i)->pwd, (locker+1)->cap);
        }

}
