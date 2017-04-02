#include "stdafx.h"
#include "usersave.h"

#ifdef _DEBUG
extern FILE *pLogfile;
#endif // _DEBUG

QQGroup_t *group = NULL;
uint32_t groupcount = 1;

CRITICAL_SECTION csModifyList;

extern int ac;

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
	clearUser();
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

	EnterCriticalSection(&csModifyList);

	fwrite(&groupcount, sizeof(groupcount), 1, pf);//groupcount
	for (uint32_t i = 0; i < groupcount; i++) {//group lists
		fwrite(&group[i].groupid, sizeof(QQGroup::groupid), 1, pf);
		fwrite(&group[i].flag, sizeof(QQGroup::flag), 1, pf);
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
	LeaveCriticalSection(&csModifyList);
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
	void *pv;
		
		
	log("[Info] Loading user from: %s\n", filename);
	
	FILE *pf = fopen(filename, "rb");
	if (pf == NULL) {
		log("[Fail] Can't open savefile when loading: %s\n", filename);
		return 1;
	}
	log("[ OK ] Open file\n");

	EnterCriticalSection(&csModifyList);

	//free all data
	clearUser();

	fread(&groupcount, sizeof(groupcount), 1, pf);//groupcount
	log("[Info] %d group(s)\n", groupcount);

	pv = realloc(group, sizeof(QQGroup) * groupcount);//alloc mem
	if (pv == NULL) {
		log("[Fail] Can't alloc mem for grouplist!\n");
		fclose(pf);
		LeaveCriticalSection(&csModifyList);
		return 1;
	}
	group = (QQGroup_t *)pv;

	for (uint32_t i = 0; i < groupcount; i++) {//group lists
		fread(&group[i].groupid, sizeof(QQGroup::groupid), 1, pf);
		fread(&group[i].flag, sizeof(QQGroup::flag), 1, pf); 
		fread(&group[i].usercount, sizeof(QQGroup::usercount), 1, pf);
		uint16_t namelen;
		fread(&namelen, sizeof(namelen), 1, pf);
		if (namelen) {//name
			group[i].groupname = (char *)malloc(sizeof(*QQGroup::groupname) * (namelen + 1));//alloc mem
			if (group[i].groupname == NULL) {
				log("[Fail] Can't alloc mem for groupname!\n");
				fclose(pf);
				LeaveCriticalSection(&csModifyList);
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
			LeaveCriticalSection(&csModifyList);
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
				LeaveCriticalSection(&csModifyList);
				return 1;
			}
			fread(group[i].user[j].nickname, sizeof(*QQUser::nickname), namelen, pf);
			group[i].user[j].nickname[namelen] = '\0';
			//} else {//noname
			//	group[i].groupname = NULL;
			//}
		}
	}
	LeaveCriticalSection(&csModifyList);
	if (ferror(pf) || feof(pf)) {
		log("[Fail] Error #%d when loading user\n", ferror(pf));
		fclose(pf);
		return 1;
	}
	log("[ OK ] Load user finished\n");
	fclose(pf);
	return 0;
}

