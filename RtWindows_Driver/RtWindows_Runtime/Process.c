#include "../Include/RTWindowsServices.h"

static _Process *GetNewProcess()
{
	_Process *lpProcess;

	SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

	if (!ListEmpty(&s_SystemManager.lpSystemTable->ProcessIdleList))
	{
		lpProcess = list_entry(s_SystemManager.lpSystemTable->ProcessIdleList.Next,_Process,ProcessListItem);
		ListDel(&lpProcess->ProcessListItem);
	}
	else
	{
		lpProcess = (_Process *)GetSystemMem(sizeof(_Process));
	}

	memset(lpProcess,0,sizeof(_Process));

	SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
	
	return lpProcess;
}

void RegisteProcess(HANDLE hThread,DWORD dwThreadID)
{
	_Process *lpProcess;

	s_SystemManager.lpProcess			= GetNewProcess();

	lpProcess							= s_SystemManager.lpProcess;
	lpProcess->dwProcessID				= GetCurrentProcessId();

	INIT_LIST_HEAD(&lpProcess->ObjectHandleList);
	INIT_LIST_HEAD(&lpProcess->ProcessThreadList);
	EMPTY_LIST_HEAD(&lpProcess->ProcessListItem);

	//设置进程亲缘性为1号核（实时任务）
	//SetProcessAffinityMask(GetCurrentProcess(),1);

	RegisteMainThread(hThread, dwThreadID);

	SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

	ListAddTail(&lpProcess->ProcessListItem,&s_SystemManager.lpSystemTable->ProcessList);

	SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
}

void UnRegistProcess()
{
	_Thread *lpThread,*lpNextThread;
	_ObjectHandle *lpObjectHandle,*lpNextObjectHandle;
	_Process *lpProcess = s_SystemManager.lpProcess;
	
	RtCloseHandle(s_SystemManager.hSourceMutex);

	SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

	if (!ListEmpty(&lpProcess->ProcessThreadList))
	{
		ListForEachEntrySafe(_Thread,lpThread,lpNextThread,&lpProcess->ProcessThreadList,ProcessThreadListItem)
		{
			UnRegisteThread(lpThread->dwThreadID,TRUE);
		}
	}

	ListForEachEntrySafe(_ObjectHandle,lpObjectHandle,lpNextObjectHandle,&lpProcess->ObjectHandleList,OwnerListItem)
	{
		CloseObjectHandle(lpObjectHandle,TRUE);
	}

	ListDel(&lpProcess->ProcessListItem);
	ListAddTail(&lpProcess->ProcessListItem,&s_SystemManager.lpSystemTable->ProcessIdleList);

	s_SystemManager.lpSystemTable->bReSchedule		= 1;
	s_SystemFunc.ReSchedule();

	SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
}

VOID RtExitProcess(UINT uExitCode)
{
	if (s_SystemManager.lpSystemTable)
	{
		//UnInstallCoreOSApi();
		UnRegistProcess();
		ExitProcess(uExitCode);
	}
	else
	{
		ExitProcess(uExitCode);
	}
}

