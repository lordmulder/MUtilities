///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Win32 API
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <Psapi.h>
#include <Sensapi.h>
#include <Shellapi.h>
#include <PowrProf.h>
#include <Mmsystem.h>
#pragma warning(push)
#pragma warning(disable:4091) //for MSVC2015
#include <ShlObj.h>
#pragma warning(pop)

//Internal
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/GUI.h>
#include "CriticalSection_Win32.h"

//Qt
#include <QMap>
#include <QReadWriteLock>
#include <QLibrary>
#include <QDir>
#include <QWidget>
#include <QProcess>

//Main thread ID
static const DWORD g_main_thread_id = GetCurrentThreadId();

///////////////////////////////////////////////////////////////////////////////
// SYSTEM MESSAGE
///////////////////////////////////////////////////////////////////////////////

static const UINT g_msgBoxFlags = MB_TOPMOST | MB_TASKMODAL | MB_SETFOREGROUND;

void MUtils::OS::system_message_nfo(const wchar_t *const title, const wchar_t *const text)
{
	MessageBoxW(NULL, text, title, g_msgBoxFlags | MB_ICONINFORMATION);
}

void MUtils::OS::system_message_wrn(const wchar_t *const title, const wchar_t *const text)
{
	MessageBoxW(NULL, text, title, g_msgBoxFlags | MB_ICONWARNING);
}

void MUtils::OS::system_message_err(const wchar_t *const title, const wchar_t *const text)
{
	MessageBoxW(NULL, text, title, g_msgBoxFlags | MB_ICONERROR);
}

///////////////////////////////////////////////////////////////////////////////
// FETCH CLI ARGUMENTS
///////////////////////////////////////////////////////////////////////////////

static QReadWriteLock                          g_arguments_lock;
static QScopedPointer<MUtils::OS::ArgumentMap> g_arguments_list;

const MUtils::OS::ArgumentMap &MUtils::OS::arguments(void)
{
	QReadLocker readLock(&g_arguments_lock);

	//Already initialized?
	if(!g_arguments_list.isNull())
	{
		return (*(g_arguments_list.data()));
	}

	readLock.unlock();
	QWriteLocker writeLock(&g_arguments_lock);

	//Still not initialized?
	if(!g_arguments_list.isNull())
	{
		return (*(g_arguments_list.data()));
	}

	g_arguments_list.reset(new ArgumentMap());
	int nArgs = 0;
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	if(NULL != szArglist)
	{
		const QChar separator = QLatin1Char('=');
		const QString argPrefix = QLatin1String("--");
		for(int i = 0; i < nArgs; i++)
		{
			const QString argStr = MUTILS_QSTR(szArglist[i]).trimmed();
			if(argStr.startsWith(argPrefix))
			{
				const QString argData = argStr.mid(2).trimmed();
				if(argData.length() > 0)
				{
					const int separatorIndex = argData.indexOf(separator);
					if(separatorIndex > 0)
					{
						const QString argKey = argData.left(separatorIndex).trimmed();
						const QString argVal = argData.mid(separatorIndex + 1).trimmed();
						g_arguments_list->insertMulti(argKey.toLower(), argVal);
					}
					else
					{
						g_arguments_list->insertMulti(argData.toLower(), QString());
					}
				}
			}
		}
		LocalFree(szArglist);
	}
	else
	{
		qWarning("CommandLineToArgvW() has failed !!!");
	}

	return (*(g_arguments_list.data()));
}

///////////////////////////////////////////////////////////////////////////////
// COPY FILE
///////////////////////////////////////////////////////////////////////////////

typedef struct _progress_callback_data_t
{
	MUtils::OS::progress_callback_t callback_function;
	void *user_data;
}
progress_callback_data_t;

static DWORD __stdcall copy_file_progress(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber, DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
	if(const progress_callback_data_t *data = (progress_callback_data_t*) lpData)
	{
		const double progress = qBound(0.0, double(TotalBytesTransferred.QuadPart) / double(TotalFileSize.QuadPart), 1.0);
		return data->callback_function(progress, data->user_data) ? PROGRESS_CONTINUE : PROGRESS_CANCEL;
	}
	return PROGRESS_CONTINUE;
}

