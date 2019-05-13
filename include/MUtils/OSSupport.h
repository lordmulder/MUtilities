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

/**
 * @file
 * @brief This file contains function that wrap OS-specific functionality in a platform-independent way
 */

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

/**
* \brief Global MUtils namespace
*/
namespace MUtils
{
	/**
	* \brief MUtils OS-specific functions namespace
	*/
	namespace OS
	{
		/**
		* \brief OS version information namespace
		*/
		namespace Version
		{
			/**
			* \brief This enumeration specifies the type of the underlaying operating system
			*/
			typedef enum
			{
				OS_UNKNOWN = 0,  ///< Unknown operating system
				OS_WINDOWS = 1   ///< Microsoft(R) Windows
			}
			os_type_t;

			/**
			* \brief This struct contains version information about the underlaying operating system. See `_os_version_t` for details!
			*/
			typedef struct _os_version_t
			{
				unsigned int type;          ///< The type of the underlaying operating system, as `os_type_t`
				unsigned int versionMajor;  ///< The *major* version of the underlaying operating system
				unsigned int versionMinor;  ///< The *minor* version of the underlaying operating system
				unsigned int versionBuild;  ///< The *build* number of the underlaying operating system
				unsigned int versionSPack;  ///< The *service pack* version of the underlaying operating system
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
			MUTILS_API extern const os_version_t WINDOWS_WIN2K;  ///< \brief Operating system version constant \details Microsoft(R) Windows 2000
			MUTILS_API extern const os_version_t WINDOWS_WINXP;  ///< \brief Operating system version constant \details Microsoft(R) Windows XP
			MUTILS_API extern const os_version_t WINDOWS_XPX64;  ///< \brief Operating system version constant \details Microsoft(R) Windows XP x64-edition
			MUTILS_API extern const os_version_t WINDOWS_VISTA;  ///< \brief Operating system version constant \details Microsoft(R) Windows Vista
			MUTILS_API extern const os_version_t WINDOWS_WIN70;  ///< \brief Operating system version constant \details Microsoft(R) Windows 7
			MUTILS_API extern const os_version_t WINDOWS_WIN80;  ///< \brief Operating system version constant \details Microsoft(R) Windows 8
			MUTILS_API extern const os_version_t WINDOWS_WIN81;  ///< \brief Operating system version constant \details Microsoft(R) Windows 8.1
			MUTILS_API extern const os_version_t WINDOWS_WN100;  ///< \brief Operating system version constant \details Microsoft(R) Windows 10

			//Unknown OS
			MUTILS_API extern const os_version_t UNKNOWN_OPSYS;  ///< \brief Operating system version constant \details Unknown operating system version
		}

		/**
		* \brief This enumeration specifies "known" folder identifiers
		*/
		typedef enum
		{
			FOLDER_ROAMING_DATA = 0,  ///< Application-specific data
			FOLDER_LOCALAPPDATA = 1,  ///< Local application data (non-roaming)
			FOLDER_USER_PROFILE = 2,  ///< The user's profile folder
			FOLDER_PROGRAMFILES = 3,  ///< Program files
			FOLDER_SYSTEMFOLDER = 4,  ///< System directory
			FOLDER_SYSTROOT_DIR = 5   ///< System "root" directory
		}
		known_folder_t;
		
		/**
		* \brief This enumeration specifies network connection types
		*/
		typedef enum
		{
			NETWORK_TYPE_ERR = 0,  ///< Network connection is unknown
			NETWORK_TYPE_NON = 1,  ///< Computer is **not** connected to a network
			NETWORK_TYPE_YES = 2   ///< Computer *is* connected to a network
		}
		network_type_t;
		
