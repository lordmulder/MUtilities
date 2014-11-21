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

#include <QString>
#include <QDate>

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace OS
	{
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
		
		//Get known Folder
		const QString &known_folder(known_folder_t folder_id);

		//Current Date
		QDate current_date(void);

		//Network Status
		int network_status(void);

		//Error handling
		void fatal_exit(const char* const errorMessage);
	}
}

///////////////////////////////////////////////////////////////////////////////
