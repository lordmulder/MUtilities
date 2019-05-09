///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2019 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <WinIoCtl.h>
#pragma warning(push)
#pragma warning(disable:4091) //for MSVC2015
#include <ShlObj.h>
#pragma warning(pop)

//CRT
#include <io.h>

//Internal
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/GUI.h>
#include "Internal.h"
#include "CriticalSection_Win32.h"
#include "Utils_Win32.h"

//Qt
#include <QMap>
#include <QReadWriteLock>
#include <QDir>
#include <QWidget>
#include <QProcess>
#include <QSet>

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

const QStringList MUtils::OS::crack_command_line(const QString &command_line)
{
	int nArgs = 0;
	LPWSTR *szArglist = CommandLineToArgvW(command_line.isNull() ? GetCommandLineW() : MUTILS_WCHR(command_line), &nArgs);

	QStringList command_line_tokens;
	if(NULL != szArglist)
	{
		for(int i = 0; i < nArgs; i++)
		{
			const QString argStr = MUTILS_QSTR(szArglist[i]).trimmed();
			if(!argStr.isEmpty())
			{
				command_line_tokens << argStr;
			}
		}
		LocalFree(szArglist);
	}

	return command_line_tokens;
}

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
	const QStringList argList = crack_command_line();

	if(!argList.isEmpty())
	{
		const QString argPrefix = QLatin1String("--");
		const QChar   separator = QLatin1Char('=');

		bool firstToken = true;
		for(QStringList::ConstIterator iter = argList.constBegin(); iter != argList.constEnd(); iter++)
		{
			if(firstToken)
			{
				firstToken = false;
				continue; /*skip executable file name*/
			}
			if(iter->startsWith(argPrefix))
			{
				const QString argData = iter->mid(2).trimmed();
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
	}
	else if(argList.empty())
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
	{ MUtils::OS::Version::WINDOWS_WN100, "Windows 10 or Windows Server 2016"             },	//10
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
			const os_version_t WINDOWS_WIN2K = { OS_WINDOWS,  5, 0,  2195, 0 };	// 2000
			const os_version_t WINDOWS_WINXP = { OS_WINDOWS,  5, 1,  2600, 0 };	// XP
			const os_version_t WINDOWS_XPX64 = { OS_WINDOWS,  5, 2,  3790, 0 };	// XP_x64
			const os_version_t WINDOWS_VISTA = { OS_WINDOWS,  6, 0,  6000, 0 };	// Vista
			const os_version_t WINDOWS_WIN70 = { OS_WINDOWS,  6, 1,  7600, 0 };	// 7
			const os_version_t WINDOWS_WIN80 = { OS_WINDOWS,  6, 2,  9200, 0 };	// 8
			const os_version_t WINDOWS_WIN81 = { OS_WINDOWS,  6, 3,  9600, 0 };	// 8.1
			const os_version_t WINDOWS_WN100 = { OS_WINDOWS, 10, 0, 10240, 0 };	// 10

			//Unknown OS
			const os_version_t UNKNOWN_OPSYS = { OS_UNKNOWN, 0,  0,     0, 0 };	// N/A
		}
	}
}

static inline DWORD SAFE_ADD(const DWORD &a, const DWORD &b, const DWORD &limit = MAXDWORD)
{
	return ((a >= limit) || (b >= limit) || ((limit - a) <= b)) ? limit : (a + b);
}


static void initialize_os_version(OSVERSIONINFOEXW *const osInfo)
{
	memset(osInfo, 0, sizeof(OSVERSIONINFOEXW));
	osInfo->dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
}

static inline DWORD initialize_step_size(const DWORD &limit)
{
	DWORD result = 1;
	while (result < limit)
	{
		result = SAFE_ADD(result, result);
	}
	return result;
}

#pragma warning(push)
#pragma warning(disable: 4996)

static bool rtl_get_version(OSVERSIONINFOEXW *const osInfo)
{
	typedef LONG(__stdcall *RtlGetVersion)(LPOSVERSIONINFOEXW);
	if (const HMODULE ntdll = GetModuleHandleW(L"ntdll"))
	{
		if (const RtlGetVersion pRtlGetVersion = (RtlGetVersion)GetProcAddress(ntdll, "RtlGetVersion"))
		{
			initialize_os_version(osInfo);
			if (pRtlGetVersion(osInfo) == 0)
			{
				return true;
			}
		}
	}

	//Fallback
	initialize_os_version(osInfo);
	return (GetVersionExW((LPOSVERSIONINFOW)osInfo) != FALSE);
}

