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

/**
* @file
* @brief This file contains miscellaneous functions that are generally useful for Qt-based applications
*/

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

/**
* \mainpage MuldeR's Utilities for Qt
* 
* The **MUtilities** library is a collection of routines and classes to extend the [*Qt cross-platform framework*](http://qt-project.org/). It contains various convenience and utility functions as well as wrappers for OS-specific functionalities. The library was originally created as a "side product" of the [**LameXP**](http://lamexp.sourceforge.net/) application: Over the years, a lot of code, **not** really specific to *LameXP*, had accumulated in the *LameXP* code base. Some of that code even had been used in other projects too, in a "copy & paste" fashion &ndash; which had lead to redundancy and much complicated maintenance. In order to clean-up the LameXP code base, to eliminate the ugly redundancy and to simplify maintenance, the code in question has finally been refactored into the **MUtilities** (aka "MuldeR's Utilities for Qt") library. This library now forms the foundation of *LameXP* and [*other OpenSource projects*](https://github.com/lordmulder).
* 
* 
* ### API-Documentation
* 
* The public API of the *MUtilities* library is defined in the following header files:
* - **Global.h** &ndash; miscellaneous useful functions
* 
* 
* ### License
* 
* This library is free software. It is released under the terms of the [GNU Lesser General Public License (LGPL), Version 2.1](https://www.gnu.org/licenses/lgpl-2.1.html).
* 
* ```
* MUtilities - MuldeR's Utilities for Qt
* Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>. Some rights reserved.
* 
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
* 
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
* 
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
* ```
*/

namespace MUtils
{
	/**
	* \brief Rerieves the full path of the application's *Temp* folder.
	*
	* The application's *Temp* folder is a unique application-specific folder, intended to store any temporary files that the application may need. It will be created when this function is called for the first time (lazy initialization); subsequent calls are guaranteed to return the same path. Usually the application's *Temp* folder will be created as a sub-folder of the system's global *Temp* folder, as indicated by the `TMP` or `TEMP` environment variables. However, it may be created at a different place (e.g. in the users *Profile* directory), if those environment variables don't point to a usable directory. In any case, this function makes sure that the application's *Temp* folder exists for the whole lifetime of the application and that it is writable. When the application is about to terminate, the application's *Temp* folder and all files or sub-directories thereof will be *removed* automatically!
	*
	* \return If the function succeeds, it returns a read-only reference to a QString holding the full path of the application's *Temp* folder; otherwise a read-only reference to a default-constructed QString is returned.
	*/
	MUTILS_API const QString& temp_folder(void);

	//Process Utils
	MUTILS_API void init_process(QProcess &process, const QString &wokringDir, const bool bReplaceTempDir = true, const QStringList *const extraPaths = NULL);

	/**
	* \brief Generates a *random* unsigned 32-Bit value.
	*
	* The *random* value is created using a "strong" PRNG of the underlying system, if possible. Otherwise a fallback PRNG is used. It is **not** required or useful to call `srand()` or `qsrand()` prior to using this function. If necessary, the seeding of the PRNG happen *automatically* on the first call.
	*
	* \return The function retruns a *random* unsigned 32-Bit value.
	*/
	MUTILS_API quint32 next_rand_u32(void);
	
	/**
	* \brief Generates a *random* unsigned 64-Bit value.
	*
	* The *random* value is created using a "strong" PRNG of the underlying system, if possible. Otherwise a fallback PRNG is used. It is **not** required or useful to call `srand()` or `qsrand()` prior to using this function. If necessary, the seeding of the PRNG happen *automatically* on the first call.
	*
	* \return The function retruns a *random* unsigned 64-Bit value.
	*/
	MUTILS_API quint64 next_rand_u64(void);
	
	/**
	* \brief Generates a *random* string.
	*
	* The random string is generated using the same PRNG as the `next_rand_u64()` function. The *random* bytes are converted to a hexadecimal string and, if necessary, zero-padded to a toal length of 16 or 32 characters. There is **no** `0x`-prefix included in the result.
	*
	* \param bLong If set to `true`, a "long" random string (32 characters) will be generated; if set to `false`, a "short" random string (16 characters) is generated.
	*
	* \return The function retruns a QString holding a *random* hexadecimal string
	*/
	MUTILS_API QString next_rand_str(const bool &bLong = false);

