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
	
	//소켓 설정
	clientfd = socket(AF_UNIX, SOCK_STREAM, DEFULAT_PROTOCOL);
	serverUNIXaddr.sun_family = AF_UNIX;
	strcpy(serverUNIXaddr.sun_path, "convert");

	do {
		//서버에 연결 요청
		result = connect(clientfd, &serverUNIXaddr, sizeof(serverUNIXaddr));
		if (result == -1) sleep(1);
	} while (result == -1);

	//서버에서 전송한 데이터를 읽어 그에 맞는 동작 수행
	while(readLine(clientfd, inmsg)) {
		writeInfo(clientfd, inmsg, outmsg);
	}
	exit(0);
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

//서버와 데이터 주고 받기
void writeInfo(int clientfd, char* inmsg, char* outmsg){
	if(strncmp(inmsg,"?",1)==0){//클라이언트의 답변이 필요한 경우
		write(clientfd, "?", 2);
		readLine(clientfd, inmsg);
		printf(inmsg);
		//답변 입력하여 서버에 전송
		scanf("%s", outmsg);
		write(clientfd, outmsg, strlen(outmsg)+1);
	}else{//답변이 필요 없을 경우 -> 출력만
		printf(inmsg);
	}
}