#pragma warning(pop) 

static bool rtl_verify_version(OSVERSIONINFOEXW *const osInfo, const ULONG typeMask, const ULONGLONG condMask)
{
	typedef LONG(__stdcall *RtlVerifyVersionInfo)(LPOSVERSIONINFOEXW, ULONG, ULONGLONG);
	if (const HMODULE ntdll = GetModuleHandleW(L"ntdll"))
	{
		if (const RtlVerifyVersionInfo pRtlVerifyVersionInfo = (RtlVerifyVersionInfo)GetProcAddress(ntdll, "RtlVerifyVersionInfo"))
		{
			if (pRtlVerifyVersionInfo(osInfo, typeMask, condMask) == 0)
			{
				return true;
			}
		}
	}

	//Fallback
	return (VerifyVersionInfoW(osInfo, typeMask, condMask) != FALSE);
}

static bool verify_os_version(const DWORD major, const DWORD minor)
{
	OSVERSIONINFOEXW osvi;
	DWORDLONG dwlConditionMask = 0;
	initialize_os_version(&osvi);

	//Initialize the OSVERSIONINFOEX structure
	osvi.dwMajorVersion = major;
	osvi.dwMinorVersion = minor;
	osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;

	//Initialize the condition mask
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_PLATFORMID,   VER_EQUAL);

	// Perform the test
	const BOOL ret = rtl_verify_version(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_PLATFORMID, dwlConditionMask);

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

static bool verify_os_build(const DWORD build)
{
	OSVERSIONINFOEXW osvi;
	DWORDLONG dwlConditionMask = 0;
	initialize_os_version(&osvi);

	//Initialize the OSVERSIONINFOEX structure
	osvi.dwBuildNumber = build;
	osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;

	//Initialize the condition mask
	VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_PLATFORMID, VER_EQUAL);

	// Perform the test
	const BOOL ret = rtl_verify_version(&osvi, VER_BUILDNUMBER | VER_PLATFORMID, dwlConditionMask);

	//Error checking
	if (!ret)
	{
		if (GetLastError() != ERROR_OLD_WIN_VERSION)
		{
			qWarning("VerifyVersionInfo() system call has failed!");
		}
	}

	return (ret != FALSE);
}

static bool verify_os_spack(const WORD spack)
{
	OSVERSIONINFOEXW osvi;
	DWORDLONG dwlConditionMask = 0;
	initialize_os_version(&osvi);

	//Initialize the OSVERSIONINFOEX structure
	osvi.wServicePackMajor = spack;
	osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;

	//Initialize the condition mask
	VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_PLATFORMID, VER_EQUAL);

	// Perform the test
	const BOOL ret = rtl_verify_version(&osvi, VER_SERVICEPACKMAJOR | VER_PLATFORMID, dwlConditionMask);

	//Error checking
	if (!ret)
	{
		if (GetLastError() != ERROR_OLD_WIN_VERSION)
		{
			qWarning("VerifyVersionInfo() system call has failed!");
		}
	}

	return (ret != FALSE);
}

