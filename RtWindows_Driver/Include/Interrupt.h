#pragma once
/************************************************************************
* 文件说明：启动APIC并调用中断处理函数
* 作者：蒋善锋
* 日期：2011年11月15日
* 修改：杨皓
* 时间：2013年6月3日
* 修改：王延钊
* 时间：2016年4月1日
*************************************************************************/

/***********************************头文件**************************************/
#include <NTDDK.h>
#include "Driver.h"

/***********************************宏定义**************************************/
//获取地址中低16位内容
#define P2C_LOW16_OF_32(data) \
	((P2C_U16)(((P2C_U32)data) & 0xffff))
//获取地址中高16位内容
#define P2C_HIGH16_OF_32(data) \
	((P2C_U16)(((P2C_U32)data) >> 16))

/***********************************系统数据结构申明区域**************************************/
//重命名数据结构
typedef UCHAR		 P2C_U8;
typedef USHORT		 P2C_U16;
typedef ULONG		 P2C_U32;

#pragma once
//IDT数据结构
#pragma pack(push,1)
typedef struct P2C_IDTR_
{
	USHORT limit;
	ULONG base;
}P2C_IDTR, *PP2C_IDTR;
#pragma pack(pop)

//IDT具体表项结构
#pragma pack(push,1)
typedef struct P2C_IDT_ENTRY_ {
	P2C_U16 offset_low;
	P2C_U16 selector;
	P2C_U8 reserved;
	P2C_U8 type : 4;
	P2C_U8 always0 : 1;
	P2C_U8 dpl : 2;
	P2C_U8 present : 1;
	P2C_U16 offset_high;
} P2C_IDTENTRY, *PP2C_IDTENTRY;
#pragma pack(pop)

/***********************************系统变量定义***********************************************/
//apic timer初始计数
extern unsigned long apic_init_count;

//通过用户态设置的APIC 周期
extern unsigned long apic_period;

//记录APIC timer原有状态
extern unsigned long OldInt;

//设置APIC中断计数
#pragma LOCKEDDATA
extern unsigned long cInterruptCount;

/***********************************函数定义***************************************************/
/*
* 返回中断描述符表IDT的基址
* PVOID 返回值 IDT地址
*/
static PVOID GetIdt();

/*
* 添加时钟中断处理例程
*/
static void AddInterruptHandle();

/*
* 中断处理例程
*/
 static void Interrupt();

 /*
 * 设置APIC时钟中断
 */
void SetInterrupt();

/*
* 卸载APIC中断
*/
BOOLEAN UnsetInterrupt();

/*
* 初始化函数地址
*/
static void InitAddress();

/*
* 中断服务例程
*/
static void ServiceRoutine();