	/**
	* \brief Generates a temporary file name.
	*
	* The function generates a file name that contains a *random* component and that is guaranteed to **not** exist yet. The generated file name follows a `"<basedir>/<random>.<ext>"` pattern. This is useful (not only) for creating temporary files.
	*
	* \param basePath Specifies the "base" directory where the temporary file is supposed to be created. This must be a valid *existing* directory.
	*
	* \param extension Specifies the desired file extensions of the temporary file. Do **not** include a leading dot (`.`) character.
	*
	* \param placeholder If set to `true`, the function creates an empty "placeholder" file under the returned file name; if set to `false`, it does *not*.
	*
	* \return If the function succeeds, it returns a QString holding the full path of the temporary file; otherwise it returns a default-constructed QString.
	*/
	MUTILS_API QString make_temp_file(const QString &basePath, const QString &extension, const bool placeholder = false);

	/**
	* \brief Generates a unique file name.
	*
	* The function generates a unique file name in the specified directory. The function guarantees that the returned file name does *not* exist yet. If necessary, a *counter* will be included in the file name in order to ensure its uniqueness.
	*
	* \param basePath Specifies the "base" directory where the unique file is supposed to be created. This must be a valid *existing* directory.
	*
	* \param baseName Specifies the desired "base" file name of the unqiue file. Do **not** include a file extension.
	*
	* \param extension Specifies the desired file extensions of the unqiue file. Do **not** include a leading dot (`.`) character.
	*
	* \param fancy If set to `true`, the file name is generated according to the `"<basedir>/<basename> (N).<ext>"` pattern; if set to `false`, the file name is generated according to the `"<basedir>/<basename>.XXXX.<ext>"` pattern.
	*
	* \return If the function succeeds, it returns a QString holding the full path of the unique file; otherwise it returns a default-constructed QString.
	*/
	MUTILS_API QString make_unique_file(const QString &basePath, const QString &baseName, const QString &extension, const bool fancy = false);

	/**
	* \brief Computes the *parity* of the given unsigned 32-Bit value
	*
	* \param value The 32-Bit unsigned value from which the parity is to be computed.
	*
	* \return The function returns `true`, if the number of **1** bits in the given value is *odd*; it returns `false`, if the number of **1** bits in the given value is *even*.
	*/
	MUTILS_API bool parity(quint32 value);

	/**
	* \brief Deletes the specified file
	*
	* The function deletes the specified file, even if it has the "read only" flag set. If the file is currently locked (e.g. by another process), the function retries multiple times to delete the file before it fails.
	*
	* \param fileName The path to the file to be deleted. This should be a full path.
	*
	* \return The function returns `true`, if the file was deleted successfully or if the file doesn't exist; it returns `false`, if the file could *not* be deleted.
	*/
	MUTILS_API bool remove_file(const QString &fileName);

	/**
	* \brief Recursively deletes the specified directory
	*
	* The function deletes the specified directory. In *recusive* mode, the directory will be removed including all of its files and sub-directories. Files are deleted using the `remove_file()` function.
	*
	* \param folderPath The path to the directory to be deleted. This should be a full path.
	*
	* \param recursive If set to `true` the function removes all files and sub-directories in the specified directory; if set to `false`, the function will *not* try to delete any files or sub-directories, which means that it will fail on non-empty directories.
	*
	* \return The function returns `true`, if the directory was deleted successfully or if the directory doesn't exist; it returns `false`, if the directory could *not* be deleted.
	*/
	MUTILS_API bool remove_directory(const QString &folderPath, const bool &recursive);

	/**
	* \brief Remove *trailing* white-space characters
	*
	* The function removes all *trailing* white-space characters from the specified string. Leading white-space characters are *not* removed. White-space characters are defined by the `\s` character class.
	*
	* \param str A reference to the QString object to be trimmed. This QString object will be modified directly.
	*
	* \return A reference to the trimmed QString object. This is the same QString object that was specified in the `str` parameter.
	*/
	MUTILS_API QString& trim_right(QString &str);

	/**
	* \brief Remove *leading* white-space characters
	*
	* The function removes all *leading* white-space characters from the specified string. Trailing white-space characters are *not* removed. White-space characters are defined by the `\s` character class.
	*
	* \param str A reference to the QString object to be trimmed. This QString object will be modified directly.
	*
	* \return A reference to the trimmed QString object. This is the same QString object that was specified in the `str` parameter.
	*/
	MUTILS_API QString& trim_left(QString &str);

	/**
	* \brief Remove *trailing* white-space characters
	*
	* The function removes all *trailing* white-space characters from the specified string. Leading white-space characters are *not* removed. White-space characters are defined by the `\s` character class.
	*
	* \param str A read-only reference to the QString object to be trimmed. The original QString object is *not* modified.
	*
	* \return A new QString object that equals the original QString object, except that it has all *trailing* white-space characters removed.
	*/
	MUTILS_API QString trim_right(const QString &str);

