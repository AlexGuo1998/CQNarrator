#include "stdafx.h"
#include "usersave.h"

#ifdef _DEBUG
extern FILE *pLogfile;
#endif // _DEBUG

QQGroup_t *group = NULL;
uint32_t groupcount = 0;

CRITICAL_SECTION csModifyList;

//savefile:groupcount(32)(1=maingroup)
//grouplist[count]: GroupID(64) usercount(32) NameLen(16) Name
//userlist[count]: QQID(64) Info(32) NicknameLen(16) NickName 

bool initUser(void) {
	InitializeCriticalSection(&csModifyList);
	group = (QQGroup_t *)malloc(sizeof(QQGroup));
	group->groupid = 0;
	group->groupname = NULL;
	group->usercount = 0;
	group->user = NULL;
	return true;
}

int saveUser(const char *filename) {
	FILE *pf = fopen(filename, "wb");
	if (pf == NULL) {
		loga("Can't open savefile when saving:\n%s\n", filename);
		return 1;
	}
	fwrite(&groupcount, sizeof(groupcount), 1, pf);//groupcount
	for (uint32_t i = 0; i < groupcount; i++) {//group lists
		fwrite(&group[i].groupid, sizeof(QQGroup::groupid), 1, pf);
		fwrite(&group[i].usercount, sizeof(QQGroup::usercount), 1, pf);
		uint16_t namelen;
		if (group[i].groupname == NULL) {//noname
			namelen = 0;
			fwrite(&namelen, sizeof(namelen), 1, pf);
		} else {//have name
			namelen = (uint16_t)strlen(group[i].groupname);
			fwrite(&namelen, sizeof(namelen), 1, pf);
			fwrite(group[i].groupname, sizeof(*QQGroup::groupname), namelen, pf);
		}
	}
	for (uint32_t i = 0; i < groupcount; i++) {//user lists
		for (uint32_t j = 0; j < group[i].usercount; j++) {
			fwrite(&group[i].user[j].qqid, sizeof(QQUser::qqid), 1, pf);
			fwrite(&group[i].user[j].info, sizeof(QQUser::info), 1, pf);
			uint16_t namelen = (uint16_t)strlen(group[i].user[j].nickname);//indicate: must have name
			fwrite(&namelen, sizeof(namelen), 1, pf);
			fwrite(group[i].user[j].nickname, sizeof(*QQUser::nickname), namelen, pf);
		}
	}
	fclose(pf);
	return 0;
}

int loadUser(const char *filename) {
	//free all data
	freeUserData();

	void *pv;

	FILE *pf = fopen(filename, "rb");
	if (pf == NULL) {
		loga("Can't open savefile when loading:\n%s\n", filename);
		return 1;
	}

	fread(&groupcount, sizeof(groupcount), 1, pf);//groupcount
	pv = realloc(group, sizeof(QQGroup) * groupcount);//alloc mem
	if (pv == NULL) {
		log("Can't alloc mem for grouplist!\n");
		return 1;
	}
	group = (QQGroup_t *)pv;

	for (uint32_t i = 0; i < groupcount; i++) {//group lists
		fread(&group[i].groupid, sizeof(QQGroup::groupid), 1, pf);
		fread(&group[i].usercount, sizeof(QQGroup::usercount), 1, pf);
		uint16_t namelen;
		fread(&namelen, sizeof(namelen), 1, pf);
		if (namelen) {//name
			group[i].groupname = (char *)malloc(sizeof(*QQGroup::groupname) * (namelen + 1));//alloc mem
			if (group[i].groupname == NULL) {
				log("Can't alloc mem for groupname!\n");
				return 1;
			}
			fread(group[i].groupname, sizeof(*QQGroup::groupname), namelen, pf);
			group[i].groupname[namelen] = '\0';
		} else {//noname
			group[i].groupname = NULL;
		}
	}
	for (uint32_t i = 0; i < groupcount; i++) {//user lists
		group[i].user = (QQUser_t *)malloc(sizeof(QQUser) * group[i].usercount);
		if (group[i].user == NULL) {
			log("Can't alloc mem for userlist!\n");
			return 1;
		}

		for (uint32_t j = 0; j < group[i].usercount; j++) {
			fread(&group[i].user[j].qqid, sizeof(QQUser::qqid), 1, pf);
			fread(&group[i].user[j].info, sizeof(QQUser::info), 1, pf);

			uint16_t namelen;
			fread(&namelen, sizeof(namelen), 1, pf);
			//if (namelen) {//name
			group[i].user[j].nickname = (char *)malloc(sizeof(*QQUser::nickname) * (namelen + 1));//alloc mem
			if (group[i].user[j].nickname == NULL) {
				log("Can't alloc mem for username!\n");
				return 1;
			}
			fread(group[i].user[j].nickname, sizeof(*QQUser::nickname), namelen, pf);
			group[i].user[j].nickname[namelen] = '\0';
			//} else {//noname
			//	group[i].groupname = NULL;
			//}
		}
	}
	fclose(pf);
	return 0;
}

void freeUserData(void) {
	for (uint32_t i = 0; i < groupcount; i++) {
		for (uint32_t j = 0; j < group[i].usercount; j++) {
			free(group[i].user[j].nickname);
		}
		free(group[i].user);
		free(group[i].groupname);
	}
	free(group);

	group = (QQGroup_t *)malloc(sizeof(QQGroup));
	group->groupid = 0;
	group->groupname = NULL;
	group->usercount = 0;
	group->user = NULL;
}