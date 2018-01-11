/***********************************头文件**************************************/
#include "../Include/RTWindowsScheduler.h"
#include "../Include/RTWindowsServices.h"
#include "../Include/RTWindowsRuntime.h"

/***********************************系统变量定义***********************************************/
_SystemManager		s_SystemManager = { 0 };
_SystemFunc			s_SystemFunc = { 0 };
_TimeManager			s_TimeManager = { 0 };

/***********************************函数定义***************************************************/
#pragma LOCKEDCODE
void SpinLock(_SystemFunc *lpSystemFunc, LONG *SpinLockSection)
{
	while (*SpinLockSection)
	{
		lpSystemFunc->CoreOSDelay();
	}
	INTERLOCKED_INCREMENT(SpinLockSection);
	while (*SpinLockSection > 1)
	{
		INTERLOCKED_DECREMENT(SpinLockSection);
		do
		{
			lpSystemFunc->CoreOSDelay();
		} while (*SpinLockSection);
		INTERLOCKED_INCREMENT(SpinLockSection);
	}
	while (*SpinLockSection != 1)
	{
		lpSystemFunc->CoreOSDelay();
	}
}

/***********************************函数定义***************************************************/
#pragma LOCKEDCODE
int TrySpinLock(long *SpinLockSection)
{
	INTERLOCKED_INCREMENT(SpinLockSection);
	if (*SpinLockSection > 1)
	{
		INTERLOCKED_DECREMENT(SpinLockSection);
		return 0;
	}
	else
	{
		return 1;
	}
}

#pragma LOCKEDCODE
void SpinUnlock(long *SpinLockSection)
{
	INTERLOCKED_DECREMENT(SpinLockSection);
}

#pragma LOCKEDCODE
void InitSysytemManager(_SystemManager *lpSystemManager, int bCreate)
{
	if (bCreate)
	{
		int i;
		lpSystemManager->lpSystemTable->nMemHead = sizeof(_SystemTable);

		lpSystemManager->lpSystemTable->SystemSpinLockSection = 0;
		lpSystemManager->lpSystemTable->bReSchedule = 0;
		lpSystemManager->lpSystemTable->nSystemScheduleTime = 0;

		for (i = 0; i<MAX_CPU_NUM; i++)
		{
			lpSystemManager->lpSystemTable->lpRunningThread[i] = 0;
		}
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->ProcessList);
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->ThreadList);

		for (i = 0; i<MAX_PRIORITY_VALUE; i++)
		{
			INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->ReadyList[i]);
		}
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->WaitingList);
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->SuspendedList);
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->YieldList);
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->ExitList);

		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->EventList);
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->MutexList);
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->SemaphoreList);
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->MsgQueueList);

		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->ProcessIdleList);
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->ThreadIdleList);
		INIT_LIST_HEAD(&lpSystemManager->lpSystemTable->ObjectIdleList);

		lpSystemManager->lpSystemTable->nBlockNum = 0;
		lpSystemManager->lpSystemTable->nBlockedNum = 0;
		lpSystemManager->lpSystemTable->nWaitingNum = 0;
		lpSystemManager->lpSystemTable->nReadyNum = 0;
		lpSystemManager->lpSystemTable->nSuspendedNum = 0;
		lpSystemManager->lpSystemTable->nTimeoutNum = 0;
		lpSystemManager->lpSystemTable->nPreemptiveNum = 0;
		lpSystemManager->lpSystemTable->nScheduleNum = 0;
		lpSystemManager->lpSystemTable->nObjectHandleNum = 0;
	}
	lpSystemManager->hScheduleEvent = 0;
	lpSystemManager->bStartup = 0;
}

#pragma LOCKEDCODE
static void WaitingListInsert(_SystemTable *lpSystemTable, _Thread *lpWaitingThread)
{
	_Thread *lpThread;
	int bInsert = 0;

	ListForEachEntry(_Thread, lpThread, &lpSystemTable->WaitingList, StateListItem)
	{
		if (lpThread->dwReleaseTime > lpWaitingThread->dwReleaseTime)
		{
			bInsert = 1;
			break;
		}
	}
	if (bInsert)
	{
		ListInsert(&lpWaitingThread->StateListItem, &lpThread->StateListItem);
	}
	else
	{
		ListAddTail(&lpWaitingThread->StateListItem, &lpSystemTable->WaitingList);
	}
}

