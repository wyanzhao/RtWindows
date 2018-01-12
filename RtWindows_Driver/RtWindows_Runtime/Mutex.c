#include "../Include/RTWindowsServices.h"

_Thread *BlockedMutex(HANDLE hObject,DWORD dwMilliseconds,DWORD *dwRet)
{
	_ProcessObject *lpMutex = hObject;

	if (!lpMutex->ObjectValues.MutexValues.dwThreadID)
	{
		lpMutex->ObjectValues.MutexValues.dwThreadID = GetCurrentThreadId();
		lpMutex->ObjectValues.MutexValues.nMutexCount++;
		*dwRet = WAIT_OBJECT_0;
	}
	else if (lpMutex->ObjectValues.MutexValues.dwThreadID == GetCurrentThreadId())
	{
		lpMutex->ObjectValues.MutexValues.nMutexCount++;
		*dwRet = WAIT_OBJECT_0;
	}
	else if (!dwMilliseconds)
	{
		*dwRet = WAIT_TIMEOUT;
	}
	else
	{
		return ThreadBlocked(dwMilliseconds,&lpMutex->BlockedList);
	}
	return 0;
}

HANDLE RtCreateMutexEx(LPSECURITY_ATTRIBUTES lpMutexAttributes,BOOL bInitialOwner,LPCTSTR lpName,DWORD dwOption)
{
	BOOL bNewMutex = FALSE;
	if (s_SystemManager.lpSystemTable)
	{
		_ProcessObject *lpMutex = 0;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpName)
		{
			lpMutex = FindProcessObject(lpName,&s_SystemManager.lpSystemTable->EventList);
			if (lpMutex)
			{
				lpMutex->nRefCount++;
			}
			else
			{
				if (!SearchObject(MUTEX_OBJECT,lpName))
				{
					bNewMutex = TRUE;
				}
			}
		}
		else
		{
			bNewMutex = TRUE;
		}
		if (bNewMutex)
		{
			lpMutex = GetNewProcessObject(lpMutexAttributes,lpName);
			lpMutex->dwOption	  = dwOption;
			lpMutex->nHandleClass = MUTEX_OBJECT;

			lpMutex->ObjectValues.MutexValues.nMutexCount		= 0;
			if (bInitialOwner)
			{
				lpMutex->ObjectValues.MutexValues.dwThreadID	= GetCurrentThreadId();
				lpMutex->ObjectValues.MutexValues.nMutexCount++;
			}
			else
			{
				lpMutex->ObjectValues.MutexValues.dwThreadID	= 0;
			}

			ListAddTail(&lpMutex->ObjectListItem,&s_SystemManager.lpSystemTable->MutexList);
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpMutex)
		{
			return GetObjectHandle(MUTEX_OBJECT,lpMutex);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return CreateMutex(lpMutexAttributes,bInitialOwner,lpName);
	}
}

HANDLE RtCreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes,BOOL bInitialOwner,LPCTSTR lpName)
{
	return RtCreateMutexEx(lpMutexAttributes,bInitialOwner,lpName,0);
}

HANDLE RtOpenMutex(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCTSTR lpName)
{
	if (s_SystemManager.lpSystemTable)
	{
		_ProcessObject *lpMutex;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpMutex = FindProcessObject(lpName,&s_SystemManager.lpSystemTable->MutexList);
		if (lpMutex)
		{
			lpMutex->nRefCount++;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpMutex)
		{
			return GetObjectHandle(MUTEX_OBJECT,lpMutex);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return OpenMutex(dwDesiredAccess,bInheritHandle,lpName);
	}
}

BOOL MutexReleaseMutex(HANDLE hMutex,BOOL bLock)
{
	if (s_SystemManager.lpSystemTable)
	{
		_ObjectHandle *lpObjectHandle = hMutex;
		_ProcessObject *lpMutex;
		BOOL bRet = FALSE;
		_Thread  *lpThread,*lpNextThread;
		int nPriority = MAX_PRIORITY_VALUE;

		if (CheckObjectHandle(lpObjectHandle))
		{
			if (lpObjectHandle->nHandleClass == MUTEX_OBJECT)
			{
				lpMutex = lpObjectHandle->hObjectHandle;

				if (!bLock)
				{
					SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
				}

				if (lpMutex->ObjectValues.MutexValues.dwThreadID == GetCurrentThreadId())
				{
					lpMutex->ObjectValues.MutexValues.nMutexCount--;
					if (!lpMutex->ObjectValues.MutexValues.nMutexCount)
					{
						lpMutex->ObjectValues.MutexValues.dwThreadID = 0;
						ListForEachEntrySafe(_Thread,lpThread,lpNextThread,&lpMutex->BlockedList,BlockedListItem)
						{
							ListDel(&lpThread->BlockedListItem);
							ListDel(&lpThread->StateListItem);
							ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->ReadyList[lpThread->nPriority]);

							lpThread->nState								= READY;
							lpThread->nUnBlockState							= WAIT_OBJECT_0;
							lpThread->bSuspended							= 1;

							if (lpThread->nPriority < nPriority)
							{
								nPriority = lpThread->nPriority;
							}
							lpMutex->ObjectValues.MutexValues.nMutexCount++;
							lpMutex->ObjectValues.MutexValues.dwThreadID	= lpThread->dwThreadID;

							if (!bLock)
							{
								break;
							}
						}
					}
					bRet = TRUE;
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
		return bRet;
	}
	else
	{
		return ReleaseMutex(hMutex);
	}
}

BOOL RtReleaseMutex(HANDLE hMutex)
{
	return MutexReleaseMutex(hMutex,FALSE);
}

