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

//Forward Declarations
class QApplication;

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Startup
	{
		//Main Function
		typedef int (main_function_t)(int &argc, char **argv);

		//Startup Application
		MUTILS_API int startup(int &argc, char **argv, main_function_t *const entry_point, const char* const appName, const bool &debugConsole);

		//Initialize Qt
		MUTILS_API QApplication *create_qt(int &argc, char **argv, const QString &appName, const QString &appAuthor = QLatin1String("LoRd_MuldeR"), const QString &appDomain = QLatin1String("muldersoft.com"));
	}
}

///////////////////////////////////////////////////////////////////////////////
