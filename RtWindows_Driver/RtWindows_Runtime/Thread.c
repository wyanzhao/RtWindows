#define _CRT_SECURE_NO_WARNINGS

#include "../Include/RTWindowsServices.h"

DWORD TaskThread(void *arg)
{
	_Thread *lpThread = (_Thread *)arg;

	__try
	{
		lpThread->Entry(lpThread->arg);
	}
	__except(SEHFiler(GetExceptionCode()))
	{
	}
	if (!s_SystemManager.lpSystemTable)
	{
		_ThreadArgv *lpThreadArgv = (_ThreadArgv *)lpThread->arg;
		if (lpThreadArgv->nMagic == 0x52744F53 && lpThreadArgv->bAuto)
		{
			free(lpThreadArgv);
		}
		free(lpThread);
	}
	return 0;
}

_Thread *GetNewThread()
{
	_Thread *lpThread;

	SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

	if (!ListEmpty(&s_SystemManager.lpSystemTable->ThreadIdleList))
	{
		lpThread = list_entry(s_SystemManager.lpSystemTable->ThreadIdleList.Next,_Thread,StateListItem);
		ListDel(&lpThread->StateListItem);
	}
	else
	{
		lpThread = (_Thread *)GetSystemMem(sizeof(_Thread));
	}
	memset(lpThread,0,sizeof(_Thread));

	SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

	return lpThread;
}

static _Thread *FindThreadbyId(DWORD dwThreadID)
{
	_Thread  *lpThread;

	ListForEachEntry(_Thread,lpThread,&s_SystemManager.lpProcess->ProcessThreadList,ProcessThreadListItem)
	{
		if (lpThread->dwThreadID == dwThreadID)
		{
			return lpThread;
		}
	}
	return 0;
}

static _Thread *FindThreadbyHandle(HANDLE hHandle)
{
	_Thread  *lpThread;

	ListForEachEntry(_Thread,lpThread,&s_SystemManager.lpProcess->ProcessThreadList,ProcessThreadListItem)
	{
		if (lpThread->hThread == hHandle)
		{
			return lpThread;
		}
	}
	return 0;
}

void RegisteThread(_Thread *lpThread,DWORD dwCreationFlags)
{
	_Process *lpProcess;
	_ObjectHandle *lpObjectHandle;

	SetThreadPriority(lpThread->hThread,THREAD_PRIORITY_TIME_CRITICAL);

	//设定线程亲缘性
	SetThreadAffinityMask(lpThread->hThread,1);

	lpProcess				= s_SystemManager.lpProcess;

	lpThread->nSuspendCount = 0;
	lpThread->lpProcess		= PTR_OFFSET(_Process,lpProcess);

	EMPTY_LIST_HEAD(&lpThread->ProcessThreadListItem);
	EMPTY_LIST_HEAD(&lpThread->StateListItem);
	EMPTY_LIST_HEAD(&lpThread->BlockedListItem);

	lpThread->hSafeEvent	= RtCreateEventEx(0,FALSE,FALSE,0,0);
	lpObjectHandle = lpThread->hSafeEvent;
	ListDel(&lpObjectHandle->OwnerListItem);

	SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

	ListAddTail(&lpThread->ProcessThreadListItem,&lpProcess->ProcessThreadList);

	if (dwCreationFlags == CREATE_SUSPENDED)
	{
		lpThread->nState = SUSPENDED;
		lpThread->nSuspendCount++;
		ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->SuspendedList);
	}
	else
	{
		lpThread->nState = READY;
		ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->ReadyList[lpThread->nPriority]);
		s_SystemManager.lpSystemTable->bReSchedule = 1;
	}

	SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

	if (s_SystemManager.lpSystemTable->bReSchedule)
	{
		s_SystemFunc.ReSchedule();
	}
}