MUTILS_API bool MUtils::OS::copy_file(const QString &sourcePath, const QString &outputPath, const bool &overwrite, const progress_callback_t callback, void *const userData)
{
	progress_callback_data_t callback_data = { callback, userData };
	BOOL cancel = FALSE;
	const BOOL result = CopyFileExW(MUTILS_WCHR(QDir::toNativeSeparators(sourcePath)), MUTILS_WCHR(QDir::toNativeSeparators(outputPath)), ((callback_data.callback_function) ? copy_file_progress : NULL), ((callback_data.callback_function) ? &callback_data : NULL), &cancel, (overwrite ? 0 : COPY_FILE_FAIL_IF_EXISTS));

	if(result == FALSE)
	{
		const DWORD errorCode = GetLastError();
		if(errorCode != ERROR_REQUEST_ABORTED)
		{
			qWarning("CopyFile() failed with error code 0x%08X!", errorCode);
		}
		else
		{
			qWarning("CopyFile() operation was abroted by user!");
		}
	}

	return (result != FALSE);
}

///////////////////////////////////////////////////////////////////////////////
// GET FILE VERSION
///////////////////////////////////////////////////////////////////////////////

static bool get_file_version_helper(const QString fileName, PVOID buffer, const size_t &size, quint16 *const major, quint16 *const minor, quint16 *const patch, quint16 *const build)
{
	if(!GetFileVersionInfo(MUTILS_WCHR(fileName), 0, size, buffer))
	{
		qWarning("GetFileVersionInfo() has failed, file version cannot be determined!");
		return false;
	}

	VS_FIXEDFILEINFO *verInfo;
	UINT verInfoLen;
	if(!VerQueryValue(buffer, L"\\", (LPVOID*)(&verInfo), &verInfoLen))
	{
		qWarning("VerQueryValue() has failed, file version cannot be determined!");
		return false;
	}

	if(major) *major = quint16((verInfo->dwFileVersionMS >> 16) & 0x0000FFFF);
	if(minor) *minor = quint16((verInfo->dwFileVersionMS)       & 0x0000FFFF);
	if(patch) *patch = quint16((verInfo->dwFileVersionLS >> 16) & 0x0000FFFF);
	if(build) *build = quint16((verInfo->dwFileVersionLS)       & 0x0000FFFF);

	return true;
}

bool MUtils::OS::get_file_version(const QString fileName, quint16 *const major, quint16 *const minor, quint16 *const patch, quint16 *const build)
{
	if(major) *major = 0U; if(minor) *minor = 0U;
	if(patch) *patch = 0U; if(build) *build = 0U;

	const DWORD size = GetFileVersionInfoSize(MUTILS_WCHR(fileName), NULL);
	if(size < 1)
	{
		qWarning("GetFileVersionInfoSize() has failed, file version cannot be determined!");
		return false;
	}
	
	PVOID buffer = _malloca(size);
	if(!buffer)
	{
		qWarning("Memory allocation has failed!");
		return false;
	}

	const bool success = get_file_version_helper(fileName, buffer, size, major, minor, patch, build);

	_freea(buffer);
	return success;
}

///////////////////////////////////////////////////////////////////////////////
// OS VERSION DETECTION
///////////////////////////////////////////////////////////////////////////////

static bool g_os_version_initialized = false;
static MUtils::OS::Version::os_version_t g_os_version_info = MUtils::OS::Version::UNKNOWN_OPSYS;
static QReadWriteLock g_os_version_lock;

//Maps marketing names to the actual Windows NT versions
static const struct
{
	MUtils::OS::Version::os_version_t version;
	const char friendlyName[64];
}
g_os_version_lut[] =
{
	{ MUtils::OS::Version::WINDOWS_WIN2K, "Windows 2000"                                  },	//2000
	{ MUtils::OS::Version::WINDOWS_WINXP, "Windows XP or Windows XP Media Center Edition" },	//XP
	{ MUtils::OS::Version::WINDOWS_XPX64, "Windows Server 2003 or Windows XP x64"         },	//XP_x64
	{ MUtils::OS::Version::WINDOWS_VISTA, "Windows Vista or Windows Server 2008"          },	//Vista
	{ MUtils::OS::Version::WINDOWS_WIN70, "Windows 7 or Windows Server 2008 R2"           },	//7
	{ MUtils::OS::Version::WINDOWS_WIN80, "Windows 8 or Windows Server 2012"              },	//8
	{ MUtils::OS::Version::WINDOWS_WIN81, "Windows 8.1 or Windows Server 2012 R2"         },	//8.1
	{ MUtils::OS::Version::WINDOWS_WN100, "Windows 10 or Windows Server 2014 (Preview)"   },	//10
	{ MUtils::OS::Version::UNKNOWN_OPSYS, "N/A" }
};