void clearUser(void) {
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

int getUserInfo(int64_t qqid, int64_t groupid, QQGroup_t **retgroup, QQUser_t **groupuser, QQUser_t **defaultuser, bool reenter = false) {
	//1.find user
	//int i;
	log("[Info] Get info for user=%I64d group=%I64d\n", qqid, groupid);
	uint32_t i;
	for (i = 0; i < group[0].usercount; i++) {
		if (group[0].user[i].qqid == qqid) {
			log("[Info] User found at #%u\n", i);
			*defaultuser = &group[0].user[i];
			break;
		}
		if (i == group[0].usercount - 1) {

		}
	}
	if (i == group[0].usercount) {
		log("[Info] User NOT found\n");
		//getuser
		if (reenter) {

			const char *data = CQ_getStrangerInfo(ac, qqid, 0);
			log("[Info] return data %s\n", data);
			size_t lendata = strlen(data);
			char *outdata = (char *)malloc(lendata);
			if (outdata == NULL) {
				log("[Fail] Alloc mem for base64dec\n");
				return 1;
			}
			base64dec(data, outdata);

			void *p = realloc(group[0].user, sizeof(QQGroup) * (group[0].usercount + 1));
			if (p == NULL) {
				log("[Fail] realloc user list\n");
				return 1;
			}
			group[0].user = (QQUser_t *)p;
			group[0].user[group[0].usercount].qqid = qqid;
			size_t namelen = (outdata[8] << 8) | outdata[9];
			log("[Info] Namelen=%d\n", namelen);
			group[0].user[group[0].usercount].nickname = (char *)malloc(sizeof(char) * (namelen + 1));
			if (group[0].user[group[0].usercount].nickname == NULL) {
				log("[Fail] Can't alloc mem for username!(user) %u\n", sizeof(char) * (namelen + 1));
				return 1;
			}
			strncpy(group[0].user[group[0].usercount].nickname, outdata + 10, namelen);
			group[0].user[group[0].usercount].nickname[namelen] = '\0';
			log("[Info] NickName=%s\n", group[0].user[group[0].usercount].nickname);
			group[0].user[group[0].usercount].info = ((outdata[10 + namelen + 3] << 8) & 0x7F) | outdata[10 + namelen + 7];

			*defaultuser = &group[0].user[group[0].usercount];
			group[0].usercount++;
			return 0;
		} else {
			EnterCriticalSection(&csModifyList);
			int ret = getUserInfo(qqid, groupid, retgroup, groupuser, defaultuser, true);
			LeaveCriticalSection(&csModifyList);
			if (ret) return ret;
		}
		

	}
	//2.find group
	if (groupid == 0) {
		log("[Info] Skip group\n");
		*retgroup = group;
		*groupuser = *defaultuser;
		return 0;
	}

	//find group
	uint32_t j;
	for (j = 0; j < groupcount; j++) {
		if (group[j].groupid == groupid) {
			log("[Info] Group found at #%u\n", i);
			break;
		}
	}
	if (j == groupcount) {
		log("[Info] Group NOT found\n");

		if (reenter) {
			void *p = realloc(group, sizeof(QQGroup) * (groupcount + 1));
			if (p == NULL) {
				log("[Fail] realloc group list\n");
				return 1;
			}
			group = (QQGroup_t *)p;
			group[groupcount].groupid = groupid;
			group[groupcount].flag = 0;
			group[groupcount].groupname = NULL;
			group[groupcount].user = NULL;
			group[groupcount].usercount = 0;
			groupcount++;
			return 0;
		} else {
			EnterCriticalSection(&csModifyList);
			int ret = getUserInfo(qqid, groupid, retgroup, groupuser, defaultuser, true);
			LeaveCriticalSection(&csModifyList);
			if (ret) return ret;
		}


	}
	*retgroup = &group[j];
	for (i = 0; i < group[j].usercount; i++) {
		if (group[j].user[i].qqid == qqid) {
			log("[Info] User(group) found at #%u\n", i);
			*groupuser = &group[j].user[i];
			break;
		}
	}
	if (i == group[j].usercount) {
		log("[Info] User(group) NOT found\n");

		if (reenter) {
			//getuser
			const char *data = CQ_getGroupMemberInfoV2(ac, groupid, qqid, 0);
			log("[Info] return data %s\n", data);
			size_t lendata = strlen(data);
			char *outdata = (char *)malloc(lendata);
			if (outdata == NULL) {
				log("[Fail] Alloc mem for base64dec (group)\n");
				return 1;
			}
			base64dec(data, outdata);

			void *p = realloc(group[j].user, sizeof(QQGroup) * (group[j].usercount + 1));
			if (p == NULL) {
				log("[Fail] realloc user list (group)\n");
				return 1;
			}
			group[j].user = (QQUser_t *)p;
			group[j].user[group[j].usercount].qqid = qqid;
			size_t name1len = (outdata[16] << 8) | outdata[17];
			size_t name2len = (outdata[18 + name1len] << 8) | outdata[18 + name1len + 1];
			log("[Info] Namelen=%d,%d\n", name1len, name2len);
			group[j].user[group[j].usercount].nickname = (char *)malloc(sizeof(char) * (name2len + 1));
			if (group[j].user[group[j].usercount].nickname == NULL) {
				log("[Fail] Can't alloc mem for username!(group) %u\n", sizeof(char) * (name2len + 1));
				return 1;
			}
			strncpy(group[j].user[group[j].usercount].nickname, outdata + 18 + name1len + 2, name2len);
			group[j].user[group[j].usercount].nickname[name2len] = '\0';
			log("[Info] NickName=%s\n", group[j].user[group[j].usercount].nickname);
			group[j].user[group[j].usercount].info = ((outdata[18 + name1len + 2 + name2len + 3] << 8) & 0x7F) | outdata[18 + name1len + 2 + name2len + 7];

			*groupuser = &group[j].user[group[j].usercount];
			group[j].usercount++;
			return 0;
		} else {
			EnterCriticalSection(&csModifyList);
			int ret = getUserInfo(qqid, groupid, retgroup, groupuser, defaultuser, true);
			LeaveCriticalSection(&csModifyList);
			if (ret) return ret;
		}
	}
	return 0;
}