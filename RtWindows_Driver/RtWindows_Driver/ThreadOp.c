/***********************************头文件**************************************/
#include "../Include/ThreadOp.h"
#include "../Include/InsertSSDT.h"

/***********************************系统变量定义***********************************************/
//线程操作方法入口地址
#pragma LOCKEDDATA
static THREAD_OPERATE_ADDR Thread_Operate_Address;

//方法服务ID
#pragma LOCKEDDATA
static THREAD_FUNCTION_SERVICE_ID Function_Service_Id;

//方法特征码
#pragma LOCKEDDATA
static THREAD_FUNCTION_KEYCODE Function_KeyCode;

#pragma LOCKEDDATA
__declspec(dllimport) SERVICE_DESCRIPTOR_TABLE KeServiceDescriptorTable;

/***********************************函数定义***************************************************/
//获得SSDT地址
#pragma LOCKEDCODE
static ULONG GetSSDTFunctionAddress(ULONG Index)
{
	ULONG stb = 0, ret = 0;
	PSERVICE_DESCRIPTOR_TABLE ssdt = (PSERVICE_DESCRIPTOR_TABLE) (&KeServiceDescriptorTable);
	stb = (ULONG)(ssdt->ServiceTableBase);
	ret = (ULONG)stb + (Index)* 4;
	return *(PULONG)ret;
}

//得到Nt函数地址
#pragma LOCKEDCODE
static ULONG GetNtFunctionAddress(ULONG Index)
{
	ULONG stb = 0, ret = 0;
	PSERVICE_DESCRIPTOR_TABLE ssdt = (PSERVICE_DESCRIPTOR_TABLE)(&KeServiceDescriptorTable);
	stb = (ULONG)(ssdt->ServiceTableBase);
	ret = (ULONG)stb + (Index) * 4;
	return *(PULONG)ret;
}

//获得挂起线程的Ps函数入口
#pragma LOCKEDCODE
static ULONG GetPsSuspendThreadAddr()
{
	PUCHAR StartSearchAddress = NULL;
	(ULONG)StartSearchAddress = (ULONG)GetSSDTFunctionAddress(Function_Service_Id.NtSuspendThread_ServiceId);
	PUCHAR EndSearchAddress = StartSearchAddress + 0x500;
	PUCHAR i = NULL;
	UCHAR b1 = 0, b2 = 0, b3 = 0;
	ULONG templong = 0;
	ULONG addr = 0;
	for (i = StartSearchAddress; i<EndSearchAddress; i++)
	{
		if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
		{
			b1 = *i;
			b2 = *(i + 1);
			b3 = *(i + 2);
			if (b1 == PsSuspendThread_KeyCode1_Win7 && b2 == PsSuspendThread_KeyCode2_Win7 && b3 == PsSuspendThread_KeyCode3_Win7) //e83200
			{
				memcpy(&templong, i + 1, 4);
				addr = templong + (ULONG)i + 5;
				return addr;
			}
		}
	}
	return 0;
}

//获得恢复线程的Ke函数入口
#pragma LOCKEDCODE
static ULONG GetKeResumeThreadAddr()
{
	PUCHAR StartSearchAddress = NULL;
	(ULONG)StartSearchAddress = (ULONG)GetSSDTFunctionAddress(Function_Service_Id.NtResumeThread_ServiceId);
	PUCHAR EndSearchAddress = StartSearchAddress + 0x500;
	PUCHAR i = NULL;
	UCHAR b1 = 0, b2 = 0, b3 = 0;
	ULONG templong = 0;
	ULONG addr = 0;
	for (i = StartSearchAddress; i<EndSearchAddress; i++)
	{
		if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
		{
			b1 = *i;
			b2 = *(i + 1);
			b3 = *(i + 2);
			if (b1 == KeResumeThread_KeyCode1_Win7 && b2 == KeResumeThread_KeyCode2_Win7 && b3 == KeResumeThread_KeyCode3_Win7) //e89316
			{
				memcpy(&templong, i + 4, 4);
				addr = (ULONG)templong + (ULONG)i + 5 + 3;
				return addr;
			}
		}
	}
	return 0;
}


/*
* 挂起线程操作的Ps函数
* PETHREAD Thread 当前线程
* PreviousSuspendCount OPTIONAL 当前已经挂起的线程个数
* NTSTATUS 是否成功
*/
#pragma LOCKEDCODE
NTSTATUS PsSuspendThread(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL)
{
	//函数功能：调用未导出函数PsSuspendThread挂起Thread线程
	ULONG OldCount;
	
	_asm
	{
			push 0
			push [Thread]
			call Thread_Operate_Address.PsSuspendThreadAddress
			mov OldCount, eax
	}
	if (PreviousSuspendCount) *PreviousSuspendCount = OldCount;

	return STATUS_SUCCESS;
}

