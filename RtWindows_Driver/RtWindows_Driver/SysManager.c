/***********************************头文件**************************************/
#include <NTDDK.h>

#include "../Include/SysManager.h"
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//										内核系统管理
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#pragma LOCKEDDATA
char* s_pBase = NULL;

#pragma LOCKEDDATA
PMDL s_pMdl = NULL;

#pragma LOCKEDDATA
unsigned long cMemoryCount = 0;

/***********************************函数定义***************************************************/
//内核态的睡眠函数，但是DPC中断优先级下不允许睡眠
#pragma LOCKEDCODE
void _CoreOSDelay()
{
	return;
}  

//挂起线程
#pragma LOCKEDCODE
DWORD _SuspendThread(HANDLE hHandle)
{	
	ULONG ret;
	NTSTATUS Status;

	Status =  PsSuspendThread((PETHREAD)hHandle, &ret);
	
	return Status;
}

//恢复线程
#pragma LOCKEDCODE
DWORD _ResumeThread(HANDLE hHandle)
{
	ULONG ret;
	NTSTATUS Status;

	Status = PsResumeThread((PETHREAD)hHandle, &ret);

	return Status;
}

//获得线程句柄
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

//初始化系统
#pragma LOCKEDCODE
BOOL  SystemInitialization()
{
	s_SystemFunc.CoreOSDelay = _CoreOSDelay;
	s_SystemFunc.GetThreadHandle = _GetThreadHandle;
	s_SystemFunc.SuspendThread = _SuspendThread;
	s_SystemFunc.ResumeThread = _ResumeThread;
	s_SystemFunc.ReSchedule = 0;
	
	//初始化SystemManager
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