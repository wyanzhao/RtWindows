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
	
	//��ʼ��TimeManager
	s_pBase = (char*)ExAllocatePoolWithTag(NonPagedPool, TIME_MEM_SIZE, 'TIME');
	if (s_pBase == NULL)return FALSE;
	RtlZeroMemory(s_pBase, TIME_MEM_SIZE);

	s_TimeManager.lpShareMemBase = s_pBase;
	s_AddressOffset = (ULONG)s_TimeManager.lpShareMemBase - 1;
	s_TimeManager.lpn64SystemTime = (SYSTEM_TIME_TYPE *)s_TimeManager.lpShareMemBase;
	*s_TimeManager.lpn64SystemTime = 0;

	return TRUE;
}