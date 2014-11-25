///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

#pragma once

//Internal
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include "CriticalSection_Win32.h"

//Win32 API
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <Objbase.h>
#include <Psapi.h>
#include <Sensapi.h>

//Qt
#include <QMap>
#include <QReadWriteLock>
#include <QLibrary>
#include <QDir>

//Main thread ID
static const DWORD g_main_thread_id = GetCurrentThreadId();

///////////////////////////////////////////////////////////////////////////////
// SYSTEM MESSAGE
///////////////////////////////////////////////////////////////////////////////

void MUtils::OS::system_message_nfo(const wchar_t *const title, const wchar_t *const text)
{
	MessageBoxW(NULL, text, title, MB_TOPMOST | MB_TASKMODAL | MB_ICONINFORMATION);
}

void MUtils::OS::system_message_wrn(const wchar_t *const title, const wchar_t *const text)
{
	MessageBoxW(NULL, text, title, MB_TOPMOST | MB_TASKMODAL | MB_ICONWARNING);
}

void MUtils::OS::system_message_err(const wchar_t *const title, const wchar_t *const text)
{
	MessageBoxW(NULL, text, title, MB_TOPMOST | MB_TASKMODAL | MB_ICONERROR);
}

///////////////////////////////////////////////////////////////////////////////
// KNWON FOLDERS
///////////////////////////////////////////////////////////////////////////////

typedef QMap<size_t, QString> KFMap;
typedef HRESULT (WINAPI *SHGetKnownFolderPath_t)(const GUID &rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
typedef HRESULT (WINAPI *SHGetFolderPath_t)(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);

static QScopedPointer<KFMap>  g_known_folders_map;
static SHGetKnownFolderPath_t g_known_folders_fpGetKnownFolderPath;
static SHGetFolderPath_t      g_known_folders_fpGetFolderPath;
static QReadWriteLock         g_known_folders_lock;

const QString &MUtils::OS::known_folder(known_folder_t folder_id)
{
	static const int CSIDL_FLAG_CREATE = 0x8000;
	typedef enum { KF_FLAG_CREATE = 0x00008000 } kf_flags_t;
	
	struct
	{
		const int csidl;
		const GUID guid;
	}
	static s_folders[] =
	{
		{ 0x001c, {0xF1B32785,0x6FBA,0x4FCF,{0x9D,0x55,0x7B,0x8E,0x7F,0x15,0x70,0x91}} },  //CSIDL_LOCAL_APPDATA
		{ 0x0026, {0x905e63b6,0xc1bf,0x494e,{0xb2,0x9c,0x65,0xb7,0x32,0xd3,0xd2,0x1a}} },  //CSIDL_PROGRAM_FILES
		{ 0x0024, {0xF38BF404,0x1D43,0x42F2,{0x93,0x05,0x67,0xDE,0x0B,0x28,0xFC,0x23}} },  //CSIDL_WINDOWS_FOLDER
		{ 0x0025, {0x1AC14E77,0x02E7,0x4E5D,{0xB7,0x44,0x2E,0xB1,0xAE,0x51,0x98,0xB7}} },  //CSIDL_SYSTEM_FOLDER
	};

	size_t folderId = size_t(-1);

	switch(folder_id)
	{
		case FOLDER_LOCALAPPDATA: folderId = 0; break;
		case FOLDER_PROGRAMFILES: folderId = 1; break;
		case FOLDER_SYSTROOT_DIR: folderId = 2; break;
		case FOLDER_SYSTEMFOLDER: folderId = 3; break;
	}

	if(folderId == size_t(-1))
	{
		qWarning("Invalid 'known' folder was requested!");
		return *reinterpret_cast<QString*>(NULL);
	}

	QReadLocker readLock(&g_known_folders_lock);

	//Already in cache?
	if(!g_known_folders_map.isNull())
	{
		if(g_known_folders_map->contains(folderId))
		{
			return g_known_folders_map->operator[](folderId);
		}
	}

	//Obtain write lock to initialize
	readLock.unlock();
	QWriteLocker writeLock(&g_known_folders_lock);

	//Still not in cache?
	if(!g_known_folders_map.isNull())
	{
		if(g_known_folders_map->contains(folderId))
		{
			return g_known_folders_map->operator[](folderId);
		}
	}

	//Initialize on first call
	if(g_known_folders_map.isNull())
	{
		QLibrary shell32("shell32.dll");
		if(shell32.load())
		{
			g_known_folders_fpGetFolderPath =      (SHGetFolderPath_t)      shell32.resolve("SHGetFolderPathW");
			g_known_folders_fpGetKnownFolderPath = (SHGetKnownFolderPath_t) shell32.resolve("SHGetKnownFolderPath");
		}
		g_known_folders_map.reset(new QMap<size_t, QString>());
	}

	QString folderPath;

	//Now try to get the folder path!
	if(g_known_folders_fpGetKnownFolderPath)
	{
		WCHAR *path = NULL;
		if(g_known_folders_fpGetKnownFolderPath(s_folders[folderId].guid, KF_FLAG_CREATE, NULL, &path) == S_OK)
		{
			//MessageBoxW(0, path, L"SHGetKnownFolderPath", MB_TOPMOST);
			QDir folderTemp = QDir(QDir::fromNativeSeparators(MUTILS_QSTR(path)));
			if(folderTemp.exists())
			{
				folderPath = folderTemp.canonicalPath();
			}
			CoTaskMemFree(path);
		}
	}
	else if(g_known_folders_fpGetFolderPath)
	{
		QScopedArrayPointer<WCHAR> path(new WCHAR[4096]);
		if(g_known_folders_fpGetFolderPath(NULL, s_folders[folderId].csidl | CSIDL_FLAG_CREATE, NULL, NULL, path.data()) == S_OK)
		{
			//MessageBoxW(0, path, L"SHGetFolderPathW", MB_TOPMOST);
			QDir folderTemp = QDir(QDir::fromNativeSeparators(MUTILS_QSTR(path.data())));
			if(folderTemp.exists())
			{
				folderPath = folderTemp.canonicalPath();
			}
		}
	}

	//Update cache
	g_known_folders_map->insert(folderId, folderPath);
	return g_known_folders_map->operator[](folderId);
}

///////////////////////////////////////////////////////////////////////////////
// CURRENT DATA (SAFE)
///////////////////////////////////////////////////////////////////////////////

QDate MUtils::OS::current_date(void)
{
	const DWORD MAX_PROC = 1024;
	QScopedArrayPointer<DWORD> processes(new DWORD[MAX_PROC]);
	DWORD bytesReturned = 0;
	
	if(!EnumProcesses(processes.data(), sizeof(DWORD) * MAX_PROC, &bytesReturned))
	{
		return QDate::currentDate();
	}

	const DWORD procCount = bytesReturned / sizeof(DWORD);
	ULARGE_INTEGER lastStartTime;
	memset(&lastStartTime, 0, sizeof(ULARGE_INTEGER));

	for(DWORD i = 0; i < procCount; i++)
	{
		if(HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processes[i]))
		{
			FILETIME processTime[4];
			if(GetProcessTimes(hProc, &processTime[0], &processTime[1], &processTime[2], &processTime[3]))
			{
				ULARGE_INTEGER timeCreation;
				timeCreation.LowPart  = processTime[0].dwLowDateTime;
				timeCreation.HighPart = processTime[0].dwHighDateTime;
				if(timeCreation.QuadPart > lastStartTime.QuadPart)
				{
					lastStartTime.QuadPart = timeCreation.QuadPart;
				}
			}
			CloseHandle(hProc);
		}
	}
	
	FILETIME lastStartTime_fileTime;
	lastStartTime_fileTime.dwHighDateTime = lastStartTime.HighPart;
	lastStartTime_fileTime.dwLowDateTime  = lastStartTime.LowPart;

	FILETIME lastStartTime_localTime;
	if(!FileTimeToLocalFileTime(&lastStartTime_fileTime, &lastStartTime_localTime))
	{
		memcpy(&lastStartTime_localTime, &lastStartTime_fileTime, sizeof(FILETIME));
	}
	
	SYSTEMTIME lastStartTime_system;
	if(!FileTimeToSystemTime(&lastStartTime_localTime, &lastStartTime_system))
	{
		memset(&lastStartTime_system, 0, sizeof(SYSTEMTIME));
		lastStartTime_system.wYear = 1970; lastStartTime_system.wMonth = lastStartTime_system.wDay = 1;
	}

	const QDate currentDate = QDate::currentDate();
	const QDate processDate = QDate(lastStartTime_system.wYear, lastStartTime_system.wMonth, lastStartTime_system.wDay);
	return (currentDate >= processDate) ? currentDate : processDate;
}

