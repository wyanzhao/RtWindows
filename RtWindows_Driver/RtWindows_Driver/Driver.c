/***********************************头文件**************************************/
#include <NTDDK.h>
#include <windef.h>

#include "../Include/Driver.h"  
#include "../Include/RTWindowsScheduler.h"
#include "../Include/SysOperator.h"
#include "../Include/SysManager.h"
#include "../Include/Interrupt.h"
#include "../Include/Affinity.h"

/***********************************函数定义***************************************************/
#pragma INITCODE
NTSTATUS DriverEntry( IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryString)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG i = 0;

	// 用户使用的符号连接名
	UNICODE_STRING                  ntDeviceName;
	UNICODE_STRING                  win32DeviceName;
	PDEVICE_OBJECT                  deviceObject = NULL;
	BOOLEAN                         fSymbolicLink = FALSE;
	PDEVICE_EXTENSION               pDeviceExten;

	DbgPrint("Enter DriverEntry\n");
	UNREFERENCED_PARAMETER(pRegistryString);

	do
	{
		//初始化设备名
		RtlInitUnicodeString(&ntDeviceName, DEVICE_NAME);

		status = IoCreateDevice(pDriverObj,
			sizeof(DEVICE_EXTENSION),
			&ntDeviceName,
			FILE_DEVICE_UNKNOWN,
			FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&deviceObject);

		if (!NT_SUCCESS(status))
		{
			return status;
		}

		// 为上面的设备创建符号连接
		RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);
		status = IoCreateSymbolicLink(&win32DeviceName, &ntDeviceName);
		fSymbolicLink = TRUE;

		//设置派遣函数
		for (i = 0; i < arraysize(pDriverObj->MajorFunction); i++)
			pDriverObj->MajorFunction[i] = DriverDispatchRoutin;
		pDriverObj->MajorFunction[IRP_MJ_CREATE] = DriverCreate;
		pDriverObj->MajorFunction[IRP_MJ_CLOSE] = DriverClose;
		pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = DriverCleanup;
		pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverIoControl;
		pDriverObj->DriverUnload = DriverUnload;

		status = STATUS_SUCCESS;

		// 设置为缓冲区I/O方式
		deviceObject->Flags |= DO_DIRECT_IO;
		pDeviceExten = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
		pDeviceExten->pDevice = deviceObject;
		pDeviceExten->ustrDeviceName = ntDeviceName;

	} while (FALSE);

	//内核初始化
	InitSystemService();

	if (!NT_SUCCESS(status))
	{
		if (deviceObject)
		{
			// 删除设备对象
			IoDeleteDevice(deviceObject);
		}

		if (fSymbolicLink)
		{
			// 删除符号连接
			IoDeleteSymbolicLink(&win32DeviceName);
		}

	}
	return status;
}

#pragma LOCKEDCODE
NTSTATUS DriverCreate(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	PIO_STACK_LOCATION      pIrpSp;
	NTSTATUS                NtStatus = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(pDevObj);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	pIrpSp->FileObject->FsContext = NULL;


	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return NtStatus;
}

#pragma LOCKEDCODE
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObj)
{
	PDEVICE_OBJECT	  pNextObject;
	PDEVICE_EXTENSION pDeviceExtern;
	UNICODE_STRING    pLinkName;
	KIRQL					oldIrql;

	DbgPrint("Enter DriverUnload\n");
	KeRaiseIrql(HIGH_LEVEL, &oldIrql);
	CleanOutSystem();
	KeLowerIrql(oldIrql);

	pNextObject = pDriverObj->DeviceObject;
	while (pNextObject != NULL)
	{
		pDeviceExtern = (PDEVICE_EXTENSION)pNextObject->DeviceExtension;
		//删除符号链接
		pLinkName = pDeviceExtern->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObject = pNextObject->NextDevice;
		IoDeleteDevice(pDeviceExtern->pDevice);
	}
}

