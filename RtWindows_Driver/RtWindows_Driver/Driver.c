/***********************************ͷ�ļ�**************************************/
#include <NTDDK.h>
#include <windef.h>

#include "../Include/Driver.h"  
#include "../Include/RTWindowsScheduler.h"
#include "../Include/SysOperator.h"
#include "../Include/SysManager.h"
#include "../Include/Interrupt.h"
#include "../Include/Affinity.h"

/***********************************��������***************************************************/
#pragma INITCODE
NTSTATUS DriverEntry( IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryString)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG i = 0;

	// �û�ʹ�õķ���������
	UNICODE_STRING                  ntDeviceName;
	UNICODE_STRING                  win32DeviceName;
	PDEVICE_OBJECT                  deviceObject = NULL;
	BOOLEAN                         fSymbolicLink = FALSE;
	PDEVICE_EXTENSION               pDeviceExten;

	KdPrint(("Enter DriverEntry\n"));
	UNREFERENCED_PARAMETER(pRegistryString);

	do
	{
		//��ʼ���豸��
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

		// Ϊ������豸������������
		RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);
		status = IoCreateSymbolicLink(&win32DeviceName, &ntDeviceName);
		fSymbolicLink = TRUE;

		//������ǲ����
		for (i = 0; i < arraysize(pDriverObj->MajorFunction); i++)
			pDriverObj->MajorFunction[i] = DriverDispatchRoutin;
		pDriverObj->MajorFunction[IRP_MJ_CREATE] = DriverCreate;
		pDriverObj->MajorFunction[IRP_MJ_CLOSE] = DriverClose;
		pDriverObj->MajorFunction[IRP_MJ_CLEANUP] = DriverCleanup;
		pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverIoControl;
		pDriverObj->DriverUnload = DriverUnload;

		status = STATUS_SUCCESS;

		// ����Ϊ������I/O��ʽ
		deviceObject->Flags |= DO_DIRECT_IO;
		pDeviceExten = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
		pDeviceExten->pDevice = deviceObject;
		pDeviceExten->ustrDeviceName = ntDeviceName;

	} while (FALSE);

	//�ں˳�ʼ��
	InitSystemService();

	if (!NT_SUCCESS(status))
	{
		if (deviceObject)
		{
			// ɾ���豸����
			IoDeleteDevice(deviceObject);
		}

		if (fSymbolicLink)
		{
			// ɾ����������
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

	KdPrint(("Enter DriverUnload\n"));
	KeRaiseIrql(HIGH_LEVEL, &oldIrql);
	CleanOutSystem();
	KeLowerIrql(oldIrql);

	pNextObject = pDriverObj->DeviceObject;
	while (pNextObject != NULL)
	{
		pDeviceExtern = (PDEVICE_EXTENSION)pNextObject->DeviceExtension;
		//ɾ����������
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

	//����ʱ���жϴ�������
	cInterruptCount--;
	if (cInterruptCount == 0)
	{
		KdPrint(("Clean Interrupt\n"));
		UnsetInterrupt();
		OldInt = 0;
	}
	
	//��¼ϵͳ�������̸�����Ϊ0ʱ˵�����һ�������˳����ָ�ϵͳ��Ե��
	cAffinityCount--;
	if (cAffinityCount == 0)
	{
		KdPrint(("Clean Affinity\n"));
		RtSetSystemAffinity(SYSTEMAFFINITY);
	}
	
	//ȡ��ӳ���ڴ�
	cMemoryCount--;
	if (cMemoryCount == 0)
	{
		KdPrint(("UnMapAndFreeMemory\n"));
		UnMapAndFreeMemory(s_pBase, s_pMdl, UserMode);//ÿ�����̹ر�ʱ�����ͷ�ӳ��
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
	case IOCTL_GET_START_SERVICEID: //��ȡ��ӵķ���id
		*(unsigned int*)pIrp->AssociatedIrp.SystemBuffer = serviceId;
		KdPrint(("First Service ID 0x%x\n", *(ULONG*)pIrp->AssociatedIrp.SystemBuffer));
		NtStatus = STATUS_SUCCESS;
		BytesReturned = 4;
		break;

	case IOCTL_MAP_MERMORY://�ڴ�ӳ�䣬��ȡϵͳ�����ڴ��ַ
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
		if (cInterruptCount == 0)	//��ʼ��ʱ��
		{
			KdPrint(("Initialize the Interrupt\n"));
			apic_init_count = *(unsigned long*)pIrp->AssociatedIrp.SystemBuffer;
			apic_period = 1000; // ÿ���жϼ��,��λ΢��
			SetInterrupt();
			cInterruptCount++;
		}	else
		{
			if ( *(ULONG*)pIrp->AssociatedIrp.SystemBuffer == 0 ) //����ʱ��
			{
				KdPrint(("Freeze the APIC\n"));
				UnsetInterrupt();
			}	else //����ʱ��Ƶ��
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
* ��������:DispatchRoutin
* ��������:�Զ�IRP���д���
* �����б�:
* pDevObj:�����豸����
* pIrp:��IO�����
* ���� ֵ:����״̬
*************************************************************************/
#pragma PAGEDCODE
 NTSTATUS DriverDispatchRoutin(
	IN PDEVICE_OBJECT pDevObj,
	IN PIRP pIrp)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	NTSTATUS status = STATUS_SUCCESS;

	//����һ���ַ���������IRP���Ͷ�Ӧ����
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

	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;	// bytes xfered
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}

