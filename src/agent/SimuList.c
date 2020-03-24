#include <stdlib.h>
#include <memory.h>

#include "SimuList.h"

int SimuList_Create(SimuList_t *pList)
{
	SimuListNode_t *pNode = NULL;

	if( pList == NULL){
		return -1;
	}
	pList->nCount = 0;
	
	pNode = (SimuListNode_t *)malloc(sizeof(SimuListNode_t));
	if( pNode == NULL){
		return -2;
	}
	pNode->nKey = 0;
	pNode->pData = NULL;
	pNode->pNext = NULL;
	pList->pHead = pNode;
	return 0;
}

SimuListNode_t *SimuList_Get(SimuList_t *pList, int key)
{
	SimuListNode_t *pNode = NULL;
	SimuListNode_t *pFind = NULL;

	if( pList == NULL || pList->nCount <= 0){
		return NULL;
	}

	pNode = pList->pHead->pNext;
	while( pNode != NULL)
	{
		if( pNode->nKey == key){
			pFind = pNode;
			break;
		}
		pNode = pNode->pNext;
	}
	return pFind;
}

int SimuList_Size(SimuList_t *pList)
{
	return pList->nCount;
}

int SimuList_Add(SimuList_t *pList, int key, void *pData, int nIgnoreKey)
{
	SimuListNode_t *pNode = NULL;
	SimuListNode_t *pNewNode = NULL;

	if( pList == NULL){
		return -1;
	}

	if( nIgnoreKey != 1){
		pNode = SimuList_Get(pList, key);
		if( pNode != NULL){
			return 1;
		}
	}

	pNewNode = (SimuListNode_t *)malloc(sizeof(SimuListNode_t));
	if( pNewNode == NULL){
		return -2;
	}

	pNewNode->nKey = key;
	pNewNode->pData = pData;
	pNewNode->pNext = NULL;

	pNode = pList->pHead;
	while( pNode->pNext != NULL){
		pNode = pNode->pNext;
	}
	pNode->pNext = pNewNode;
	
	pList->nCount++;
	return 0;
}

int SimuList_Del(SimuList_t *pList, int key, SimuList_FreeNodeDataCallback pFreeFun)
{
	SimuListNode_t *p1;
	SimuListNode_t *p2;

	if( pList == NULL || pList->nCount == 0){
		return -1;
	}

	p1 = pList->pHead->pNext;
	p2 = pList->pHead;
	while( p1 != NULL){
		if( p1->nKey == key){
			break;
		}
		p2 = p1;
		p1 = p1->pNext;
	}
	if( p1 != NULL){
		p2->pNext = p1->pNext;
		if( pFreeFun != NULL)
			pFreeFun(p1->pData);
		free(p1);
		pList->nCount--;
		return 0;
	}
	return -1;
}

int SimuList_Destory(SimuList_t *pList, SimuList_FreeNodeDataCallback pFreeFun)
{
	SimuListNode_t *pNode = NULL;
	SimuListNode_t *pNode2 = NULL;

	if( pList == NULL){
		return -1;
	}
	pNode = pList->pHead->pNext;
	free(pList->pHead);
	pList->pHead = NULL;
	pList->nCount = 0;
	if( pNode != NULL){
		pNode2 = pNode;
		pNode = pNode->pNext;
		if( pFreeFun != NULL)
			pFreeFun(pNode2->pData);
		free(pNode2);
	}
	return 0;
}

int SimuList_Print(SimuList_t *pList, SimuList_PrintNodeDataCallback pPrintFun, const char *szListName)
{
	SimuListNode_t *pNode = NULL;
	if( pList == NULL){
		return -1;
	}
	pNode = pList->pHead->pNext;

	printf("---------------------------%s =>nCount:%d------------------------- \n", szListName, pList->nCount);
	while( pNode != NULL){
		printf("Key=%d =>", pNode->nKey);
		if( pPrintFun != NULL)
			pPrintFun(pNode->pData);
		pNode = pNode->pNext;
	}
	printf("============================================================== \n");
	return 0;
}
