///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2026 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <stdint.h>
#include <MUtils/Global.h>
class QIcon;

namespace MUtils
{
	namespace Win32Utils
	{
		uintptr_t qicon_to_hicon(const QIcon *const icon, const int w, const int h);
		const uintptr_t &resolve_helper(const QString &libraryName, const QString &functionName);

		template<class T>
		T resolve(const QString &libraryName, const QString &functionName)
		{
			return reinterpret_cast<T>(resolve_helper(libraryName, functionName));
		}
	}
}
