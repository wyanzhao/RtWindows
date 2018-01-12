/************************************************************************
* 文件说明：获取系统未导出函数地址
*************************************************************************/

/***********************************头文件**************************************/
#pragma once
#include <NTDDK.h>
#include "Driver.h"

/***********************************宏定义**************************************/
//服务号
//thread operate
#define NtSuspendThread_ServiceId_Win7 0x16F
#define NtResumeThread_ServiceId_Win7 0x130

//特征码
//线程挂起恢复操作特征码(Pe函数)
#define PsSuspendThread_KeyCode1_Win7 0xe8
#define PsSuspendThread_KeyCode2_Win7 0x66
#define PsSuspendThread_KeyCode3_Win7 0x00

#define KeResumeThread_KeyCode1_Win7 0x8b
#define KeResumeThread_KeyCode2_Win7 0x45
#define KeResumeThread_KeyCode3_Win7 0xe4

/***********************************系统数据结构申明区域**************************************/
typedef struct _THREAD_OPERATE_ADDRESS
{
	ULONG NtSuspendThreadAddress;
	ULONG NtResumeThreadAddress;

	ULONG PsSuspendThreadAddress;
	ULONG KeResumeThreadAddress;
}THREAD_OPERATE_ADDR, *PTHREAD_OPERATE_ADDR;

//线程服务ID数据结构
typedef struct _FUNCTION_SERVICE_ID
{
	ULONG NtSuspendThread_ServiceId;
	ULONG NtResumeThread_ServiceId;
}THREAD_FUNCTION_SERVICE_ID, *PTHREAD_FUNCTION_SERVICE_ID;

typedef struct _THREAD_KEYCODE
{
	ULONG KeyCode1;
	ULONG KeyCode2;
	ULONG KeyCode3;
}THREAD_KEYCODE, *PTHREAD_KEYCODE;

//进程线程操作特征码数据结构
typedef struct _THREAD_FUNCTION_KEYCODE
{
	//thread operate
	THREAD_KEYCODE PsSuspendThread_KeyCode;
	THREAD_KEYCODE KeResumeThread_KeyCode;
}THREAD_FUNCTION_KEYCODE, *PTHREAD_FUNCTION_KEYCODE;

/***********************************函数定义***********************************************/
//根据线程ID获取线程句柄
__declspec(dllimport) NTSTATUS PsLookupThreadByThreadId(IN HANDLE ThreadId, OUT PETHREAD *Thread);

//得到Nt函数地址
static ULONG GetNtFunctionAddress(ULONG Index);

//获得挂起线程的Ps函数入口
static ULONG GetPsSuspendThreadAddr();

//获得挂起线程的Ke函数入口
static ULONG GetKeResumeThreadAddr();

//初始化线程操作方法入口地址
BOOLEAN InitThreadOperatorAddress();

/*
* 获取SSDT内部函数地址
* ULONG 返回值函数地址
*/
static ULONG GetSSDTFunctionAddress(ULONG Index);	//获取SSDT地址

/*
* 挂起线程操作的Ps函数
* PETHREAD Thread 当前线程
* PreviousSuspendCount OPTIONAL 当前已经挂起的线程个数
* NTSTATUS 是否成功
*/
NTSTATUS PsSuspendThread(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL);

/*
* 恢复线程操作的Ps函数
* PETHREAD Thread 当前线程
* PreviousSuspendCount OPTIONAL 当前已经挂起的线程个数
* NTSTATUS 是否成功
*/
NTSTATUS PsResumeThread(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL);

/*
* 挂起线程操作的Ps函数
* PKTHREAD Thread 当前线程
* NTSTATUS 返回值 是否成功
*/
NTSTATUS SuspendThreadByHandle(IN HANDLE hThread);

/*
* 恢复线程操作的Ps函数
* PKTHREAD Thread 当前线程
* NTSTATUS 返回值 是否成功
*/
NTSTATUS ResumeThreadByHandle(IN HANDLE hThread);



