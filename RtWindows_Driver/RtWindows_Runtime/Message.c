#include "../Include/RTWindowsServices.h"

#define MSG_QUEUE_RECV		0
#define MSG_QUEUE_SEND		1

static int MsgQueueRelease(LIST_HEAD *BlockedList,BOOL bAll)
{
	_Thread *lpThread,*lpNextThread;
	int nPriority = MAX_PRIORITY_VALUE;

	ListForEachEntrySafe(_Thread,lpThread,lpNextThread,BlockedList,BlockedListItem)
	{
		ListDel(&lpThread->BlockedListItem);
		ListDel(&lpThread->StateListItem);
		ListAddTail(&lpThread->StateListItem,&s_SystemManager.lpSystemTable->ReadyList[lpThread->nPriority]);

		lpThread->nState			= READY;
		lpThread->nUnBlockState		= WAIT_OBJECT_0;
		lpThread->bSuspended		= 1;
		nPriority					= lpThread->nPriority;
		if (!bAll)
		{
			break;
		}
	}
	return nPriority;
}

void MsgQueueReleaseAll(HANDLE hHandle)
{
	_ObjectHandle *lpObjectHandle;
	_ProcessObject *lpMsgQueue;
	_MessageQueue *lpMessageQueue;

	lpObjectHandle = hHandle;
	lpMsgQueue = lpObjectHandle->hObjectHandle;
	lpMessageQueue = PTR(_MessageQueue,lpMsgQueue->ObjectValues.MsgQueueValues.lpMessageQueue);

	MsgQueueRelease(&lpMsgQueue->BlockedList,TRUE);
	MsgQueueRelease(&lpMessageQueue->BlockedList,TRUE);
}

static DWORD MsgQueueProcess(int nMode,HANDLE hHandle, LPCTSTR lpszBuffer, UINT nBytes,DWORD dwMilliseconds,int nPriority,long *lpBytes)
{
	DWORD dwRet = 0;
	if (s_SystemManager.lpSystemTable)
	{
		_ObjectHandle *lpObjectHandle = hHandle;
		_ProcessObject *lpMsgQueue;
		_Thread *lpThread;
		_MessageQueue *lpMessageQueue;
		int nLen;
		int nPriority = MAX_PRIORITY_VALUE;

		if (CheckObjectHandle(lpObjectHandle))
		{
			if (lpObjectHandle->nHandleClass == MSG_QUEUE_OBJECT)
			{
				lpMsgQueue = lpObjectHandle->hObjectHandle;
				if (lpMsgQueue->ObjectValues.MsgQueueValues.nMaxLength >= (long)nBytes || nMode == MSG_QUEUE_RECV)
				{
					while (1)
					{
						lpThread = 0;
						SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

						lpMessageQueue = PTR(_MessageQueue,lpMsgQueue->ObjectValues.MsgQueueValues.lpMessageQueue);

						if ((nMode == MSG_QUEUE_SEND && lpMessageQueue->nMsgNum == lpMsgQueue->ObjectValues.MsgQueueValues.nMaxMsgs) ||
																							(nMode == MSG_QUEUE_RECV && !lpMessageQueue->nMsgNum))
						{
							if (dwMilliseconds)
							{
								if (nMode == MSG_QUEUE_SEND)
								{
									lpThread = ThreadBlocked(dwMilliseconds,&lpMsgQueue->BlockedList);
								}
								else
								{
									lpThread = ThreadBlocked(dwMilliseconds,&lpMessageQueue->BlockedList);
								}
							}
							else
							{
								dwRet = WAIT_TIMEOUT;
							}
						}
						else
						{
							if (nMode == MSG_QUEUE_SEND)
							{
								memcpy(&lpMessageQueue->lpQueue[lpMessageQueue->nTail * lpMsgQueue->ObjectValues.MsgQueueValues.nMaxLength],lpszBuffer,nBytes);
								lpMessageQueue->MsgLen[lpMessageQueue->nTail] = nBytes;

								lpMessageQueue->nTail =	(++lpMessageQueue->nTail) % lpMsgQueue->ObjectValues.MsgQueueValues.nMaxMsgs;
								lpMessageQueue->nMsgNum++;
								dwRet = 0;
								nPriority = MsgQueueRelease(&lpMessageQueue->BlockedList,FALSE);
							}
							else
							{
								nLen = ((int)nBytes >= lpMessageQueue->MsgLen[lpMessageQueue->nHead])?lpMessageQueue->MsgLen[lpMessageQueue->nHead]:nBytes;
								memcpy((char *)lpszBuffer,&lpMessageQueue->lpQueue[lpMessageQueue->nHead * lpMsgQueue->ObjectValues.MsgQueueValues.nMaxLength],nLen);
								if (lpBytes)
								{
									*lpBytes = lpMessageQueue->MsgLen[lpMessageQueue->nHead];
								}

								lpMessageQueue->nHead =	(++lpMessageQueue->nHead) % lpMsgQueue->ObjectValues.MsgQueueValues.nMaxMsgs;
								lpMessageQueue->nMsgNum--;
								nPriority = MsgQueueRelease(&lpMsgQueue->BlockedList,FALSE);
							}
							dwRet = 0;
						}

						if (nPriority < MAX_PRIORITY_VALUE)
						{
							if (!Preemptive(nPriority))
							{
								SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
							}
							break;
						}
						else
						{
							SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

							if (lpThread)
							{
								s_SystemFunc.ReSchedule();
								while (!lpThread->bSuspended)
								{
									s_SystemFunc.CoreOSDelay();
								}
								dwRet = lpThread->nUnBlockState;
								if (dwRet == WAIT_TIMEOUT)
								{
									break;
								}
							}
							else
							{
								break;
							}
						}
					}
				}
			}
		}
	}
	return dwRet;
}

