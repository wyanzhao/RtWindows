/***********************************头文件**************************************/
#include "../Include/SysOperator.h"

/***********************************函数定义***********************************************/
#pragma LOCKEDCODE
BOOLEAN GetVersion()
{
	ULONG MajorVersion;
	ULONG MinorVersion;
	MinorVersion = 0;
	MajorVersion = 0;

	PsGetVersion(&MajorVersion, &MinorVersion, NULL, NULL);

	if (MajorVersion == 6)
	{
		if (MinorVersion == 0)
		{
			DbgPrint("Windows Vista\n");
			return FALSE;
		}
		else if (MinorVersion == 1)
		{
			DbgPrint("Your system is Windows 7\n");
			return TRUE;
		}
		else if (MinorVersion == 2)
		{
			DbgPrint("Windows 8\n");
			return FALSE;
		}
		else if (MinorVersion == 3)
		{
			DbgPrint("Windows 10\n");
			return FALSE;
		}
	}
	return FALSE;
}

//初始化内核服务信息和方法
#pragma LOCKEDCODE
BOOLEAN InitSystemService()
{
	if (GetVersion())
	{
		//在系统服务表中添加系统调用
		AddServices();
		//初始化线程操作方法入口地址
		InitThreadOperatorAddress();

		KeInitializeSpinLock(&dpc_spinlock);
		KeInitializeDpc(&schedulingDpc, &SchedulDpc, NULL);

		return TRUE;
	}
	return FALSE;
}

#pragma LOCKEDCODE
VOID CleanOutSystem()
{
	//恢复ssdt
	SetBackServices();
}