//OS version data dtructures
namespace MUtils
{
	namespace OS
	{
		namespace Version
		{
			//Comparision operators for os_version_t
			bool os_version_t::operator== (const os_version_t &rhs) const { return (type == rhs.type) && (versionMajor == rhs.versionMajor) && ((versionMinor == rhs.versionMinor)); }
			bool os_version_t::operator!= (const os_version_t &rhs) const { return (type != rhs.type) || (versionMajor != rhs.versionMajor) || ((versionMinor != rhs.versionMinor)); }
			bool os_version_t::operator>  (const os_version_t &rhs) const { return (type == rhs.type) && ((versionMajor > rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor >  rhs.versionMinor))); }
			bool os_version_t::operator>= (const os_version_t &rhs) const { return (type == rhs.type) && ((versionMajor > rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor >= rhs.versionMinor))); }
			bool os_version_t::operator<  (const os_version_t &rhs) const { return (type == rhs.type) && ((versionMajor < rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor <  rhs.versionMinor))); }
			bool os_version_t::operator<= (const os_version_t &rhs) const { return (type == rhs.type) && ((versionMajor < rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor <= rhs.versionMinor))); }

			//Known Windows NT versions
			const os_version_t WINDOWS_WIN2K = { OS_WINDOWS,  5, 0 };	// 2000
			const os_version_t WINDOWS_WINXP = { OS_WINDOWS,  5, 1 };	// XP
			const os_version_t WINDOWS_XPX64 = { OS_WINDOWS,  5, 2 };	// XP_x64
			const os_version_t WINDOWS_VISTA = { OS_WINDOWS,  6, 0 };	// Vista
			const os_version_t WINDOWS_WIN70 = { OS_WINDOWS,  6, 1 };	// 7
			const os_version_t WINDOWS_WIN80 = { OS_WINDOWS,  6, 2 };	// 8
			const os_version_t WINDOWS_WIN81 = { OS_WINDOWS,  6, 3 };	// 8.1
			const os_version_t WINDOWS_WN100 = { OS_WINDOWS, 10, 0 };	// 10

			//Unknown OS
			const os_version_t UNKNOWN_OPSYS = { OS_UNKNOWN, 0, 0 };	// N/A
		}
	}
}

static bool verify_os_version(const DWORD major, const DWORD minor)
{
	OSVERSIONINFOEXW osvi;
	DWORDLONG dwlConditionMask = 0;

	//Initialize the OSVERSIONINFOEX structure
	memset(&osvi, 0, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = major;
	osvi.dwMinorVersion = minor;
	osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;

	//Initialize the condition mask
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_PLATFORMID,   VER_EQUAL);

	// Perform the test
	const BOOL ret = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_PLATFORMID, dwlConditionMask);

	//Error checking
	if(!ret)
	{
		if(GetLastError() != ERROR_OLD_WIN_VERSION)
		{
			qWarning("VerifyVersionInfo() system call has failed!");
		}
	}

	return (ret != FALSE);
}

