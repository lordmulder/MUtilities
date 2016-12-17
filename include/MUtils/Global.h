///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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
#		ifndef MUTILS_STATIC_LIB
#			define MUTILS_API __declspec(dllimport)
#		else
#			define MUTILS_API /*static lib*/
#		endif
#	endif
#else
#	define MUTILS_API
#endif

//Helper Macros
#define MUTILS_MAKE_STRING_HELPER(X) #X
#define MUTILS_MAKE_STRING(X) MUTILS_MAKE_STRING_HELPER(X)
#define MUTILS_COMPILER_WARNING(TXT) __pragma(message(__FILE__ "(" MUTILS_MAKE_STRING(__LINE__) ") : warning: " TXT))

//Check Debug Flags
#if defined(_DEBUG) || defined(DEBUG) || (!defined(NDEBUG))
#	define MUTILS_DEBUG (1)
#	if defined(NDEBUG) || defined(QT_NO_DEBUG) || (!defined(QT_DEBUG))
#		error Inconsistent DEBUG flags have been detected!
#	endif
#else
#	define MUTILS_DEBUG (0)
#	if (!defined(NDEBUG)) || (!defined(QT_NO_DEBUG)) || defined(QT_DEBUG)
#		error Inconsistent DEBUG flags have been detected!
#	endif
#endif

//Check CPU options
#if defined(_MSC_VER) && (!defined(__INTELLISENSE__)) && (!defined(_M_X64)) && defined(_M_IX86_FP)
	#if (_M_IX86_FP != 0)
		#error We should not enabled SSE or SSE2 in release builds!
	#endif
#endif

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	//Temp Folder
	MUTILS_API const QString& temp_folder(void);

	//Process Utils
	MUTILS_API void init_process(QProcess &process, const QString &wokringDir, const bool bReplaceTempDir = true, const QStringList *const extraPaths = NULL);

	//Random
	MUTILS_API void seed_rand(void);
	MUTILS_API QString next_rand_str(const bool &bLong = false);
	MUTILS_API quint32 next_rand_u32(void);
	MUTILS_API quint64 next_rand_u64(void);

	//File Name
	MUTILS_API QString make_temp_file(const QString &basePath, const QString &extension, const bool placeholder = false);
	MUTILS_API QString make_unique_file(const QString &basePath, const QString &baseName, const QString &extension, const bool fancy = false);

	//Parity
	MUTILS_API bool parity(quint32 value);

	//Remove File/Dir
	MUTILS_API bool remove_file(const QString &fileName);
	MUTILS_API bool remove_directory(const QString &folderPath, const bool &recursive);

	//String utils
	MUTILS_API QString& trim_right(QString &str);
	MUTILS_API QString& trim_left(QString &str);
	MUTILS_API QString trim_right(const QString &str);
	MUTILS_API QString trim_left(const QString &str);

	//String sorting
	MUTILS_API void natural_string_sort(QStringList &list, const bool bIgnoreCase);

	//Clean file path
	MUTILS_API QString clean_file_name(const QString &name);
	MUTILS_API QString clean_file_path(const QString &path);

	//Regular expressions
	MUTILS_API bool regexp_parse_uint32(const QRegExp &regexp, quint32 &value);
	MUTILS_API bool regexp_parse_uint32(const QRegExp &regexp, quint32 *values, const size_t &count);

	//Internationalization
	MUTILS_API QStringList available_codepages(const bool &noAliases = true);

	//Internal
	namespace Internal
	{
		MUTILS_API int selfTest(const char *const buildKey, const bool debug);
		static const int s_selfTest = selfTest(__DATE__ "@" __TIME__, MUTILS_DEBUG);
	}
}

///////////////////////////////////////////////////////////////////////////////

//Delete helper
#define MUTILS_DELETE(PTR) do { if((PTR)) { delete (PTR); (PTR) = NULL; } } while(0)
#define MUTILS_DELETE_ARRAY(PTR) do { if((PTR)) { delete [] (PTR); (PTR) = NULL; } } while(0)

//Zero memory
#define MUTILS_ZERO_MEMORY(PTR) memset(&(PTR), 0, sizeof((PTR)))

//String conversion macros
#define MUTILS_WCHR(STR) (reinterpret_cast<const wchar_t*>((STR).utf16()))
#define MUTILS_UTF8(STR) ((STR).toUtf8().constData())
#define MUTILS_QSTR(STR) (QString::fromUtf16(reinterpret_cast<const unsigned short*>((STR))))

//Boolean helper
#define MUTILS_BOOL2STR(X) ((X) ? "1" : "0")
