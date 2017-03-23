#include "stdafx.h"
#include "speech.h"

#include <sapi.h>

#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"sapi.lib")


ISpeechVoice *pSpVoice = NULL;

typedef struct speechQueue {
	struct speechQueue *next;
	char *text;
	int instruction; //-1:kill
} speechQueue_t;

speechQueue_t queueHead = {NULL, NULL, 0};

HANDLE hSemaphore = NULL;
CRITICAL_SECTION csLastNodeChange;

HANDLE hThdReader = NULL;

size_t queueCount = 0;

#ifdef _DEBUG
extern FILE *pLogfile;
#endif // _DEBUG

extern bool enabled;

DWORD WINAPI thdReader(PVOID pParam) {
	if (FAILED(::CoInitialize(NULL))) return 1;
	if (FAILED(CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpeechVoice, (void **)&pSpVoice))) {
		::CoUninitialize();
		return 1;
	}


	for (;;) {
		WaitForSingleObject(hSemaphore, INFINITE);//todo raise err
		log("Ready to read\n");
		//unlink "next" node
		speechQueue_t *nownode = queueHead.next;
		if (nownode->next == NULL) {
			//last node - protect
			log("Last in queue\n");
			EnterCriticalSection(&csLastNodeChange);
			queueHead.next = nownode->next;//maybe not NULL now
			LeaveCriticalSection(&csLastNodeChange);
		} else {
			log("NOT Last in queue\n");
			queueHead.next = nownode->next;
		}
		//read
		loga("Read: %s\n", nownode->text);
		DWORD dwCount = MultiByteToWideChar(CP_ACP, 0, nownode->text, -1, NULL, 0);
		wchar_t *wc = (wchar_t *)malloc(sizeof(wchar_t) * dwCount);//todo realloc
		MultiByteToWideChar(CP_ACP, 0, nownode->text, -1, wc, dwCount);
		long stmno;
		pSpVoice->Speak(wc, (SpeechVoiceSpeakFlags)(SVSFIsNotXML), &stmno);
		log("Read finished\n");
		queueCount--;
		log("count--\n");
		//free data
		free(wc);
		log("freewc\n");
		free(nownode->text);
		log("Free text OK\n");

		//process
		switch (nownode->instruction) {
		case -1: //Kill
			log("Terminating thread\n");
			pSpVoice->Release();
			pSpVoice = NULL;
			::CoUninitialize();
			queueCount = 0;
			//todo free all nodes
			free(nownode);
			enabled = false;
			log("Terminate OK\n");
			return 0;
			break;
		default:
			break;
		}
		
		free(nownode);
		log("Free node OK\n");

	}
	
	return 0;
}

bool initSpVoice(void) {
	hSemaphore = CreateSemaphoreA(NULL, 0, LONG_MAX, NULL);
	InitializeCriticalSection(&csLastNodeChange);
	queueCount = 0;
	read("QQ讲述人已启动", 0);
	hThdReader = CreateThread(NULL, 0, thdReader, NULL, 0, NULL);
	//pSpVoice->Speak(L"QQ讲述人已启动", (SpeechVoiceSpeakFlags)(SVSFlagsAsync | SVSFIsNotXML), NULL);//TODO list
	return true;
}

long read(const char *str, int instruction) {
	/*
	DWORD dwCount = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	wchar_t *wc = (wchar_t *)malloc(dwCount);
	MultiByteToWideChar(CP_ACP, 0, str, -1, wc, dwCount);
	pSpVoice->Speak(wc, (SpeechVoiceSpeakFlags)(SVSFIsNotXML), NULL);
	free(wc);*/

	//todo check valid
	if (!enabled) return 1;

	speechQueue_t *newnode, *node = &queueHead;
	//alloc mem
	log("alloc mem for queue\n");
	if (!((newnode = (speechQueue_t *)malloc(sizeof(struct speechQueue))) && (newnode->text = (char *)malloc(strlen(str) + 1)))) 0;//todo raise err

	strcpy(newnode->text, str);
	newnode->next = NULL;
	newnode->instruction = instruction;

	//find tail
	log("find tail\n");
	EnterCriticalSection(&csLastNodeChange);
	while (node->next) {
		node = node->next;
	}
	//now is last node
	node->next = newnode;
	LeaveCriticalSection(&csLastNodeChange);
	
	log("Add one\n");
	queueCount++;
	ReleaseSemaphore(hSemaphore, 1, NULL);
	log("Add OK\n");
	return 0;
}

void destorySpVoice(bool wait) {
	if (pSpVoice) {
		read("QQ讲述人已停止", -1);
	}
	//::CoUninitialize();

	if (wait) {
		WaitForSingleObject(hThdReader, INFINITE);
	}
	CloseHandle(hThdReader);
	hThdReader = NULL;

	return;
}
