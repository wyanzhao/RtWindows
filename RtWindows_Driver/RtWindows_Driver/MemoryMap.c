/***********************************头文件**************************************/
#include "../Include/MemoryMap.h"

/***********************************函数定义***********************************************/
#pragma LOCKEDCODE
NTSTATUS CreateAndMapMemory(IN PVOID pNonPagedMemToAllocate,//已经分配好的空间地址
	IN int ByteOfSize,		//传入参数为整个映射内存的大小，单位为B	
	IN PMDL pMemMdl,
	IN KPROCESSOR_MODE mode,//UserMode(内核到用户映射) 或 KernelMode（用户到内核映射）
	OUT PVOID* pBaseAddr)
{
	//Allocate a ByteOfSize buffer to share with the application

	if (pMemMdl == NULL)
	{
		if (pNonPagedMemToAllocate == NULL)
		{
			*pBaseAddr = NULL;
			KdPrint(("The pNonPagedMemToAllocate is empty\n"));
			return STATUS_UNSUCCESSFUL;
		}
		pMemMdl = IoAllocateMdl(pNonPagedMemToAllocate, ByteOfSize, FALSE, FALSE, NULL);	//为pKePagedMemToAllocate建立对应MDL

		if (!pMemMdl)
		{
			KdPrint(("STATUS_INSUFFICIENT_RESOURCES\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		MmBuildMdlForNonPagedPool(pMemMdl);	//分配物理内存
	}

	//The preferred way to map the buffer into user/kernel space
	*pBaseAddr = MmMapLockedPagesSpecifyCache(pMemMdl,        //MDL 映射一块物理页面并返回地址
		mode,   //Mode
		MmNonCached,   //Caching
		NULL,       //Address
		FALSE,      //Bugcheck
		NormalPagePriority);//Priority
	if (!(*pBaseAddr))
	{
		KdPrint(("UserVAToRetrun is NULL\n"));
		IoFreeMdl(pMemMdl);
		*pBaseAddr = NULL;
		return STATUS_UNSUCCESSFUL;
	}

	return STATUS_SUCCESS;
}

#pragma LOCKEDCODE
NTSTATUS UnMapAndFreeMemory(IN PVOID pBaseAddr, IN PMDL PMdl, IN KPROCESSOR_MODE mode)
{
	//Make sure we have an MDL to free
	if (!PMdl) {
		return STATUS_UNSUCCESSFUL;
	}
	if (mode == KernelMode)
	{
		KdPrint(("You can't Use KernelMode with MmBuildMdlForNonPagedPool\n"));
		return STATUS_UNSUCCESSFUL;
	}
	//return the allocated resource
	MmUnmapLockedPages(pBaseAddr, PMdl);
	IoFreeMdl(PMdl);
	PMdl = NULL;

	return STATUS_SUCCESS;
}