static bool get_real_os_version(unsigned int *const major, unsigned int *const minor, unsigned int *const build, unsigned int *const spack, bool *const pbOverride)
{
	static const DWORD MAX_VERSION = MAXWORD;
	static const DWORD MAX_BUILDNO = MAXINT;
	static const DWORD MAX_SRVCPCK = MAXWORD;

	*major = *minor = *build = *spack = 0U;
	*pbOverride = false;
	
	//Initialize local variables
	OSVERSIONINFOEXW osvi;
	memset(&osvi, 0, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

	//Try GetVersionEx() first
	if(rtl_get_version(&osvi) == FALSE)
	{
		qWarning("GetVersionEx() has failed, cannot detect Windows version!");
		return false;
	}

	//Make sure we are running on NT
	if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		*major = osvi.dwMajorVersion;
		*minor = osvi.dwMinorVersion;
		*build = osvi.dwBuildNumber;
		*spack = osvi.wServicePackMajor;
	}
	else
	{
		if (verify_os_version(4, 0))
		{
			*major = 4;
			*build = 1381;
			*pbOverride = true;
		}
		else
		{
			qWarning("Not running on Windows NT, unsupported operating system!");
			return false;
		}
	}

	//Major Version
	for (DWORD nextMajor = (*major) + 1; nextMajor <= MAX_VERSION; nextMajor++)
	{
		if (verify_os_version(nextMajor, 0))
		{
			*major = nextMajor;
			*minor = 0;
			*pbOverride = true;
			continue;
		}
		break;
	}

	//Minor Version
	for (DWORD nextMinor = (*minor) + 1; nextMinor <= MAX_VERSION; nextMinor++)
	{
		if (verify_os_version((*major), nextMinor))
		{
			*minor = nextMinor;
			*pbOverride = true;
			continue;
		}
		break;
	}

	//Build Version
	if (verify_os_build(SAFE_ADD((*build), 1, MAX_BUILDNO)))
	{
		DWORD stepSize = initialize_step_size(MAX_BUILDNO);
		for (DWORD nextBuildNo = SAFE_ADD((*build), stepSize, MAX_BUILDNO); (*build) < MAX_BUILDNO; nextBuildNo = SAFE_ADD((*build), stepSize, MAX_BUILDNO))
		{
			if (verify_os_build(nextBuildNo))
			{
				*build = nextBuildNo;
				*pbOverride = true;
				continue;
			}
			if (stepSize > 1)
			{
				stepSize = stepSize / 2;
				continue;
			}
			break;
		}
	}

	//Service Pack
	if (verify_os_spack(SAFE_ADD((*spack), 1, MAX_SRVCPCK)))
	{
		DWORD stepSize = initialize_step_size(MAX_SRVCPCK);
		for (DWORD nextSPackNo = SAFE_ADD((*spack), stepSize, MAX_SRVCPCK); (*spack) < MAX_SRVCPCK; nextSPackNo = SAFE_ADD((*spack), stepSize, MAX_SRVCPCK))
		{
			if (verify_os_spack(nextSPackNo))
			{
				*build = nextSPackNo;
				*pbOverride = true;
				continue;
			}
			if (stepSize > 1)
			{
				stepSize = stepSize / 2;
				continue;
			}
			break;
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
	unsigned int major, minor, build, spack; bool overrideFlg;
	if(get_real_os_version(&major, &minor, &build, &spack, &overrideFlg))
	{
		g_os_version_info.type = Version::OS_WINDOWS;
		g_os_version_info.versionMajor = major;
		g_os_version_info.versionMinor = minor;
		g_os_version_info.versionBuild = build;
		g_os_version_info.versionSPack = spack;
		g_os_version_info.overrideFlag = overrideFlg;
	}
	else
	{
		qWarning("Failed to determine the operating system version!");
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
	void *const ptr = MUtils::Win32Utils::resolve<void*>(QLatin1String("ntdll"), QLatin1String("wine_get_version"));
	return (ptr != NULL);
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

static QReadWriteLock                         g_known_folders_lock;
static QScopedPointer<QHash<size_t, QString>> g_known_folders_data;

typedef HRESULT (WINAPI *SHGetKnownFolderPath_t)(const GUID &rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath);
typedef HRESULT (WINAPI *SHGetFolderPath_t)     (HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, LPWSTR pszPath);

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
		{ 0x001a, {0x3EB685DB,0x65F9,0x4CF6,{0xA0,0x3A,0xE3,0xEF,0x65,0x72,0x9F,0x3D}} },  //CSIDL_APPDATA
		{ 0x001c, {0xF1B32785,0x6FBA,0x4FCF,{0x9D,0x55,0x7B,0x8E,0x7F,0x15,0x70,0x91}} },  //CSIDL_LOCAL_APPDATA
		{ 0x0028, {0x5E6C858F,0x0E22,0x4760,{0x9A,0xFE,0xEA,0x33,0x17,0xB6,0x71,0x73}} },  //CSIDL_PROFILE
		{ 0x0026, {0x905e63b6,0xc1bf,0x494e,{0xb2,0x9c,0x65,0xb7,0x32,0xd3,0xd2,0x1a}} },  //CSIDL_PROGRAM_FILES
		{ 0x0025, {0x1AC14E77,0x02E7,0x4E5D,{0xB7,0x44,0x2E,0xB1,0xAE,0x51,0x98,0xB7}} },  //CSIDL_SYSTEM_FOLDER
		{ 0x0024, {0xF38BF404,0x1D43,0x42F2,{0x93,0x05,0x67,0xDE,0x0B,0x28,0xFC,0x23}} },  //CSIDL_WINDOWS_FOLDER
	};

	size_t folderId = size_t(-1);

	switch(folder_id)
	{
		case FOLDER_ROAMING_DATA: folderId = 0; break;
		case FOLDER_LOCALAPPDATA: folderId = 1; break;
		case FOLDER_USER_PROFILE: folderId = 2; break;
		case FOLDER_PROGRAMFILES: folderId = 3; break;
		case FOLDER_SYSTEMFOLDER: folderId = 4; break;
		case FOLDER_SYSTROOT_DIR: folderId = 5; break;
		default:
			qWarning("Invalid 'known' folder was requested!");
			return Internal::g_empty;
	}

	QReadLocker readLock(&g_known_folders_lock);

	//Already in cache?
	if(!g_known_folders_data.isNull())
	{
		if(g_known_folders_data->contains(folderId))
		{
			return (*g_known_folders_data)[folderId];
		}
	}

	//Obtain write lock to initialize
	readLock.unlock();
	QWriteLocker writeLock(&g_known_folders_lock);

	//Still not in cache?
	if(!g_known_folders_data.isNull())
	{
		if(g_known_folders_data->contains(folderId))
		{
			return (*g_known_folders_data)[folderId];
		}
	}

	//Initialize on first call
	if(g_known_folders_data.isNull())
	{
		g_known_folders_data.reset(new QHash<size_t, QString>());
	}

	QString folderPath;

	//Try SHGetKnownFolderPath() first!
	if(const SHGetKnownFolderPath_t known_folders_fpGetKnownFolderPath = MUtils::Win32Utils::resolve<SHGetKnownFolderPath_t>(QLatin1String("shell32"), QLatin1String("SHGetKnownFolderPath")))
	{
		WCHAR *path = NULL;
		if(known_folders_fpGetKnownFolderPath(s_folders[folderId].guid, KF_FLAG_CREATE, NULL, &path) == S_OK)
		{
			//MessageBoxW(0, path, L"SHGetKnownFolderPath", MB_TOPMOST);
			const QDir folderTemp = QDir(QDir::fromNativeSeparators(MUTILS_QSTR(path)));
			if(folderTemp.exists())
			{
				folderPath = folderTemp.canonicalPath();
			}
			CoTaskMemFree(path);
		}
	}

	//Fall back to SHGetFolderPathW()
	if (folderPath.isEmpty())
	{
		if (const SHGetFolderPath_t known_folders_fpGetFolderPath = MUtils::Win32Utils::resolve<SHGetFolderPath_t>(QLatin1String("shell32"), QLatin1String("SHGetFolderPathW")))
		{
			QScopedArrayPointer<WCHAR> path(new WCHAR[4096]);
			if (known_folders_fpGetFolderPath(NULL, s_folders[folderId].csidl | CSIDL_FLAG_CREATE, NULL, NULL, path.data()) == S_OK)
			{
				//MessageBoxW(0, path, L"SHGetFolderPathW", MB_TOPMOST);
				const QDir folderTemp = QDir(QDir::fromNativeSeparators(MUTILS_QSTR(path.data())));
				if (folderTemp.exists())
				{
					folderPath = folderTemp.canonicalPath();
				}
			}
		}
	}

	//Update cache
	if (!folderPath.isEmpty())
	{
		g_known_folders_data->insert(folderId, folderPath);
		return (*g_known_folders_data)[folderId];
	}

	return Internal::g_empty;
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
// FILE PATH FROM FD
///////////////////////////////////////////////////////////////////////////////

typedef DWORD(_stdcall *GetPathNameByHandleFun)(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);

static QString get_file_path_drive_list(void)
{
	QString list;
	const DWORD len = GetLogicalDriveStringsW(0, NULL);
	if (len > 0)
	{
		if (wchar_t *const buffer = (wchar_t*) _malloca(sizeof(wchar_t) * len))
		{
			const DWORD ret = GetLogicalDriveStringsW(len, buffer);
			if ((ret > 0) && (ret < len))
			{
				const wchar_t *ptr = buffer;
				while (const size_t current_len = wcslen(ptr))
				{
					list.append(QChar(*reinterpret_cast<const ushort*>(ptr)));
					ptr += (current_len + 1);
				}
			}
			_freea(buffer);
		}
	}
	return list;
}

static void get_file_path_translate(QString &path)
{
	static const DWORD BUFSIZE = 2048;
	wchar_t buffer[BUFSIZE], drive[3];

	const QString driveList = get_file_path_drive_list();
	wcscpy_s(drive, 3, L"?:");
	for (const wchar_t *current = MUTILS_WCHR(driveList); *current; current++)
	{
		drive[0] = (*current);
		if (QueryDosDeviceW(drive, buffer, MAX_PATH))
		{
			const QString prefix = MUTILS_QSTR(buffer);
			if (path.startsWith(prefix, Qt::CaseInsensitive))
			{
				path.remove(0, prefix.length()).prepend(QLatin1Char(':')).prepend(QChar(*reinterpret_cast<const ushort*>(current)));
				break;
			}
		}
	}
}

static QString get_file_path_fallback(const HANDLE &hFile)
{
	QString filePath;

	const HANDLE hFileMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 1, NULL);
	if (hFileMap)
	{
		void *const pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);
		if (pMem)
		{
			static const size_t BUFFSIZE = 2048;
			wchar_t buffer[BUFFSIZE];
			if (GetMappedFileNameW(GetCurrentProcess(), pMem, buffer, BUFFSIZE) > 0)
			{
				filePath = MUTILS_QSTR(buffer);
			}
			UnmapViewOfFile(pMem);
		}
		CloseHandle(hFileMap);
	}

	if (!filePath.isEmpty())
	{
		get_file_path_translate(filePath);
	}

	return filePath;
}

QString MUtils::OS::get_file_path(const int &fd)
{
	if (fd >= 0)
	{
		const GetPathNameByHandleFun getPathNameByHandleFun = MUtils::Win32Utils::resolve<GetPathNameByHandleFun>(QLatin1String("kernel32"), QLatin1String("GetFinalPathNameByHandleW"));
		if (!getPathNameByHandleFun)
		{
			return get_file_path_fallback((HANDLE)_get_osfhandle(fd));
		}

		const HANDLE handle = (HANDLE) _get_osfhandle(fd);
		const DWORD len = getPathNameByHandleFun(handle, NULL, 0, FILE_NAME_OPENED);
		if (len > 0)
		{
			if (wchar_t *const buffer = (wchar_t*)_malloca(sizeof(wchar_t) * len))
			{
				const DWORD ret = getPathNameByHandleFun(handle, buffer, len, FILE_NAME_OPENED);
				if ((ret > 0) && (ret < len))
				{
					const QString path(MUTILS_QSTR(buffer));
					return path.startsWith(QLatin1String("\\\\?\\")) ? path.mid(4) : path;
				}
				_freea(buffer);
			}
		}
	}

	return QString();
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

static int g_library_as_image_resource_supported = -1;
static QReadWriteLock g_library_as_image_resource_supported_lock;

static bool library_as_image_resource_supported()
{
	QReadLocker readLocker(&g_library_as_image_resource_supported_lock);
	if (g_library_as_image_resource_supported >= 0)
	{
		return (g_library_as_image_resource_supported > 0);
	}

	readLocker.unlock();
	QWriteLocker writeLocker(&g_library_as_image_resource_supported_lock);

	if (g_library_as_image_resource_supported < 0)
	{
		g_library_as_image_resource_supported = 0;
		OSVERSIONINFOEXW osvi;
		if (rtl_get_version(&osvi))
		{
			if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && (osvi.dwMajorVersion >= 6U))
			{
				g_library_as_image_resource_supported = 1;
			}
		}
	}

	return (g_library_as_image_resource_supported > 0);
}

bool MUtils::OS::is_executable_file(const QString &path)
{
	DWORD binaryType;
	if(GetBinaryType(MUTILS_WCHR(QDir::toNativeSeparators(path)), &binaryType))
	{
		return ((binaryType == SCS_32BIT_BINARY) || (binaryType == SCS_64BIT_BINARY));
	}

	const DWORD errorCode = GetLastError();
	qWarning("GetBinaryType() failed with error: 0x%08X", errorCode);
	return false;
}

bool MUtils::OS::is_library_file(const QString &path)
{
	const DWORD flags = library_as_image_resource_supported() ? (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE) : (LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES);
	if (const HMODULE hMod = LoadLibraryEx(MUTILS_WCHR(QDir::toNativeSeparators(path)), NULL, flags))
	{
		FreeLibrary(hMod);
		return true;
	}

	const DWORD errorCode = GetLastError();
	qWarning("LoadLibraryEx() failed with error: 0x%08X", errorCode);
	return false;
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
// DRIVE TYPE
///////////////////////////////////////////////////////////////////////////////

static wchar_t get_drive_letter(const QString &path)
{
	QString nativePath = QDir::toNativeSeparators(path);
	while (nativePath.startsWith("\\\\?\\") || nativePath.startsWith("\\\\.\\"))
	{
		nativePath = QDir::toNativeSeparators(nativePath.mid(4));
	}
	if ((path.length() > 1) && (path[1] == QLatin1Char(':')))
	{
		const wchar_t letter = static_cast<wchar_t>(path[0].unicode());
		if (((letter >= 'A') && (letter <= 'Z')) || ((letter >= 'a') && (letter <= 'z')))
		{
			return towupper(letter);
		}
	}
	return L'\0'; /*invalid path spec*/
}

static QSet<DWORD> get_physical_drive_ids(const wchar_t drive_letter)
{
	QSet<DWORD> physical_drives;
	wchar_t driveName[8];
	_snwprintf_s(driveName, 8, _TRUNCATE, L"\\\\.\\%c:", drive_letter);
	const HANDLE hDrive = CreateFileW(driveName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hDrive && (hDrive != INVALID_HANDLE_VALUE))
	{
		const size_t BUFF_SIZE = sizeof(VOLUME_DISK_EXTENTS) + (32U * sizeof(DISK_EXTENT));
		VOLUME_DISK_EXTENTS *const diskExtents = (VOLUME_DISK_EXTENTS*)_malloca(BUFF_SIZE);
		DWORD dwSize;
		if (DeviceIoControl(hDrive, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, (LPVOID)diskExtents, (DWORD)BUFF_SIZE, (LPDWORD)&dwSize, NULL))
		{
			for (DWORD index = 0U; index < diskExtents->NumberOfDiskExtents; ++index)
			{
				physical_drives.insert(diskExtents->Extents[index].DiskNumber);
			}
		}
		_freea(diskExtents);
		CloseHandle(hDrive);
	}
	return physical_drives;
}

static bool incurs_seek_penalty(const DWORD device_id)
{
	wchar_t driveName[24];
	_snwprintf_s(driveName, 24, _TRUNCATE, L"\\\\?\\PhysicalDrive%u", device_id);
	const HANDLE hDevice = CreateFileW(driveName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	bool seeking_penalty = true;
	if (hDevice && (hDevice != INVALID_HANDLE_VALUE))
	{
		STORAGE_PROPERTY_QUERY spq;
		DEVICE_SEEK_PENALTY_DESCRIPTOR dspd;
		memset(&spq, 0, sizeof(STORAGE_PROPERTY_QUERY));
		spq.PropertyId = (STORAGE_PROPERTY_ID)StorageDeviceSeekPenaltyProperty;
		spq.QueryType = PropertyStandardQuery;
		DWORD dwSize;
		if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, (LPVOID)&spq, (DWORD)sizeof(spq), (LPVOID)&dspd, (DWORD)sizeof(dspd), (LPDWORD)&dwSize, NULL))
		{
			seeking_penalty = dspd.IncursSeekPenalty;
		}
		CloseHandle(hDevice);
	}
	return seeking_penalty;
}

static bool is_fast_seeking_drive(const wchar_t drive_letter)
{
	bool fast_seeking = false;
	const QSet<DWORD> physical_drive_ids = get_physical_drive_ids(drive_letter);
	if (!physical_drive_ids.empty())
	{
		fast_seeking = true;
		for (QSet<DWORD>::const_iterator iter = physical_drive_ids.constBegin(); iter != physical_drive_ids.constEnd(); ++iter)
		{
			fast_seeking = fast_seeking && (!incurs_seek_penalty(*iter));
		}
	}
	return fast_seeking;
}

MUtils::OS::drive_type_t MUtils::OS::get_drive_type(const QString &path, bool *fast_seeking)
{
	drive_type_t driveType = DRIVE_TYPE_ERR;
	const wchar_t driveLetter = get_drive_letter(path);
	if (driveLetter)
	{
		wchar_t driveName[8];
		_snwprintf_s(driveName, 8, _TRUNCATE, L"\\\\.\\%c:\\", driveLetter);
		switch (GetDriveTypeW(driveName))
		{
			case DRIVE_REMOVABLE: driveType = DRIVE_TYPE_FDD; break;
			case DRIVE_FIXED:     driveType = DRIVE_TYPE_HDD; break;
			case DRIVE_REMOTE:    driveType = DRIVE_TYPE_NET; break;
			case DRIVE_CDROM:     driveType = DRIVE_TYPE_OPT; break;
			case DRIVE_RAMDISK:   driveType = DRIVE_TYPE_RAM; break;
		}
	}
	if (fast_seeking)
	{
		if (driveType == DRIVE_TYPE_HDD)
		{
			*fast_seeking = is_fast_seeking_drive(driveLetter);
		}
		else
		{
			*fast_seeking = (driveType == DRIVE_TYPE_RAM);
		}
	}
	return driveType;
}

///////////////////////////////////////////////////////////////////////////////
// SHELL OPEN
///////////////////////////////////////////////////////////////////////////////

bool MUtils::OS::shell_open(const QWidget *parent, const QString &url, const QString &parameters, const QString &directory, const bool explore)
{
	return ((int) ShellExecuteW((parent ? reinterpret_cast<HWND>(parent->winId()) : NULL), (explore ? L"explore" : L"open"), MUTILS_WCHR(url), ((!parameters.isEmpty()) ? MUTILS_WCHR(parameters) : NULL), ((!directory.isEmpty()) ? MUTILS_WCHR(directory) : NULL), SW_SHOW)) > 32;
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

quint32 MUtils::OS::process_id(void)
{
	return GetCurrentProcessId();
}

quint32 MUtils::OS::process_id(const QProcess *const proc)
{
	PROCESS_INFORMATION *procInf = proc->pid();
	return (procInf) ? procInf->dwProcessId : 0;
}

quint32 MUtils::OS::thread_id(void)
{
	return GetCurrentThreadId();
}

quint32 MUtils::OS::thread_id(const QProcess *const proc)
{
	PROCESS_INFORMATION *procInf = proc->pid();
	return (procInf) ? procInf->dwThreadId : 0;
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
// SET FILE TIME
///////////////////////////////////////////////////////////////////////////////

static QScopedPointer<QDateTime> s_epoch;
static QReadWriteLock            s_epochLock;

static const QDateTime *get_epoch(void)
{
	QReadLocker rdLock(&s_epochLock);

	if (s_epoch.isNull())
	{
		rdLock.unlock();
		QWriteLocker wrLock(&s_epochLock);
		if (s_epoch.isNull())
		{
			s_epoch.reset(new QDateTime(QDate(1601, 1, 1), QTime(0, 0, 0, 0), Qt::UTC));
		}
		wrLock.unlock();
		rdLock.relock();
	}

	return s_epoch.data();
}

static FILETIME *qt_time_to_file_time(FILETIME *const fileTime, const QDateTime &dateTime)
{
	memset(fileTime, 0, sizeof(FILETIME));

	if (const QDateTime *const epoch = get_epoch())
	{
		const qint64 msecs = epoch->msecsTo(dateTime);
		if (msecs > 0)
		{
			const quint64 ticks = 10000U * quint64(msecs);
			fileTime->dwHighDateTime = ((ticks >> 32) & 0xFFFFFFFF);
			fileTime->dwLowDateTime = (ticks & 0xFFFFFFFF);
			return fileTime;
		}
	}

	return NULL;
}

static bool set_file_time(const HANDLE hFile, const QDateTime &created, const QDateTime &lastMod, const QDateTime &lastAcc)
{
	FILETIME ftCreated, ftLastMod, ftLastAcc;

	FILETIME *const pCreated = created.isValid() ? qt_time_to_file_time(&ftCreated, created) : NULL;
	FILETIME *const pLastMod = lastMod.isValid() ? qt_time_to_file_time(&ftLastMod, lastMod) : NULL;
	FILETIME *const pLastAcc = lastAcc.isValid() ? qt_time_to_file_time(&ftLastAcc, lastAcc) : NULL;

	if (pCreated || pLastMod || pLastAcc)
	{
		return (SetFileTime(hFile, pCreated, pLastAcc, pLastMod) != FALSE);
	}

	return false;
}

bool MUtils::OS::set_file_time(const QFile &file, const QDateTime &created, const QDateTime &lastMod, const QDateTime &lastAcc)
{
	const int fd = file.handle();
	if (fd >= 0)
	{
		return set_file_time((HANDLE)_get_osfhandle(fd), created, lastMod, lastAcc);
	}
	return false;
}

bool MUtils::OS::set_file_time(const QString &path, const QDateTime &created, const QDateTime &lastMod, const QDateTime &lastAcc)
{
	const HANDLE hFile = CreateFileW(MUTILS_WCHR(path), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	bool okay = false;
	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		okay = set_file_time(hFile, created, lastMod, lastAcc);
		CloseHandle(hFile);
	}
	return okay;
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

typedef BOOL (_stdcall *Wow64DisableWow64FsRedirectionFun)(PVOID *OldValue);
typedef BOOL (_stdcall *Wow64RevertWow64FsRedirectionFun) (PVOID  OldValue);

bool MUtils::OS::wow64fsredir_disable(uintptr_t &oldValue)
{
	oldValue = reinterpret_cast<uintptr_t>(nullptr);
	const Wow64DisableWow64FsRedirectionFun wow64redir_disable = MUtils::Win32Utils::resolve<Wow64DisableWow64FsRedirectionFun>(QLatin1String("kernel32"), QLatin1String("Wow64DisableWow64FsRedirection"));
	if(wow64redir_disable)
	{
		PVOID temp = NULL;
		if (wow64redir_disable(&temp))
		{
			oldValue = reinterpret_cast<uintptr_t>(temp);
			return true;
		}
	}
	return false;
}

bool MUtils::OS::wow64fsredir_revert(const uintptr_t oldValue)
{
	const Wow64RevertWow64FsRedirectionFun wow64redir_disable = MUtils::Win32Utils::resolve<Wow64RevertWow64FsRedirectionFun>(QLatin1String("kernel32"), QLatin1String("Wow64RevertWow64FsRedirection"));
	if (wow64redir_disable)
	{
		if (wow64redir_disable(reinterpret_cast<PVOID>(oldValue)))
		{
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// DEBUGGER CHECK
///////////////////////////////////////////////////////////////////////////////

QString MUtils::OS::get_envvar(const QString &name)
{
	wchar_t *buffer = NULL;
	size_t requiredSize = 0, buffSize = 0;
	QString result;

	forever
	{
		//Adjust the buffer size as required first!
		if (buffSize < requiredSize)
		{
			if (buffer)
			{
				_freea(buffer);
			}
			if (!(buffer = (wchar_t*)_malloca(sizeof(wchar_t) * requiredSize)))
			{
				break; /*out of memory error!*/
			}
			buffSize = requiredSize;
		}

		//Try to fetch the environment variable now
		const errno_t error = _wgetenv_s(&requiredSize, buffer, buffSize, MUTILS_WCHR(name));
		if(!error)
		{
			if (requiredSize > 0)
			{
				result = MUTILS_QSTR(buffer);
			}
			break; /*done*/
		}
		else if (error != ERANGE)
		{
			break; /*somethging else went wrong!*/
		}
	}
	
	if (buffer)
	{
		_freea(buffer);
		buffSize = 0;
		buffer = NULL;
	}

	return result;
}

bool MUtils::OS::set_envvar(const QString &name, const QString &value)
{
	if (!_wputenv_s(MUTILS_WCHR(name), MUTILS_WCHR(value)))
	{
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// NULL DEVICE
///////////////////////////////////////////////////////////////////////////////

static const QLatin1String NULL_DEVICE("NUL");

const QLatin1String &MUtils::OS::null_device(void)
{
	return NULL_DEVICE;
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
static QAtomicInt g_fatal_exit_flag;

static DWORD WINAPI fatal_exit_helper(LPVOID lpParameter)
{
	MUtils::OS::system_message_err(L"GURU MEDITATION", (LPWSTR) lpParameter);
	return 0;
}

void MUtils::OS::fatal_exit(const wchar_t* const errorMessage)
{
	g_fatal_exit_lock.enter();
	
	if(!g_fatal_exit_flag.testAndSetOrdered(0, 1))
	{
		return; /*prevent recursive invocation*/
	}

	if(g_main_thread_id != GetCurrentThreadId())
	{
		if(HANDLE hThreadMain = OpenThread(THREAD_SUSPEND_RESUME, FALSE, g_main_thread_id))
		{
			SuspendThread(hThreadMain); /*stop main thread*/
		}
	}

	if(HANDLE hThread = CreateThread(NULL, 0, fatal_exit_helper, (LPVOID)errorMessage, 0, NULL))
	{
		WaitForSingleObject(hThread, INFINITE);
	}
	else
	{
		fatal_exit_helper((LPVOID)errorMessage);
	}

	for(;;)
	{
		TerminateProcess(GetCurrentProcess(), 666);
	}
}

///////////////////////////////////////////////////////////////////////////////
