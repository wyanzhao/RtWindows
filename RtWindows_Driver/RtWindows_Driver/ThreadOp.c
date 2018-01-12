/***********************************ͷ�ļ�**************************************/
#include "../Include/ThreadOp.h"
#include "../Include/InsertSSDT.h"

/***********************************ϵͳ��������***********************************************/
//�̲߳���������ڵ�ַ
#pragma LOCKEDDATA
static THREAD_OPERATE_ADDR Thread_Operate_Address;

//��������ID
#pragma LOCKEDDATA
static THREAD_FUNCTION_SERVICE_ID Function_Service_Id;

//����������
#pragma LOCKEDDATA
static THREAD_FUNCTION_KEYCODE Function_KeyCode;

#pragma LOCKEDDATA
__declspec(dllimport) SERVICE_DESCRIPTOR_TABLE KeServiceDescriptorTable;

/***********************************��������***************************************************/
//���SSDT��ַ
#pragma LOCKEDCODE
static ULONG GetSSDTFunctionAddress(ULONG Index)
{
	ULONG stb = 0, ret = 0;
	PSERVICE_DESCRIPTOR_TABLE ssdt = (PSERVICE_DESCRIPTOR_TABLE) (&KeServiceDescriptorTable);
	stb = (ULONG)(ssdt->ServiceTableBase);
	ret = (ULONG)stb + (Index)* 4;
	return *(PULONG)ret;
}

//�õ�Nt������ַ
#pragma LOCKEDCODE
static ULONG GetNtFunctionAddress(ULONG Index)
{
	ULONG stb = 0, ret = 0;
	PSERVICE_DESCRIPTOR_TABLE ssdt = (PSERVICE_DESCRIPTOR_TABLE)(&KeServiceDescriptorTable);
	stb = (ULONG)(ssdt->ServiceTableBase);
	ret = (ULONG)stb + (Index) * 4;
	return *(PULONG)ret;
}

//��ù����̵߳�Ps�������
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

//��ûָ��̵߳�Ke�������
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
* �����̲߳�����Ps����
* PETHREAD Thread ��ǰ�߳�
* PreviousSuspendCount OPTIONAL ��ǰ�Ѿ�������̸߳���
* NTSTATUS �Ƿ�ɹ�
*/
#pragma LOCKEDCODE
NTSTATUS PsSuspendThread(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL)
{
	//�������ܣ�����δ��������PsSuspendThread����Thread�߳�
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
* �ָ��̲߳�����Ps����
* PETHREAD Thread ��ǰ�߳�
* PreviousSuspendCount OPTIONAL ��ǰ�Ѿ��ָ����̸߳���
* NTSTATUS �Ƿ�ɹ�
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

//��ʼ����������ֵFunction_KeyCode
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

//��ʼ����������ID
#pragma LOCKEDCODE
static BOOLEAN InitializeFunctionServiceId()
{
	Function_Service_Id.NtSuspendThread_ServiceId = NtSuspendThread_ServiceId_Win7;
	Function_Service_Id.NtResumeThread_ServiceId = NtResumeThread_ServiceId_Win7;

	return TRUE;
}

//��ʼ���̲߳���������ڵ�ַ
#pragma LOCKEDCODE
BOOLEAN InitThreadOperatorAddress()
{
	InitializeFunctionServiceId();
	InitializeThreadFunctionKeyCode();
	//�������ܣ���ʼ��������ַ
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
* ��ͣ�̲߳�������
* PKTHREAD Thread ��ǰ�߳�
* NTSTATUS ����ֵ ״̬
*/
NTSTATUS SuspendThreadByHandle(IN HANDLE hThread)
{
	//�������ܣ�ͨ���̵߳ľ�������߳�
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
* �ָ��̲߳�������
* PKTHREAD Thread ��ǰ�߳�
* NTSTATUS ����ֵ ״̬
*/
#pragma LOCKEDCODE
NTSTATUS ResumeThreadByHandle(IN HANDLE hThread)
{
	
	//�������ܣ�ͨ���̵߳ľ�������߳�
	PETHREAD pet;
	ULONG ret;
	NTSTATUS Status;
	
	Status = ObReferenceObjectByHandle(hThread, THREAD_SUSPEND_RESUME, *PsThreadType, KernelMode, (PVOID*)&pet, NULL);
	if (!NT_SUCCESS(Status)) return Status;
	Status = PsResumeThread(pet, &ret);
	ObDereferenceObject(pet);

	return Status;
}

