/************************************************************************
* �ļ�˵����DPC���ݽṹ�ͺ�������
* ���ߣ����
* ʱ�䣺2012��11��9��
*************************************************************************/

/***********************************ͷ�ļ�**************************************/
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

/***********************************ϵͳ��������***********************************************/
//��ǰPDC
#pragma LOCKEDDATA
extern KDPC schedulingDpc;

//DPC������
#pragma LOCKEDDATA
extern KSPIN_LOCK dpc_spinlock;

//Dpc���̴������Ⱥ�����
extern KDEFERRED_ROUTINE  SchedulDpc;

//APIC����
extern unsigned long apic_period;

/***********************************��������***************************************************/
//��ʼ������
VOID InitScheduler();