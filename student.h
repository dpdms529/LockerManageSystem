#include "locker.h"
#define START_ID 200000000

struct student{
	char name[20];
	int id;
 	struct locker myLocker;
};
