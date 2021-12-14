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
int lockerInquiry(int, int,  int, struct locker*);
void update(int, int, int, struct student*, struct locker*);
int readLine(int, char*);

int main(int argc, char *argv[]){
	int fd, lockfd, listenfd, connfd, clientlen;
	struct sockaddr_un serverUNIXaddr, clientUNIXaddr;
	int lockerNum, bigNum, pwdLen;
	struct student record;
	struct locker * locker;
	int id,menu,lockerId,n,check;
	char outmsg[MAXLINE], inmsg[MAXLINE], pwdCheck[MAXLINE];

	//소켓 설정
	clientlen = sizeof(clientUNIXaddr);

	listenfd = socket(AF_UNIX, SOCK_STREAM, DEFAULT_PROTOCOL);
	serverUNIXaddr.sun_family = AF_UNIX;
	strcpy(serverUNIXaddr.sun_path, "convert");
	unlink("convert");
	bind(listenfd, &serverUNIXaddr,sizeof(serverUNIXaddr));

	//사물함 개수, 대형 사물함 개수, 비밀번호 자릿수 설정
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

	//파일 디스크립터 생성, stdb:학생정보저장 lcdb:사물함정보저장
	if((fd = open("stdb", O_CREAT|O_TRUNC, 0640))==-1){
		perror("stdb");
		exit(2);
	}
	close(fd);
	if((lockfd = open("lcdb", O_RDWR|O_CREAT, 0640)) == -1){
		perror("lcdb");
		exit(2);
	}

	//사물함 생성 및 초기화
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
		lseek(lockfd,i*sizeof(struct locker),SEEK_SET);
		write(lockfd,&locker[i],sizeof(struct locker));
	}
	close(lockfd);

	//클라이언트 연결 요청 listen
	listen(listenfd, 5);

	while(1){
		//클라이언트 연결 요청 수락
		connfd = accept(listenfd, &clientUNIXaddr, &clientlen);
		//자식 프로세스 생성하여 클라이언트와 작업 수행
		if(fork() == 0){
			if((fd = open("stdb", O_RDWR)) == -1){
                        	perror("stdb");
                        	exit(2);
			}
			if((lockfd = open("lcdb",O_RDWR)) == -1){
				perror("lcdb");
				exit(2);
			}
			//사용자 학번 입력 요청
			writeInfo(connfd, "학번 : ", inmsg, 1);
			id = atoi(inmsg);
			if(id < START_ID){
				printf("wrong id");
				exit(2);
			}
			printf("\n로그인! 학번 : %d\n", id);
			lseek(fd,(id-START_ID)*sizeof(record),SEEK_SET);
			n = read(fd, &record, sizeof(record));
			//처음 이용하는 사용자일 경우 이름을 입력받아 사용자 정보 저장
			if(n<=0||record.id==0){
				printf("회원가입\n");
				record.id = id;
				writeInfo(connfd, "이름 : ", inmsg, 1);
				strncpy(record.name, inmsg, 20);
				record.lockerId = -1;
				lseek(fd,(record.id-START_ID)*sizeof(record),SEEK_SET);
				write(fd,&record,sizeof(record));
			}else{ //이미 등록되어 있는 사용자일 경우 
				printf("%s 회원\n", record.name);
			}
			//로그인한 회원 정보 로그 출력
			printf("로그인한 회원 정보\n");
			printf("학번 : %d\t 이름 : %s\t 사물함 번호 : %d\n", record.id, record.name, record.lockerId);
			sprintf(outmsg,"\n%s님 안녕하세요!\n", record.name);
			writeInfo(connfd,outmsg,inmsg,0);
			while(1){
				//메뉴 출력
				writeInfo(connfd, "\n-----메뉴-----\n 1. 사물함 신청\n 2. 내 사물함 보기\n 3. 종료\n", inmsg, 0);
				writeInfo(connfd, "\n메뉴 선택 : ", inmsg, 1);
				menu = atoi(inmsg);
				//사물함 신청
				if(menu == 1){
					printf("\n%s : 사물함 신청\n",record.name);
					update(fd, lockfd, lockerNum, &record, locker);
					if(record.lockerId != -1) { //이미 사물함을 가지고 있는 경우
						writeInfo(connfd, "이미 신청한 사물함이 있습니다.\n", inmsg, 0);
					} else { //가지고 있는 사물함이 없을 경우 신청 가능한 사물함 목록 출력
						writeInfo(connfd,"신청 가능한 사물함 목록\n",inmsg, 0);
						for(int i = 0;i<lockerNum;i++){
							if(strlen(locker[i].pwd) == 0) {
								sprintf(outmsg,"사물함 번호 : %d\t 대형여부 : %d\t 용량 : %d\n", locker[i].id, locker[i].isBig, locker[i].cap);
								writeInfo(connfd, outmsg, inmsg,0);
							}
						}
						writeInfo(connfd, "\n사물함 선택 : ", inmsg, 1);
                   				lockerId = atoi(inmsg) - 1;
                   				if(-1>=lockerId || lockerId >= lockerNum) { //선택할 수 있는 사물함 번호 범위가 아닌 번호를 입력했을 때
	                        		writeInfo(connfd, "선택할 수 없는 번호입니다.\n", inmsg, 0);
						}else { //다른 사용자가 사용중인 사물함 번호를 입력했을 때
							if(strlen(locker[lockerId].pwd) != 0){
							writeInfo(connfd, "이미 사용자가 있는 사물함입니다.\n", inmsg, 0);
							continue;
							}
							do{ //비밀번호 설정
								writeInfo(connfd, "비밀번호 설정 : ", inmsg, 1);
                	            				if(strlen(inmsg)!=pwdLen){ //비밀번호 자리수 확인
                    	            					sprintf(outmsg, "비밀번호는 %d자리여야 합니다.\n",pwdLen);
                        	        				writeInfo(connfd, outmsg, inmsg, 0);
                            					}
	                        			}while(strlen(inmsg)!=pwdLen);
							do{ //비밀번호 확인
								writeInfo(connfd, "비밀번호 확인 : ", pwdCheck, 1);
								check = strncmp(inmsg, pwdCheck, pwdLen);
								if(strncmp(inmsg, pwdCheck, pwdLen)!=0){
									writeInfo(connfd, "비밀번호가 일치하지 않습니다.\n", inmsg, 0);
								}
							}while(check != 0);
							//사용자 사물함 정보 저장 -> 사물함 신청완료
							strncpy(locker[lockerId].pwd, inmsg,pwdLen);
        	                			lseek(lockfd, lockerId*sizeof(struct locker),SEEK_SET);
            	            				write(lockfd, &locker[lockerId], sizeof(struct locker));
                	        			record.lockerId = lockerId;
                    	    				lseek(fd,-sizeof(record),SEEK_CUR);
                        				write(fd,&record,sizeof(record));
  	                      				printf("\n사물함 신청 완료!\n%d\t %s\t %d\t %d\t %s\n",record.id, record.name, record.lockerId, locker[record.lockerId].id, locker[record.lockerId].pwd);
    	                			}

					}
				}else if (menu == 2){
					//내 사물함 보기
					printf("\n%s : 내 사물함 보기\n", record.name);
					update(fd, lockfd, lockerNum, &record, locker);
					if(record.lockerId == -1) { //사물함을 가지고 있지 않을 경우
						writeInfo(connfd, "신청한 사물함이 없습니다.\n", inmsg, 0);
					} else { //사물함을 가지고 있는 경우
						//비밀번호 확인
						writeInfo(connfd, "비밀번호 : ", inmsg, 1);
						if(strcmp(locker[record.lockerId].pwd, inmsg)==0){
							//사물함 관리 시작
				                	n = lockerInquiry(connfd, fd, pwdLen, &locker[record.lockerId]);
							lseek(lockfd, record.lockerId*sizeof(struct locker), SEEK_SET);
                        				write(lockfd, &locker[record.lockerId], sizeof(struct locker));
                        				if(n){ //사물함 반납 처리
                            					record.lockerId = -1;
                            					lseek(fd,(record.id-START_ID)*sizeof(record),SEEK_SET);
                           					write(fd,&record,sizeof(record));
                            					printf("반납! 학번 : %d\t 사물함 번호 : %d\n", record.id, record.lockerId);
                        				}							
                    				}else{ //비밀번호 틀렸을 경우
                        				writeInfo(connfd, "비밀번호가 틀렸습니다.\n", inmsg, 0);
							//비밀번호를 3번 이상 틀렸을 경우 입력 제한
							if(++locker[record.lockerId].wrongCnt >= 3) {
								printf("%d: 입력 제한\n", id);
								writeInfo(connfd, "비밀번호 입력 횟수를 초과했습니다. 10초간 입력이 제한됩니다.\n", inmsg, 0);
								int cnt = 10;
								while(cnt!=0){
									sprintf(outmsg,"입력 제한 %d초 남음\n",cnt);
									writeInfo(connfd,outmsg,inmsg,0);
									sleep(1);
									cnt--;
								}
								printf("%d: 입력 제한 해제\n", record.id);
								locker[record.lockerId].wrongCnt = 0;
							}

							lseek(lockfd, record.lockerId*sizeof(struct locker), SEEK_SET);
							write(lockfd, &locker[record.lockerId], sizeof(struct locker));
                    				}
					}
				}else if(menu == 3){
					//종료(로그아웃)
					printf("\n%s님이 로그아웃했습니다\n", record.name);
					close(lockfd);
					close(fd);
					exit(0);
				}else {
					//메뉴 범위를 벗어난 메뉴 번호를 입력했을 경우
					writeInfo(connfd, "1~3 사이의 숫자를 입력해 주세요.\n", inmsg, 0);
				}
			}

		}else close(connfd);
	}
}

