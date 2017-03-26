#include "stdafx.h"
#include "usersave.h"

#ifdef _DEBUG
extern FILE *pLogfile;
#endif // _DEBUG

QQGroup_t *group = NULL;
uint32_t groupcount = 0;

CRITICAL_SECTION csModifyList;

//savefile:groupcount(32)(1=maingroup)
//grouplist[count]: groupid(64) usercount(32) NameLen(16) Name
//userlist[count]: qqid(64) Info(32) NicknameLen(16) NickName 

bool initUser(void) {
	log("[Info] initializing userlist\n");
	InitializeCriticalSection(&csModifyList);
	group = (QQGroup_t *)malloc(sizeof(QQGroup));
	group->groupid = 0;
	group->groupname = NULL;
	group->usercount = 0;
	group->user = NULL;
	log("[ OK ] initialized userlist\n");
	return true;
}

void destroyUser(void) {
	log("[Info] Destroying userlist\n");
	DeleteCriticalSection(&csModifyList);
	freeUserData();
	log("[ OK ] Destroied userlist\n");
	return;
}

int saveUser(const char *filename) {
	log("[Info] Saving user to: %s\n", filename);
	FILE *pf = fopen(filename, "wb");
	if (pf == NULL) {
		log("[Fail] Can't open savefile when saving:\n%s\n", filename);
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
	if (ferror(pf) || feof(pf)) {
		log("[Fail] Error #%d when saving user\n", ferror(pf));
		fclose(pf);
		return 1;
	}
	log("[ OK ] Save user finished\n");
	fclose(pf);
	return 0;
}

int loadUser(const char *filename) {
	log("[Info] Loading user from: %s\n", filename);
	
	//free all data
	freeUserData();

	void *pv;

	FILE *pf = fopen(filename, "rb");
	if (pf == NULL) {
		log("[Fail] Can't open savefile when loading: %s\n", filename);
		return 1;
	}
	log("[ OK ] Open file\n");

	fread(&groupcount, sizeof(groupcount), 1, pf);//groupcount
	log("[Info] %d group(s)\n", groupcount);

	pv = realloc(group, sizeof(QQGroup) * groupcount);//alloc mem
	if (pv == NULL) {
		log("[Fail] Can't alloc mem for grouplist!\n");
		fclose(pf);
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
				log("[Fail] Can't alloc mem for groupname!\n");
				fclose(pf);
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
			log("[Fail] Can't alloc mem for userlist!\n");
			fclose(pf);
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
				log("[Fail] Can't alloc mem for username!\n");
				fclose(pf);
				return 1;
			}
			fread(group[i].user[j].nickname, sizeof(*QQUser::nickname), namelen, pf);
			group[i].user[j].nickname[namelen] = '\0';
			//} else {//noname
			//	group[i].groupname = NULL;
			//}
		}
	}
	if (ferror(pf) || feof(pf)) {
		log("[Fail] Error #%d when loading user\n", ferror(pf));
		fclose(pf);
		return 1;
	}
	log("[ OK ] Load user finished\n");
	fclose(pf);
	return 0;
}

void freeUserData(void) {
	log("[Info] Free user info data\n");
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
	log("[ OK ] Free user info data\n");
}

int getUserInfo(int64_t qqid, int64_t groupid, QQUser_t *groupuser, QQUser_t *defaultuser) {
	//1.find user
	//int i;
	uint32_t i;
	for (i = 0; i < group[0].usercount; i++) {
		if (group[0].user[i].qqid == qqid) {
			defaultuser = &group[0].user[i];
			break;
		}
	}
	if (i == group[0].usercount) {
		//TODO getuser
		//defaultuser = xxx;
	}
	//2.find group
	if (groupid == 0) {
		groupuser = defaultuser;
		return 0;
	}

	//find group
	uint32_t j;
	for (j = 0; j < groupcount; j++) {
		if (group[j].groupid == groupid) {
			break;
		}
	}
	if (j == groupcount) {
		//TODO enter...
		void *p = realloc(group, sizeof(QQGroup) * (groupcount + 1));
		if (p == NULL) {
			log("[Fail] realloc group list\n");
			return 1;
		}
		group = (QQGroup_t *)p;
		group[groupcount].groupid = groupid;
		group[groupcount].groupname = NULL;
		group[groupcount].user = NULL;
		group[groupcount].usercount = 0;
		groupcount++;
	}

	for (i = 0; i < group[j].usercount; i++) {
		if (group[j].user[i].qqid == qqid) {
			groupuser = &group[j].user[i];
			break;
		}
	}
	if (i == group[j].usercount) {
		//TODO getuser
		//defaultuser = xxx;
	}
	return 0;
}