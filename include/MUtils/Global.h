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

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	//Random
	void seed_rand(void);
	QString rand_str(const bool &bLong = false);
	quint32 next_rand32(void);
	quint64 next_rand64(void);

	//Temp Folder
	const QString &temp_folder(void);
}

///////////////////////////////////////////////////////////////////////////////

#define MUTILS_DELETE(PTR) do \
{ \
	if((PTR)) \
	{ \
		delete (PTR); \
		(PTR) = NULL; \
	} \
} \
while(0)
