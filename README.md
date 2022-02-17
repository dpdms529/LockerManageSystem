# LockerManageSystem
# 유닉스 시스템 프로젝트 보고서

# 1. 팀원 및 역할분담

| 이름 | 역할 |
| --- | --- |
| 조예은 | 소켓 통신 구현, 사용자 및 사물함 정보 저장, 반납/양도 기능 구현 |
| 김자연 | 사물함 조회 기능 구현, 물건 넣기/빼기 기능 구현, 비밀번호 변경기능 구현, 비밀번호 틀릴 시 입력 제한 기능 구현 |

# 2. 전체 흐름

![Untitled](https://user-images.githubusercontent.com/60471550/154383342-77da9f0a-64a0-4598-b286-d7308fb0bd12.png)

![Untitled 1](https://user-images.githubusercontent.com/60471550/154383375-1f94b40e-adf5-43c6-bfc9-39cfe51a59b8.png)
## server.c

- client.c와 소켓 통신
- 사용자 관리
- 사물함 관리

## client.c

- server.c에게 소켓 연결 요청 및 통신
- 사물함 관리 시스템의 고객 역할

## student.h

- 사용자 이름
- 학번
- 신청한 사물함 번호

## locker.h

- 사물함 번호
- 비밀번호
- 대형 사물함 여부
- 남은 공간 (넣을 수 있는 물건 개수)
- 비밀번호 틀린 횟수

# 3. 기능 설명

## 1) 기본 기능

### 서버

- 사물함 관리
- 사용자들에게 사물함 정보 제공
- 사용자에게 사물함 할당 시 비밀번호 지정하도록 함
- 사용자가 본인 사물함에 접근 시 입력하는 비밀번호가 맞는지 확인하고, 맞으면 접근 가능하도록 함

### 클라이언트

- 사물함 신청 및 사용
- 비밀번호 설정

## 2) 추가 기능

| 기능 | 설명 |
| --- | --- |
| 사물함 양도 | 사물함이 없는 사용자에게 사물함을 양도할 수 있다. |
| 사용자 동시접근 | 동시에 여러 사용자가 서버에 접근하고 사물함 정보를 실시간으로 반영한다. |
| 비밀번호 자릿수 설정 | 서버를 열 때, 비밀번호 자릿수를 매번 다르게 설정할 수 있다. |
| 사물함 용량 정보 제공 | 사물함의 용량을 제공하여 넣고 뺄 수 있는 물건의 개수를 제공한다. |
| 대형 사물함 제공 | 일반 사물함과 대형 사물함으로 구분하여 용량에 차이를 둔다. |
| 사물함 개수 설정 | 서버를 열 때, 전체 사물함 개수와 대형 사물함 개수를 설정할 수 있다. |
| 비밀번호 오류 횟수 초과 시 입력 제한 | 사용자는 비밀번호를 3번 이상 틀릴 시 10초간 입력 제한을 받는다. |
| 비밀번호 확인 | 비밀번호를 설정 시 두 번 입력하여 제대로 비밀번호를 설정할 수 있도록 한다. |
| 사물함 반납 | 사물함을 반납할 수 있으며, 해당 사물함은 초기화된다. |
| 사물함 접근 제한 | 다른 사용자가 사용 중인 사물함에는 다른 사용자가 접근할 수 없도록 한다. |

# 4. 주요 코드

### 데이터 주고받기

```c
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

------------------------------------------------------------------------------

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
```

1. 서버는 클라이언트의 답변이 필요한 경우 '?'를 보내 클라이언트에게 답변이 필요함을 알린다.
2. 클라이언트는 '?'를 서버에 보내 질문을 보내달라는 요청을 한다.
3. 서버가 질문을 보내면 클라이언트는 질문에 해당하는 답변을 입력하여 서버에게 전송한다.

### 사물함 양도

```c
// 사물함 양도
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

------------------------------------------------------------------------------

// 메인 함수, 양도한 학생의 사물함 번호 초기화 및 반납 처리
if(n){
			record.lockerId = -1;
			lseek(fd,(record.id-START_ID)*sizeof(record),SEEK_SET);
			write(fd,&record,sizeof(record));
			printf("반납! 학번 : %d\t 사물함 번호 : %d\n", record.id, record.lockerId);
}
```

1. 양도할 학생의 학번을 입력받는다.
2. 파일 디스크립터의 인덱스를 양도할 학생의 정보가 저장되어 있는 위치로 이동한다.
3. 양도받는 학생의 사물함 번호에 양도할 학생의 사물함 번호를 저장한다.
4. 1을 리턴하여 메인 함수로 돌아가면 양도한 학생의 사물함 번호를 -1로 초기화한다.

# 5. 결과 화면

### [사물함 정보 설정]

- 서버를 열 때 전체 사물함 개수, 대형 사물함 개수, 비밀번호 자릿수 설정
    
    ![Untitled 2](https://user-images.githubusercontent.com/60471550/154383613-6cd9f111-7eb2-47e9-982a-9475ad633d50.png)
    

### [로그인]

- 사용자 학번이 student db에 존재하지 않는 경우
    - 이름을 입력받고 db에 추가 후 인사메세지 출력

![Untitled 3](https://user-images.githubusercontent.com/60471550/154383641-e15a8907-35b3-497c-b58c-0b7016ec37b1.png)

![Untitled 4](https://user-images.githubusercontent.com/60471550/154383697-916f01e4-e226-46a9-a2f5-34982d26a900.png)
Server

- 사용자 학번이 student db에 존재하는 경우
    - 인사메세지 출력

![Untitled 5](https://user-images.githubusercontent.com/60471550/154383714-f7281e97-03f6-41a7-b67f-aa46a71ea155.png)
![Untitled 4](https://user-images.githubusercontent.com/60471550/154383703-bf0f2805-7d1d-4e80-a2c7-9f311465d86a.png)
Server

### [처음 메뉴]

**1. 사물함 신청**

- 신청 가능한 사물함 정보 목록을 출력
- 사물함을 선택하면 비밀번호 설정 후 사물함 할당
    - 비밀번호 확인시 strncmp함수를 사용하여 입력한 문자가 설정된 자릿수까지는 맞고 그 뒷자리 문자부터 다른경우 비밀번호가 맞다고 처리하는 오류 존재 → strcmp함수를 사용해야 함
![Untitled 6](https://user-images.githubusercontent.com/60471550/154383741-d2edc380-bb62-4df4-b272-2717f4a1a01b.png)

![Untitled 7](https://user-images.githubusercontent.com/60471550/154383762-b2d58272-9151-4b9e-a3fe-87b75798fc39.png)
Server

cf) locker 구조체의 사물함 번호에는 사물함 배열 index + 1, student 구조체의 사물함 번호에는 사물함 배열 index가 저장되어 있음

- 이미 사용 중인 사물함이 있는 경우 오류 메세지 출력
    
    ![Untitled 8](https://user-images.githubusercontent.com/60471550/154383797-d673ca7f-d505-4c4e-ac4d-9df485dd359c.png)
    

**2. 내 사물함 보기**

- 비밀번호를 입력하고 사물함 관리 메뉴 진입
    
    ![Untitled 9](https://user-images.githubusercontent.com/60471550/154383824-4a91669c-c47b-4066-ac0f-6b19a70a6ab8.png)
    

- 비밀번호 틀릴 시 사물함 관리 메뉴 진입 불가
    
    ![Untitled 10](https://user-images.githubusercontent.com/60471550/154383840-695b1bf8-af1e-4955-b62b-f10c720285fe.png)
    

- 3번 틀릴 경우 10초간 입력 제한
    
    ![Untitled 11](https://user-images.githubusercontent.com/60471550/154383854-3995506e-4958-4728-a4b8-91c56ebbe771.png)
    

- 사물함이 없을 경우 사물함 관리 메뉴 진입 불가
    
    ![Untitled 12](https://user-images.githubusercontent.com/60471550/154383875-cce4d470-5604-495b-91a5-0e9ede8468a2.png)
    

**3. 종료**

![Untitled 13](https://user-images.githubusercontent.com/60471550/154383892-7450eda3-8047-4298-b1dd-a2bce3160ec6.png)

### [사물함 관리 메뉴]

**1. 내 사물함 보기**

- 내 사물함 정보 출력
    
    ![Untitled 14](https://user-images.githubusercontent.com/60471550/154383905-223f85e3-3317-4520-869a-06bb48c0545a.png)
    

**2. 물건 넣기**

- 남은 공간보다 많이 넣을 경우
    - 오류메세지 출력
        
        ![Untitled 15](https://user-images.githubusercontent.com/60471550/154383920-22fb96de-4e0a-47ae-bcab-f560b2a7eb00.png)
        
- 남은 공간과 같거나 적게 넣을 경우
    - 완료 메세지와 남은 공간 출력
        
        ![Untitled 16](https://user-images.githubusercontent.com/60471550/154383933-01f74060-8f47-4750-ac3f-d68689642398.png)
        
        ![Untitled 17](https://user-images.githubusercontent.com/60471550/154383942-7db2ef26-7df4-407d-802f-e95c82eea9a3.png)
        Server
        

**3. 물건 빼기**

- 현재 물건 개수보다 많이 뺄 경우
    - 오류 메세지와 현재 물건 개수 출력
    
    ![Untitled 18](https://user-images.githubusercontent.com/60471550/154383958-05e2a1d5-cc5f-450a-9cba-a5aa6d91baaf.png)
    
- 현재 물건 개수와 같거나 적게 뺄 경우
    - 완료 메세지와 남은 용량 출력
    
    ![Untitled 19](https://user-images.githubusercontent.com/60471550/154383980-3e1dfada-ee52-4d3a-a752-0c582576393e.png)
    
    ![Untitled 20](https://user-images.githubusercontent.com/60471550/154383988-7bbe6031-47b1-4bfc-9e17-89f3c5403d20.png)
    Server
    

**4. 사물함 반납**

- 해당 학번의 사물함 번호를 -1로 초기화하고 처음 메뉴로 돌아감
    
    ![Untitled 21](https://user-images.githubusercontent.com/60471550/154384004-1d8f5420-bf6d-4154-848a-0518e53fe384.png)
    
    ![Untitled 22](https://user-images.githubusercontent.com/60471550/154384014-aac3c16f-dc20-4a4d-a317-962297096953.png)
    Server
    

**5. 사물함 양도**

1. 양도할 수 있는(사용자 정보는 있으나 신청한 사물함이 없는) 학생 목록 출력
    
    ![Untitled 23](https://user-images.githubusercontent.com/60471550/154384031-9b359471-89e5-412f-9b51-b2a137819ebe.png)
    
2. 양도받을 학생의 학번을 입력하면 양도한 학생의 사물함 번호를 초기화하고 처음 메뉴로 돌아감
- **jayeon(201912352) 학생이 yeeun(201819186) 학생에게 양도**

    ![Untitled 24](https://user-images.githubusercontent.com/60471550/154384044-57b61b18-a11b-4ab2-b791-2ba5844121e4.png)
    Server

    - jayeon: 5번 사물함 점유
    - yeeun: 사용자 정보는 있으나 사용 중인 사물함 없음
- 양도 후 사물함 조회:
    - jayeon 사용자는 신청한 사물함이 없다는 메세지가 출력
    - yeeun 사용자는 5번 사물함 정보 출력

    ![Untitled 25](https://user-images.githubusercontent.com/60471550/154384065-90c45bd3-6f68-4c59-b8b7-808ab15e41a9.png)
    Client: 양도할 학생(jayeon)

    ![Untitled 26](https://user-images.githubusercontent.com/60471550/154384078-48b9cb0e-eb4d-405e-99bd-369410f7cca2.png)
    Client: 양도받은 학생(yeeun)

**6. 비밀번호 변경**

- 현재 비밀번호 입력 후 변경할 비밀번호 입력

  ![Untitled 27](https://user-images.githubusercontent.com/60471550/154384093-cf510582-edd4-47b4-b76a-ad42f629cfd7.png)

  ![Untitled 28](https://user-images.githubusercontent.com/60471550/154384100-862d5def-91dd-435b-9188-042d3538cbdc.png)
  Server

**7. 이전으로**

- 처음 메뉴로 돌아감
    
    ![Untitled 29](https://user-images.githubusercontent.com/60471550/154384114-49eb877c-7cb8-4483-b9d8-68ac8c926789.png)
