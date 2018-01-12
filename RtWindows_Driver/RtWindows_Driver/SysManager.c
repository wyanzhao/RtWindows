/***********************************ͷ�ļ�**************************************/
#include <NTDDK.h>

#include "../Include/SysManager.h"
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//										�ں�ϵͳ����
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#pragma LOCKEDDATA
char* s_pBase = NULL;

#pragma LOCKEDDATA
PMDL s_pMdl = NULL;

#pragma LOCKEDDATA
unsigned long cMemoryCount = 0;

/***********************************��������***************************************************/
//�ں�̬��˯�ߺ���������DPC�ж����ȼ��²�����˯��
#pragma LOCKEDCODE
void _CoreOSDelay()
{
	return;
}  

//�����߳�
#pragma LOCKEDCODE
DWORD _SuspendThread(HANDLE hHandle)
{	
	ULONG ret;
	NTSTATUS Status;

	Status =  PsSuspendThread((PETHREAD)hHandle, &ret);
	
	return Status;
}

//�ָ��߳�
#pragma LOCKEDCODE
DWORD _ResumeThread(HANDLE hHandle)
{
	ULONG ret;
	NTSTATUS Status;

	Status = PsResumeThread((PETHREAD)hHandle, &ret);

	return Status;
}

//����߳̾��
#pragma LOCKEDCODE
HANDLE _GetThreadHandle(DWORD dwThreadID)
{
	PETHREAD peThread;
	NTSTATUS Status;

	Status = PsLookupThreadByThreadId((HANDLE)dwThreadID, &peThread);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("Get Thread Handle failed\n"));
		return 0;
	}

	return (HANDLE)peThread;
}

//��ʼ��ϵͳ
#pragma LOCKEDCODE
BOOL  SystemInitialization()
{
	s_SystemFunc.CoreOSDelay = _CoreOSDelay;
	s_SystemFunc.GetThreadHandle = _GetThreadHandle;
	s_SystemFunc.SuspendThread = _SuspendThread;
	s_SystemFunc.ResumeThread = _ResumeThread;
	s_SystemFunc.ReSchedule = 0;
	
	//��ʼ��SystemManager
	s_pBase = (char*)ExAllocatePoolWithTag(NonPagedPool, TIME_MEM_SIZE, 'TIME');
	if (s_pBase == NULL)return FALSE;
	KdPrint(("s_pBase = %0x", s_pBase));

	RtlZeroMemory(s_pBase, TIME_MEM_SIZE);

	s_SystemManager.lpShareMemBase = s_pBase;
	s_AddressOffset = (ULONG)s_SystemManager.lpShareMemBase - 1;
	s_SystemManager.lpSystemTable = (_SystemTable *)s_SystemManager.lpShareMemBase;
	s_SystemManager.lpSystemTable->dwSystemTime = 0;
	KdPrint(("s_AddressOffset=%0x\n", s_AddressOffset));
	KdPrint(("lpSystemTable = %0x\n", s_SystemManager.lpSystemTable));

	InitSysytemManager(&s_SystemManager, TRUE);

	return TRUE;
}