/*
* 恢复线程操作的Ps函数
* PETHREAD Thread 当前线程
* PreviousSuspendCount OPTIONAL 当前已经恢复的线程个数
* NTSTATUS 是否成功
*/
#pragma LOCKEDCODE
NTSTATUS PsResumeThread(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL)
{
	ULONG OldCount;
	PKTHREAD pkThread = (PKTHREAD)Thread;

	_asm
	{
		push[pkThread]
		call Thread_Operate_Address.KeResumeThreadAddress
		mov OldCount, eax
	}
	if (PreviousSuspendCount) *PreviousSuspendCount = OldCount;

	return STATUS_SUCCESS;
}

//初始化方法特征值Function_KeyCode
#pragma LOCKEDCODE
static BOOLEAN InitializeThreadFunctionKeyCode()
{
	Function_KeyCode.PsSuspendThread_KeyCode.KeyCode1 = PsSuspendThread_KeyCode1_Win7;
	Function_KeyCode.PsSuspendThread_KeyCode.KeyCode2 = PsSuspendThread_KeyCode2_Win7;
	Function_KeyCode.PsSuspendThread_KeyCode.KeyCode3 = PsSuspendThread_KeyCode3_Win7;

	Function_KeyCode.KeResumeThread_KeyCode.KeyCode1 = KeResumeThread_KeyCode1_Win7;
	Function_KeyCode.KeResumeThread_KeyCode.KeyCode2 = KeResumeThread_KeyCode2_Win7;
	Function_KeyCode.KeResumeThread_KeyCode.KeyCode3 = KeResumeThread_KeyCode3_Win7;

	return TRUE;
}

//初始化方法服务ID
#pragma LOCKEDCODE
static BOOLEAN InitializeFunctionServiceId()
{
	Function_Service_Id.NtSuspendThread_ServiceId = NtSuspendThread_ServiceId_Win7;
	Function_Service_Id.NtResumeThread_ServiceId = NtResumeThread_ServiceId_Win7;

	return TRUE;
}

//初始化线程操作方法入口地址
#pragma LOCKEDCODE
BOOLEAN InitThreadOperatorAddress()
{
	InitializeFunctionServiceId();
	InitializeThreadFunctionKeyCode();
	//函数功能：初始化函数地址
	Thread_Operate_Address.NtSuspendThreadAddress =
		GetNtFunctionAddress(Function_Service_Id.NtSuspendThread_ServiceId);
	Thread_Operate_Address.NtResumeThreadAddress =
		GetNtFunctionAddress(Function_Service_Id.NtResumeThread_ServiceId);

	Thread_Operate_Address.KeResumeThreadAddress = GetKeResumeThreadAddr();
	Thread_Operate_Address.PsSuspendThreadAddress = GetPsSuspendThreadAddr();

	KdPrint(("NtSuspendThreadAddress 0x%0x\n", Thread_Operate_Address.NtSuspendThreadAddress));
	KdPrint(("NtResumeThreadAddress 0x%0x\n", Thread_Operate_Address.NtResumeThreadAddress));
	KdPrint(("PsSuspendThreadAddress 0x%0x\n", Thread_Operate_Address.PsSuspendThreadAddress));
	KdPrint(("KeResumeThreadAddress 0x%0x\n", Thread_Operate_Address.KeResumeThreadAddress));

	return TRUE;
}

/*
* 暂停线程操作函数
* PKTHREAD Thread 当前线程
* NTSTATUS 返回值 状态
*/
NTSTATUS SuspendThreadByHandle(IN HANDLE hThread)
{
	//函数功能：通过线程的句柄挂起线程
	PETHREAD pet;
	ULONG ret;
	NTSTATUS Status;

	Status = ObReferenceObjectByHandle(hThread, THREAD_SUSPEND_RESUME, *PsThreadType, KernelMode, (PVOID*)&pet, NULL);
	if (!NT_SUCCESS(Status)) return Status;
	Status = PsSuspendThread(pet, &ret);
	ObDereferenceObject(pet);

	return Status;
}


/*
* 恢复线程操作函数
* PKTHREAD Thread 当前线程
* NTSTATUS 返回值 状态
*/
#pragma LOCKEDCODE
NTSTATUS ResumeThreadByHandle(IN HANDLE hThread)
{
	
	//函数功能：通过线程的句柄唤醒线程
	PETHREAD pet;
	ULONG ret;
	NTSTATUS Status;
	
	Status = ObReferenceObjectByHandle(hThread, THREAD_SUSPEND_RESUME, *PsThreadType, KernelMode, (PVOID*)&pet, NULL);
	if (!NT_SUCCESS(Status)) return Status;
	Status = PsResumeThread(pet, &ret);
	ObDereferenceObject(pet);

	return Status;
}

