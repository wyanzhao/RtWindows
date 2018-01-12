/***********************************ͷ�ļ�**************************************/
#include "../Include/Scheduler.h"
#include "../Include/SysManager.h"
#include "../Include/Interrupt.h"

/***********************************ϵͳ��������***********************************************/
//����dpc�б�
#pragma LOCKEDDATA
KDPC schedulingDpc;

//��¼��ǰϵͳʱ��
#pragma LOCKEDDATA
static SYSTEM_TIME_TYPE nCurrentTime;

//������
#pragma LOCKEDDATA
KSPIN_LOCK dpc_spinlock;

/***********************************��������***********************************************/
#pragma LOCKEDCODE
VOID InitScheduler()
{
	KeInsertQueueDpc(&schedulingDpc, NULL, NULL);
}

#pragma LOCKEDCODE
VOID SchedulDpc(IN PKDPC pDpc, IN PVOID pContext, IN PVOID SysArg1, IN PVOID SysArg2)
{
	s_SystemManager.lpSystemTable->dwSystemTime = s_SystemManager.lpSystemTable->dwSystemTime + apic_period;
	nCurrentTime = s_SystemManager.lpSystemTable->dwSystemTime;

	if (TrySpinLock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection))
	{
		KeAcquireSpinLockAtDpcLevel(&dpc_spinlock);
		RtScheduler(s_SystemManager.lpSystemTable, &s_SystemFunc, nCurrentTime);
		KeReleaseSpinLockFromDpcLevel(&dpc_spinlock);
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
	else
	{
		s_SystemManager.lpSystemTable->bReSchedule = 1;
	}
}
