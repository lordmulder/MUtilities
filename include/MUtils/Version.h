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

#define MUTILS_MAKE_STRING_HELPER(X) #X
#define MUTILS_MAKE_STRING(X) MUTILS_MAKE_STRING_HELPER(X)
#define MUTILS_COMPILER_WARNING(TXT) __pragma(message(__FILE__ "(" MUTILS_MAKE_STRING(__LINE__) ") : warning: " TXT))

namespace MUtils
{
	namespace Version
	{
		//Raw Build date
		const char *const BUILD_DATE = __DATE__;

		//Get Build Data
		const QDate build_date(const char *const raw_date = BUILD_DATE);

		//Compiler detection
		#if defined(__INTEL_COMPILER)
			#if (__INTEL_COMPILER >= 1500)
				static const char *COMPILER_VERS = "ICL 15." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
			#elif (__INTEL_COMPILER >= 1400)
				static const char *COMPILER_VERS = "ICL 14." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
			#elif (__INTEL_COMPILER >= 1300)
				static const char *COMPILER_VERS = "ICL 13." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
			#elif (__INTEL_COMPILER >= 1200)
				static const char *COMPILER_VERS = "ICL 12." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
			#elif (__INTEL_COMPILER >= 1100)
				static const char *COMPILER_VERS = "ICL 11.x";
			#elif (__INTEL_COMPILER >= 1000)
				static const char *COMPILER_VERS = "ICL 10.x";
			#else
				#error Compiler is not supported!
			#endif
		#elif defined(_MSC_VER)
			#if (_MSC_VER == 1800)
				#if (_MSC_FULL_VER == 180021005)
					static const char *COMPILER_VERS = "MSVC 2013";
				#elif (_MSC_FULL_VER == 180030501)
					static const char *COMPILER_VERS = "MSVC 2013.2";
				#elif (_MSC_FULL_VER == 180030723)
					static const char *COMPILER_VERS = "MSVC 2013.3";
				#elif (_MSC_FULL_VER == 180031101)
					static const char *COMPILER_VERS = "MSVC 2013.4";
				#else
					#error Compiler version is not supported yet!
				#endif
			#elif (_MSC_VER == 1700)
				#if (_MSC_FULL_VER == 170050727)
					static const char *COMPILER_VERS = "MSVC 2012";
				#elif (_MSC_FULL_VER == 170051106)
					static const char *COMPILER_VERS = "MSVC 2012.1";
				#elif (_MSC_FULL_VER == 170060315)
					static const char *COMPILER_VERS = "MSVC 2012.2";
				#elif (_MSC_FULL_VER == 170060610)
					static const char *COMPILER_VERS = "MSVC 2012.3";
				#elif (_MSC_FULL_VER == 170061030)
					static const char *COMPILER_VERS = "MSVC 2012.4";
				#else
					#error Compiler version is not supported yet!
				#endif
			#elif (_MSC_VER == 1600)
				#if (_MSC_FULL_VER >= 160040219)
					static const char *COMPILER_VERS = "MSVC 2010-SP1";
				#else
					static const char *COMPILER_VERS = "MSVC 2010";
				#endif
			#elif (_MSC_VER == 1500)
				#if (_MSC_FULL_VER >= 150030729)
					static const char *COMPILER_VERS = "MSVC 2008-SP1";
				#else
					static const char *COMPILER_VERS = "MSVC 2008";
				#endif
			#else
				#error Compiler is not supported!
			#endif

			// Note: /arch:SSE and /arch:SSE2 are only available for the x86 platform
			#if !defined(_M_X64) && defined(_M_IX86_FP)
				#if (_M_IX86_FP == 1)
					MUTILS_COMPILER_WARNING("SSE instruction set is enabled!")
				#elif (_M_IX86_FP == 2)
					MUTILS_COMPILER_WARNING("SSE2 (or higher) instruction set is enabled!")
				#endif
			#endif
		#else
			#error Compiler is not supported!
		#endif

		//Architecture detection
		#if defined(_M_X64)
			static const char *COMPILER_ARCH = "x64";
		#elif defined(_M_IX86)
			static const char *COMPILER_ARCH = "x86";
		#else
			#error Architecture is not supported!
		#endif
	}
}

///////////////////////////////////////////////////////////////////////////////
