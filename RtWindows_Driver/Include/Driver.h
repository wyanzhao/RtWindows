/************************************************************************
* �ļ�˵��������������Ҫ�궨��ͻص�����
* ���ߣ�Tesla.Angela(GDUT.HWL)
* �޸ģ�������
* ʱ�䣺2016��3��5��
*************************************************************************/
#pragma once

#include <NTDDK.h>

//�����ҳ���Ƿ�ҳ�ڴ�
#ifndef PAGE_OPTION
#define PAGE_OPTION
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")
#endif // !PAGE_OPTION

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

//��ʼ����������
#define DEVICE_NAME          L"\\Device\\RtWindows"
#define DOS_DEVICE_NAME         L"\\??\\RtWindows"
#define LINK_NAME			L"\\DosDevices\\RtWindows"
#define LINK_GLOBAL_NAME	L"\\DosDevices\\Global\\RtWindows"

//��ȡ��ʼ������ID
#define IOCTL_GET_START_SERVICEID \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8001,METHOD_BUFFERED,FILE_ANY_ACCESS)

//�ڴ�ӳ��
#define IOCTL_MAP_MERMORY \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8002,METHOD_BUFFERED,FILE_ANY_ACCESS)

//�趨ʱ������
#define IOCTL_SET_CLOCK\
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8003,METHOD_BUFFERED,FILE_ANY_ACCESS)

//�趨ϵͳ��Ե�ԣ���ʵʱ���ʵʱ������
#define IOCTL_SET_SYSAFFINITY\
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8004,METHOD_BUFFERED,FILE_ANY_ACCESS)

//��ȡ��ǰ�߳̾��
#define IOCTL_GET_THREAD_HANDLE \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8006,METHOD_BUFFERED,FILE_ANY_ACCESS)

//��RT�����ʼ��
#define IOCTL_INIT_DLL \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8007,METHOD_BUFFERED,FILE_ANY_ACCESS)

/***********************************ϵͳ���ݽṹ��������**************************************/
//�豸��չ���ݽṹ����
typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT pDevice;         //�豸����
	UNICODE_STRING ustrDeviceName;	//�豸����
	UNICODE_STRING ustrSymLinkName;	//����������
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/***********************************��������***********************************************/
/*
* ��������������
* IN PDRIVER_OBJECT   pDriverObject ��������ָ��
* IN PUNICODE_STRING  pRegistryPath ע��·��ָ��
* NTSTATUS ����ֵ��״̬��־
*/ 
 NTSTATUS
DriverEntry(
IN PDRIVER_OBJECT   pDriverObject,
IN PUNICODE_STRING  pRegistryPath
);

/*
* ����ж�غ���
* IN PDRIVER_OBJECT DriverObject ��������ָ��
* VOID ����ֵ Ϊ��
*/
 VOID
DriverUnload(
IN PDRIVER_OBJECT DriverObject
);

/*
* ���豸����
* IN PDRIVER_OBJECT DriverObject ��������ָ��
* IN PIRP pIrp �رն�Ӧ��IRP
*/
 NTSTATUS
DriverCreate(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);

/*
* �����رպ��� ��ӦIRP_MJ_CLOSE
* IN PDRIVER_OBJECT DriverObject ��������ָ��
* IN PIRP        pIrp �رն�Ӧ��IRP
* NTSTATUS ����ֵ��״̬��־
*/
 NTSTATUS
DriverClose(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);

/*
* ����������� ��ӦIRP_MJ_CLEANUP
* IN PDRIVER_OBJECT DriverObject ��������ָ��
* IN PIRP        pIrp ��Ӧ��IRP
* NTSTATUS ����ֵ��״̬��־
*/
 NTSTATUS
DriverCleanup(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);

/*
* DispatchRoutin ȷ��IRP��Ҫ����
* IN PDRIVER_OBJECT DriverObject ��������ָ��
* IN PIRP        pIrp ��Ӧ��IRP
* NTSTATUS ����ֵ��״̬��־
*/
 NTSTATUS
DriverDispatchRoutin(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);

/*
* ����������� ��ӦIRP_MJ_DEVICE_CONTROL
* IN PDRIVER_OBJECT DriverObject ��������ָ��
* IN PIRP        pIrp ��Ӧ��IRP
* NTSTATUS ����ֵ��״̬��־
*/
 NTSTATUS
DriverIoControl(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);