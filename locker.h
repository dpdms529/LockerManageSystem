struct locker{
	int id;
	char* pwd;
	int isBig;
	int cap;
	int wrongCnt = 0;
	int islock = 0;
	int lockTime;
}
