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

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QString>
#include <QDate>

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
			typedef struct _os_info_t
			{
				unsigned int type;
				unsigned int versionMajor;
				unsigned int versionMinor;
				bool overrideFlag;

				//comparision operators
				inline bool operator== (const _os_info_t &rhs) const { return (type == rhs.type) && (versionMajor == rhs.versionMajor) && ((versionMinor == rhs.versionMinor)); }
				inline bool operator!= (const _os_info_t &rhs) const { return (type != rhs.type) || (versionMajor != rhs.versionMajor) || ((versionMinor != rhs.versionMinor)); }
				inline bool operator>  (const _os_info_t &rhs) const { return (type == rhs.type) && ((versionMajor > rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor >  rhs.versionMinor))); }
				inline bool operator>= (const _os_info_t &rhs) const { return (type == rhs.type) && ((versionMajor > rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor >= rhs.versionMinor))); }
				inline bool operator<  (const _os_info_t &rhs) const { return (type == rhs.type) && ((versionMajor < rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor <  rhs.versionMinor))); }
				inline bool operator<= (const _os_info_t &rhs) const { return (type == rhs.type) && ((versionMajor < rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor <= rhs.versionMinor))); }
			}
			os_version_t;

			//Known Windows NT versions
			static const os_version_t WINDOWS_WIN2K = { OS_WINDOWS, 5, 0 };	// 2000
			static const os_version_t WINDOWS_WINXP = { OS_WINDOWS, 5, 1 };	// XP
			static const os_version_t WINDOWS_XPX64 = { OS_WINDOWS, 5, 2 };	// XP_x64
			static const os_version_t WINDOWS_VISTA = { OS_WINDOWS, 6, 0 };	// Vista
			static const os_version_t WINDOWS_WIN70 = { OS_WINDOWS, 6, 1 };	// 7
			static const os_version_t WINDOWS_WIN80 = { OS_WINDOWS, 6, 2 };	// 8
			static const os_version_t WINDOWS_WIN81 = { OS_WINDOWS, 6, 3 };	// 8.1
			static const os_version_t WINDOWS_WN100 = { OS_WINDOWS, 6, 4 };	// 10

			//Unknown OS
			static const os_version_t UNKNOWN_OPSYS = { OS_UNKNOWN, 0, 0 };	// N/A
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

		//Get the OS version
		MUTILS_API const Version::os_version_t &os_version(void);
		MUTILS_API const char *os_friendly_name(const MUtils::OS::Version::os_version_t &os_version);

		//Get known Folder
		MUTILS_API const QString &known_folder(known_folder_t folder_id);

		//Current Date
		MUTILS_API QDate current_date(void);

		//Network Status
		MUTILS_API int network_status(void);

		//Error handling
		MUTILS_API void fatal_exit(const wchar_t* const errorMessage);
	}
}

///////////////////////////////////////////////////////////////////////////////
