#include "../Include/RTWindowsServices.h"

_Thread *BlockedSemaphore(HANDLE hObject,DWORD dwMilliseconds,DWORD *dwRet)
{
	_ProcessObject *lpSemaphore = hObject;

	if (lpSemaphore->ObjectValues.SemaphoreValues.nSemaphoreCount > 0)
	{
		*dwRet = WAIT_OBJECT_0;
		lpSemaphore->ObjectValues.SemaphoreValues.nSemaphoreCount--;
	}
	else if (!dwMilliseconds)
	{
		*dwRet = WAIT_TIMEOUT;
	}
	else
	{
		return ThreadBlocked(dwMilliseconds,&lpSemaphore->BlockedList);
	}
	return 0;
}

HANDLE RtCreateSemaphoreEx(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
												LONG lInitialCount,LONG lMaximumCount,LPCTSTR lpName,DWORD dwOption)
{
	BOOL bNewSemaphore = FALSE;

	if (s_SystemManager.lpSystemTable)
	{
		_ProcessObject *lpSemaphore = 0;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpName)
		{
			lpSemaphore = FindProcessObject(lpName,&s_SystemManager.lpSystemTable->SemaphoreList);
			if (lpSemaphore)
			{
				lpSemaphore->nRefCount++;
			}
			else
			{
				if (!SearchObject(SEMAPHORE_OBJECT,lpName))
				{
					bNewSemaphore = TRUE;
				}
			}
		}
		else
		{
			bNewSemaphore = TRUE;
		}

		if (bNewSemaphore)
		{
			lpSemaphore = GetNewProcessObject(lpSemaphoreAttributes,lpName);
			lpSemaphore->nHandleClass = SEMAPHORE_OBJECT;
			lpSemaphore->dwOption	  = dwOption;

			lpSemaphore->ObjectValues.SemaphoreValues.nSemaphoreCount	= lInitialCount;
			lpSemaphore->ObjectValues.SemaphoreValues.nMaximumCount		= lMaximumCount;

			ListAddTail(&lpSemaphore->ObjectListItem,&s_SystemManager.lpSystemTable->SemaphoreList);
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpSemaphore)
		{
			return GetObjectHandle(SEMAPHORE_OBJECT,lpSemaphore);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return CreateSemaphore(lpSemaphoreAttributes,lInitialCount,lMaximumCount,lpName);
	}
}

HANDLE RtCreateSemaphore(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
												LONG lInitialCount,LONG lMaximumCount,LPCTSTR lpName)
{
	return RtCreateSemaphoreEx(lpSemaphoreAttributes,lInitialCount,lMaximumCount,lpName,0);
}

HANDLE RtOpenSemaphore(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCTSTR lpName)
{
	if (s_SystemManager.lpSystemTable)
	{
		_ProcessObject *lpSemaphore;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpSemaphore = FindProcessObject(lpName,&s_SystemManager.lpSystemTable->SemaphoreList);
		if (lpSemaphore)
		{
			lpSemaphore->nRefCount++;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpSemaphore)
		{
			return GetObjectHandle(SEMAPHORE_OBJECT,lpSemaphore);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return OpenSemaphore(dwDesiredAccess,bInheritHandle,lpName);
	}
}

BOOL SemaphoreReleaseSemaphore(HANDLE hSemaphore,LONG lReleaseCount,LPLONG lpPreviousCount,BOOL bLock)
{
	if (s_SystemManager.lpSystemTable)
	{
		_ObjectHandle *lpObjectHandle = hSemaphore;
		_ProcessObject *lpSemaphore;
		BOOL bRet = FALSE;
		_Thread  *lpThread,*lpNextThread;
		int nPriority = MAX_PRIORITY_VALUE;

		if (CheckObjectHandle(lpObjectHandle))
		{
			if (lReleaseCount > 0)
			{
				if (lpObjectHandle->nHandleClass == SEMAPHORE_OBJECT)
				{
					lpSemaphore = lpObjectHandle->hObjectHandle;

					if (!bLock)
					{
						SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
					}
					if (lpSemaphore->ObjectValues.SemaphoreValues.nSemaphoreCount + lReleaseCount <= 
																	lpSemaphore->ObjectValues.SemaphoreValues.nMaximumCount)
					{
						if (lpPreviousCount)
						{
							*lpPreviousCount = lpSemaphore->ObjectValues.SemaphoreValues.nSemaphoreCount;
						}
						lpSemaphore->ObjectValues.SemaphoreValues.nSemaphoreCount += lReleaseCount;
						ListForEachEntrySafe(_Thread,lpThread,lpNextThread,&lpSemaphore->BlockedList,BlockedListItem)
						{
							ListDel(&lpThread->BlockedListItem);
							ListDel(&lpThread->StateListItem);
							ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->ReadyList[lpThread->nPriority]);

							lpThread->nState							= READY;
							lpThread->nUnBlockState						= WAIT_OBJECT_0;
							lpThread->bSuspended						= 1;
							if (lpThread->nPriority < nPriority)
							{
								nPriority = lpThread->nPriority;
							}
							lpSemaphore->ObjectValues.SemaphoreValues.nSemaphoreCount--;
							if (!bLock && !lpSemaphore->ObjectValues.SemaphoreValues.nSemaphoreCount)
							{
								break;
							}
						}
						bRet = TRUE;
					}
					else
					{
						bRet = FALSE;
					}
					if (!bLock)
					{
						if (!Preemptive(nPriority))
						{
							SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
						}
					}
				}
			}
		}
		return bRet;
	}
	else
	{
		return ReleaseSemaphore(hSemaphore,lReleaseCount,lpPreviousCount);
	}
}

BOOL RtReleaseSemaphore(HANDLE hSemaphore,LONG lReleaseCount,LPLONG lpPreviousCount)
{
	return SemaphoreReleaseSemaphore(hSemaphore,lReleaseCount,lpPreviousCount,FALSE);
}