//한줄 읽기
int readLine(int fd, char* str)
{
        int n;
        do {
        	n = read(fd, str, 1);
        } while(n > 0 && *str++ != '\0');
        return (n > 0);
}

//클라이언트와 데이터 주고 받기
void writeInfo(int connfd, char* outmsg, char* inmsg, int re){
	if(re){ //클라이언트의 답변이 필요한 경우
		write(connfd, "?",2);
		read(connfd, inmsg, MAXLINE);
		write(connfd, outmsg, strlen(outmsg) + 1);
		read(connfd, inmsg, MAXLINE);
	}else{ //클라이언트의 답변이 필요 없는 경우 -> 출력만
		write(connfd, outmsg, strlen(outmsg) + 1);
	}
}

//사물함 관리
int lockerInquiry(int connfd, int fd, int pwdLen, struct locker *locker) {
	int menu,amount,temp,originCap,response;
	char outmsg[MAXLINE], inmsg[MAXLINE];
	struct student record;
	while(1) {
		//사물함 관리 메뉴 출력
		writeInfo(connfd, "\n-----사물함 관리-----\n1. 내 사물함 보기\n2. 물건 넣기\n3. 물건 빼기\n4. 사물함 반납\n5. 사물함 양도\n6. 비밀번호 변경\n7. 이전으로\n", inmsg, 0);
		writeInfo(connfd, "\n메뉴 선택 : ", inmsg, 1);
		menu = atoi(inmsg);
		if(menu == 1) {
			//내 사물함 보기(사물함 번호, 남은 공간)
			printf("\n%d : 내 사물함 보기\n", locker->id);
			sprintf(outmsg, "사물함 번호 : %d\t 남은 공간 : %d\n", locker->id, locker->cap);
			writeInfo(connfd, outmsg, inmsg, 0);
		} else if (menu == 2) {
			//물건 넣기
			printf("\n%d : 물건 넣기\n", locker->id);
			writeInfo(connfd, "넣을 물건 개수 입력: ", inmsg, 1);
			amount = atoi(inmsg);
			temp = locker->cap - amount;
			if(temp < 0) {//사물함 남은 공간보다 많이 넣었을 경우
				sprintf(outmsg, "용량이 부족합니다. 남은 공간: %d\n", locker->cap);
				writeInfo(connfd, outmsg, inmsg, 0);
			} else {//물건 넣기 성공
				locker->cap = temp;
				sprintf(outmsg, "물건 넣기 성공. 남은 공간: %d\n", locker->cap);
				writeInfo(connfd, outmsg, inmsg, 0);
			}
		} else if (menu == 3) {
			//물건 빼기
			printf("\n%d : 물건 빼기\n", locker->id);
			writeInfo(connfd, "뺼 물건 개수 입력: ", inmsg, 1);
			amount = atoi(inmsg);
			temp = amount + locker->cap;
			if(locker->isBig) {
				originCap = 10;
			} else {
				originCap = 5;
			}
			if(temp > originCap) {//사물함에 들어있는 물건의 개수보다 많이 빼려고 할 경우
				sprintf(outmsg, "물건을 %d개 뺼 수 없습니다. 현재 물건 개수: %d\n", amount, originCap-locker->cap);
				writeInfo(connfd, outmsg, inmsg, 0);
			} else {//물건 빼기 성공
				locker->cap = temp;
				sprintf(outmsg, "물건 %d개 빼기 성공. 남은 용량: %d\n", amount, locker->cap);
				writeInfo(connfd, outmsg, inmsg, 0);
			}
		} else if(menu == 4) {
			//사물함 반납
			printf("\n%d : 사물함 반납\n",locker->id);
			writeInfo(connfd, "반납하시겠습니까? (예 : 1, 아니오 : 2)\n", inmsg, 1);
			response = atoi(inmsg);
			if(response == 1) {//사물함 반납 -> 사물함 초기화
				locker->pwd[0] = '\0';
				if(locker->isBig == 1) {
					locker->cap = 10;
				} else {
					locker->cap = 5;
				}
				locker->wrongCnt = 0;
				writeInfo(connfd, "반납되었습니다.\n", inmsg, 0);
				return 1;
			} else {//사물함 반납 취소
				writeInfo(connfd, "반납 취소되었습니다.\n", inmsg, 0);
			}
		} else if (menu == 5) {
			//사물함 양도
			printf("\n%d : 사물함 양도\n", locker->id);
			lseek(fd,0,SEEK_SET);
			int cnt = 0;
			int n;
			//양도가능한 학생 명단 출력(사물함을 가지고 있지 않은 학생)
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
			if(cnt>0){//양도가능한 학생이 있을 경우
				//양도할 학생 학번을 입력받아 해당 학생에게 양도 처리
				writeInfo(connfd, "\n양도할 학생의 학번 입력: ", inmsg, 1);
				response = atoi(inmsg);
				lseek(fd,(response-START_ID)*sizeof(record),SEEK_SET);
				read(fd, &record, sizeof(record));
				record.lockerId = locker->id - 1;
				printf("양도! 학번 : %d 사물함 번호 : %d\n",record.id, record.lockerId);
				lseek(fd,-sizeof(record),SEEK_CUR);
				write(fd, &record, sizeof(record));
				return 1;
			}else{//양도가능한 학생이 없을 경우
				writeInfo(connfd,"양도가능한 학생이 없습니다.\n",inmsg,0);
			}
		} else if (menu == 6) {
			//비밀번호 변경
			printf("\n%d : 비밀번호 변경\n", locker->id);
			writeInfo(connfd, "현재 비밀번호 입력: ", inmsg, 1);
			if(strcmp(locker->pwd, inmsg)==0) {//현재 비밀번호를 맞게 입력했을 경우
				//비밀번호 변경 수행
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
			} else {//현재 비밀번호를 잘못 입력했을 경우
				writeInfo(connfd, "비밀번호가 틀립니다.\n", inmsg, 0);
			}
		} else if(menu == 7) {
			//사물함 관리 종료 -> 이전 메뉴로 이동
			return 0;
		} else {
			//사물함 관리 메뉴 범위를 벗어난 메뉴 번호를 입력했을 경우
			writeInfo(connfd, "1~7 사이의 숫자를 입력해 주세요.\n", inmsg, 0);
		}
	}
}

//학생 레코드와 사물함 레코드 정보 업데이트
void update(int fd, int lockfd, int lockerNum, struct student* record, struct locker* locker){
	int n;
	
	lseek(fd, (record->id-START_ID)*sizeof(*record), SEEK_SET);
        read(fd, record, sizeof(*record));

	printf("\n-----업데이트-----\n");
	printf(" 학번 : %d\t 이름 : %s\t 사물함 번호 : %d\n", record->id, record->name, record->lockerId);

        for(int i = 0;i<lockerNum;i++){
		lseek(lockfd,i*sizeof(struct locker),SEEK_SET);
                n = read(lockfd, (locker+i), sizeof(struct locker));
                printf("사물함 번호 : %d\t 사물함 비밀번호 : %s\t 사물함 용량 : %d\n", (locker+i)->id, (locker+i)->pwd, (locker+i)->cap);
        }

}
