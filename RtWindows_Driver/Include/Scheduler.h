/************************************************************************
* 文件说明：DPC数据结构和函数声明
* 作者：杨皓
* 时间：2012年11月9日
*************************************************************************/

/***********************************头文件**************************************/
#pragma once
#include <NTDDK.h>

#ifndef PAGE_OPTION
#define PAGE_OPTION
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")
#endif // !PAGE_OPTION

/***********************************系统变量定义***********************************************/
//当前PDC
#pragma LOCKEDDATA
extern KDPC schedulingDpc;

//DPC自旋锁
#pragma LOCKEDDATA
extern KSPIN_LOCK dpc_spinlock;

//Dpc例程处理（调度函数）
extern KDEFERRED_ROUTINE  SchedulDpc;

//APIC周期
extern unsigned long apic_period;

/***********************************函数定义***************************************************/
//初始化调度
VOID InitScheduler();