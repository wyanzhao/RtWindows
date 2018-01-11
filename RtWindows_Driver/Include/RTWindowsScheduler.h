/***********************************头文件**************************************/
#pragma once

#ifdef _RTWIN32
#include <windows.h>
#else
#include <NTDDK.h>
#endif

#include "Queue.h"
/***********************************宏定义**************************************/
#ifndef WINAPI
#define  WINAPI				STDAPICALLTYPE
#endif // !STDAPICALLTYPE

#define MAX_PATH			260

#ifndef _RTWIN32
#define WAIT_TIMEOUT        STATUS_TIMEOUT
#endif

#define THREAD_NAME_LEN			16
#define DEFAULE_PRIORITY		5

#define	MAX_MSG_NUM				100

#define SYSTEM_MEM_SIZE			(4 * 1024 * 1024)
#define	TIME_MEM_SIZE				(4 * 1024)
#define PTR(A,B)				(B?(A *)((char *)B + s_AddressOffset):0)
#define PTR_OFFSET(A,B)			(B?(A *)((char *)B - s_AddressOffset):0)

#define SPIN_LOCK_KERNAL					0
#define SPIN_LOCK_USER						1

#define MAX_CPU_NUM							1
#define MAX_PRIORITY_VALUE					256

#ifndef PAGE_OPTION
#define PAGE_OPTION
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")
#endif 

#define INTERLOCKED_INCREMENT(A)				\
do {										\
__asm	mov eax,1						\
__asm	mov ecx,A						\
__asm	lock xadd dword ptr [ecx],eax	\
} while (0)

#define INTERLOCKED_DECREMENT(A)				\
do {										\
__asm	mov eax,0FFFFFFFFh				\
__asm	mov ecx,A						\
__asm	lock xadd dword ptr [ecx],eax	\
} while (0)

/***********************************系统数据结构申明区域**************************************/
typedef unsigned long		DWORD;
typedef int                 BOOL;
typedef void				*LPVOID;
typedef HANDLE(__stdcall *OPENTHREAD) (DWORD dwFlag, BOOL bUnknow, DWORD dwThreadId);
typedef unsigned __int64		SYSTEM_TIME_TYPE;
typedef	DWORD(*ENTRY)(void *arg);
struct _Process;

