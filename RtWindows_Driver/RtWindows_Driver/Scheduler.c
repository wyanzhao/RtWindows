/***********************************头文件**************************************/
#include "../Include/Scheduler.h"
#include "../Include/SysManager.h"
#include "../Include/Interrupt.h"

/***********************************系统变量定义***********************************************/
//调度dpc列表
#pragma LOCKEDDATA
KDPC schedulingDpc;

//记录当前系统时间
#pragma LOCKEDDATA
static SYSTEM_TIME_TYPE nCurrentTime;

//自旋锁
#pragma LOCKEDDATA
KSPIN_LOCK dpc_spinlock;

/***********************************函数定义***********************************************/
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
