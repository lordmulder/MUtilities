///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QString>
#include <QMap>
#include <QDate>
#include <QWidget>

//Forward declaration
class QFile;

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace OS
	{
		namespace Version
		{
			//Supported OS types
			typedef enum
			{
				OS_UNKNOWN = 0,
				OS_WINDOWS = 1
			}
			os_type_t;

			//OS version struct
			typedef struct _os_version_t
			{
				unsigned int type;
				unsigned int versionMajor;
				unsigned int versionMinor;
				unsigned int versionBuild;
				bool overrideFlag;

				MUTILS_API bool operator== (const _os_version_t &rhs) const;
				MUTILS_API bool operator!= (const _os_version_t &rhs) const;
				MUTILS_API bool operator>  (const _os_version_t &rhs) const;
				MUTILS_API bool operator>= (const _os_version_t &rhs) const;
				MUTILS_API bool operator<  (const _os_version_t &rhs) const;
				MUTILS_API bool operator<= (const _os_version_t &rhs) const;
			}
			os_version_t;

			//Known Windows NT versions
			MUTILS_API extern const os_version_t WINDOWS_WIN2K;	// 2000
			MUTILS_API extern const os_version_t WINDOWS_WINXP;	// XP
			MUTILS_API extern const os_version_t WINDOWS_XPX64;	// XP_x64
			MUTILS_API extern const os_version_t WINDOWS_VISTA;	// Vista
			MUTILS_API extern const os_version_t WINDOWS_WIN70;	// 7
			MUTILS_API extern const os_version_t WINDOWS_WIN80;	// 8
			MUTILS_API extern const os_version_t WINDOWS_WIN81;	// 8.1
			MUTILS_API extern const os_version_t WINDOWS_WN100;	// 10

			//Unknown OS
			MUTILS_API extern const os_version_t UNKNOWN_OPSYS;	// N/A
		}

		//Known Folders IDs
		typedef enum
		{
			FOLDER_LOCALAPPDATA = 0,
			FOLDER_PROGRAMFILES = 2,
			FOLDER_SYSTEMFOLDER = 3,
			FOLDER_SYSTROOT_DIR = 4
		}
		known_folder_t;
		
		//Network connection types
		typedef enum
		{
			NETWORK_TYPE_ERR = 0,	/*unknown*/
			NETWORK_TYPE_NON = 1,	/*not connected*/
			NETWORK_TYPE_YES = 2	/*connected*/
		}
		network_type_t;
				
		//System message
		MUTILS_API void system_message_nfo(const wchar_t *const title, const wchar_t *const text);
		MUTILS_API void system_message_wrn(const wchar_t *const title, const wchar_t *const text);
		MUTILS_API void system_message_err(const wchar_t *const title, const wchar_t *const text);

		//CLI Arguments
		typedef QMap<QString,QString> ArgumentMap;
		MUTILS_API const QStringList crack_command_line(const QString &command_line = QString());
		MUTILS_API const ArgumentMap &arguments(void);

		//Copy file
		typedef bool (*progress_callback_t)(const double &progress, void *const userData);
		MUTILS_API bool copy_file(const QString &sourcePath, const QString &outputPath, const bool &overwrite = true, const progress_callback_t callback = NULL, void *const userData = NULL);

		//Get file version
		MUTILS_API bool get_file_version(const QString fileName, quint16 *const major = NULL, quint16 *const minor = NULL, quint16 *const patch = NULL, quint16 *const build = NULL);

		//Get the OS version
		MUTILS_API const Version::os_version_t &os_version(void);
		MUTILS_API const char *os_friendly_name(const MUtils::OS::Version::os_version_t &os_version);
		MUTILS_API const bool &running_on_wine(void);

		//Get known Folder
		MUTILS_API const QString &known_folder(known_folder_t folder_id);

		//Current Date & Time
		MUTILS_API QDate current_date(void);
		MUTILS_API quint64 current_file_time(void);

		//Check for process elevation
		MUTILS_API bool is_elevated(bool *bIsUacEnabled = NULL);
		MUTILS_API bool user_is_admin(void);

		//Network Status
		MUTILS_API int network_status(void);

		//Message handler
		MUTILS_API bool handle_os_message(const void *const message, long *result);

		//Sleep
		MUTILS_API void sleep_ms(const size_t &duration);

		//Is executable/library file?
		MUTILS_API bool is_executable_file(const QString &path);
		MUTILS_API bool is_library_file(const QString &path);

		//Shutdown & Hibernation
		MUTILS_API bool is_hibernation_supported(void);
		MUTILS_API bool shutdown_computer(const QString &message, const unsigned long timeout, const bool forceShutdown, const bool hibernate);

		//Free diskspace
		MUTILS_API bool free_diskspace(const QString &path, quint64 &freeSpace);

		//Shell open
		MUTILS_API bool shell_open(const QWidget *parent, const QString &url, const bool explore = false);
		MUTILS_API bool shell_open(const QWidget *parent, const QString &url, const QString &parameters, const QString &directory, const bool explore = false);

		//Open media file
		MUTILS_API bool open_media_file(const QString &mediaFilePath);

		//Process priority
		MUTILS_API bool change_process_priority(const int priority);
		MUTILS_API bool change_process_priority(const QProcess *proc, const int priority);

		//Process ID
		MUTILS_API quint32 process_id(void);
		MUTILS_API quint32 process_id(const QProcess *const proc);

		//Thread ID
		MUTILS_API quint32 thread_id(void);
		MUTILS_API quint32 thread_id(const QProcess *const proc);

		//Suspend or resume processv
		MUTILS_API bool suspend_process(const QProcess *proc, const bool suspend);

		//System timer resolution
		MUTILS_API bool setup_timer_resolution(const quint32 &interval = 1);
		MUTILS_API bool reset_timer_resolution(const quint32 &interval = 1);

		//Set file time
		MUTILS_API bool set_file_time(const QFile &file,   const QDateTime &created = QDateTime(), const QDateTime &modified = QDateTime(), const QDateTime &accessed = QDateTime());
		MUTILS_API bool set_file_time(const QString &path, const QDateTime &created = QDateTime(), const QDateTime &modified = QDateTime(), const QDateTime &accessed = QDateTime());

		//Keyboard support
		MUTILS_API bool check_key_state_esc(void);

		//Shell notification
		MUTILS_API void shell_change_notification(void);

		//Get file path from descriptor
		MUTILS_API QString get_file_path(const int &fd);

		//WOW64 redirection
		MUTILS_API bool wow64fsredir_disable(void *oldValue);
		MUTILS_API bool wow64fsredir_revert (void *oldValue);

		//Environment variables
		MUTILS_API QString get_envvar(const QString &name);
		MUTILS_API bool set_envvar(const QString &name, const QString &value);

		//Check if debugger is present
		MUTILS_API void check_debugger(void);

		//Error handling
		MUTILS_API void fatal_exit(const wchar_t* const errorMessage);
	}
}

///////////////////////////////////////////////////////////////////////////////
