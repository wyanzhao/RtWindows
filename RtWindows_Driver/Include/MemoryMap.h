/************************************************************************
* 内核下分配两种类型的内存，映射到用户态，提供给用户态下的程序申请
* 作者：李轩
* 时间：2011年11月15日
* 修改：杨皓
* 时间：2012年10月15日
* 作者：王延钊
* 时间：2016年3月5日
*************************************************************************/

/***********************************头文件**************************************/
#pragma once
#include <NTDDK.h>

//代码段和数据段的定义
#ifndef PAGE_OPTION
#define PAGE_OPTION
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")
#endif // !PAGE_OPTION

/***********************************函数定义***************************************************/
/*
* 创建共享内存
* IN PVOID pNonPagedMemToAllocate 已经分配好的内核空间地址
* IN int ByteOfSize  传入参数为整个映射内存的大小，单位为B	
* IN PMDL pMemMdl 指向MDL的指针
* IN KPROCESSOR_MODE mode UserMode(内核到用户映射) 或 KernelMode（用户到内核映射）
* OUT PVOID* pBaseAddr 映射得到的地址
* NTSTATUS 返回值，状态标志
*/
#pragma LOCKEDCODE
NTSTATUS CreateAndMapMemory(IN PVOID pNonPagedMemToAllocate,	
	IN int ByteOfSize, 
	IN PMDL pMemMdl, 
	IN KPROCESSOR_MODE mode, 
	OUT PVOID* pBaseAddr);

/*
* 卸载共享内存
* IN PVOID pBaseAddr 映射得到的地址
* IN PMDL pMemMdl 指向MDL的指针
* IN KPROCESSOR_MODE mode UserMode(内核到用户映射) 或 KernelMode（用户到内核映射）
* NTSTATUS 返回值，状态标志
*/
#pragma LOCKEDCODE
NTSTATUS UnMapAndFreeMemory(IN PVOID pBaseAddr, IN PMDL PMdl, IN KPROCESSOR_MODE mode);


