/***********************************ͷ�ļ�**************************************/
#include <NTDDK.h>

#include "../Include/InsertSSDT.h"
#include "../Include/ThreadOp.h"
#include "../Include/Scheduler.h"
#include "../Include/SysManager.h"

/***********************************ϵͳ��������***********************************************/
#pragma LOCKEDDATA
static ULONG *NewServiceTableBase; /* Pointer to new SSDT */
#pragma LOCKEDDATA
static ULONG *NewParamTableBase; /* Pointer to new SSPT */
#pragma LOCKEDDATA
static ULONG NewNumberOfServices; /* New number of services */
#pragma LOCKEDDATA
static ULONG StartingServiceId;
#pragma LOCKEDDATA
static ULONG *OldServiceTableBase; /* Pointer to new SSDT */
#pragma LOCKEDDATA
static ULONG OldParamTableBase; /* Pointer to new SSPT */
#pragma LOCKEDDATA
static ULONG OldNumberOfServices; /* New number of services */
#pragma LOCKEDDATA
static ULONG StartingServiceId;
#pragma LOCKEDDATA
ULONG serviceId;

__declspec(dllimport) SERVICE_DESCRIPTOR_TABLE KeServiceDescriptorTable;
/************************************************************************
*                                                                      *
*                             ���ӵķ�����                            *
*                                                                      *
************************************************************************/
void TestResumeThread(ULONG threadid);
void TestSuspendThread(ULONG threadid);
void Reshedule();
void testParam(int a);

/* ��Ҫ���ӵ�ϵͳ�����ַ�� */
unsigned int ServiceTableBase[] = { (unsigned int)TestResumeThread,
(unsigned int)TestSuspendThread,
(unsigned int)Reshedule,
(unsigned int)testParam
};

/*��Ҫ���ӵ�ϵͳ���������*/
unsigned char ParamTableBase[] = { 4,
4,
0,
4
};
//����������
//����������

void testParam(int a)
{
	KdPrint(("param1=%d  praram1 addr:%x\n", a, &a));
}

void TestResumeThread(ULONG threadid)                       // ���ӵĵ�һ������
{
	PETHREAD peThread;
	NTSTATUS Status;
	ULONG ret;

	KdPrint(("Thread Id=%x\n", (ULONG)threadid));

	Status = PsLookupThreadByThreadId((HANDLE)threadid, &peThread);

	if (!NT_SUCCESS(Status))
	{
		
		return;
	}

	Status = PsResumeThread(peThread, &ret);
	ObDereferenceObject(peThread);
}


void TestSuspendThread(ULONG threadid)              // ���ӵĵڶ�������
{
	PETHREAD peThread;
	NTSTATUS Status;
	ULONG ret;

	KdPrint(("Thread Id=%x\n", (ULONG)threadid));

	Status = PsLookupThreadByThreadId((HANDLE)threadid, &peThread);

	if (!NT_SUCCESS(Status))
	{
			KdPrint(("%d\n", Status));
		return;
	}

	Status = PsSuspendThread(peThread, &ret);
	ObDereferenceObject(peThread);
}

void Reshedule()  // ���ӵĵ���������
{
	KeInsertQueueDpc(&schedulingDpc, NULL, NULL);
}


/************************************************************************
*                                                                      *
*                             Struct Define                            *
*                                                                      *
************************************************************************/
extern __declspec(dllimport) __stdcall KeAddSystemServiceTable(ULONG, ULONG, ULONG, ULONG, ULONG);

#pragma LOCKEDCODE
NTSTATUS AddServices()
{
	ULONG Addrtemp;

	unsigned int NumberOfServices;
	KdPrint(("AddServices��ʼִ��\n"));

	NumberOfServices = sizeof(ServiceTableBase) / sizeof(ServiceTableBase[0]);                   // ��Ҫ���ӵķ������ĸ���
	NewNumberOfServices = KeServiceDescriptorTable.NumberOfServices + NumberOfServices;         //���Ӻ������Ŀ
	StartingServiceId = KeServiceDescriptorTable.NumberOfServices;                            //�õ��������ӷ����е�һ����������

	KdPrint(("���ӵĵ�һ���������StartingServiceId  %x \n", StartingServiceId));
	/* Allocate suffcient memory to hold the exising services as well as
	the services you want to add */
	NewServiceTableBase = (unsigned int *)ExAllocatePool(NonPagedPool, NewNumberOfServices*sizeof(unsigned int));
	if (NewServiceTableBase == NULL) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	NewParamTableBase = (PULONG)(unsigned char *)ExAllocatePool(NonPagedPool, NewNumberOfServices);
	if (NewParamTableBase == NULL) {
		ExFreePool(NewServiceTableBase);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	//ԭ����SSDT�����ַ�����������NewServiceTableBase�ڴ���ȥ
	memcpy(NewServiceTableBase, (char *)(KeServiceDescriptorTable.ServiceTableBase),    
		KeServiceDescriptorTable.NumberOfServices*sizeof(unsigned int));
	//ԭ����SSDT�����������·����NewParamTableBase�ڴ���ȥ
	memcpy(NewParamTableBase, (char *)(KeServiceDescriptorTable.ParamTableBase),       
		KeServiceDescriptorTable.NumberOfServices);

	/*�Ե�ַ��Ͳ�����׷��*/
	memcpy(NewServiceTableBase + KeServiceDescriptorTable.NumberOfServices,
		ServiceTableBase, sizeof(ServiceTableBase));
	memcpy(NewParamTableBase + KeServiceDescriptorTable.NumberOfServices,
		ParamTableBase, sizeof(ParamTableBase));
	//��¼�ϵ���Ϣ��������ԭ
	OldServiceTableBase = KeServiceDescriptorTable.ServiceTableBase;
	OldParamTableBase = (ULONG)KeServiceDescriptorTable.ParamTableBase;
	OldNumberOfServices = KeServiceDescriptorTable.NumberOfServices;

	/*����KeServiceDescriptorTableEntry��SSDT�� SSPT*/
	KeServiceDescriptorTable.ServiceTableBase = NewServiceTableBase;
	KeServiceDescriptorTable.ParamTableBase = (PULONG)NewParamTableBase;
	KeServiceDescriptorTable.NumberOfServices = NewNumberOfServices;
	//��������µ�SSDT �������ĵ�ַ
	KdPrint(("New KeServiceDescriptorTable->ServiceTableBase = 0x%0x\n", KeServiceDescriptorTable.ServiceTableBase));

	//��ӡ����һ������ĵ�ַ
	Addrtemp = (ULONG)KeServiceDescriptorTable.ServiceTableBase;
	Addrtemp = Addrtemp + StartingServiceId * 4;
	Addrtemp = *(ULONG*)Addrtemp;
	KdPrint(("���ӵĵ�һ������ĵ�ַ�� Addrtemp is 0x%0x\n", Addrtemp));
	serviceId = StartingServiceId;
	return STATUS_SUCCESS;
}

#pragma LOCKEDCODE
NTSTATUS SetBackServices()
{
	/*����KeServiceDescriptorTableEntry��SSDT�� SSPT*/
	KeServiceDescriptorTable.ServiceTableBase = OldServiceTableBase;
	KeServiceDescriptorTable.ParamTableBase = (PULONG)OldParamTableBase;
	KeServiceDescriptorTable.NumberOfServices = OldNumberOfServices;

	if (NewServiceTableBase != NULL)
	{
		ExFreePool(NewServiceTableBase);
	}

	if (NewParamTableBase != NULL)
	{
		ExFreePool(NewParamTableBase);
	}

	return STATUS_SUCCESS;
}

