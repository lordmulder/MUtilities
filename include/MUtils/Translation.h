///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <QtGlobal>

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Translation
	{
		//Register new translation
		MUTILS_API bool insert(const QString &langId, const QString &qmFile, const QString &langName, const quint32 &systemId, const quint32 &country);

		//Enumerate translations
		MUTILS_API int enumerate(QStringList &list);

		//Get translation info
		MUTILS_API QString get_name   (const QString &langId);
		MUTILS_API quint32 get_sysid  (const QString &langId);
		MUTILS_API quint32 get_country(const QString &langId);

		//Install translator
		MUTILS_API bool install_translator(const QString &langId);
		MUTILS_API bool install_translator_from_file(const QString &qmFile);

		//Constant
		MUTILS_API extern const char *const DEFAULT_LANGID;
	}
}

///////////////////////////////////////////////////////////////////////////////
