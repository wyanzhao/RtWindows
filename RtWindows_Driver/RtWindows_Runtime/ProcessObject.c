#include "../Include/RTWindowsServices.h"

#define _CRT_SECURE_NO_WARNINGS

_ProcessObject *GetNewProcessObject(LPSECURITY_ATTRIBUTES lpObjectAttributes,LPCTSTR lpName)
{
	_ProcessObject *lpObject;

	if (!ListEmpty(&s_SystemManager.lpSystemTable->ObjectIdleList))
	{
		lpObject = list_entry(s_SystemManager.lpSystemTable->ObjectIdleList.Next,_ProcessObject,ObjectListItem);
		ListDel(&lpObject->ObjectListItem);
	}
	else
	{
		lpObject = (_ProcessObject *)GetSystemMem(sizeof(_ProcessObject));
	}

	memset(lpObject,0,sizeof(_ProcessObject));

	if (lpName)
	{
		strncpy(lpObject->szName,lpName,MAX_PATH);
	}
	else
	{
		lpObject->szName[0] = 0;
	}
	if (lpObjectAttributes)
	{
		memcpy(&lpObject->ObjectAttributes,lpObjectAttributes,sizeof(SECURITY_ATTRIBUTES));
	}
	else
	{
		memset(&lpObject->ObjectAttributes,0,sizeof(SECURITY_ATTRIBUTES));
	}
	lpObject->nRefCount								= 1;

	EMPTY_LIST_HEAD(&lpObject->ObjectListItem);
	INIT_LIST_HEAD(&lpObject->BlockedList);

	return lpObject;
}

BOOL CheckObjectHandle(HANDLE hObjectHandle)
{
	long nHandle = (long)hObjectHandle;

	if (nHandle > 0x1000)
	{
		_ObjectHandle *lpObjectHandle = hObjectHandle;
		__try
		{
			if (lpObjectHandle->nMagic == 0x52744F53)
			{
				return TRUE;
			}
		}
		__except(SEHFiler(GetExceptionCode()))
		{
		}
	}
	return FALSE;
}

_ObjectHandle *GetObjectHandle(int nClass,_ProcessObject *lpProcessObject)
{
	_ObjectHandle *lpObjectHandle;

	lpObjectHandle					= (_ObjectHandle *)malloc(sizeof(_ObjectHandle));
	lpObjectHandle->nMagic			= 0x52744F53;
	lpObjectHandle->nHandleClass	= nClass;
	lpObjectHandle->hObjectHandle	= lpProcessObject;

	EMPTY_LIST_HEAD(&lpObjectHandle->OwnerListItem);

	ListAddTail(&lpObjectHandle->OwnerListItem,&s_SystemManager.lpProcess->ObjectHandleList);

	s_SystemManager.lpSystemTable->nObjectHandleNum++;

	return lpObjectHandle;
}

_ProcessObject *FindProcessObject(LPCTSTR lpName,LIST_HEAD *lpObjectList)
{
	_ProcessObject *lpProcessObject;

	ListForEachEntry(_ProcessObject,lpProcessObject,lpObjectList,ObjectListItem)
	{
		if (!strcmp(lpProcessObject->szName,lpName))
		{
			return lpProcessObject;
		}
	}
	return 0;
}