void RegisteMainThread(HANDLE hThread,DWORD dwThreadID)
{
	_Thread  *lpThread;

	if (hThread)
	{
		lpThread						= GetNewThread();
		lpThread->nPriority				= 0;
		lpThread->hThread				= hThread;
		lpThread->hSchedulerHandle		= s_SystemFunc.GetThreadHandle(dwThreadID);//注册主线程时，直接获取句柄。内核态通过ioctl，用户直接返回0
		//lpThread->hSchedulerHandle		= 0;
		lpThread->dwThreadID			= dwThreadID;

		RegisteThread(lpThread,0);
	}
}

void UnRegisteThread(DWORD dwThreadID,BOOL bLock)
{
	_Process *lpProcess;
	_Thread  *lpThread;

	lpProcess = s_SystemManager.lpProcess;

	if (!bLock)
	{
		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
	lpThread = FindThreadbyId(dwThreadID);

	if (lpThread)
	{
		lpThread->bSuspended = 1;
		ListDel(&lpThread->ProcessThreadListItem);
		ListDel(&lpThread->StateListItem);
		ListDel(&lpThread->BlockedListItem);
		if (bLock)
		{
			ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->ExitList);
		}
		
		EventSetEvent(lpThread->hSafeEvent,TRUE);
		CloseObjectHandle(lpThread->hSafeEvent,TRUE);
		
		s_SystemManager.lpSystemTable->bReSchedule		= 1;
		s_SystemFunc.ReSchedule();
	}
	
	if (!bLock)
	{
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		if (ListEmpty(&lpProcess->ProcessThreadList))
		{
			RtExitProcess(0);
		}
	}
}

_Thread *ThreadBlocked(DWORD dwMilliseconds,LIST_HEAD *lpBlockedList)
{
	_Thread  *lpThread;

	lpThread = FindThreadbyId(GetCurrentThreadId());
	if (lpThread)
	{
		if (dwMilliseconds == INFINITE)
		{
			lpThread->nState							= BLOCKED;
		}
		else
		{
			lpThread->nState							= WAITING;
			lpThread->dwReleaseTime						= s_SystemManager.lpSystemTable->dwSystemTime + dwMilliseconds * 1000;
		}
		ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->YieldList);
		ListAddTail(&lpThread->BlockedListItem,lpBlockedList);

		lpThread->bSuspended							= 0;
		s_SystemManager.lpSystemTable->bReSchedule		= 1;

		s_SystemManager.lpSystemTable->nBlockNum++;
		return lpThread;
	}
	return 0;
}

