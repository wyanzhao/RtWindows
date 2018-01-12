/************************************************************************
* �ļ�˵�����ں������ó������Ե�Ժ���ʵ��
* ���ߣ����Ʒ�
* ʱ�䣺2011��12��13��
* �޸ģ��½���
* ʱ�䣺2012��6��5��
* �޸ģ����
* ʱ�䣺2013��6��1��
* �޸ģ�������
* ʱ�䣺2016��3��5��
*************************************************************************/

/***********************************ͷ�ļ�**************************************/
#include "../Include/Affinity.h"

/***********************************ϵͳ��������***********************************************/
#pragma LOCKEDDATA
ULONG s_systemAffinity = SYSTEMAFFINITY;

#pragma LOCKEDDATA
ULONG cAffinityCount = 0;

/***********************************��������***************************************************/
/*
* �����߳���Ե��
* PETHREAD �߳̽ṹ��
* BOOLEAN ����ֵ �Ƿ�ɹ�
*/
#pragma LOCKEDCODE
BOOLEAN RtSetThreadAffinity(PETHREAD eThread, ULONG cpuMask)	//64bit�¸ĳ�ULONG64 32bit��ΪULONG
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
* ���ý���ʵʱ��Ե��
* HANDLE ���̾��
* BOOLEAN ����ֵ �Ƿ�ɹ�
*/
#pragma LOCKEDCODE
BOOLEAN RtSetProcessAffinity(PEPROCESS eProcess, ULONG  cpuMask)
{
	PLIST_ENTRY threadListHead = NULL, threadList = NULL;
	PVOID pAffinityProcess = NULL;
	PETHREAD curThread = NULL;

	//���ý��̵���Ե��
	pAffinityProcess = (PUCHAR)eProcess + KPROCESS_AFFINITY_OFFSET + 0x0008;	//��Ҫ���� pAffinityProcess = (PUCHAR)curProcess + KPROCESS_AFFINITY_OFFSET + 0x0008
	if (pAffinityProcess == NULL)
		return FALSE;
	if (*(PULONG)pAffinityProcess == 0)
		return TRUE;
	*(PULONG)pAffinityProcess = cpuMask;

	threadListHead = GET_PTR(PLIST_ENTRY, eProcess, KPROCESS_THREADLISTHEAD);
	threadList = threadListHead->Flink;

	//�����߳�
	do
	{
		//�޸�AFFINITY��
		curThread = GET_PTR(PETHREAD, threadList, - KTHREAD_LISTENTRY_OFFSET);
		RtSetThreadAffinity(curThread, cpuMask);
		threadList = threadList->Flink;
	} while (threadList != threadListHead);

	return TRUE;
}

/*
* ����ϵͳ��Ե�ԣ����ı��idle system���̵����н�����Ե��
* cpuMask ��Ե������
* BOOLEAN ����ֵ �Ƿ�ɹ�
*/
#pragma LOCKEDCODE
BOOLEAN RtSetSystemAffinity(ULONG cpuMask)
{
	//˼·
	//PsGetCurrentProcess���ص�ǰ�߳�EPROCESS
	//EPROCESS�ṹ�µ�ActiveProcessLinks��LIST_ENTRY�ṹ
	//Ȼ��ͨ������ETHREAD �ҵ�KTHREAD�µ�Affinity��
	//����KTHREAD�ṹ�����У�0x218��ΪAffinity��
	PLIST_ENTRY curProcessList = NULL, processListHead = NULL;
	PEPROCESS curProcess = NULL;

	curProcess = PsGetCurrentProcess();
	processListHead = GET_PTR(PLIST_ENTRY, curProcess, EPROCESS_ACTIVELIST_OFFSET);
	curProcessList = processListHead;
	//�ų�idlesystem����,active�����������һ������idle����
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