BOOL SearchObject(int nClass,LPCTSTR lpName)
{
	if (nClass != EVENT_OBJECT)
	{
		if (FindProcessObject(lpName,&s_SystemManager.lpSystemTable->EventList))
		{
			return TRUE;
		}
	}
	if (nClass != MUTEX_OBJECT)
	{
		if (FindProcessObject(lpName,&s_SystemManager.lpSystemTable->MutexList))
		{
			return TRUE;
		}
	}
	if (nClass != SEMAPHORE_OBJECT)
	{
		if (FindProcessObject(lpName,&s_SystemManager.lpSystemTable->SemaphoreList))
		{
			return TRUE;
		}
	}
	if (nClass != MSG_QUEUE_OBJECT)
	{
		if (FindProcessObject(lpName,&s_SystemManager.lpSystemTable->MsgQueueList))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Preemptive(int nPriority)
{
	_Thread *lpThread;
	DWORD dwThreadID;
	BOOL bPreemptived = FALSE;

	if (nPriority < MAX_PRIORITY_VALUE)
	{
		dwThreadID  = GetCurrentThreadId();
		ListForEachEntry(_Thread,lpThread,&s_SystemManager.lpProcess->ProcessThreadList,ProcessThreadListItem)
		{
			if (lpThread->dwThreadID == dwThreadID)
			{
				if (!lpThread->nPreemptionLock && lpThread->nPriority > nPriority)
				{
					lpThread->nState = READY;
					lpThread->bSuspended							= 0;
					ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->YieldList);

					s_SystemManager.lpSystemTable->bReSchedule		= 1;
					bPreemptived = TRUE;
				}
				break;
			}
		}
		if (bPreemptived)
		{
			SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

			s_SystemFunc.ReSchedule();
			while (!lpThread->bSuspended)
			{
				s_SystemFunc.CoreOSDelay();
			}
		}
	}
	return bPreemptived;
}

DWORD RtWaitForSingleObject(HANDLE hHandle,DWORD dwMilliseconds)
{
	_ObjectHandle *lpObjectHandle = hHandle;
	_Thread  *lpThread = 0;

	DWORD dwRet = 0;

	if (CheckObjectHandle(lpObjectHandle))
	{
		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		switch (lpObjectHandle->nHandleClass)
		{
			case EVENT_OBJECT:
				lpThread = BlockedEvent(lpObjectHandle->hObjectHandle,dwMilliseconds,&dwRet);
				break;
			case MUTEX_OBJECT:
				lpThread = BlockedMutex(lpObjectHandle->hObjectHandle,dwMilliseconds,&dwRet);
				break;
			case SEMAPHORE_OBJECT:
				lpThread = BlockedSemaphore(lpObjectHandle->hObjectHandle,dwMilliseconds,&dwRet);
				break;
			case MSG_QUEUE_OBJECT:
				lpThread = 0;
				break;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpThread)
		{
			s_SystemFunc.ReSchedule();
			while (!lpThread->bSuspended)
			{
				s_SystemFunc.CoreOSDelay();
			}
			dwRet = lpThread->nUnBlockState;
		}
		return dwRet;
	}
	else
	{
		return WaitForSingleObject(hHandle,dwMilliseconds);
	}
}

BOOL CloseObjectHandle(HANDLE hObject,BOOL bLock)
{
	_ObjectHandle *lpObjectHandle = hObject;
	_ProcessObject *LpProcessObject;

	if (CheckObjectHandle(lpObjectHandle))
	{
		if (!bLock)
		{
			SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		}
		ListDel(&lpObjectHandle->OwnerListItem);

		LpProcessObject = lpObjectHandle->hObjectHandle;

		LpProcessObject->nRefCount--;
		if (!LpProcessObject->nRefCount)
		{
			switch (lpObjectHandle->nHandleClass)
			{
				case EVENT_OBJECT:
					EventSetEvent(lpObjectHandle,TRUE);
					break;
				case MUTEX_OBJECT:
					MutexReleaseMutex(lpObjectHandle,TRUE);
					break;
				case SEMAPHORE_OBJECT:
					SemaphoreReleaseSemaphore(lpObjectHandle,1,0,TRUE);
					break;
				case MSG_QUEUE_OBJECT:
					MsgQueueReleaseAll(lpObjectHandle);
					break;
			}
			ListDel(&LpProcessObject->ObjectListItem);
			ListAddTail(&LpProcessObject->ObjectListItem,&s_SystemManager.lpSystemTable->ObjectIdleList);
		}

		if (!bLock)
		{
			SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		}
		free(lpObjectHandle);
		s_SystemManager.lpSystemTable->nObjectHandleNum--;
		return TRUE;
	}
	else
	{
		return CloseHandle(hObject);
	}
}

void RtReleaseObject(HANDLE hObject)
{
	_ObjectHandle *lpObjectHandle = hObject;

	if (CheckObjectHandle(lpObjectHandle))
	{
		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		switch (lpObjectHandle->nHandleClass)
		{
			case EVENT_OBJECT:
				EventSetEvent(lpObjectHandle,TRUE);
				break;
			case MUTEX_OBJECT:
				MutexReleaseMutex(lpObjectHandle,TRUE);
				break;
			case SEMAPHORE_OBJECT:
				SemaphoreReleaseSemaphore(lpObjectHandle,1,0,TRUE);
				break;
			case MSG_QUEUE_OBJECT:
				MsgQueueReleaseAll(lpObjectHandle);
				break;
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
}

BOOL RtCloseHandle(HANDLE hObject)
{
	return CloseObjectHandle(hObject,FALSE);
}
