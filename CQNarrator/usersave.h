#pragma once

typedef struct QQUser {
	uint64_t qqid;		//QQ number
	uint32_t info;		//a bbbbbbb cccccccc
						//a=name definded? b=sex(0M/1F/255Unknown) c=age
	char *nickname;		//User's nickname
} QQUser_t;

typedef struct QQGroup {
	int64_t groupid;		//Group number
	char *groupname;		//User definded name
	uint32_t usercount;		//User count
	QQUser_t *user;	//User list
} QQGroup_t;

bool initUser(void);
void destroyUser(void);
int saveUser(const char *filename);
int loadUser(const char *filename);
void freeUserData(void);
int getUserInfo(int64_t QQID, int64_t GroupID, QQUser_t *groupuser, QQUser_t *defaultuser);