#pragma once
/************************************************************************
* �ļ�˵�����ں������ó������Ե�Ժ���
* ���ߣ����Ʒ�
* ʱ�䣺2011��12��13��
* �޸ģ��½���
* ʱ�䣺2012��6��1��
* �޸ģ�������
* ʱ�䣺2016��3��5��
*************************************************************************/

/***********************************ͷ�ļ�**************************************/
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

//�����붨�壬��������ϵͳ�汾�����ݽṹƫ������ͬ��������WinDbg����������dt _EPROCESS
//�õ�EPROCESS�ڸ����ݽṹ��ƫ����

//�����붨��
//ƫ��λ��0x0b8 ActiveProcessLinks : _LIST_ENTRY��ϵͳ�е�ȫ�ֽ����б���ѭ������ṹ
#define EPROCESS_ACTIVELIST_OFFSET 0x0b8

//ƫ��λ��0x02c ThreadListHead   : _LIST_ENTRY�������̵��߳�(KTHREAD)����
#define KPROCESS_THREADLISTHEAD 0x02c

//ƫ��λ��0x038 Affinity         : _KAFFINITY_EX��Ĭ���׺ʹ���������
#define KPROCESS_AFFINITY_OFFSET 0x038

//ƫ��λ��0x1e0 ThreadListEntry  : _LIST_ENTRY��
//����KPROCESS�е��̶߳��о����߳̽ṹ���׵�ַ��ƫ����
#define KTHREAD_LISTENTRY_OFFSET 0x1e0 

//ƫ��λ��0x154 Affinity         : KAFFINITY����Ե��
#define KTHREAD_AFFINITY_OFFSET 0x154

//ʵʱ�Զ���
#define UNRTWORKAFFINITY 0x0E
#define RTWORKAFFINITY   0x01
#define SYSTEMAFFINITY   0x0F

//ǰһ������ȡ����ַ���ֵ����һ������ֻ�õ�Ŀ���ַ
#define GET_PTRVALUE(type,baseaddr,offset) \
	*((type *)((PUCHAR)(baseaddr) + offset))
#define GET_PTR(type,baseaddr,offset) \
	(type )((PUCHAR)(baseaddr) + offset)

/***********************************ϵͳ��������***********************************************/
//��Ե��
#pragma LOCKEDDATA
extern ULONG s_systemAffinity;
//ϵͳ��Ե�����ü���
#pragma LOCKEDDATA
extern unsigned long cAffinityCount;

/***********************************��������***************************************************/
/*
* �����߳���Ե��
* PETHREAD �߳̽ṹ��
* BOOLEAN ����ֵ �Ƿ�ɹ�
*/
#pragma LOCKEDCODE
BOOLEAN RtSetThreadAffinity(PETHREAD eThread, ULONG cpuMask);

/*
* ���ý���ʵʱ��Ե��
* HANDLE ���̾��
* BOOLEAN ����ֵ �Ƿ�ɹ�
*/
#pragma LOCKEDCODE
BOOLEAN RtSetProcessAffinity(PEPROCESS eProcess, ULONG cpuMask);

/*
* ����ϵͳ��Ե�ԣ����ı��idle system���̵����н�����Ե��
* cpuMask ��Ե������
* BOOLEAN ����ֵ �Ƿ�ɹ�
*/
#pragma LOCKEDCODE
BOOLEAN RtSetSystemAffinity(ULONG cpuMask);