#pragma LOCKEDCODE
NTSTATUS DriverClose( IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	NTSTATUS                NtStatus;
	PIO_STACK_LOCATION      pIrpSp;

	KdPrint(("Enter DriverClose\n"));
	UNREFERENCED_PARAMETER(pDevObj);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	pIrpSp->FileObject->FsContext = NULL;

	NtStatus = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return NtStatus;
}

#pragma LOCKEDCODE
 NTSTATUS DriverCleanup(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
)
{
	NTSTATUS                NtStatus;

	//清理时钟中断处理例程
	cInterruptCount--;
	if (cInterruptCount == 0)
	{
		KdPrint(("Clean Interrupt\n"));
		UnsetInterrupt();
		OldInt = 0;
	}
	
	//记录系统启动进程个数，为0时说明最后一个任务退出，恢复系统亲缘性
	cAffinityCount--;
	if (cAffinityCount == 0)
	{
		KdPrint(("Clean Affinity\n"));
		RtSetSystemAffinity(SYSTEMAFFINITY);
	}
	
	//取消映射内存
	cMemoryCount--;
	if (cMemoryCount == 0)
	{
		KdPrint(("UnMapAndFreeMemory\n"));
		UnMapAndFreeMemory(s_pBase, s_pMdl, UserMode);//每个进程关闭时，会释放映射
		KdPrint(("ExFreePoolWithTag\n"));
		ExFreePoolWithTag(s_pBase, 'TIME');
	}

	UNREFERENCED_PARAMETER(pDeviceObject);

	NtStatus = STATUS_SUCCESS;

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return NtStatus;
}

#pragma LOCKEDCODE
 NTSTATUS DriverIoControl( IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	PIO_STACK_LOCATION pIrpStack = NULL;
	PVOID pIoBuffer = NULL;
	PIO_STACK_LOCATION      pIrpSp = NULL;
	NTSTATUS                NtStatus = STATUS_SUCCESS;
	PVOID					pInBuf = NULL;
	PVOID					pOutBuf = NULL;
	PVOID					pUserBuf = NULL;
	PETHREAD				peThread = NULL;
	DWORD					threadID = 0;
	ULONG                   BytesReturned = 0;
	ULONG FunctionCode = 0;
	ULONG uInSize = 0;
	ULONG uOutSize = 0;  

	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	FunctionCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
	uInSize = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	uOutSize = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (FunctionCode)
	{
	case IOCTL_GET_START_SERVICEID: //获取添加的服务id
		*(unsigned int*)pIrp->AssociatedIrp.SystemBuffer = serviceId;
		KdPrint(("First Service ID 0x%x\n", *(ULONG*)pIrp->AssociatedIrp.SystemBuffer));
		NtStatus = STATUS_SUCCESS;
		BytesReturned = 4;
		break;

	case IOCTL_MAP_MERMORY://内存映射，获取系统管理内存地址
		if (cMemoryCount == 0)
		{
			KdPrint(("Map the Memory\n"));
			SystemInitialization();
			cMemoryCount++;
		}
		CreateAndMapMemory((PVOID)s_pBase, TIME_MEM_SIZE, s_pMdl, UserMode, &pUserBuf);
		*(char**)pIrp->AssociatedIrp.SystemBuffer = pUserBuf;
		NtStatus = STATUS_SUCCESS;
		BytesReturned = 4;

		KdPrint(("User Memory Addr: 0x%x\n", pUserBuf));
		KdPrint(("Sys Memory Addr :0x%x\n", s_pBase));
		break;

	case IOCTL_SET_CLOCK:
		if (cInterruptCount == 0)	//初始化时钟
		{
			KdPrint(("Initialize the Interrupt\n"));
			apic_init_count = *(unsigned long*)pIrp->AssociatedIrp.SystemBuffer;
			apic_period = 1000; // 每次中断间隔,单位微秒
			SetInterrupt();
			cInterruptCount++;
		}	else
		{
			if ( *(ULONG*)pIrp->AssociatedIrp.SystemBuffer == 0 ) //冻结时钟
			{
				KdPrint(("Freeze the APIC\n"));
				UnsetInterrupt();
			}	else //更改时钟频率
			{
				KdPrint(("Set the APIC\n"));
				apic_init_count = *(unsigned long*)pIrp->AssociatedIrp.SystemBuffer;
				UnsetInterrupt();
				SetInterrupt();
			}
		}
		NtStatus = STATUS_SUCCESS;
		BytesReturned = 0;
		break;

	case IOCTL_SET_SYSAFFINITY:
		if (cAffinityCount == 0)
		{
			KdPrint(("SystemAffinity is %0x\n", *(ULONG*)pIrp->AssociatedIrp.SystemBuffer));
			RtSetSystemAffinity(*(ULONG*)pIrp->AssociatedIrp.SystemBuffer);
			cAffinityCount++;
		}
		NtStatus = STATUS_SUCCESS;
		BytesReturned = 0;
		break;

	case IOCTL_GET_THREAD_HANDLE:
		threadID = *(DWORD*)pIrp->AssociatedIrp.SystemBuffer;
		PsLookupThreadByThreadId((HANDLE)threadID, &peThread);
		*(HANDLE*)pIrp->AssociatedIrp.SystemBuffer = (HANDLE)peThread;
		NtStatus = STATUS_SUCCESS;
		BytesReturned = 4;
		KdPrint(("Thread id is 0x%x\n", (ULONG)threadID));
		KdPrint(("ThreadHandle: 0x%x\n", *(HANDLE*)pIrp->AssociatedIrp.SystemBuffer));
		break;

	default:
		KdPrint(("Can't found Function Code\n"));
		break;
	}

	if (NtStatus == STATUS_SUCCESS)
		pIrp->IoStatus.Information = uOutSize;
	else
		pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = NtStatus;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return NtStatus;
}

