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
#include <QStringList>

namespace MUtils
{
	namespace CPUFetaures
	{
		//CPU flags
		static const quint32 FLAG_MMX   = 0x01;
		static const quint32 FLAG_SSE   = 0x02;
		static const quint32 FLAG_SSE2  = 0x04;
		static const quint32 FLAG_SSE3  = 0x08;
		static const quint32 FLAG_SSSE3 = 0x10;
		static const quint32 FLAG_SSE4  = 0x20;
		static const quint32 FLAG_SSE42 = 0x40;

		//CPU features
		typedef struct _cpu_info_t
		{
			quint32 family;
			quint32 model;
			quint32 stepping;
			quint32 count;
			quint32 features;
			bool x64;
			bool intel;
			char vendor[0x40];
			char brand[0x40];
		}
		cpu_info_t;

		MUTILS_API cpu_info_t detect(const QStringList &argv);
	}
}