HANDLE RtCreateThreadEx(LPSECURITY_ATTRIBUTES lpThreadAttributes,DWORD dwStackSize,
			LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter,DWORD dwCreationFlags,LPCTSTR lpName,
																LONG nPriority,DWORD dwOption,LPDWORD lpThreadId)
{
	HANDLE hHandle;
	DWORD  dwThreadID;
	_Thread *lpThread;

	if (s_SystemManager.lpSystemTable)
	{

		lpThread		= GetNewThread();

		hHandle = CreateThread(lpThreadAttributes, dwStackSize, (LPTHREAD_START_ROUTINE)TaskThread, lpThread, CREATE_SUSPENDED, &dwThreadID);

		if (hHandle)
		{
			if (lpName)
			{
				strncpy(lpThread->szName,lpName,THREAD_NAME_LEN);
			}
			else
			{
				sprintf(lpThread->szName,"t%08x\n",dwThreadID);
			}

			lpThread->Entry				= (ENTRY)lpStartAddress;
			lpThread->arg				= lpParameter;
			lpThread->nPriority			= nPriority;
			lpThread->dwOption			= dwOption;
			lpThread->hThread			= hHandle;
			lpThread->hSchedulerHandle	= s_SystemFunc.GetThreadHandle(dwThreadID);//创建线程时，直接获取句柄。内核态通过ioctl，用户直接返回0
			//lpThread->hSchedulerHandle	= 0;
			lpThread->dwThreadID		= dwThreadID;

			RegisteThread(lpThread,dwCreationFlags);

			if (lpThreadId)
			{
				*lpThreadId = dwThreadID;
			}
		}
		return hHandle;
	}
	else
	{
		_Thread *lpThread = (_Thread *)malloc(sizeof(_Thread));
		_ThreadArgv *lpThreadArgv = (_ThreadArgv *)lpParameter;

		memset(lpThread,0,sizeof(_Thread));

		lpThread->Entry				= (ENTRY)lpStartAddress;

		if (!lpThreadArgv)
		{
			lpThreadArgv = (_ThreadArgv *)malloc(sizeof(_ThreadArgv));
			lpThreadArgv->nMagic = 0x52744F53;
			EMPTY_LIST_HEAD(&lpThreadArgv->ThreadArgvListItem);
			lpThreadArgv->bAuto = 1;
			lpThreadArgv->bVxWorks = 0;
		}
		else
		{
			lpThreadArgv->bAuto = 0;
		}
		EMPTY_LIST_HEAD(&lpThreadArgv->ThreadArgvListItem);
		lpThread->arg				= lpThreadArgv;

		hHandle = CreateThread(lpThreadAttributes,dwStackSize,(LPTHREAD_START_ROUTINE)TaskThread,lpThread,dwCreationFlags,&lpThreadArgv->dwThreadId);
		if (lpThreadId)
		{
			*lpThreadId = lpThreadArgv->dwThreadId;
		}
		if (lpName)
		{
			strncpy(lpThreadArgv->name,lpName,THREAD_NAME_LEN);
		}
		else
		{
			sprintf(lpThreadArgv->name,"t%08x\n",lpThreadArgv->dwThreadId);
		}
		ListAddTail(&lpThreadArgv->ThreadArgvListItem,&s_SystemManager.ThreadArgvList);
		return hHandle;
	}
}

HANDLE RtCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,DWORD dwStackSize,
			LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter,DWORD dwCreationFlags,LPDWORD lpThreadId)
{
	return RtCreateThreadEx(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, 0, DEFAULE_PRIORITY, 0, lpThreadId);
}

DWORD RtResumeThread(HANDLE hThread)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;
		int nSuspendCount = 0xFFFFFFFF;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyHandle(hThread);
		if (lpThread)
		{
			nSuspendCount = lpThread->nSuspendCount;
			if (nSuspendCount)
			{
				lpThread->nSuspendCount--;
				if (nSuspendCount == 1)
				{
					ListDel(&lpThread->StateListItem);
					ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->ReadyList[lpThread->nPriority]);

					lpThread->nState							= READY;
					s_SystemManager.lpSystemTable->bReSchedule	= 1;
				}
			}
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (nSuspendCount != 0xFFFFFFFF)
		{
			if (nSuspendCount >= 0)
			{
				if (nSuspendCount == 1)
				{
					s_SystemFunc.ReSchedule();
				}
			}
		}
		return nSuspendCount;
	}
	else
	{
		return ResumeThread(hThread);
	}
}

BOOL RtResumeThreadEx(DWORD dwThreadID)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		lpThread = FindThreadbyId(dwThreadID);
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpThread)
		{
			RtResumeThread(lpThread->hThread);
			return TRUE;
		}
	}
	else
	{
		HANDLE hHandle;
		hHandle = s_SystemManager.lpfnOpenThread(THREAD_SUSPEND_RESUME,FALSE,dwThreadID);
		if (hHandle)
		{
			ResumeThread(hHandle);
			CloseHandle(hHandle);
			return TRUE;
		}
	}
	return FALSE;
}