/************************************************************************
* 函数名称:DispatchRoutin
* 功能描述:对读IRP进行处理
* 参数列表:
* pDevObj:功能设备对象
* pIrp:从IO请求包
* 返回 值:返回状态
*************************************************************************/
#pragma PAGEDCODE
 NTSTATUS DriverDispatchRoutin(
	IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS status = STATUS_SUCCESS;

	//建立一个字符串数组与IRP类型对应起来
	static char* irpname[] =
	{
		"IRP_MJ_CREATE",
		"IRP_MJ_CREATE_NAMED_PIPE",
		"IRP_MJ_CLOSE",
		"IRP_MJ_READ",
		"IRP_MJ_WRITE",
		"IRP_MJ_QUERY_INFORMATION",
		"IRP_MJ_SET_INFORMATION",
		"IRP_MJ_QUERY_EA",
		"IRP_MJ_SET_EA",
		"IRP_MJ_FLUSH_BUFFERS",
		"IRP_MJ_QUERY_VOLUME_INFORMATION",
		"IRP_MJ_SET_VOLUME_INFORMATION",
		"IRP_MJ_DIRECTORY_CONTROL",
		"IRP_MJ_FILE_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CONTROL",
		"IRP_MJ_INTERNAL_DEVICE_CONTROL",
		"IRP_MJ_SHUTDOWN",
		"IRP_MJ_LOCK_CONTROL",
		"IRP_MJ_CLEANUP",
		"IRP_MJ_CREATE_MAILSLOT",
		"IRP_MJ_QUERY_SECURITY",
		"IRP_MJ_SET_SECURITY",
		"IRP_MJ_POWER",
		"IRP_MJ_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CHANGE",
		"IRP_MJ_QUERY_QUOTA",
		"IRP_MJ_SET_QUOTA",
		"IRP_MJ_PNP",
	};

	UCHAR type = stack->MajorFunction;

	if (type >= arraysize(irpname))
		KdPrint((" - Unknown IRP, major type %X\n", type));
	else
		KdPrint(("\t%s\n", irpname[type]));

	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}