#pragma LOCKEDCODE
BOOL SchedulerPreemptive(_SystemTable *lpSystemTable, _SystemFunc *lpSystemFunc, _Thread *lpThread)
{
	int i, k = -1;
	_Thread *lpSuspendedThread;
	int nPriority = 0;

	for (i = 0; i<MAX_CPU_NUM; i++)
	{
			if (!lpSystemTable->lpRunningThread[i])
			{
				k = i;
				break;
			}
			else if (!lpSystemTable->lpRunningThread[i]->nPreemptionLock && lpSystemTable->lpRunningThread[i]->nPriority > lpThread->nPriority)
			{
				if (nPriority < lpSystemTable->lpRunningThread[i]->nPriority)
				{
					nPriority = lpSystemTable->lpRunningThread[i]->nPriority;
					k = i;
				}
			}
		
	}
	if (k >= 0)
	{
		if (lpSystemTable->lpRunningThread[k])
		{
			lpSuspendedThread = (_Thread *)lpSystemTable->lpRunningThread[k];
			lpSuspendedThread->nState = READY;
			k = s_SystemFunc.SuspendThread(lpSuspendedThread->hSchedulerHandle);
			ListAddTail(&lpSuspendedThread->StateListItem, &lpSystemTable->ReadyList[lpSuspendedThread->nPriority]);
			lpSystemTable->nPreemptiveNum++;
		}

		ListDel(&lpThread->StateListItem);

		lpThread->nState = RUNNING;
		lpThread->bSuspended = 1;
		if (!lpThread->hSchedulerHandle)
		{
			lpThread->hSchedulerHandle = s_SystemFunc.GetThreadHandle(lpThread->dwThreadID);
		}
		s_SystemFunc.ResumeThread(lpThread->hSchedulerHandle);
		lpSystemTable->lpRunningThread[k] = lpThread;
		lpSystemTable->nScheduleNum++;
		return TRUE;
	}
	return FALSE;
}

#pragma LOCKEDCODE
void RtScheduler(_SystemTable *lpSystemTable, _SystemFunc *lpSystemFunc, SYSTEM_TIME_TYPE nCurrentTime)
{
	int i;
	_Thread *lpThread, *lpNextThread;

	lpSystemTable->bReSchedule = 0;
	lpSystemTable->nSystemScheduleTime = 0;

	if (!ListEmpty(&lpSystemTable->ExitList)) //终止线程
	{
		ListForEachEntrySafe(_Thread, lpThread, lpNextThread, &lpSystemTable->ExitList, StateListItem)
		{
			
			lpThread->bSuspended = 1;
			ListDel(&lpThread->StateListItem);

			ListAddTail(&lpThread->StateListItem, &lpSystemTable->ThreadIdleList);
			for (i = 0; i<MAX_CPU_NUM; i++)
			{
				if (lpSystemTable->lpRunningThread[i] == lpThread)
				{
					lpSystemTable->lpRunningThread[i] = 0;
					break;
				}
			}
		}
	}
	//让出CPU
	if (!ListEmpty(&lpSystemTable->YieldList))
	{
		ListForEachEntrySafe(_Thread, lpThread, lpNextThread, &lpSystemTable->YieldList, StateListItem)
		{	
			if (!lpThread->bSuspended)
			{
				if (!lpThread->hSchedulerHandle)
				{
					lpThread->hSchedulerHandle = s_SystemFunc.GetThreadHandle(lpThread->dwThreadID);
				}
				s_SystemFunc.SuspendThread(lpThread->hSchedulerHandle);
				for (i = 0; i<MAX_CPU_NUM; i++)
				{
					if (lpSystemTable->lpRunningThread[i] == lpThread)
					{
						lpSystemTable->lpRunningThread[i] = 0;
						break;
					}
				}
			}
			ListDel(&lpThread->StateListItem);
			switch (lpThread->nState)
			{
			case SUSPENDED:
				ListAddTail(&lpThread->StateListItem, &lpSystemTable->SuspendedList);
				lpSystemTable->nSuspendedNum++;
				break;
			case WAITING:
				WaitingListInsert(lpSystemTable, lpThread);
				lpSystemTable->nWaitingNum++;
				break;
			case READY:
				ListAddTail(&lpThread->StateListItem, &lpSystemTable->ReadyList[lpThread->nPriority]);
				lpSystemTable->nReadyNum++;
				break;
			case BLOCKED:
				lpSystemTable->nBlockedNum++;
				break;
			}
		}
	}

	ListForEachEntrySafe(_Thread, lpThread, lpNextThread, &lpSystemTable->WaitingList, StateListItem)
	{	
		if (lpThread->dwReleaseTime <= nCurrentTime)
		{ 
			lpThread->nState = READY;
			lpThread->bSuspended = 1;
			lpThread->nUnBlockState = WAIT_TIMEOUT;
			ListDel(&lpThread->BlockedListItem);
			ListDel(&lpThread->StateListItem);
			ListAddTail(&lpThread->StateListItem, &lpSystemTable->ReadyList[lpThread->nPriority]);
			lpSystemTable->nTimeoutNum++;
		}
		else
		{
			lpSystemTable->nSystemScheduleTime = lpThread->dwReleaseTime;
			break;
		}
	}

	for (i = 0; i<MAX_PRIORITY_VALUE; i++)
	{
		ListForEachEntrySafe(_Thread, lpThread, lpNextThread, &lpSystemTable->ReadyList[i], StateListItem)
		{
			if (!SchedulerPreemptive(lpSystemTable, lpSystemFunc, lpThread))
			{  
				return;
			}
		}
	}
}
