/***********************************ͷ�ļ�**************************************/
#pragma once

#include "RTWindowsRuntime.h"
#include "RTWindowsScheduler.h"
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//											ϵ  ͳ  ��
//
/////////////////////////////////////////////////////////////////////////////////////////////////
/***********************************�궨��*****************************************************/
#define ADDRESS_ALIGN		4

/***********************************ϵͳ���ݽṹ��������**************************************/
typedef HANDLE(__stdcall *OPENTHREAD) (DWORD dwFlag, BOOL bUnknow, DWORD dwThreadId);

/***********************************��������***************************************************/
#if defined(__cplusplus)
extern "C" {
#endif
	void InitSysytemManager(_SystemManager *lpSystemManager, int bCreate);
#ifdef _RTWIN32
	
	void *GetSystemMem(int nLen);

	_Process *GetNewProcess();
	void RegisteProcess(HANDLE hThread, DWORD dwThreadID);
	void UnRegistProcess();

	_Thread *GetNewThread();
	void RegisteMainThread(HANDLE hThread, DWORD dwThreadID);
	void RegisteThread(_Thread *lpThread, DWORD dwCreationFlags);
	void UnRegisteThread(DWORD dwThreadID, BOOL bLock);
	_Thread *ThreadBlocked(DWORD dwMilliseconds, LIST_HEAD *lpBlockedList);

	_ProcessObject *GetNewProcessObject(LPSECURITY_ATTRIBUTES lpObjectAttributes, LPCTSTR lpName);
	BOOL CheckObjectHandle(HANDLE hObjectHandle);
	_ProcessObject *FindProcessObject(LPCTSTR lpName, LIST_HEAD *lpObjectList);
	BOOL SearchObject(int nClass, LPCTSTR lpName);
	_ObjectHandle *GetObjectHandle(int nClass, _ProcessObject *lpProcessObject);
	BOOL Preemptive(int nPriority);

	_Thread *BlockedEvent(HANDLE hObject, DWORD dwMilliseconds, DWORD *dwRet);

	_Thread *BlockedMutex(HANDLE hObject, DWORD dwMilliseconds, DWORD *dwRet);

	_Thread *BlockedSemaphore(HANDLE hObject, DWORD dwMilliseconds, DWORD *dwRet);

	BOOL CloseObjectHandle(HANDLE hObject, BOOL bLock);

	BOOL EventSetEvent(HANDLE hEvent, BOOL bLock);
	BOOL MutexReleaseMutex(HANDLE hMutex, BOOL bLock);
	BOOL SemaphoreReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount, BOOL bLock);
	void MsgQueueReleaseAll(HANDLE hHandle);

#endif
#if defined(__cplusplus)
}
#endif