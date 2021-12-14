struct locker{
	int id;		//사물함 번호
	char pwd[100];	//비밀번호
	int isBig;	//대형 사물함인지 여부
	int cap;	//남은 공간(넣을 수 있는 물건 개수)
	int wrongCnt;	//비밀번호 틀린 횟수
};