DWORD RtSuspendThread(HANDLE hThread)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Process *lpProcess;
		_Thread  *lpThread;
		HANDLE hSelfHandle;
		DWORD dwThreadID;

		int nSuspendCount = 0xFFFFFFFF;

		hSelfHandle	= GetCurrentThread();
		dwThreadID	= GetCurrentThreadId();
		lpProcess	= s_SystemManager.lpProcess;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		ListForEachEntry(_Thread,lpThread,&lpProcess->ProcessThreadList,ProcessThreadListItem)
		{
			if (lpThread->hThread == hThread || (hThread == hSelfHandle && lpThread->dwThreadID == dwThreadID))
			{
				nSuspendCount = lpThread->nSuspendCount;

				lpThread->nSuspendCount++;
				if (lpThread->nSuspendCount == 1)
				{				
					ListDel(&lpThread->StateListItem);
					if (lpThread->nState == RUNNING)
					{
						lpThread->nState = READY;
						lpThread->bSuspended = 0;
						ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->YieldList);
					}
					else
					{
						ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->SuspendedList);
					}
					lpThread->nState							= SUSPENDED;
					s_SystemManager.lpSystemTable->bReSchedule	= 1;
				}
				break;
			}
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (nSuspendCount != 0xFFFFFFFF)
		{
			if (s_SystemManager.lpSystemTable->bReSchedule)
			{
				s_SystemFunc.ReSchedule();
				if (lpThread->dwThreadID == dwThreadID)
				{
					while (!lpThread->bSuspended)
					{
						s_SystemFunc.CoreOSDelay();
					}
				}
			}
		}
		return nSuspendCount;
	}
	else
	{
		return SuspendThread(hThread);
	}
}

BOOL RtSuspendThreadEx(DWORD dwThreadID)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		lpThread = FindThreadbyId(dwThreadID);
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpThread)
		{
			RtSuspendThread(lpThread->hThread);
			return TRUE;
		}
	}
	else
	{
		HANDLE hHandle;
		hHandle = s_SystemManager.lpfnOpenThread(THREAD_SUSPEND_RESUME,FALSE,dwThreadID);
		if (hHandle)
		{
			SuspendThread(hHandle);
			CloseHandle(hHandle);
			return TRUE;
		}
	}
	return FALSE;
}

VOID RtExitThread(DWORD dwExitCode)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(GetCurrentThreadId());
		if (lpThread)
		{
			lpThread->dwExitCode	= dwExitCode;
			UnRegisteThread(lpThread->dwThreadID,TRUE);
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		if (lpThread)
		{
			ExitThread(dwExitCode);
		}
	}
	else
	{
		ExitThread(dwExitCode);
	}
}

static BOOL ThreadTerminate(_Thread  *lpThread,DWORD dwExitCode,BOOL bForce)
{
	lpThread->bTerminate = TRUE;
	if (lpThread->nSafeCount && !bForce)
	{
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		RtWaitForSingleObject(lpThread->hSafeEvent,INFINITE);
		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		return FALSE;
	}
	else
	{
		lpThread->nState = TERMIMATE;
		lpThread->dwExitCode	= dwExitCode;
		UnRegisteThread(lpThread->dwThreadID,TRUE);
		TerminateThread(lpThread->hThread,dwExitCode);
		return TRUE;
	}
}

BOOL RtTerminateThread(HANDLE hThread,DWORD dwExitCode)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;
		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		while (1)
		{
			lpThread = FindThreadbyHandle(hThread);
			if (lpThread)
			{
				if (ThreadTerminate(lpThread,dwExitCode,FALSE))
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpThread)
		{
			return TRUE;
		}
	}
	else
	{
		return TerminateThread(hThread,dwExitCode);
	}
	return FALSE;
}

BOOL RtTerminateThreadEx(DWORD dwThreadID,DWORD dwExitCode)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;
		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		while (1)
		{
			lpThread = FindThreadbyId(dwThreadID);
			if (lpThread)
			{
				if (ThreadTerminate(lpThread,dwExitCode,FALSE))
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpThread)
		{
			return TRUE;
		}
	}
	else
	{
		HANDLE hHandle;
		hHandle = s_SystemManager.lpfnOpenThread(THREAD_SUSPEND_RESUME,FALSE,dwThreadID);
		if (hHandle)
		{
			TerminateThread(hHandle,dwExitCode);
			CloseHandle(hHandle);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL RtTerminateThreadForce(HANDLE hThread,DWORD dwExitCode)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;
		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		lpThread = FindThreadbyHandle(hThread);
		if (lpThread)
		{
			ThreadTerminate(lpThread,dwExitCode,TRUE);
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpThread)
		{
			return TRUE;
		}
	}
	else
	{
		return TerminateThread(hThread,dwExitCode);
	}
	return FALSE;
}

