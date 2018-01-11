#pragma once
/************************************************************************
* 文件说明：内核下设置程序的亲缘性函数
* 作者：蒋善锋
* 时间：2011年12月13日
* 修改：陈进朝
* 时间：2012年6月1日
* 修改：王延钊
* 时间：2016年3月5日
*************************************************************************/

/***********************************头文件**************************************/
#include <NTDDK.h>
#include "Driver.h"

#ifndef PAGE_OPTION
#define PAGE_OPTION
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")
#endif

//操作码定义，各个操作系统版本的数据结构偏移量不同，可以在WinDbg中利用命令dt _EPROCESS
//得到EPROCESS内各数据结构的偏移量

//操作码定义
//偏移位置0x0b8 ActiveProcessLinks : _LIST_ENTRY，系统中的全局进程列表，是循环链表结构
#define EPROCESS_ACTIVELIST_OFFSET 0x0b8

//偏移位置0x02c ThreadListHead   : _LIST_ENTRY，本进程的线程(KTHREAD)队列
#define KPROCESS_THREADLISTHEAD 0x02c

//偏移位置0x038 Affinity         : _KAFFINITY_EX，默认亲和处理器集合
#define KPROCESS_AFFINITY_OFFSET 0x038

//偏移位置0x1e0 ThreadListEntry  : _LIST_ENTRY，
//挂入KPROCESS中的线程队列距离线程结构体首地址的偏移量
#define KTHREAD_LISTENTRY_OFFSET 0x1e0 

//偏移位置0x154 Affinity         : KAFFINITY，亲缘性
#define KTHREAD_AFFINITY_OFFSET 0x154

//实时性定义
#define UNRTWORKAFFINITY 0x0E
#define RTWORKAFFINITY   0x01
#define SYSTEMAFFINITY   0x0F

//前一个操作取出地址里的值，后一个操作只得到目标地址
#define GET_PTRVALUE(type,baseaddr,offset) \
	*((type *)((PUCHAR)(baseaddr) + offset))
#define GET_PTR(type,baseaddr,offset) \
	(type )((PUCHAR)(baseaddr) + offset)

/***********************************系统变量定义***********************************************/
//亲缘性
#pragma LOCKEDDATA
extern ULONG s_systemAffinity;
//系统亲缘性设置计数
#pragma LOCKEDDATA
extern unsigned long cAffinityCount;

/***********************************函数定义***************************************************/
/*
* 设置线程亲缘性
* PETHREAD 线程结构体
* BOOLEAN 返回值 是否成功
*/
#pragma LOCKEDCODE
BOOLEAN RtSetThreadAffinity(PETHREAD eThread, ULONG cpuMask);

/*
* 设置进程实时亲缘性
* HANDLE 进程句柄
* BOOLEAN 返回值 是否成功
*/
#pragma LOCKEDCODE
BOOLEAN RtSetProcessAffinity(PEPROCESS eProcess, ULONG cpuMask);

/*
* 设置系统亲缘性，即改变除idle system进程的所有进程亲缘性
* cpuMask 亲缘性掩码
* BOOLEAN 返回值 是否成功
*/
#pragma LOCKEDCODE
BOOLEAN RtSetSystemAffinity(ULONG cpuMask);


