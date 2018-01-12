//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//										文件说明：提供时间相关接口
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "../Include/RTWindowsServices.h"
#include <stdio.h>

//设定时钟周期
#define IOCTL_SET_CLOCK	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8003,METHOD_BUFFERED,FILE_ANY_ACCESS)

//获取RTWINDOWS时间
#define IOCTL_GET_SYSTIME\
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8005,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_INIT_DLL \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x8007,METHOD_BUFFERED,FILE_ANY_ACCESS)

static HANDLE s_hDeviceTime;
static HANDLE s_hDeviceAPIC;
static HANDLE s_hDevice;

BOOL SetClockFrequence(int accuracy, int type)
{
	BOOLEAN bRet = FALSE;
	DWORD BytesReturned = 0;
	unsigned long Mhz = 100;
	unsigned long clockFre = (accuracy * Mhz / 1000 * type);

	if (!s_hDeviceAPIC)
	{
		s_hDeviceAPIC = CreateFile("\\\\.\\RtWindows", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (s_hDeviceAPIC != INVALID_HANDLE_VALUE)
			bRet = DeviceIoControl(s_hDeviceTime, IOCTL_INIT_DLL, NULL, 0, NULL, 0, &BytesReturned, NULL);
	}
	if (s_hDeviceAPIC != INVALID_HANDLE_VALUE)
	{
		bRet = DeviceIoControl(s_hDeviceAPIC, IOCTL_SET_CLOCK, &clockFre, sizeof(clockFre), NULL, 0, &BytesReturned, NULL);
	}
	return FALSE;
}