#ifndef _RTWIN32
typedef struct _SECURITY_ATTRIBUTES {
	DWORD nLength;
	LPVOID lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
#endif

typedef struct
{
	long nMagic;
	int  bAuto;
	int  bVxWorks;
	char name[THREAD_NAME_LEN];
	DWORD dwThreadId;
	LIST_HEAD ThreadArgvListItem;
} _ThreadArgv;

typedef struct
{
	char						szName[THREAD_NAME_LEN];
	ENTRY						Entry;
	void *						arg;
	volatile int				nPriority;
	DWORD						dwOption;
	struct _Process *			lpProcess;
	HANDLE						hThread;
	HANDLE						hSchedulerHandle;
	HANDLE						hSafeEvent;
	DWORD						dwThreadID;
	volatile int				nState;
	volatile int				bTerminate;
	volatile int				dwExitCode;
	volatile int				nSafeCount;
	volatile int				nPreemptionLock;
	int							bSuspended;
	volatile unsigned			nSuspendCount;
	volatile SYSTEM_TIME_TYPE 	dwReleaseTime;
	volatile int				nUnBlockState;

	LIST_HEAD					ProcessThreadListItem;
	LIST_HEAD					StateListItem;
	LIST_HEAD					BlockedListItem;
} _Thread;

typedef struct _Process
{
	volatile DWORD				dwProcessID;
	LIST_HEAD					ObjectHandleList;
	LIST_HEAD					ProcessThreadList;
	LIST_HEAD					ProcessListItem;
} _Process;

typedef struct
{
	LIST_HEAD					BlockedList;
	long						nHead;
	long						nTail;
	long						nMsgNum;
	long						MsgLen[MAX_MSG_NUM];
	char						lpQueue[1];
} _MessageQueue;

typedef union
{
	struct
	{
		BOOL					bManualReset;
		volatile BOOL			bState;
	} EventValues;

	struct
	{
		volatile long			nMutexCount;
		volatile DWORD			dwThreadID;
	} MutexValues;

	struct
	{
		long					nMaximumCount;
		volatile long			nSemaphoreCount;
	} SemaphoreValues;

	struct
	{
		long					nMaxMsgs;
		long					nMaxLength;
		_MessageQueue *			lpMessageQueue;
	} MsgQueueValues;
} _ObjectValues;

typedef struct _Object
{
	char						szName[MAX_PATH];
	int							nHandleClass;
	DWORD						dwOption;
	SECURITY_ATTRIBUTES			ObjectAttributes;
	volatile long				nRefCount;

	_ObjectValues				ObjectValues;

	LIST_HEAD					BlockedList;
	LIST_HEAD					ObjectListItem;
} _ProcessObject;

typedef struct
{
	volatile int				nMemHead;
	volatile int				bReSchedule;
	volatile SYSTEM_TIME_TYPE	nSystemScheduleTime;
	volatile SYSTEM_TIME_TYPE	dwSystemTime;

	volatile long				SystemSpinLockSection;

	volatile _Thread *			lpRunningThread[MAX_CPU_NUM];

	LIST_HEAD					ProcessList;

	LIST_HEAD					ThreadList;
	LIST_HEAD					WaitingList;
	LIST_HEAD					SuspendedList;
	LIST_HEAD					YieldList;
	LIST_HEAD					ExitList;
	LIST_HEAD					ReadyList[MAX_PRIORITY_VALUE];

	LIST_HEAD					EventList;
	LIST_HEAD					MutexList;
	LIST_HEAD					SemaphoreList;
	LIST_HEAD					MsgQueueList;

	LIST_HEAD					ProcessIdleList;
	LIST_HEAD					ThreadIdleList;
	LIST_HEAD					ObjectIdleList;

	volatile int				nBlockNum;
	volatile int				nBlockedNum;
	volatile int				nWaitingNum;
	volatile int				nReadyNum;
	volatile int				nSuspendedNum;
	volatile int				nTimeoutNum;
	volatile int				nScheduleNum;
	volatile int				nPreemptiveNum;
	volatile int				nObjectHandleNum;
} _SystemTable;

typedef struct
{
	void(*CoreOSDelay)();
	void(*ReSchedule)();

	DWORD(WINAPI *SuspendThread)(HANDLE hHandle);
	DWORD(WINAPI *ResumeThread)(HANDLE hHandle);
	DWORD(WINAPI *TerminateThread)(HANDLE hHandle, DWORD dwExitCode);
	HANDLE(*GetThreadHandle)(DWORD dwThreadID);
} _SystemFunc;

typedef struct
{
	long						nMagic;
	int							nHandleClass;
	HANDLE						hObjectHandle;
	LIST_HEAD					OwnerListItem;
} _ObjectHandle;

typedef struct
{
	int							bStartup;
	HANDLE						hShareMemHandle;
	char *						lpShareMemBase;
	_SystemTable *				lpSystemTable;
	_Process *					lpProcess;
	HANDLE						hScheduleEvent;
	OPENTHREAD					lpfnOpenThread;

	HANDLE						hSourceMutex;

	LIST_HEAD					ThreadArgvList;
} _SystemManager;

typedef struct
{
	char *						lpShareMemBase;
	volatile SYSTEM_TIME_TYPE *lpn64SystemTime;
} _TimeManager;

/***********************************系统变量定义***********************************************/
extern _SystemFunc				s_SystemFunc;
extern _SystemManager s_SystemManager; 
extern _TimeManager s_TimeManager;
extern	ULONG			s_AddressOffset;

/***********************************函数定义***************************************************/
#if defined(__cplusplus)
extern "C" {
#endif
#pragma LOCKEDCODE
int  TrySpinLock(long *SpinLockSection);

#pragma LOCKEDCODE
void SpinLock(_SystemFunc *lpSystemFunc, LONG *SpinLockSection);

#pragma LOCKEDCODE
void SpinUnlock(long *SpinLockSection);

#pragma LOCKEDCODE
void RtScheduler(_SystemTable *lpSystemTable, _SystemFunc *lpSystemFunc, SYSTEM_TIME_TYPE nCurrentTime);

#if defined(__cplusplus)
}
#endif

