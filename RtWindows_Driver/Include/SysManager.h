#pragma once
/************************************************************************
* 文件说明：系统管理器
* 作者：杨皓
* 时间：2012年11月9日
*************************************************************************/

/***********************************头文件**************************************/
#include "Queue.h"
#include "RTWindowsScheduler.h"
#include "RTWindowsServices.h"
#include "RTWindowsRuntime.h"
#include "ThreadOp.h"
#include "Affinity.h"
#include "Scheduler.h"
#include "MemoryMap.h"

/***********************************系统变量定义***********************************************/
//内核态申请内存的指针
extern char* s_pBase;
//MDL的指针
extern PMDL s_pMdl;
//内存映射引用计数
#pragma LOCKEDDATA
extern unsigned long cMemoryCount;

/***********************************函数定义***********************************************/
/*
* 系统初始化
* BOOL 返回值 是否成功
*/
#pragma LOCKEDCODE
BOOL  SystemInitialization();