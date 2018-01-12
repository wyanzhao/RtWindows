/************************************************************************
* 文件说明：内核下设置程序的亲缘性函数实现
* 作者：蒋善锋
* 时间：2011年12月13日
* 修改：陈进朝
* 时间：2012年6月5日
* 修改：杨皓
* 时间：2013年6月1日
* 修改：王延钊
* 时间：2016年3月5日
*************************************************************************/

/***********************************头文件**************************************/
#include "../Include/Affinity.h"

/***********************************系统变量定义***********************************************/
#pragma LOCKEDDATA
ULONG s_systemAffinity = SYSTEMAFFINITY;

#pragma LOCKEDDATA
ULONG cAffinityCount = 0;

/***********************************函数定义***************************************************/
/*
* 设置线程亲缘性
* PETHREAD 线程结构体
* BOOLEAN 返回值 是否成功
*/
#pragma LOCKEDCODE
BOOLEAN RtSetThreadAffinity(PETHREAD eThread, ULONG cpuMask)	//64bit下改成ULONG64 32bit下为ULONG
{
	PUCHAR pAffinityThread = NULL;

	pAffinityThread = (PUCHAR)eThread + KTHREAD_AFFINITY_OFFSET;
	if (pAffinityThread != NULL)
		return FALSE;
	if (*pAffinityThread == 0x0)
		return TRUE;

	if (*(PULONG)pAffinityThread == s_systemAffinity)
	{
		*(PULONG)pAffinityThread = cpuMask;

		return TRUE;
	}

	return FALSE;
}


/*
* 设置进程实时亲缘性
* HANDLE 进程句柄
* BOOLEAN 返回值 是否成功
*/
#pragma LOCKEDCODE
BOOLEAN RtSetProcessAffinity(PEPROCESS eProcess, ULONG  cpuMask)
{
	PLIST_ENTRY threadListHead = NULL, threadList = NULL;
	PVOID pAffinityProcess = NULL;
	PETHREAD curThread = NULL;

	//设置进程的亲缘性
	pAffinityProcess = (PUCHAR)eProcess + KPROCESS_AFFINITY_OFFSET + 0x0008;	//需要更改 pAffinityProcess = (PUCHAR)curProcess + KPROCESS_AFFINITY_OFFSET + 0x0008
	if (pAffinityProcess == NULL)
		return FALSE;
	if (*(PULONG)pAffinityProcess == 0)
		return TRUE;
	*(PULONG)pAffinityProcess = cpuMask;

	threadListHead = GET_PTR(PLIST_ENTRY, eProcess, KPROCESS_THREADLISTHEAD);
	threadList = threadListHead->Flink;

	//遍历线程
	do
	{
		//修改AFFINITY域
		curThread = GET_PTR(PETHREAD, threadList, - KTHREAD_LISTENTRY_OFFSET);
		RtSetThreadAffinity(curThread, cpuMask);
		threadList = threadList->Flink;
	} while (threadList != threadListHead);

	return TRUE;
}

/*
* 设置系统亲缘性，即改变除idle system进程的所有进程亲缘性
* cpuMask 亲缘性掩码
* BOOLEAN 返回值 是否成功
*/
#pragma LOCKEDCODE
BOOLEAN RtSetSystemAffinity(ULONG cpuMask)
{
	//思路
	//PsGetCurrentProcess返回当前线程EPROCESS
	//EPROCESS结构下的ActiveProcessLinks是LIST_ENTRY结构
	//然后通过遍历ETHREAD 找到KTHREAD下的Affinity域
	//对于KTHREAD结构，其中，0x218处为Affinity域
	PLIST_ENTRY curProcessList = NULL, processListHead = NULL;
	PEPROCESS curProcess = NULL;

	curProcess = PsGetCurrentProcess();
	processListHead = GET_PTR(PLIST_ENTRY, curProcess, EPROCESS_ACTIVELIST_OFFSET);
	curProcessList = processListHead;
	//排除idlesystem进程,active进程链的最后一个总是idle进程
	processListHead = processListHead->Blink;
	do
	{
		curProcess = GET_PTR(PEPROCESS, curProcessList, - EPROCESS_ACTIVELIST_OFFSET);
		RtSetProcessAffinity(curProcess, cpuMask);

		curProcessList = curProcessList->Flink;

	} while (curProcessList != processListHead);

	s_systemAffinity = cpuMask;

	return TRUE;
}