static bool get_real_os_version(unsigned int *major, unsigned int *minor, bool *pbOverride)
{
	*major = *minor = 0;
	*pbOverride = false;
	
	//Initialize local variables
	OSVERSIONINFOEXW osvi;
	memset(&osvi, 0, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

	//Try GetVersionEx() first
	if(GetVersionExW((LPOSVERSIONINFOW)&osvi) == FALSE)
	{
		qWarning("GetVersionEx() has failed, cannot detect Windows version!");
		return false;
	}

	//Make sure we are running on NT
	if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		*major = osvi.dwMajorVersion;
		*minor = osvi.dwMinorVersion;
	}
	else
	{
		qWarning("Not running on Windows NT, unsupported operating system!");
		return false;
	}

	//Determine the real *major* version first
	forever
	{
		const DWORD nextMajor = (*major) + 1;
		if(verify_os_version(nextMajor, 0))
		{
			*pbOverride = true;
			*major = nextMajor;
			*minor = 0;
			continue;
		}
		break;
	}

	//Now also determine the real *minor* version
	forever
	{
		const DWORD nextMinor = (*minor) + 1;
		if(verify_os_version((*major), nextMinor))
		{
			*pbOverride = true;
			*minor = nextMinor;
			continue;
		}
		break;
	}

	//Workaround for the mess that is sometimes referred to as "Windows 10"
	if(((*major) > 6) || (((*major) == 6) && ((*minor) >= 2)))
	{
		quint16 kernel32_major, kernel32_minor;
		if(MUtils::OS::get_file_version(QLatin1String("kernel32"), &kernel32_major, &kernel32_minor))
		{
			if((kernel32_major > (*major)) || ((kernel32_major == (*major)) && (kernel32_minor > (*minor))))
			{
				*major = kernel32_major;
				*minor = kernel32_minor;
				*pbOverride = true;
			}
		}
	}

	return true;
}

const MUtils::OS::Version::os_version_t &MUtils::OS::os_version(void)
{
	QReadLocker readLock(&g_os_version_lock);

	//Already initialized?
	if(g_os_version_initialized)
	{
		return g_os_version_info;
	}
	
	readLock.unlock();
	QWriteLocker writeLock(&g_os_version_lock);

	//Initialized now?
	if(g_os_version_initialized)
	{
		return g_os_version_info;
	}

	//Detect OS version
	unsigned int major, minor; bool overrideFlg;
	if(get_real_os_version(&major, &minor, &overrideFlg))
	{
		g_os_version_info.type = Version::OS_WINDOWS;
		g_os_version_info.versionMajor = major;
		g_os_version_info.versionMinor = minor;
		g_os_version_info.overrideFlag = overrideFlg;
	}
	else
	{
		qWarning("Failed to determin the operating system version!");
	}

	//Completed
	g_os_version_initialized = true;
	return g_os_version_info;
}