HANDLE RtCreateMsgQueue(long nMaxMsgs,long nMaxLength,char *lpName,DWORD dwOption)
{
	_MessageQueue *lpMessageQueue;
	BOOL bNewMsgQueue = FALSE;
	if (s_SystemManager.lpSystemTable)
	{
		_ProcessObject *lpMsgQueue = 0;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpName)
		{
			lpMsgQueue = FindProcessObject(lpName,&s_SystemManager.lpSystemTable->MsgQueueList);
			if (lpMsgQueue)
			{
				lpMsgQueue->nRefCount++;
			}
			else
			{
				if (!SearchObject(MSG_QUEUE_OBJECT,lpName))
				{
					bNewMsgQueue = TRUE;
				}
			}
		}
		else
		{
			bNewMsgQueue = TRUE;
		}
		if (bNewMsgQueue)
		{
			lpMessageQueue	= (_MessageQueue *)GetSystemMem(sizeof(_MessageQueue) + nMaxMsgs * nMaxLength - 1);

			INIT_LIST_HEAD(&lpMessageQueue->BlockedList);

			lpMessageQueue->nHead	= 0;
			lpMessageQueue->nTail	= 0;
			lpMessageQueue->nMsgNum	= 0;

			lpMsgQueue = GetNewProcessObject(0,lpName);
			lpMsgQueue->nHandleClass		= MSG_QUEUE_OBJECT;
			lpMsgQueue->dwOption			= dwOption;

			lpMsgQueue->ObjectValues.MsgQueueValues.nMaxMsgs		= nMaxMsgs;
			lpMsgQueue->ObjectValues.MsgQueueValues.nMaxLength		= nMaxLength;
			lpMsgQueue->ObjectValues.MsgQueueValues.lpMessageQueue	= PTR_OFFSET(_MessageQueue,lpMessageQueue);

			ListAddTail(&lpMsgQueue->ObjectListItem,&s_SystemManager.lpSystemTable->MsgQueueList);
		}
		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpMsgQueue)
		{
			return GetObjectHandle(MSG_QUEUE_OBJECT,lpMsgQueue);
		}
	}
	return 0;
}

HANDLE RtOpenMsgQueue(LPCTSTR lpName)
{
	if (s_SystemManager.lpSystemTable)
	{
		_ProcessObject *lpMsgQueue;

		SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		lpMsgQueue = FindProcessObject(lpName,&s_SystemManager.lpSystemTable->MsgQueueList);
		if (lpMsgQueue)
		{
			lpMsgQueue->nRefCount++;
		}

		SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

		if (lpMsgQueue)
		{
			return GetObjectHandle(MSG_QUEUE_OBJECT,lpMsgQueue);
		}
	}
	return 0;
}

DWORD RtMsgQueueSend(HANDLE hHandle, LPCTSTR lpszBuffer, UINT nBytes,DWORD dwMilliseconds, int nPriority)
{
	return MsgQueueProcess(MSG_QUEUE_SEND,hHandle,lpszBuffer,nBytes,dwMilliseconds,nPriority,NULL);
}

DWORD RtMsgQueueReceive(HANDLE hHandle, LPCTSTR lpszBuffer, UINT nMaxNBytes,DWORD dwMilliseconds,long *lpBytes)
{
	return MsgQueueProcess(MSG_QUEUE_RECV,hHandle,lpszBuffer,nMaxNBytes,dwMilliseconds,0,lpBytes);
}

int RtMsgQueueNum(HANDLE hHandle)
{
	int nNum = 0;
	if (s_SystemManager.lpSystemTable)
	{
		_ObjectHandle *lpObjectHandle = hHandle;
		_ProcessObject *lpMsgQueue;
		_MessageQueue *lpMessageQueue;

		if (CheckObjectHandle(lpObjectHandle))
		{
			if (lpObjectHandle->nHandleClass == MSG_QUEUE_OBJECT)
			{
				lpMsgQueue = lpObjectHandle->hObjectHandle;

				SpinLock(&s_SystemFunc,(long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);

				lpMessageQueue = PTR(_MessageQueue,lpMsgQueue->ObjectValues.MsgQueueValues.lpMessageQueue);

				nNum = lpMessageQueue->nMsgNum;

				SpinUnlock((long *)&s_SystemManager.lpSystemTable->SystemSpinLockSection);
			}
		}
	}
	return nNum;
}

