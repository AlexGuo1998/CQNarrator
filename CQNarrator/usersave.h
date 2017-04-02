#pragma once

typedef struct QQUser {
	uint64_t qqid;		//QQ number
	uint16_t info;		//a bbbbbbb cccccccc
						//a=name definded? b=sex(0M/1F/127Unknown) c=age
	char *nickname;		//User's nickname
} QQUser_t;

typedef struct QQGroup {
	int64_t groupid;		//Group number
	int8_t flag;			//flag 0-mute 
	char *groupname;		//User definded name
	uint32_t usercount;		//User count
	QQUser_t *user;			//User list
} QQGroup_t;

bool initUser(void);
void destroyUser(void);
int saveUser(const char *filename);
int loadUser(const char *filename);
void clearUser(void);
//int getUserInfo(int64_t qqid, int64_t groupid, QQUser_t **groupuser, QQUser_t **defaultuser);
int getUserInfo(int64_t qqid, int64_t groupid, QQGroup_t **retgroup, QQUser_t **groupuser, QQUser_t **defaultuser, bool reenter = false);
//int getUserInfo(int64_t QQID, int64_t GroupID, QQUser_t *groupuser, QQUser_t *defaultuser);