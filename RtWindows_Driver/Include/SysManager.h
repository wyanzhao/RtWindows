#pragma once
/************************************************************************
* �ļ�˵����ϵͳ������
* ���ߣ����
* ʱ�䣺2012��11��9��
*************************************************************************/

/***********************************ͷ�ļ�**************************************/
#include "Queue.h"
#include "RTWindowsScheduler.h"
#include "RTWindowsServices.h"
#include "RTWindowsRuntime.h"
#include "ThreadOp.h"
#include "Affinity.h"
#include "Scheduler.h"
#include "MemoryMap.h"

/***********************************ϵͳ��������***********************************************/
//�ں�̬�����ڴ��ָ��
extern char* s_pBase;
//MDL��ָ��
extern PMDL s_pMdl;
//�ڴ�ӳ�����ü���
#pragma LOCKEDDATA
extern unsigned long cMemoryCount;

/***********************************��������***********************************************/
/*
* ϵͳ��ʼ��
* BOOL ����ֵ �Ƿ�ɹ�
*/
#pragma LOCKEDCODE
BOOL  SystemInitialization();