/***********************************ͷ�ļ�**************************************/
#include "../Include/MemoryMap.h"

/***********************************��������***********************************************/
#pragma LOCKEDCODE
NTSTATUS CreateAndMapMemory(IN PVOID pNonPagedMemToAllocate,//�Ѿ�����õĿռ��ַ
	IN int ByteOfSize,		//�������Ϊ����ӳ���ڴ�Ĵ�С����λΪB	
	IN PMDL pMemMdl,
	IN KPROCESSOR_MODE mode,//UserMode(�ں˵��û�ӳ��) �� KernelMode���û����ں�ӳ�䣩
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
		pMemMdl = IoAllocateMdl(pNonPagedMemToAllocate, ByteOfSize, FALSE, FALSE, NULL);	//ΪpKePagedMemToAllocate������ӦMDL

		if (!pMemMdl)
		{
			KdPrint(("STATUS_INSUFFICIENT_RESOURCES\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		MmBuildMdlForNonPagedPool(pMemMdl);	//���������ڴ�
	}

	//The preferred way to map the buffer into user/kernel space
	*pBaseAddr = MmMapLockedPagesSpecifyCache(pMemMdl,        //MDL ӳ��һ������ҳ�沢���ص�ַ
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
