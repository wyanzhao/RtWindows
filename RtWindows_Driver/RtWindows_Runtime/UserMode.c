#include "../Include/RTWindowsServices.h"
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//										for User Mode
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

static void _CoreOSDelay()
{
	Sleep(0);
}

static void _ReSchedule()
{
	SetEvent(s_SystemManager.hScheduleEvent);
}

int SystemInitializationForUserMode()
{
	s_SystemFunc.CoreOSDelay		= _CoreOSDelay;
	s_SystemFunc.ReSchedule			= _ReSchedule;

	s_SystemFunc.GetThreadHandle	= 0;
	s_SystemFunc.SuspendThread		= 0;
	s_SystemFunc.ResumeThread		= 0;
	s_AddressOffset					= 0;

	s_SystemManager.hShareMemHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,"__RT_WINDOWS_SYSTEM__");
	if (s_SystemManager.hShareMemHandle != NULL && s_SystemManager.hShareMemHandle != INVALID_HANDLE_VALUE)
	{
		s_SystemManager.lpShareMemBase = (char *)MapViewOfFile(s_SystemManager.hShareMemHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
		s_AddressOffset = (DWORD)s_SystemManager.lpShareMemBase - 1;
		s_SystemManager.lpSystemTable = (_SystemTable *)s_SystemManager.lpShareMemBase;

		InitSysytemManager(&s_SystemManager,FALSE);

		s_SystemManager.hScheduleEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,"__SCHEDULER_EVENT__");

		return 1;
	}
	else
	{
		return 0;
	}
}
