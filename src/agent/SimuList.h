
#ifndef __SIMU_LIST_H__
#define __SIMU_LIST_H__
#include <stdio.h>

typedef void(*SimuList_FreeNodeDataCallback)(void *pNodeData);
typedef void(*SimuList_PrintNodeDataCallback)(void *pNodeData);

struct tagSimuListNode{
	int  nKey;
	void *pData;
	struct tagSimuListNode *pNext;
};
typedef struct tagSimuListNode SimuListNode_t;


typedef struct tagSimuList{
	int nCount;
	SimuListNode_t *pHead;
}SimuList_t;


extern int SimuList_Create(SimuList_t *pList);
extern SimuListNode_t *SimuList_Get(SimuList_t *pList, int key);
extern int SimuList_Size(SimuList_t *pList);
extern int SimuList_Add(SimuList_t *pList, int key, void *pData, int nIgnoreKey);
extern int SimuList_Del(SimuList_t *pList, int key, SimuList_FreeNodeDataCallback pFreeFun);
extern int SimuList_Destory(SimuList_t *pList, SimuList_FreeNodeDataCallback pFreeFun);
extern int SimuList_Print(SimuList_t *pList, SimuList_PrintNodeDataCallback pPrintFun, const char *szListName);

#endif
