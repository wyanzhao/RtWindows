/************************************************************************
* 文件说明：对系统服务描述表进行操作
* 修改：王延钊
* 时间：2016年3月5日
*************************************************************************/

/***********************************头文件**************************************/
#pragma once

#include <NTDDK.h>
/***********************************系统数据结构申明区域**************************************/
//服务描述表
typedef struct _SERVICE_DESCRIPTOR_TABLE {
	PVOID  		ServiceTableBase;
	PVOID  		ServiceCounterTableBase;
	ULONG  		NumberOfServices;
	PVOID  		ParamTableBase;
} SERVICE_DESCRIPTOR_TABLE, *PSERVICE_DESCRIPTOR_TABLE;

/***********************************系统变量定义***********************************************/
//当前系统函数在SSDT中的服务号
extern ULONG serviceId;

/***********************************函数定义***********************************************/
/*
* 添加系统服务
* NTSTATUS 返回值 是否成功
*/
NTSTATUS AddServices();

/*
* 删除系统服务
* NTSTATUS 返回值 是否成功
*/
NTSTATUS SetBackServices();