		/**
		* \brief This enumeration specifies drive types
		*/
		typedef enum
		{
			DRIVE_TYPE_ERR = 0,  ///< The drive type cannot be determined
			DRIVE_TYPE_FDD = 1,  ///< Floppy Drive, or Flash Card reader
			DRIVE_TYPE_HDD = 2,  ///< Hard Disk drive or Solid-State Drive
			DRIVE_TYPE_NET = 3,  ///< Remote/Network drive
			DRIVE_TYPE_OPT = 4,  ///< Optical disk srive, e.g. CD or DVD
			DRIVE_TYPE_RAM = 5   ///< RAM disk
		}
		drive_type_t;

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
		MUTILS_API const QString &known_folder(const known_folder_t folder_id);

		//Current Date & Time
		MUTILS_API QDate current_date(void);
		MUTILS_API quint64 current_file_time(void);

		//Check for process elevation
		MUTILS_API bool is_elevated(bool *bIsUacEnabled = NULL);
		MUTILS_API bool user_is_admin(void);

		/**
		* \brief Check the network status
		*
		* Checks whether the computer is *currently* connected to a network. Note that an existing network connection does **not** necessarily imply actual Internet access!
		*
		* \return The function returns the current network status as a `OS::network_type_t` value.
		*/
		MUTILS_API int network_status(void);

		//Message handler
		MUTILS_API bool handle_os_message(const void *const message, long *result);

		/**
		* \brief Suspend calling thread
		*
		* This function suspends the calling thread. The thread will give up its current time-slice and enter "sleeping" state. The thread will remain in "sleeping" for the specified duration. After the specified duration has elapsed, the thread will be resumed.
		*
		* Note that it depends on the operating system's scheduling decisions, when the thread will actually be allowed to execute again! While the thread is still in "sleeping" state, it can **not** be selected for execution by the operating system's scheduler. Once the thread is *no* longer in "sleeping" state, i.e. the specified period has elapsed, the thread *can* be selected for execution by the operating system's scheduler again - but this does **not** need to happen *immediately*! The scheduler decides which thread is allowed to execute next, taking into consideration thread priorities.
		*
		* \param duration The amount of time that the thread will be suspended, in milliseconds. A value of **0** means that the thread will *not* actually enter "sleeping" state, but it will still give up its current time-slice!
		*/
		MUTILS_API void sleep_ms(const size_t &duration);

		//Is executable/library file?
		MUTILS_API bool is_executable_file(const QString &path);
		MUTILS_API bool is_library_file(const QString &path);

		//Shutdown & Hibernation
		MUTILS_API bool is_hibernation_supported(void);
		MUTILS_API bool shutdown_computer(const QString &message, const unsigned long timeout, const bool forceShutdown, const bool hibernate);

		//Free diskspace
		MUTILS_API bool free_diskspace(const QString &path, quint64 &freeSpace);

		/**
		* \brief Detect drive type
		*
		* This function detetcs the type of the drive to which the given path is pointing.
		*
		* \param path The path to the drive whose type is to be detected. On the Windows platform, only the drive letter is relevant.
		*
		* \param fast_seeking Pointer to a variable that will be set to TRUE, if the drive supports "fast" seeking (e.g. SSD or similar device), or to FALSE otherwise. This parameter is optional and may be NULL.
		*
		* \return The function returns the type of the drive as a `OS::drive_type_t` value. In case of error, the value `DRIVE_TYPE_ERR` will be returned.
		*/
		MUTILS_API drive_type_t get_drive_type(const QString &path, bool *fast_seeking = NULL);

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
		MUTILS_API bool wow64fsredir_disable(uintptr_t &oldValue);
		MUTILS_API bool wow64fsredir_revert (const uintptr_t oldValue);

		//Environment variables
		MUTILS_API QString get_envvar(const QString &name);
		MUTILS_API bool set_envvar(const QString &name, const QString &value);

		//NULL device
		MUTILS_API const QLatin1String &null_device(void);

		//Check if debugger is present
		MUTILS_API void check_debugger(void);

		//Error handling
		MUTILS_API void fatal_exit(const wchar_t* const errorMessage);
	}
}

///////////////////////////////////////////////////////////////////////////////