void RtSleep(DWORD dwMilliseconds)
{
	_Thread  *lpThread;
	int bFind = 0;

	if (s_SystemManager.lpSystemTable)
	{
		if (!dwMilliseconds)
		{
			Sleep(dwMilliseconds);
		}
		else
		{
			SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

			lpThread = FindThreadbyId(GetCurrentThreadId());
			if (lpThread)
			{
				bFind											= 1;
				lpThread->nState								= WAITING;
				lpThread->bSuspended							= 0;

				lpThread->dwReleaseTime							= s_SystemManager.lpSystemTable->dwSystemTime + dwMilliseconds  *  1000;
				
				ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->YieldList);

				s_SystemManager.lpSystemTable->bReSchedule	= 1;
			}
			
			SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

			if (bFind)
			{	
				s_SystemFunc.ReSchedule();
				while (!lpThread->bSuspended)
				{
					s_SystemFunc.CoreOSDelay();
				}
			}
			else
			{
				Sleep(dwMilliseconds);
			}
		}
	}
	else
	{
		Sleep(dwMilliseconds);
	}
}

char *RtGetThreadName(DWORD dwThreadID)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		lpThread = FindThreadbyId(dwThreadID);
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpThread)
		{
			return lpThread->szName;
		}
	}
	else
	{
		_ThreadArgv *lpThreadArgv;

		ListForEachEntry(_ThreadArgv,lpThreadArgv,&s_SystemManager.ThreadArgvList,ThreadArgvListItem)
		{
			if (lpThreadArgv->dwThreadId == dwThreadID)
			{
				return lpThreadArgv->name;
			}
		}
	}
	return 0;
}

DWORD RtGetThreadId(char *lpszName)
{

	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;
		ListForEachEntry(_Thread,lpThread,&s_SystemManager.lpProcess->ProcessThreadList,ProcessThreadListItem)
		{
			if (!strcmp(lpThread->szName,lpszName))
			{
				if (lpThread->arg)
				{
					return (DWORD)lpThread->arg;
				}
				else
				{
					return lpThread->dwThreadID;
				}
			}
		}
	}
	else
	{
		_ThreadArgv *lpThreadArgv;

		ListForEachEntry(_ThreadArgv,lpThreadArgv,&s_SystemManager.ThreadArgvList,ThreadArgvListItem)
		{
			if (!strcmp(lpThreadArgv->name,lpszName))
			{
				if (lpThreadArgv->bVxWorks)
				{
					return (DWORD)lpThreadArgv;
				}
				else
				{
					return lpThreadArgv->dwThreadId;
				}
			}
		}
	}
	return 0;
}

