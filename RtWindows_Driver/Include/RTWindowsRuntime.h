/***********************************头文件**************************************/
#pragma once

#ifdef _RTWIN32
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#elif _RTKERNEL
#include <NTDDK.h>
#endif
/***********************************宏定义**************************************/

#define CPU						Corei7
#define THREAD_NAME_LEN			16

#ifdef _RTWIN32
#ifdef _CONSOLE
#define RTSTARTUP()	\
		RtStartup(TRUE);
#else
#define RTSTARTUP()	\
		RtStartup(FALSE);
#endif
#endif

/***********************************系统数据结构申明区域**************************************/
enum
{
	EVENT_OBJECT,
	MUTEX_OBJECT,
	SEMAPHORE_OBJECT,
	MSG_QUEUE_OBJECT
} _PROCESS_OBJECT;

enum
{
	UNKONWN,
	TERMIMATE,
	SUSPENDED,
	WAITING,
	READY,
	RUNNING,
	BLOCKED
} _THREAD_STATE;

typedef struct _ListItem 
{
	volatile struct _ListItem *Next;
	volatile struct _ListItem *Prev;
} LIST_ITEM;

typedef struct
{
	long nMagic;
	int  bAuto;
	int  bVxWorks;
	char name[THREAD_NAME_LEN];
	DWORD dwThreadId;
	LIST_ITEM ThreadArgvListItem;
} _ThreadArgvHead;
/***********************************函数定义***********************************************/
#ifdef _RTWIN32
#if defined(__cplusplus)
extern "C" {
#endif
	__declspec(dllexport) HANDLE RtCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize, LPTHREAD_START_ROUTINE
		lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
	__declspec(dllexport) HANDLE RtCreateThreadEx(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize,
		LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags,
		LPCTSTR lpName, LONG nPriority, DWORD dwOption, LPDWORD lpThreadId);
	__declspec(dllexport) VOID   RtExitThread(DWORD dwExitCode);

	__declspec(dllexport) DWORD  RtResumeThread(HANDLE hThread);
	__declspec(dllexport) BOOL	 RtResumeThreadEx(DWORD dwThreadID);
	__declspec(dllexport) DWORD  RtSuspendThread(HANDLE hThread);
	__declspec(dllexport) BOOL	 RtSuspendThreadEx(DWORD dwThreadID);
	__declspec(dllexport) BOOL   RtTerminateThread(HANDLE hThread, DWORD dwExitCode);
	__declspec(dllexport) BOOL   RtTerminateThreadEx(DWORD dwThreadID, DWORD dwExitCode);
	__declspec(dllexport) BOOL   RtTerminateThreadForce(HANDLE hThread, DWORD dwExitCode);

	__declspec(dllexport) void * RtGetThreadArgv(DWORD dwThreadID);
	__declspec(dllexport) void	 RtSetThreadArgv(DWORD dwThreadID, void *argv);
	__declspec(dllexport) HANDLE RtGetThreadHandle(DWORD dwThreadID);

	__declspec(dllexport) void   RtThreadSafe();
	__declspec(dllexport) void   RtThreadUnsafe();

	__declspec(dllexport) void   RtThreadPreemptionLock();
	__declspec(dllexport) void   RtThreadPreemptionUnlock();

	__declspec(dllexport) int    RtGetThreadState(HANDLE hThread);

	__declspec(dllexport) BOOL	 RtPrioritySet(DWORD dwThreadID, LONG nPriority);
	__declspec(dllexport) BOOL	 RtPriorityGet(DWORD dwThreadID, LONG *nPriority);

	__declspec(dllexport) BOOL	 RtOptionsGet(DWORD dwThreadId, int *lpOptions);
	__declspec(dllexport) BOOL	 RtOptionsSet(DWORD dwThreadId, int mask, int newOptions);

	__declspec(dllexport) char * RtGetThreadName(DWORD dwThreadID);
	__declspec(dllexport) DWORD  RtGetThreadId(char *lpszName);

	__declspec(dllexport) VOID   RtExitProcess(UINT uExitCode);

	__declspec(dllexport) void   RtSleep(DWORD dwMilliseconds);

	__declspec(dllexport) DWORD  RtWaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
	__declspec(dllexport) void   RtReleaseObject(HANDLE hObject);
	__declspec(dllexport) BOOL   RtCloseHandle(HANDLE hObject);

	__declspec(dllexport) HANDLE RtCreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName);
	__declspec(dllexport) HANDLE RtCreateEventEx(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName, DWORD dwOption);
	__declspec(dllexport) HANDLE RtOpenEvent(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName);
	__declspec(dllexport) BOOL   RtSetEvent(HANDLE hEvent);
	__declspec(dllexport) BOOL   RtResetEvent(HANDLE hEvent);
	__declspec(dllexport) BOOL   RtPulseEvent(HANDLE hEvent);

	__declspec(dllexport) HANDLE RtCreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName);
	__declspec(dllexport) HANDLE RtCreateMutexEx(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName, DWORD dwOption);
	__declspec(dllexport) HANDLE RtOpenMutex(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName);
	__declspec(dllexport) BOOL   RtReleaseMutex(HANDLE hMutex);

	__declspec(dllexport) HANDLE RtCreateSemaphore(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
		LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName);
	__declspec(dllexport) HANDLE RtCreateSemaphoreEx(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
		LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName, DWORD dwOption);
	__declspec(dllexport) HANDLE RtOpenSemaphore(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName);
	__declspec(dllexport) BOOL   RtReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount);

	__declspec(dllexport) HANDLE RtCreateMsgQueue(long nMaxMsgs, long nMaxLength, char *lpName, DWORD dwOption);
	__declspec(dllexport) HANDLE RtOpenMsgQueue(LPCTSTR lpName);
	__declspec(dllexport) DWORD  RtMsgQueueSend(HANDLE hHandle, LPCTSTR lpszBuffer, UINT nBytes, DWORD dwMilliseconds, int nPriority);
	__declspec(dllexport) DWORD  RtMsgQueueReceive(HANDLE hHandle, LPCTSTR lpszBuffer, UINT nMaxNBytes, DWORD dwMilliseconds, long *lpBytes);
	__declspec(dllexport) int	 RtMsgQueueNum(HANDLE hHandle);

	__declspec(dllexport) int	SetClockFrequence(int accuracy, int type);

	__declspec(dllexport) int	 RtPrintf(const char *format, ...);
	__declspec(dllexport) BOOL	 RtStartup(BOOL bConsole);

	__declspec(dllexport) void * GetSystemMem(int nLen);

	__declspec(dllexport) int    SEHFiler(int code);
#endif
#if defined(__cplusplus)
}
#endif
