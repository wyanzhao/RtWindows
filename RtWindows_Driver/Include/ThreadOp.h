/************************************************************************
* �ļ�˵������ȡϵͳδ����������ַ
*************************************************************************/

/***********************************ͷ�ļ�**************************************/
#pragma once
#include <NTDDK.h>
#include "Driver.h"

/***********************************�궨��**************************************/
//�����
//thread operate
#define NtSuspendThread_ServiceId_Win7 0x16F
#define NtResumeThread_ServiceId_Win7 0x130

//������
//�̹߳���ָ�����������(Pe����)
#define PsSuspendThread_KeyCode1_Win7 0xe8
#define PsSuspendThread_KeyCode2_Win7 0x66
#define PsSuspendThread_KeyCode3_Win7 0x00

#define KeResumeThread_KeyCode1_Win7 0x8b
#define KeResumeThread_KeyCode2_Win7 0x45
#define KeResumeThread_KeyCode3_Win7 0xe4

/***********************************ϵͳ���ݽṹ��������**************************************/
typedef struct _THREAD_OPERATE_ADDRESS
{
	ULONG NtSuspendThreadAddress;
	ULONG NtResumeThreadAddress;

	ULONG PsSuspendThreadAddress;
	ULONG KeResumeThreadAddress;
}THREAD_OPERATE_ADDR, *PTHREAD_OPERATE_ADDR;

//�̷߳���ID���ݽṹ
typedef struct _FUNCTION_SERVICE_ID
{
	ULONG NtSuspendThread_ServiceId;
	ULONG NtResumeThread_ServiceId;
}THREAD_FUNCTION_SERVICE_ID, *PTHREAD_FUNCTION_SERVICE_ID;

typedef struct _THREAD_KEYCODE
{
	ULONG KeyCode1;
	ULONG KeyCode2;
	ULONG KeyCode3;
}THREAD_KEYCODE, *PTHREAD_KEYCODE;

//�����̲߳������������ݽṹ
typedef struct _THREAD_FUNCTION_KEYCODE
{
	//thread operate
	THREAD_KEYCODE PsSuspendThread_KeyCode;
	THREAD_KEYCODE KeResumeThread_KeyCode;
}THREAD_FUNCTION_KEYCODE, *PTHREAD_FUNCTION_KEYCODE;

/***********************************��������***********************************************/
//�����߳�ID��ȡ�߳̾��
__declspec(dllimport) NTSTATUS PsLookupThreadByThreadId(IN HANDLE ThreadId, OUT PETHREAD *Thread);

//�õ�Nt������ַ
static ULONG GetNtFunctionAddress(ULONG Index);

//��ù����̵߳�Ps�������
static ULONG GetPsSuspendThreadAddr();

//��ù����̵߳�Ke�������
static ULONG GetKeResumeThreadAddr();

//��ʼ���̲߳���������ڵ�ַ
BOOLEAN InitThreadOperatorAddress();

/*
* ��ȡSSDT�ڲ�������ַ
* ULONG ����ֵ������ַ
*/
static ULONG GetSSDTFunctionAddress(ULONG Index);	//��ȡSSDT��ַ

/*
* �����̲߳�����Ps����
* PETHREAD Thread ��ǰ�߳�
* PreviousSuspendCount OPTIONAL ��ǰ�Ѿ�������̸߳���
* NTSTATUS �Ƿ�ɹ�
*/
NTSTATUS PsSuspendThread(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL);

/*
* �ָ��̲߳�����Ps����
* PETHREAD Thread ��ǰ�߳�
* PreviousSuspendCount OPTIONAL ��ǰ�Ѿ�������̸߳���
* NTSTATUS �Ƿ�ɹ�
*/
NTSTATUS PsResumeThread(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL);

/*
* �����̲߳�����Ps����
* PKTHREAD Thread ��ǰ�߳�
* NTSTATUS ����ֵ �Ƿ�ɹ�
*/
NTSTATUS SuspendThreadByHandle(IN HANDLE hThread);

/*
* �ָ��̲߳�����Ps����
* PKTHREAD Thread ��ǰ�߳�
* NTSTATUS ����ֵ �Ƿ�ɹ�
*/
NTSTATUS ResumeThreadByHandle(IN HANDLE hThread);



