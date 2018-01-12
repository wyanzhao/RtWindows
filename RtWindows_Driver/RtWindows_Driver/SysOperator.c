/***********************************ͷ�ļ�**************************************/
#include "../Include/SysOperator.h"

/***********************************��������***********************************************/
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

//��ʼ���ں˷�����Ϣ�ͷ���
#pragma LOCKEDCODE
BOOLEAN InitSystemService()
{
	if (GetVersion())
	{
		//��ϵͳ����������ϵͳ����
		AddServices();
		//��ʼ���̲߳���������ڵ�ַ
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
	//�ָ�ssdt
	SetBackServices();
}
