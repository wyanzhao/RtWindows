#include <iostream>
#include <string.h>
#include <windows.h>
#include <winbase.h>

#include "../include/RTWindowsRuntime.h"

//#define TEST
#define _CRT_SECURE_NO_WARNINGS

#ifndef TEST
#define RT_TEST
#endif

#define MICROSECOND 1000*1000
#define MILLISECOND 1000
#define SECOND 1

const int N = 10;

ULONG32 Time[N] = { 0 };

using namespace std;

DWORD LogOutTime();

int main()
{
	LARGE_INTEGER BeginTime, EndTime, Freq, TimePassed;
	char ch;

	std::cout << "Ready to test" << std::endl;
	std::cin >> ch;

	RTSTARTUP();
	QueryPerformanceFrequency(&Freq);
	for (int i = 0; i < N; ++i)
	{
		QueryPerformanceCounter(&BeginTime);
		RtSleep(5);
		QueryPerformanceCounter(&EndTime);
		TimePassed.QuadPart = (EndTime.QuadPart - BeginTime.QuadPart) * MICROSECOND / Freq.QuadPart;

		Time[i] = TimePassed.QuadPart;
		Sleep(100);
	}
	LogOutTime();

	return 0;
}

DWORD LogOutTime()
{
	HANDLE hFile;
	char str[255];
	DWORD dwBytesToWrite = 0;
	DWORD dwBytesWritten = 0;
	BOOL	bErrorFlag = FALSE;

	hFile = CreateFile(L"time_data.txt",
		GENERIC_ALL,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		cout << "Unable to create file" << endl;
		return GetLastError();
	}


	for (int i = 0; i < N; i++)
	{
		memset(str, 0, sizeof(str));
		_itoa((int)Time[i], str, 10);
		strcat(str, "\r\n");

		dwBytesToWrite = strlen(str);

		bErrorFlag = WriteFile(
			hFile,
			str,
			dwBytesToWrite,
			&dwBytesWritten,
			NULL);

		if (FALSE == bErrorFlag)
		{
			cout << "Unable to write file" << endl;
			return GetLastError();
		}

		else
		{
			if (dwBytesWritten != dwBytesToWrite)
			{
				cout << "Error: dwBytesWritten != dwBytesWrite" << endl;
			}
		}
	}

	CloseHandle(hFile);
	return 0;
}