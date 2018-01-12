/************************************************************************
* 文件说明：声明驱动主要宏定义和回调函数
* 作者：Tesla.Angela(GDUT.HWL)
* 修改：王延钊
* 时间：2016年3月5日
*************************************************************************/
#pragma once

#include <NTDDK.h>

//定义分页、非分页内存
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

//初始化符号链接
#define DEVICE_NAME          L"\\Device\\RtWindows"
#define DOS_DEVICE_NAME         L"\\??\\RtWindows"
#define LINK_NAME			L"\\DosDevices\\RtWindows"
#define LINK_GLOBAL_NAME	L"\\DosDevices\\Global\\RtWindows"

//获取初始化服务ID
#define IOCTL_GET_START_SERVICEID \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8001,METHOD_BUFFERED,FILE_ANY_ACCESS)

//内存映射
#define IOCTL_MAP_MERMORY \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8002,METHOD_BUFFERED,FILE_ANY_ACCESS)

//设定时钟周期
#define IOCTL_SET_CLOCK\
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8003,METHOD_BUFFERED,FILE_ANY_ACCESS)

//设定系统亲缘性（对实时与非实时分区）
#define IOCTL_SET_SYSAFFINITY\
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8004,METHOD_BUFFERED,FILE_ANY_ACCESS)

//获取当前线程句柄
#define IOCTL_GET_THREAD_HANDLE \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8006,METHOD_BUFFERED,FILE_ANY_ACCESS)

//非RT主体初始化
#define IOCTL_INIT_DLL \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8007,METHOD_BUFFERED,FILE_ANY_ACCESS)

/***********************************系统数据结构申明区域**************************************/
//设备扩展数据结构定义
typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT pDevice;         //设备对象
	UNICODE_STRING ustrDeviceName;	//设备名称
	UNICODE_STRING ustrSymLinkName;	//符号链接名
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/***********************************函数定义***********************************************/
/*
* 驱动加载主函数
* IN PDRIVER_OBJECT   pDriverObject 驱动对象指针
* IN PUNICODE_STRING  pRegistryPath 注册路径指针
* NTSTATUS 返回值，状态标志
*/ 
 NTSTATUS
DriverEntry(
IN PDRIVER_OBJECT   pDriverObject,
IN PUNICODE_STRING  pRegistryPath
);

/*
* 驱动卸载函数
* IN PDRIVER_OBJECT DriverObject 驱动对象指针
* VOID 返回值 为空
*/
 VOID
DriverUnload(
IN PDRIVER_OBJECT DriverObject
);

/*
* 打开设备驱动
* IN PDRIVER_OBJECT DriverObject 驱动对象指针
* IN PIRP pIrp 关闭对应的IRP
*/
 NTSTATUS
DriverCreate(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);

/*
* 驱动关闭函数 对应IRP_MJ_CLOSE
* IN PDRIVER_OBJECT DriverObject 驱动对象指针
* IN PIRP        pIrp 关闭对应的IRP
* NTSTATUS 返回值，状态标志
*/
 NTSTATUS
DriverClose(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);

/*
* 驱动清除函数 对应IRP_MJ_CLEANUP
* IN PDRIVER_OBJECT DriverObject 驱动对象指针
* IN PIRP        pIrp 对应的IRP
* NTSTATUS 返回值，状态标志
*/
 NTSTATUS
DriverCleanup(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);

/*
* DispatchRoutin 确定IRP主要类型
* IN PDRIVER_OBJECT DriverObject 驱动对象指针
* IN PIRP        pIrp 对应的IRP
* NTSTATUS 返回值，状态标志
*/
 NTSTATUS
DriverDispatchRoutin(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);

/*
* 驱动清除函数 对应IRP_MJ_DEVICE_CONTROL
* IN PDRIVER_OBJECT DriverObject 驱动对象指针
* IN PIRP        pIrp 对应的IRP
* NTSTATUS 返回值，状态标志
*/
 NTSTATUS
DriverIoControl(
IN PDEVICE_OBJECT   pDeviceObject,
IN PIRP             pIrp
);