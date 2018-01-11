/***********************************头文件**************************************/
#include <NTDDK.h>

#include "../Include/InsertSSDT.h"
#include "../Include/ThreadOp.h"
#include "../Include/Scheduler.h"
#include "../Include/SysManager.h"

/***********************************系统变量定义***********************************************/
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
*                             增加的服务函数                            *
*                                                                      *
************************************************************************/
void TestResumeThread(ULONG threadid);
void TestSuspendThread(ULONG threadid);
void Reshedule();
void testParam(int a);

/* 将要增加的系统服务地址表 */
unsigned int ServiceTableBase[] = { (unsigned int)TestResumeThread,
(unsigned int)TestSuspendThread,
(unsigned int)Reshedule,
(unsigned int)testParam
};

/*将要增加的系统服务参数表*/
unsigned char ParamTableBase[] = { 4,
4,
0,
4
};
//服务描述表
//服务描述表

void testParam(int a)
{
	KdPrint(("param1=%d  praram1 addr:%x\n", a, &a));
}

void TestResumeThread(ULONG threadid)                       // 增加的第一个服务
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


void TestSuspendThread(ULONG threadid)              // 增加的第二个服务
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

void Reshedule()  // 增加的第三个服务
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
	KdPrint(("AddServices开始执行\n"));

	NumberOfServices = sizeof(ServiceTableBase) / sizeof(ServiceTableBase[0]);                   // 将要增加的服务函数的个数
	NewNumberOfServices = KeServiceDescriptorTable.NumberOfServices + NumberOfServices;         //增加后服务数目
	StartingServiceId = KeServiceDescriptorTable.NumberOfServices;                            //得到后来增加服务中第一个服务的序号

	KdPrint(("增加的第一个服务号是StartingServiceId  %x \n", StartingServiceId));
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
	//原来的SSDT服务地址表拷贝到分配的NewServiceTableBase内存中去
	memcpy(NewServiceTableBase, (char *)(KeServiceDescriptorTable.ServiceTableBase),    
		KeServiceDescriptorTable.NumberOfServices*sizeof(unsigned int));
	//原来的SSDT参数表拷贝到新分配的NewParamTableBase内存中去
	memcpy(NewParamTableBase, (char *)(KeServiceDescriptorTable.ParamTableBase),       
		KeServiceDescriptorTable.NumberOfServices);

	/*对地址表和参数表追加*/
	memcpy(NewServiceTableBase + KeServiceDescriptorTable.NumberOfServices,
		ServiceTableBase, sizeof(ServiceTableBase));
	memcpy(NewParamTableBase + KeServiceDescriptorTable.NumberOfServices,
		ParamTableBase, sizeof(ParamTableBase));
	//记录老的信息，用来还原
	OldServiceTableBase = KeServiceDescriptorTable.ServiceTableBase;
	OldParamTableBase = (ULONG)KeServiceDescriptorTable.ParamTableBase;
	OldNumberOfServices = KeServiceDescriptorTable.NumberOfServices;

	/*更新KeServiceDescriptorTableEntry的SSDT和 SSPT*/
	KeServiceDescriptorTable.ServiceTableBase = NewServiceTableBase;
	KeServiceDescriptorTable.ParamTableBase = (PULONG)NewParamTableBase;
	KeServiceDescriptorTable.NumberOfServices = NewNumberOfServices;
	//这里输出新的SSDT 服务函数的地址
	KdPrint(("New KeServiceDescriptorTable->ServiceTableBase = 0x%0x\n", KeServiceDescriptorTable.ServiceTableBase));

	//打印出第一个服务的地址
	Addrtemp = (ULONG)KeServiceDescriptorTable.ServiceTableBase;
	Addrtemp = Addrtemp + StartingServiceId * 4;
	Addrtemp = *(ULONG*)Addrtemp;
	KdPrint(("增加的第一个服务的地址是 Addrtemp is 0x%0x\n", Addrtemp));
	serviceId = StartingServiceId;
	return STATUS_SUCCESS;
}

#pragma LOCKEDCODE
NTSTATUS SetBackServices()
{
	/*更新KeServiceDescriptorTableEntry的SSDT和 SSPT*/
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