const char *MUtils::OS::os_friendly_name(const MUtils::OS::Version::os_version_t &os_version)
{
	for(size_t i = 0; g_os_version_lut[i].version != MUtils::OS::Version::UNKNOWN_OPSYS; i++)
	{
		if(os_version == g_os_version_lut[i].version)
		{
			return g_os_version_lut[i].friendlyName;
		}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// WINE DETECTION
///////////////////////////////////////////////////////////////////////////////

static bool g_wine_deteced = false;
static bool g_wine_initialized = false;
static QReadWriteLock g_wine_lock;

static const bool detect_wine(void)
{
	bool is_wine = false;
	
	QLibrary ntdll("ntdll.dll");
	if(ntdll.load())
	{
		if(ntdll.resolve("wine_nt_to_unix_file_name") != NULL) is_wine = true;
		if(ntdll.resolve("wine_get_version")          != NULL) is_wine = true;
		ntdll.unload();
	}

	return is_wine;
}

const bool &MUtils::OS::running_on_wine(void)
{
	QReadLocker readLock(&g_wine_lock);

	//Already initialized?
	if(g_wine_initialized)
	{
		return g_wine_deteced;
	}

	readLock.unlock();
	QWriteLocker writeLock(&g_wine_lock);

	//Initialized now?
	if(g_wine_initialized)
	{
		return g_wine_deteced;
	}

	//Try to detect Wine
	g_wine_deteced = detect_wine();
	g_wine_initialized = true;

	return g_wine_deteced;
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
// CURRENT DATA & TIME
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

quint64 MUtils::OS::current_file_time(void)
{
	FILETIME fileTime;
	GetSystemTimeAsFileTime(&fileTime);

	ULARGE_INTEGER temp;
	temp.HighPart = fileTime.dwHighDateTime;
	temp.LowPart = fileTime.dwLowDateTime;

	return temp.QuadPart;
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS ELEVATION
///////////////////////////////////////////////////////////////////////////////

static bool user_is_admin_helper(void)
{
	HANDLE hToken = NULL;
	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		return false;
	}

	DWORD dwSize = 0;
	if(!GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwSize))
	{
		if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			CloseHandle(hToken);
			return false;
		}
	}

	PTOKEN_GROUPS lpGroups = (PTOKEN_GROUPS) malloc(dwSize);
	if(!lpGroups)
	{
		CloseHandle(hToken);
		return false;
	}

	if(!GetTokenInformation(hToken, TokenGroups, lpGroups, dwSize, &dwSize))
	{
		free(lpGroups);
		CloseHandle(hToken);
		return false;
	}

	PSID lpSid = NULL; SID_IDENTIFIER_AUTHORITY Authority = {SECURITY_NT_AUTHORITY};
	if(!AllocateAndInitializeSid(&Authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &lpSid))
	{
		free(lpGroups);
		CloseHandle(hToken);
		return false;
	}

	bool bResult = false;
	for(DWORD i = 0; i < lpGroups->GroupCount; i++)
	{
		if(EqualSid(lpSid, lpGroups->Groups[i].Sid))
		{
			bResult = true;
			break;
		}
	}

	FreeSid(lpSid);
	free(lpGroups);
	CloseHandle(hToken);
	return bResult;
}

bool MUtils::OS::is_elevated(bool *bIsUacEnabled)
{
	if(bIsUacEnabled)
	{
		*bIsUacEnabled = false;
	}

	bool bIsProcessElevated = false;
	HANDLE hToken = NULL;
	
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		TOKEN_ELEVATION_TYPE tokenElevationType;
		DWORD returnLength;
		if(GetTokenInformation(hToken, TokenElevationType, &tokenElevationType, sizeof(TOKEN_ELEVATION_TYPE), &returnLength))
		{
			if(returnLength == sizeof(TOKEN_ELEVATION_TYPE))
			{
				switch(tokenElevationType)
				{
				case TokenElevationTypeDefault:
					qDebug("Process token elevation type: Default -> UAC is disabled.\n");
					break;
				case TokenElevationTypeFull:
					qWarning("Process token elevation type: Full -> potential security risk!\n");
					bIsProcessElevated = true;
					if(bIsUacEnabled) *bIsUacEnabled = true;
					break;
				case TokenElevationTypeLimited:
					qDebug("Process token elevation type: Limited -> not elevated.\n");
					if(bIsUacEnabled) *bIsUacEnabled = true;
					break;
				default:
					qWarning("Unknown tokenElevationType value: %d", tokenElevationType);
					break;
				}
			}
			else
			{
				qWarning("GetTokenInformation() return an unexpected size!");
			}
		}
		CloseHandle(hToken);
	}
	else
	{
		qWarning("Failed to open process token!");
	}

	return bIsProcessElevated;
}

