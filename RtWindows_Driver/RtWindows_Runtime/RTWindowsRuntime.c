#include "../Include/RTWindowsServices.h"
#include <tlhelp32.h>

static int	 s_bInitialization	= 0;

int SystemInitializationForKernalMode();
int SystemInitializationForUserMode();

static BOOL RtInitialization()
{
	if (!s_bInitialization)
	{
		s_bInitialization = 1;
		if (!SystemInitializationForUserMode())
		{
			return SystemInitializationForKernalMode();
		}
	}
	return TRUE;
}

static BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	switch(CEvent)
	{
		case CTRL_C_EVENT:
			RtExitProcess(0);
			break;

		case CTRL_BREAK_EVENT:
			break;

		case CTRL_CLOSE_EVENT:
			RtExitProcess(0);
			break;

		case CTRL_LOGOFF_EVENT:
			break;

		case CTRL_SHUTDOWN_EVENT:
			break;
	}
	return TRUE;
}

BOOL RtStartup(BOOL bConsole)
{
	DWORD dwThreadID;
	HANDLE hThread;
	HMODULE hDll;
	if (!s_SystemManager.bStartup)
	{
		INIT_LIST_HEAD(&s_SystemManager.ThreadArgvList);

		hDll = LoadLibrary(L"Kernel32.dll");  
		s_SystemManager.lpfnOpenThread = (OPENTHREAD)GetProcAddress(hDll, "OpenThread");  
		if (!s_SystemManager.lpfnOpenThread)
		{
			printf("Error Code: %x\n", GetLastError());
			FreeLibrary(hDll);
			return FALSE;
		}
		FreeLibrary(hDll);
		
		if (RtInitialization())
		{
			s_SystemManager.bStartup = 1;
			
			dwThreadID = GetCurrentThreadId();
			hThread = s_SystemManager.lpfnOpenThread(THREAD_SUSPEND_RESUME,FALSE,dwThreadID); 
			
			RegisteProcess(hThread,dwThreadID);
			s_SystemManager.hSourceMutex = RtCreateMutex(NULL,FALSE,0);
		
			//InstallCoreOSApi();

		if (bConsole)
			{
				SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler,TRUE);
			}
			
			return TRUE;
		}
		else
		{
			s_SystemManager.lpSystemTable = 0;
		}
		
	}

	return FALSE;
}

int RtPrintf(const char *format, ...)
{
	int nRet;
	va_list marker;

	if (s_SystemManager.lpSystemTable)
	{
		RtWaitForSingleObject(s_SystemManager.hSourceMutex, INFINITE);
		RtThreadSafe();
	}

	va_start(marker, format);
	nRet = vprintf(format, marker);
	va_end(marker);

	if (s_SystemManager.lpSystemTable)
	{
		RtThreadUnsafe();
		ReleaseMutex(s_SystemManager.hSourceMutex);
	}

	return nRet;
}
