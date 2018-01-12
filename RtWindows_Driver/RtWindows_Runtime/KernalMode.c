#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include "../Include/RTWindowsServices.h"

static unsigned int start_serid;
static HANDLE s_hDevice;

#define IOCTL_GET_START_SERVICEID CTL_CODE(FILE_DEVICE_UNKNOWN,0x8001,METHOD_BUFFERED,FILE_ANY_ACCESS)

//�ڴ�ӳ��
#define IOCTL_MAP_MERMORY CTL_CODE(FILE_DEVICE_UNKNOWN,0x8002,METHOD_BUFFERED,FILE_ANY_ACCESS)

//�趨ʱ������
#define IOCTL_SET_CLOCK	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8003,METHOD_BUFFERED,FILE_ANY_ACCESS)

//�趨ϵͳ��Ե�ԣ���ʵʱ���ʵʱ������
#define IOCTL_SET_SYSAFFINITY CTL_CODE(FILE_DEVICE_UNKNOWN,0x8004,METHOD_BUFFERED,FILE_ANY_ACCESS)

//��ȡ�߳��ں˾��
#define IOCTL_GET_THREAD_HANDLE CTL_CODE(FILE_DEVICE_UNKNOWN,0x8006,METHOD_BUFFERED,FILE_ANY_ACCESS)

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//										for Kernal Mode
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

static void _CoreOSDelay()
{
	Sleep(0);
}

static void _ReSchedule()
{
	__asm
	{
		mov eax, start_serid
		add eax, 2                     //���ú�
		lea edx, [esp + 4]
		int 2eh
	}
}
 
static HANDLE	_GetThreadHandle(DWORD dwThreadID)
{
	HANDLE threadHandle;
	BOOLEAN bRet = FALSE;
	DWORD BytesReturned;

	bRet = DeviceIoControl(s_hDevice,IOCTL_GET_THREAD_HANDLE,&dwThreadID, 4,&threadHandle, 4,&BytesReturned,NULL);
	
	if (!bRet)
	{
#ifdef _DEBUG
		printf("get start id failed!\n");
#endif
	}
#ifdef _DEBUG
	printf("threadHandle: 0x%0x\n",(ULONG)threadHandle);
#endif
	return threadHandle;
}


static BOOL GetStartServiceId()
{
	BOOLEAN bRet = FALSE;
	DWORD BytesReturned;
	
	bRet = DeviceIoControl(s_hDevice,IOCTL_GET_START_SERVICEID,NULL,0,&start_serid,4,&BytesReturned,NULL);
#ifdef _DEBUG
	printf("start service id: 0x%x\n",start_serid);
#endif
	if (!bRet)
	{
#ifdef _DEBUG
		printf("get start id failed!\n");
#endif
	}
	return bRet;
}

//type = 1Ϊ���뼶�� 1000Ϊ΢�� 1000000Ϊ���� ��߾���֧�ֵ�15ns
static BOOL RtSetClockFrequence(int accuracy,int type)
{
	BOOLEAN bRet = FALSE;
	DWORD BytesReturned;
	unsigned long Mhz = 100;//get_bus_freq();
	unsigned long clockFre = (unsigned long)((float)accuracy*Mhz/1000*type);

	bRet = DeviceIoControl(s_hDevice,IOCTL_SET_CLOCK,&clockFre,4,NULL,0,&BytesReturned,NULL);
	if (!bRet)
	{
#ifdef _DEBUG
		printf("set clock failed!\n");
#endif
	}
	return bRet;
}

BOOL RtSharedMemory(PVOID* lpBaseAddr)
{
	BOOLEAN bRet = FALSE;
	DWORD BytesReturned;
	
	bRet = DeviceIoControl(s_hDevice,IOCTL_MAP_MERMORY,NULL,0,lpBaseAddr, 4,&BytesReturned,NULL);
#ifdef _DEBUG
	printf("base addr: 0x%0x\n", (ULONG)*lpBaseAddr);
#endif

	if (!bRet)
	{
#ifdef _DEBUG
		printf("Map Memory failed!\n");
#endif
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

static BOOL RtSetSystemAffinity(ULONG32 cpuMask)
{
	BOOLEAN bRet = FALSE;
	DWORD BytesReturned;
	ULONG sysAffinity = cpuMask;
	
	bRet = DeviceIoControl(s_hDevice,IOCTL_SET_SYSAFFINITY,&sysAffinity, 4,NULL,0,&BytesReturned,NULL);
	if (!bRet)
	{
#ifdef _DEBUG
		printf("set system Affinity failed!\n");
#endif
	}
	return bRet;
}


int SystemInitializationForKernalMode()
{
	s_SystemFunc.CoreOSDelay		= _CoreOSDelay;
	s_SystemFunc.ReSchedule			= _ReSchedule;
	s_SystemFunc.GetThreadHandle	= _GetThreadHandle;

	//�˴��򿪲��ܹرգ�ֱ�����̽������Զ��ͷž���������ں���������
	s_hDevice = CreateFile(L"\\\\.\\RtWindows",GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	
	if (s_hDevice != INVALID_HANDLE_VALUE)
	{
		//ӳ���ڴ�
		
		if(RtSharedMemory(&s_SystemManager.lpShareMemBase))
		{	
			GetStartServiceId();
			RtSetSystemAffinity(0xe);//����ϵͳ��Ե��Ϊ1�źˣ���ʵʱ��
			RtSetClockFrequence(1, 1000000); //type = 1Ϊ���뼶�� 1000Ϊ΢�� 1000000Ϊ���� ��߾���֧�ֵ�15ns

			s_AddressOffset = (ULONG)s_SystemManager.lpShareMemBase - 1;
			s_SystemManager.lpSystemTable = (_SystemTable *)s_SystemManager.lpShareMemBase;
			InitSysytemManager(&s_SystemManager,FALSE);
	
			return 1;
		}
		
	}
	
	return 0;
}