///////////////////////////////////////////////////////////////////////////////
// NETWORK STATE
///////////////////////////////////////////////////////////////////////////////

int MUtils::OS::network_status(void)
{
	DWORD dwFlags;
	const BOOL ret = IsNetworkAlive(&dwFlags);
	if(GetLastError() == 0)
	{
		return (ret != FALSE) ? NETWORK_TYPE_YES : NETWORK_TYPE_NON;
	}
	return NETWORK_TYPE_ERR;
}

///////////////////////////////////////////////////////////////////////////////
// FATAL EXIT
///////////////////////////////////////////////////////////////////////////////

static MUtils::Internal::CriticalSection g_fatal_exit_lock;
static volatile bool g_fatal_exit_flag = true;

static DWORD WINAPI fatal_exit_helper(LPVOID lpParameter)
{
	MessageBoxA(NULL, ((LPCSTR) lpParameter), "GURU MEDITATION", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND);
	return 0;
}

void MUtils::OS::fatal_exit(const char* const errorMessage)
{
	g_fatal_exit_lock.enter();
	
	if(!g_fatal_exit_flag)
	{
		return; /*prevent recursive invocation*/
	}

	g_fatal_exit_flag = false;

	if(g_main_thread_id != GetCurrentThreadId())
	{
		if(HANDLE hThreadMain = OpenThread(THREAD_SUSPEND_RESUME, FALSE, g_main_thread_id))
		{
			SuspendThread(hThreadMain); /*stop main thread*/
		}
	}

	if(HANDLE hThread = CreateThread(NULL, 0, fatal_exit_helper, (LPVOID) errorMessage, 0, NULL))
	{
		WaitForSingleObject(hThread, INFINITE);
	}

	for(;;)
	{
		TerminateProcess(GetCurrentProcess(), 666);
	}
}

///////////////////////////////////////////////////////////////////////////////