bool MUtils::OS::user_is_admin(void)
{
	bool isAdmin = false;

	//Check for process elevation and UAC support first!
	if(MUtils::OS::is_elevated(&isAdmin))
	{
		qWarning("Process is elevated -> user is admin!");
		return true;
	}
	
	//If not elevated and UAC is not available -> user must be in admin group!
	if(!isAdmin)
	{
		qDebug("UAC is disabled/unavailable -> checking for Administrators group");
		isAdmin = user_is_admin_helper();
	}

	return isAdmin;
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
// MESSAGE HANDLER
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::handle_os_message(const void *const message, long *result)
{
	const MSG *const msg = reinterpret_cast<const MSG*>(message);

	switch(msg->message)
	{
	case WM_QUERYENDSESSION:
		qWarning("WM_QUERYENDSESSION message received!");
		*result = MUtils::GUI::broadcast(MUtils::GUI::USER_EVENT_QUERYENDSESSION, false) ? TRUE : FALSE;
		return true;
	case WM_ENDSESSION:
		qWarning("WM_ENDSESSION message received!");
		if(msg->wParam == TRUE)
		{
			MUtils::GUI::broadcast(MUtils::GUI::USER_EVENT_ENDSESSION, false);
			MUtils::GUI::force_quit();
			exit(1);
		}
		*result = 0;
		return true;
	default:
		/*ignore this message and let Qt handle it*/
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// SLEEP
///////////////////////////////////////////////////////////////////////////////

void MUtils::OS::sleep_ms(const size_t &duration)
{
	Sleep((DWORD) duration);
}

///////////////////////////////////////////////////////////////////////////////
// EXECUTABLE CHECK
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::is_executable_file(const QString &path)
{
	bool bIsExecutable = false;
	DWORD binaryType;
	if(GetBinaryType(MUTILS_WCHR(QDir::toNativeSeparators(path)), &binaryType))
	{
		bIsExecutable = (binaryType == SCS_32BIT_BINARY || binaryType == SCS_64BIT_BINARY);
	}
	return bIsExecutable;
}

///////////////////////////////////////////////////////////////////////////////
// HIBERNATION / SHUTDOWN
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::is_hibernation_supported(void)
{
	bool hibernationSupported = false;

	SYSTEM_POWER_CAPABILITIES pwrCaps;
	SecureZeroMemory(&pwrCaps, sizeof(SYSTEM_POWER_CAPABILITIES));
	
	if(GetPwrCapabilities(&pwrCaps))
	{
		hibernationSupported = pwrCaps.SystemS4 && pwrCaps.HiberFilePresent;
	}

	return hibernationSupported;
}

bool MUtils::OS::shutdown_computer(const QString &message, const unsigned long timeout, const bool forceShutdown, const bool hibernate)
{
	HANDLE hToken = NULL;

	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		TOKEN_PRIVILEGES privileges;
		memset(&privileges, 0, sizeof(TOKEN_PRIVILEGES));
		privileges.PrivilegeCount = 1;
		privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		
		if(LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid))
		{
			if(AdjustTokenPrivileges(hToken, FALSE, &privileges, NULL, NULL, NULL))
			{
				if(hibernate)
				{
					if(SetSuspendState(TRUE, TRUE, TRUE))
					{
						return true;
					}
				}
				const DWORD reason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_FLAG_PLANNED;
				return InitiateSystemShutdownEx(NULL, const_cast<wchar_t*>(MUTILS_WCHR(message)), timeout, forceShutdown ? TRUE : FALSE, FALSE, reason);
			}
		}
	}
	
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// FREE DISKSPACE
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::free_diskspace(const QString &path, quint64 &freeSpace)
{
	ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
	if(GetDiskFreeSpaceExW(reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(path).utf16()), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
	{
		freeSpace = freeBytesAvailable.QuadPart;
		return true;;
	}

	freeSpace = -1;
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// SHELL OPEN
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::shell_open(const QWidget *parent, const QString &url, const QString &parameters, const QString &directory, const bool explore)
{
	return ((int) ShellExecuteW((parent ? parent->winId() : NULL), (explore ? L"explore" : L"open"), MUTILS_WCHR(url), ((!parameters.isEmpty()) ? MUTILS_WCHR(parameters) : NULL), ((!directory.isEmpty()) ? MUTILS_WCHR(directory) : NULL), SW_SHOW)) > 32;
}

bool MUtils::OS::shell_open(const QWidget *parent, const QString &url, const bool explore)
{
	return shell_open(parent, url, QString(), QString(), explore);
}

///////////////////////////////////////////////////////////////////////////////
// OPEN MEDIA FILE
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::open_media_file(const QString &mediaFilePath)
{
	const static wchar_t *registryPrefix[2] = { L"SOFTWARE\\", L"SOFTWARE\\Wow6432Node\\" };
	const static wchar_t *registryKeys[3] = 
	{
		L"Microsoft\\Windows\\CurrentVersion\\Uninstall\\{97D341C8-B0D1-4E4A-A49A-C30B52F168E9}",
		L"Microsoft\\Windows\\CurrentVersion\\Uninstall\\{DB9E4EAB-2717-499F-8D56-4CC8A644AB60}",
		L"foobar2000"
	};
	const static wchar_t *appNames[4] = { L"smplayer_portable.exe", L"smplayer.exe", L"MPUI.exe", L"foobar2000.exe" };
	const static wchar_t *valueNames[2] = { L"InstallLocation", L"InstallDir" };

	for(size_t i = 0; i < 3; i++)
	{
		for(size_t j = 0; j < 2; j++)
		{
			QString mplayerPath;
			HKEY registryKeyHandle = NULL;

			const QString currentKey = MUTILS_QSTR(registryPrefix[j]).append(MUTILS_QSTR(registryKeys[i]));
			if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, MUTILS_WCHR(currentKey), 0, KEY_READ, &registryKeyHandle) == ERROR_SUCCESS)
			{
				for(size_t k = 0; k < 2; k++)
				{
					wchar_t Buffer[4096];
					DWORD BuffSize = sizeof(wchar_t*) * 4096;
					DWORD DataType = REG_NONE;
					if(RegQueryValueExW(registryKeyHandle, valueNames[k], 0, &DataType, reinterpret_cast<BYTE*>(Buffer), &BuffSize) == ERROR_SUCCESS)
					{
						if((DataType == REG_SZ) || (DataType == REG_EXPAND_SZ) || (DataType == REG_LINK))
						{
							mplayerPath = MUTILS_QSTR(Buffer);
							break;
						}
					}
				}
				RegCloseKey(registryKeyHandle);
			}

			if(!mplayerPath.isEmpty())
			{
				QDir mplayerDir(mplayerPath);
				if(mplayerDir.exists())
				{
					for(size_t k = 0; k < 4; k++)
					{
						if(mplayerDir.exists(MUTILS_QSTR(appNames[k])))
						{
							qDebug("Player found at:\n%s\n", MUTILS_UTF8(mplayerDir.absoluteFilePath(MUTILS_QSTR(appNames[k]))));
							QProcess::startDetached(mplayerDir.absoluteFilePath(MUTILS_QSTR(appNames[k])), QStringList() << QDir::toNativeSeparators(mediaFilePath));
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// DEBUGGER CHECK
///////////////////////////////////////////////////////////////////////////////

static bool change_process_priority_helper(const HANDLE hProcess, const int priority)
{
	bool ok = false;

	switch(qBound(-2, priority, 2))
	{
	case 2:
		ok = (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS) == TRUE);
		break;
	case 1:
		if(!(ok = (SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS) == TRUE)))
		{
			ok = (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS) == TRUE);
		}
		break;
	case 0:
		ok = (SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS) == TRUE);
		break;
	case -1:
		if(!(ok = (SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS) == TRUE)))
		{
			ok = (SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS) == TRUE);
		}
		break;
	case -2:
		ok = (SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS) == TRUE);
		break;
	}

	return ok;
}

bool MUtils::OS::change_process_priority(const int priority)
{
	return change_process_priority_helper(GetCurrentProcess(), priority);
}

bool MUtils::OS::change_process_priority(const QProcess *proc, const int priority)
{
	if(Q_PID qPid = proc->pid())
	{
		return change_process_priority_helper(qPid->hProcess, priority);
	}
	else
	{
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS ID
///////////////////////////////////////////////////////////////////////////////

quint32 MUtils::OS::process_id(const QProcess *const proc)
{
	PROCESS_INFORMATION *procInf = proc->pid();
	return (procInf) ? procInf->dwProcessId : NULL;
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS SUSPEND/RESUME
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::suspend_process(const QProcess *proc, const bool suspend)
{
	if(Q_PID pid = proc->pid())
	{
		if(suspend)
		{
			return (SuspendThread(pid->hThread) != ((DWORD) -1));
		}
		else
		{
			return (ResumeThread(pid->hThread)  != ((DWORD) -1));
		}
	}
	else
	{
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// SYSTEM TIMER
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::setup_timer_resolution(const quint32 &interval)
{
	return timeBeginPeriod(interval) == TIMERR_NOERROR;
}

bool MUtils::OS::reset_timer_resolution(const quint32 &interval)
{
	return timeEndPeriod(interval) == TIMERR_NOERROR;
}

///////////////////////////////////////////////////////////////////////////////
// CHECK KEY STATE
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::check_key_state_esc(void)
{
	return (GetAsyncKeyState(VK_ESCAPE) & 0x0001) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// SHELL CHANGE NOTIFICATION
///////////////////////////////////////////////////////////////////////////////

void MUtils::OS::shell_change_notification(void)
{
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

///////////////////////////////////////////////////////////////////////////////
// WOW64 REDIRECTION
///////////////////////////////////////////////////////////////////////////////

typedef BOOL (_stdcall *Wow64DisableWow64FsRedirectionFun)(void *OldValue);
typedef BOOL (_stdcall *Wow64RevertWow64FsRedirectionFun )(void *OldValue);

static QReadWriteLock                    g_wow64redir_lock;
static QScopedPointer<QLibrary>          g_wow64redir_kernel32;
static Wow64DisableWow64FsRedirectionFun g_wow64redir_disable = NULL;
static Wow64RevertWow64FsRedirectionFun  g_wow64redir_revert  = NULL;

static bool wow64fsredir_init()
{
	QWriteLocker writeLock(&g_wow64redir_lock);
	if(g_wow64redir_disable && g_wow64redir_revert)
	{
		return true; /*already initialized*/
	}

	if(g_wow64redir_kernel32.isNull())
	{
		g_wow64redir_kernel32.reset(new QLibrary("kernel32.dll"));
	}

	if(!g_wow64redir_kernel32->isLoaded())
	{
		if(!g_wow64redir_kernel32->load())
		{
			return false; /*faild to load kernel32.dll*/
		}
	}

	g_wow64redir_disable = (Wow64DisableWow64FsRedirectionFun) g_wow64redir_kernel32->resolve("Wow64DisableWow64FsRedirection");
	g_wow64redir_revert  = (Wow64RevertWow64FsRedirectionFun)  g_wow64redir_kernel32->resolve("Wow64RevertWow64FsRedirection");

	return (g_wow64redir_disable && g_wow64redir_revert);
}

#define WOW64FSREDIR_INIT(RDLOCK) do \
{ \
	while(!(g_wow64redir_disable && g_wow64redir_revert)) \
	{ \
		(RDLOCK).unlock(); \
		if(!wow64fsredir_init()) return false; \
		(RDLOCK).relock(); \
	} \
} \
while(0)

bool MUtils::OS::wow64fsredir_disable(void *oldValue)
{
	QReadLocker readLock(&g_wow64redir_lock);
	WOW64FSREDIR_INIT(readLock);
	if(g_wow64redir_disable(oldValue))
	{
		return true;
	}
	return false;
}

bool MUtils::OS::wow64fsredir_revert(void *oldValue)
{
	QReadLocker readLock(&g_wow64redir_lock);
	WOW64FSREDIR_INIT(readLock);
	if(g_wow64redir_revert(oldValue))
	{
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// DEBUGGER CHECK
///////////////////////////////////////////////////////////////////////////////

#if (!(MUTILS_DEBUG))
static __forceinline bool is_debugger_present(void)
{
	__try
	{
		CloseHandle((HANDLE)((DWORD_PTR)-3));
	}
	__except(1)
	{
		return true;
	}

	BOOL bHaveDebugger = FALSE;
	if(CheckRemoteDebuggerPresent(GetCurrentProcess(), &bHaveDebugger))
	{
		if(bHaveDebugger) return true;
	}

	return IsDebuggerPresent();
}
static __forceinline bool check_debugger_helper(void)
{
	if(is_debugger_present())
	{
		MUtils::OS::fatal_exit(L"Not a debug build. Please unload debugger and try again!");
		return true;
	}
	return false;
}
#else
#define check_debugger_helper() (false)
#endif

void MUtils::OS::check_debugger(void)
{
	check_debugger_helper();
}

static volatile bool g_debug_check = check_debugger_helper();

///////////////////////////////////////////////////////////////////////////////
// FATAL EXIT
///////////////////////////////////////////////////////////////////////////////

static MUtils::Internal::CriticalSection g_fatal_exit_lock;
static volatile bool g_fatal_exit_flag = true;

static DWORD WINAPI fatal_exit_helper(LPVOID lpParameter)
{
	MUtils::OS::system_message_err(L"GURU MEDITATION", (LPWSTR) lpParameter);
	return 0;
}

void MUtils::OS::fatal_exit(const wchar_t* const errorMessage)
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
