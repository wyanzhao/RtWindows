/************************************************************************
* �ں��·����������͵��ڴ棬ӳ�䵽�û�̬���ṩ���û�̬�µĳ�������
* ���ߣ�����
* ʱ�䣺2011��11��15��
* �޸ģ����
* ʱ�䣺2012��10��15��
* ���ߣ�������
* ʱ�䣺2016��3��5��
*************************************************************************/

/***********************************ͷ�ļ�**************************************/
#pragma once
#include <NTDDK.h>

//����κ����ݶεĶ���
#ifndef PAGE_OPTION
#define PAGE_OPTION
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")
#endif // !PAGE_OPTION

/***********************************��������***************************************************/
/*
* ���������ڴ�
* IN PVOID pNonPagedMemToAllocate �Ѿ�����õ��ں˿ռ��ַ
* IN int ByteOfSize  �������Ϊ����ӳ���ڴ�Ĵ�С����λΪB	
* IN PMDL pMemMdl ָ��MDL��ָ��
* IN KPROCESSOR_MODE mode UserMode(�ں˵��û�ӳ��) �� KernelMode���û����ں�ӳ�䣩
* OUT PVOID* pBaseAddr ӳ��õ��ĵ�ַ
* NTSTATUS ����ֵ��״̬��־
*/
#pragma LOCKEDCODE
NTSTATUS CreateAndMapMemory(IN PVOID pNonPagedMemToAllocate,	
	IN int ByteOfSize, 
	IN PMDL pMemMdl, 
	IN KPROCESSOR_MODE mode, 
	OUT PVOID* pBaseAddr);

/*
* ж�ع����ڴ�
* IN PVOID pBaseAddr ӳ��õ��ĵ�ַ
* IN PMDL pMemMdl ָ��MDL��ָ��
* IN KPROCESSOR_MODE mode UserMode(�ں˵��û�ӳ��) �� KernelMode���û����ں�ӳ�䣩
* NTSTATUS ����ֵ��״̬��־
*/
#pragma LOCKEDCODE
NTSTATUS UnMapAndFreeMemory(IN PVOID pBaseAddr, IN PMDL PMdl, IN KPROCESSOR_MODE mode);