BOOL RtPriorityGet(DWORD dwThreadID,LONG *nPriority)
{
	BOOL bRet = FALSE;

	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(dwThreadID);
		if (lpThread)
		{
			*nPriority = lpThread->nPriority;
			bRet = TRUE;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
	return FALSE;
}

BOOL RtPrioritySet(DWORD dwThreadID,LONG nPriority)
{
	BOOL bRet = FALSE;

	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(dwThreadID);
		if (lpThread)
		{
			lpThread->nPriority = nPriority;
			bRet = TRUE;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
	return bRet;
}

BOOL RtOptionsGet(DWORD dwThreadId,int *lpOptions)
{
	BOOL bRet = FALSE;

	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(dwThreadId);
		if (lpThread)
		{
			*lpOptions = lpThread->dwOption;
			bRet = TRUE;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
	return FALSE;
}

BOOL RtOptionsSet(DWORD dwThreadId,int mask,int newOptions)
{
	BOOL bRet = FALSE;

	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(dwThreadId);
		if (lpThread)
		{
			lpThread->dwOption = (lpThread->dwOption | newOptions) & ~mask;
			bRet = TRUE;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
	return bRet;
}

void *RtGetThreadArgv(DWORD dwThreadID)
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;
		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		lpThread = FindThreadbyId(dwThreadID);
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpThread)
		{
			return lpThread->arg;
		}
	}
	else
	{
		_ThreadArgv *lpThreadArgv;

		ListForEachEntry(_ThreadArgv,lpThreadArgv,&s_SystemManager.ThreadArgvList,ThreadArgvListItem)
		{
			if (lpThreadArgv->dwThreadId == dwThreadID)
			{
				return lpThreadArgv;
			}
		}
	}
	return 0;
}

void RtSetThreadArgv(DWORD dwThreadID,void *argv)
{
	BOOL bNew = FALSE;
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;
		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		lpThread = FindThreadbyId(dwThreadID);
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpThread)
		{
			lpThread->arg = argv;
		}
	}
	else
	{
		_ThreadArgv *lpThreadArgv;

		ListForEachEntry(_ThreadArgv,lpThreadArgv,&s_SystemManager.ThreadArgvList,ThreadArgvListItem)
		{
			if (lpThreadArgv->dwThreadId == dwThreadID)
			{
				ListDel(&lpThreadArgv->ThreadArgvListItem);
				if (lpThreadArgv != argv && lpThreadArgv->bAuto)
				{
					free(lpThreadArgv);
					bNew = TRUE;
				}
				break;
			}
		}
		if (argv)
		{
			lpThreadArgv = (_ThreadArgv *)argv;
			if (bNew)
			{
				lpThreadArgv->bAuto = FALSE;
			}
			EMPTY_LIST_HEAD(&lpThreadArgv->ThreadArgvListItem);
			ListAddTail(&lpThreadArgv->ThreadArgvListItem,&s_SystemManager.ThreadArgvList);
		}
	}
}

HANDLE RtGetThreadHandle(DWORD dwThreadID)
{
	HANDLE hThread = 0;
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(GetCurrentThreadId());
		if (lpThread)
		{
			hThread = lpThread->hThread;
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
	else
	{
		hThread = s_SystemManager.lpfnOpenThread(THREAD_SUSPEND_RESUME,FALSE,dwThreadID);
	}
	return hThread;
}

void RtThreadSafe()
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(GetCurrentThreadId());
		if (lpThread)
		{
			lpThread->nSafeCount++;
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
}

void RtThreadUnsafe()
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;
		BOOL bTerminate = FALSE;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(GetCurrentThreadId());
		if (lpThread)
		{
			lpThread->nSafeCount--;
			if (!lpThread->nSafeCount && lpThread->bTerminate)
			{
				SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
				RtSetEvent(lpThread->hSafeEvent);
				bTerminate = TRUE;
			}
		}
		if (!bTerminate)
		{
			SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
		}
		else
		{
			ListDel(&lpThread->StateListItem);
			lpThread->nState = SUSPENDED;
			ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->SuspendedList);
			s_SystemFunc.ReSchedule();
		}
	}
}

int RtGetThreadState(HANDLE hThread)
{
	int nState = 0;

	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyHandle(hThread);
		if (lpThread)
		{
			nState = lpThread->nState;;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
	return nState;
}

void RtThreadPreemptionLock()
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(GetCurrentThreadId());
		if (lpThread)
		{
			lpThread->nPreemptionLock++;
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
}

void RtThreadPreemptionUnlock()
{
	if (s_SystemManager.lpSystemTable)
	{
		_Thread  *lpThread;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpThread = FindThreadbyId(GetCurrentThreadId());
		if (lpThread)
		{
			if (lpThread->nPreemptionLock > 0)
			{
				lpThread->nPreemptionLock--;
			}
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	}
}