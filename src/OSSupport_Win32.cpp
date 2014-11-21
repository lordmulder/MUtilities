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

//Win32 API
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <Objbase.h>

//Qt
#include <QMap>
#include <QReadWriteLock>
#include <QLibrary>
#include <QDir>

///////////////////////////////////////////////////////////////////////////////
// KNWON FOLDERS
///////////////////////////////////////////////////////////////////////////////

typedef HRESULT (WINAPI *SHGetKnownFolderPath_t)(const GUID &rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
typedef HRESULT (WINAPI *SHGetFolderPath_t)(HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);

static QMap<size_t, QString>* g_known_folders_map;
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
	if(g_known_folders_map)
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
	if(g_known_folders_map)
	{
		if(g_known_folders_map->contains(folderId))
		{
			return g_known_folders_map->operator[](folderId);
		}
	}

	//Initialize on first call
	if(!g_known_folders_map)
	{
		QLibrary shell32("shell32.dll");
		if(shell32.load())
		{
			g_known_folders_fpGetFolderPath =      (SHGetFolderPath_t)      shell32.resolve("SHGetFolderPathW");
			g_known_folders_fpGetKnownFolderPath = (SHGetKnownFolderPath_t) shell32.resolve("SHGetKnownFolderPath");
		}
		g_known_folders_map = new QMap<size_t, QString>();
	}

	QString folderPath;

	//Now try to get the folder path!
	if(g_known_folders_fpGetKnownFolderPath)
	{
		WCHAR *path = NULL;
		if(g_known_folders_fpGetKnownFolderPath(s_folders[folderId].guid, KF_FLAG_CREATE, NULL, &path) == S_OK)
		{
			//MessageBoxW(0, path, L"SHGetKnownFolderPath", MB_TOPMOST);
			QDir folderTemp = QDir(QDir::fromNativeSeparators(MUTILS_WCHAR2QSTR(path)));
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
			QDir folderTemp = QDir(QDir::fromNativeSeparators(MUTILS_WCHAR2QSTR(path.data())));
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
