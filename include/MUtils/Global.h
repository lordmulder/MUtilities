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

//Forward Declarations
class QProcess;

///////////////////////////////////////////////////////////////////////////////

//MUtils API
#ifdef _MSC_VER
#	ifdef MUTILS_DLL_EXPORT
#		define MUTILS_API __declspec(dllexport)
#	else
#		define MUTILS_API __declspec(dllimport)
#	endif
#else
#		define MUTILS_API
#endif

//Helper Macros
#define MUTILS_MAKE_STRING_HELPER(X) #X
#define MUTILS_MAKE_STRING(X) MUTILS_MAKE_STRING_HELPER(X)
#define MUTILS_COMPILER_WARNING(TXT) __pragma(message(__FILE__ "(" MUTILS_MAKE_STRING(__LINE__) ") : warning: " TXT))

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	//Temp Folder
	MUTILS_API const QString& temp_folder(void);

	//Process Utils
	MUTILS_API void init_process(QProcess &process, const QString &wokringDir, const bool bReplaceTempDir = true);

	//Random
	MUTILS_API void seed_rand(void);
	MUTILS_API QString rand_str(const bool &bLong = false);
	MUTILS_API quint32 next_rand32(void);
	MUTILS_API quint64 next_rand64(void);

	//Version
	MUTILS_API const char* buildDate(void);
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

#define MUTILS_DELETE_ARRAY(PTR) do \
{ \
	if((PTR)) \
	{ \
		delete [] (PTR); \
		(PTR) = NULL; \
	} \
} \
while(0)

#define MUTILS_QUTF8(STR) ((STR).toUtf8().constData())
#define MUTILS_QSTR2WCHAR(STR) (reinterpret_cast<const wchar_t*>((STR).utf16()))
#define MUTILS_WCHAR2QSTR(STR) (QString::fromUtf16(reinterpret_cast<const unsigned short*>((STR))))