	/**
	* \brief Remove *trailing* white-space characters
	*
	* The function removes all *leading* white-space characters from the specified string. Trailing white-space characters are *not* removed. White-space characters are defined by the `\s` character class.
	*
	* \param str A read-only reference to the QString object to be trimmed. The original QString object is *not* modified.
	*
	* \return A new QString object that equals the original QString object, except that it has all *leading* white-space characters removed.
	*/
	MUTILS_API QString trim_left(const QString &str);

	/**
	* \brief Sort a list of strings using "natural ordering" algorithm
	*
	* This function implements a sort algorithm that orders alphanumeric strings in the way a human being would. See [*Natural Order String Comparison*](http://sourcefrog.net/projects/natsort/) for details!
	*
	* \param list A reference to the QStringList object to be sorted. The list will be sorted "in place".
	*/
	MUTILS_API void natural_string_sort(QStringList &list, const bool bIgnoreCase);


	MUTILS_API QString clean_file_name(const QString &name);
	MUTILS_API QString clean_file_path(const QString &path);

	/**
	* \brief Parse regular expression results
	*
	* This function tries to parses the result (capture) of a regular expression as an unsigned 32-Bit value. The given regular expression must contain at least *one* capture. Only the *first* capture is considered, additional captures are ignored.
	*
	* \param regexp A read-only reference to the [QRegExp](http://doc.qt.io/qt-4.8/qregexp.html) object whose result (capture) will be parsed. This [QRegExp](http://doc.qt.io/qt-4.8/qregexp.html) must already have been *successfully* matched against the respective input string, e.g. via `QRegExp::indexIn()`, prior to calling this function.
	*
	* \param value A reference to a variable of type `quint32`, where the unsigned 32-Bit representation of the result will be stored. The contents of this variable are *undefined*, if the function failed.
	*
	* \return The function returns `true`, if the regular expression's capture could be parsed successfully; it returns `false`, if the capture contains an invalid string or if there are insufficient captures in given the [QRegExp](http://doc.qt.io/qt-4.8/qregexp.html) object.
	*/
	MUTILS_API bool regexp_parse_uint32(const QRegExp &regexp, quint32 &value);

	/**
	* \brief Parse regular expression results
	*
	* This function tries to parses the results (captures) of a regular expression as unsigned 32-Bit values. The given regular expression must contain at least \p count captures. Only the *first* \p count captures are considered, additional captures are ignored.
	*
	* \param regexp A read-only reference to the [QRegExp](http://doc.qt.io/qt-4.8/qregexp.html) object whose results (captures) will be parsed. This [QRegExp](http://doc.qt.io/qt-4.8/qregexp.html) must already have been *successfully* matched against the respective input string, e.g. via `QRegExp::indexIn()`, prior to calling this function.
	*
	* \param value A pointer to an array of type `quint32`, where the unsigned 32-Bit representations of the results will be stored (the `n`-th result is stored at `value[n-1]`). The array must be at least \p count elements in length. The contents of this array are *undefined*, if the function failed.
	*
	* \param count Specifies the number of results (captures) in the given [QRegExp](http://doc.qt.io/qt-4.8/qregexp.html) object. The function tries to parse the first \p count captures and ignores any additional captures that may exist. This parameter also determines the required (minimum) length of the \p value array.
	*
	* \return The function returns `true`, if all of the regular expression's captures could be parsed successfully; it returns `false`, if any of the captures contain an invalid string or if there are insufficient captures in given the [QRegExp](http://doc.qt.io/qt-4.8/qregexp.html) object.
	*/
	MUTILS_API bool regexp_parse_uint32(const QRegExp &regexp, quint32 *values, const size_t &count);

	/**
	* \brief Retrieve a list of all available codepages
	*
	* The function generates a list of all codepages that are available on the system. Each codepage name returned by this function may be passed to the `QTextCodec::codecForName()` function in order to obtain a corresponding [QTextCodec](http://doc.qt.io/qt-4.8/qtextcodec.html) object.
	*
	* \param noAliases If set to `true`, only distinct codepages are returned, i.e. any codepage aliases are discarded from the list; if set to `false`, the returned list may (and usually will) also contain codepage aliases.
	*
	* \return If the function succeeds, it returns a QStringList holding the names of all codepages available on the system; otherwise it returns a default-constructed QStringList.
	*/
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
