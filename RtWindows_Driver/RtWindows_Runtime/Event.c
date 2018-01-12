#include "../Include/RTWindowsServices.h"

_Thread *BlockedEvent(HANDLE hObject,DWORD dwMilliseconds,DWORD *dwRet)
{
	_ProcessObject *lpEvent = hObject;
	if (lpEvent->ObjectValues.EventValues.bState)
	{
		*dwRet = WAIT_OBJECT_0;
		if (!lpEvent->ObjectValues.EventValues.bManualReset)
		{
			lpEvent->ObjectValues.EventValues.bState = FALSE;
		}
	}
	else if (!dwMilliseconds)
	{
		*dwRet = WAIT_TIMEOUT;
	}
	else
	{
		return ThreadBlocked(dwMilliseconds,&lpEvent->BlockedList);
	}
	return 0;
}

HANDLE RtCreateEventEx(LPSECURITY_ATTRIBUTES lpEventAttributes,BOOL bManualReset,BOOL bInitialState,LPCTSTR lpName,DWORD dwOption)
{
	BOOL bNewEvent = FALSE;
	if (s_SystemManager.lpSystemTable)
	{
		_ProcessObject *lpEvent = 0;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpName)
		{
			lpEvent = FindProcessObject(lpName,&s_SystemManager.lpSystemTable->EventList);
			if (lpEvent)
			{
				lpEvent->nRefCount++;
			}
			else
			{
				if (!SearchObject(EVENT_OBJECT,lpName))
				{
					bNewEvent = TRUE;
				}
			}
		}
		else
		{
			bNewEvent = TRUE;
		}
		if (bNewEvent)
		{
			lpEvent = GetNewProcessObject(lpEventAttributes,lpName);
			lpEvent->nHandleClass = EVENT_OBJECT;
			lpEvent->dwOption	  = dwOption;

			lpEvent->ObjectValues.EventValues.bManualReset	= bManualReset;
			lpEvent->ObjectValues.EventValues.bState		= bInitialState;

			ListAddTail(&lpEvent->ObjectListItem,&s_SystemManager.lpSystemTable->EventList);
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpEvent)
		{
			return GetObjectHandle(EVENT_OBJECT,lpEvent);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return CreateEvent(lpEventAttributes,bManualReset,bInitialState,lpName);
	}
}

HANDLE RtCreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes,BOOL bManualReset,BOOL bInitialState,LPCTSTR lpName)
{
	return RtCreateEventEx(lpEventAttributes,bManualReset,bInitialState,lpName,0);
}

HANDLE RtOpenEvent(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCTSTR lpName)
{
	if (s_SystemManager.lpSystemTable)
	{
		_ProcessObject *lpEvent;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpEvent = FindProcessObject(lpName,&s_SystemManager.lpSystemTable->EventList);
		if (lpEvent)
		{
			lpEvent->nRefCount++;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpEvent)
		{
			return GetObjectHandle(EVENT_OBJECT,lpEvent);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return OpenEvent(dwDesiredAccess,bInheritHandle,lpName);
	}
}

BOOL EventSetEvent(HANDLE hEvent,BOOL bLock)
{
	if (s_SystemManager.lpSystemTable)
	{
		_ObjectHandle *lpObjectHandle = hEvent;
		_ProcessObject *lpEvent;
		_Thread  *lpThread,*lpNextThread;
		int nPriority = MAX_PRIORITY_VALUE;

		BOOL bRet = FALSE;

		if (CheckObjectHandle(lpObjectHandle))
		{
			if (lpObjectHandle->nHandleClass == EVENT_OBJECT)
			{
				lpEvent = lpObjectHandle->hObjectHandle;

				if (!bLock)
				{
					SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
				}

				lpEvent->ObjectValues.EventValues.bState = TRUE;
				ListForEachEntrySafe(_Thread,lpThread,lpNextThread,&lpEvent->BlockedList,BlockedListItem)
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
				}
				if (nPriority != MAX_PRIORITY_VALUE)
				{
					if (!lpEvent->ObjectValues.EventValues.bManualReset)
					{
						lpEvent->ObjectValues.EventValues.bState = FALSE;
					}
				}
				if (!bLock)
				{
					if (!Preemptive(nPriority))
					{
						SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
					}
				}
				bRet = TRUE;				
			}
		}
		return bRet;
	}
	else
	{
		return SetEvent(hEvent);
	}
}

BOOL RtSetEvent(HANDLE hEvent)
{
	return EventSetEvent(hEvent,FALSE);
}

BOOL RtResetEvent(HANDLE hEvent)
{
	if (s_SystemManager.lpSystemTable)
	{
		_ObjectHandle *lpObjectHandle = hEvent;
		_ProcessObject *lpEvent;
		BOOL bRet = FALSE;

		if (CheckObjectHandle(lpObjectHandle))
		{
			if (lpObjectHandle->nHandleClass == EVENT_OBJECT)
			{
				lpEvent = lpObjectHandle->hObjectHandle;

				SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

				lpEvent->ObjectValues.EventValues.bState = FALSE;
				
				SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

				bRet = TRUE;
			}
		}
		return bRet;
	}
	else
	{
		return ResetEvent(hEvent);
	}
}

BOOL RtPulseEvent(HANDLE hEvent)
{
	if (s_SystemManager.lpSystemTable)
	{
		_ObjectHandle *lpObjectHandle = hEvent;
		_ProcessObject *lpEvent;
		_Thread  *lpThread,*lpNextThread;
		int nPriority = MAX_PRIORITY_VALUE;
		BOOL bRet = FALSE;

		if (CheckObjectHandle(lpObjectHandle))
		{
			if (lpObjectHandle->nHandleClass == EVENT_OBJECT)
			{
				lpEvent = lpObjectHandle->hObjectHandle;

				SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

				lpEvent->ObjectValues.EventValues.bState = FALSE;
				ListForEachEntrySafe(_Thread,lpThread,lpNextThread,&lpEvent->BlockedList,BlockedListItem)
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
					if (!lpEvent->ObjectValues.EventValues.bManualReset)
					{
						break;
					}
				}			
				if (!Preemptive(nPriority))
				{
					SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
				}
				bRet = TRUE;
			}
		}
		return bRet;
	}
	else
	{
		return PulseEvent(hEvent);
	}
}
