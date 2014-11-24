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
#include <QString>
#include <QDate>
#include <QTime>

namespace MUtils
{
	class MUTILS_API Version
	{
	public:
		//Get Build Date
		static const QDate build_date(const char *const date_str = build_date_raw());

		//Get Build Time
		static const QTime build_time(const char *const time_str = build_time_raw());

		//Compiler detection
		static const char *const compiler_version(void)
		{
			#if defined(__INTEL_COMPILER)
				#if (__INTEL_COMPILER >= 1500)
					static const char *const COMPILER_VERS = "ICL 15." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
				#elif (__INTEL_COMPILER >= 1400)
					static const char *const COMPILER_VERS = "ICL 14." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
				#elif (__INTEL_COMPILER >= 1300)
					static const char *const COMPILER_VERS = "ICL 13." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
				#elif (__INTEL_COMPILER >= 1200)
					static const char *const COMPILER_VERS = "ICL 12." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
				#elif (__INTEL_COMPILER >= 1100)
					static const char *const COMPILER_VERS = "ICL 11.x";
				#elif (__INTEL_COMPILER >= 1000)
					static const char *const COMPILER_VERS = "ICL 10.x";
				#else
					#error Compiler is not supported!
				#endif
			#elif defined(_MSC_VER)
				#if (_MSC_VER == 1800)
					#if (_MSC_FULL_VER == 180021005)
						static const char *const COMPILER_VERS = "MSVC 2013";
					#elif (_MSC_FULL_VER == 180030501)
						static const char *const COMPILER_VERS = "MSVC 2013.2";
					#elif (_MSC_FULL_VER == 180030723)
						static const char *const COMPILER_VERS = "MSVC 2013.3";
					#elif (_MSC_FULL_VER == 180031101)
						static const char *const COMPILER_VERS = "MSVC 2013.4";
					#else
						#error Compiler version is not supported yet!
					#endif
				#elif (_MSC_VER == 1700)
					#if (_MSC_FULL_VER == 170050727)
						static const char *const COMPILER_VERS = "MSVC 2012";
					#elif (_MSC_FULL_VER == 170051106)
						static const char *const COMPILER_VERS = "MSVC 2012.1";
					#elif (_MSC_FULL_VER == 170060315)
						static const char *const COMPILER_VERS = "MSVC 2012.2";
					#elif (_MSC_FULL_VER == 170060610)
						static const char *const COMPILER_VERS = "MSVC 2012.3";
					#elif (_MSC_FULL_VER == 170061030)
						static const char *const COMPILER_VERS = "MSVC 2012.4";
					#else
						#error Compiler version is not supported yet!
					#endif
				#elif (_MSC_VER == 1600)
					#if (_MSC_FULL_VER >= 160040219)
						static const char *const COMPILER_VERS = "MSVC 2010-SP1";
					#else
						static const char *const COMPILER_VERS = "MSVC 2010";
					#endif
				#elif (_MSC_VER == 1500)
					#if (_MSC_FULL_VER >= 150030729)
						static const char *const COMPILER_VERS = "MSVC 2008-SP1";
					#else
						static const char *const COMPILER_VERS = "MSVC 2008";
					#endif
				#else
					#error Compiler is not supported!
				#endif
			#else
				#error Compiler is not supported!
			#endif

			return COMPILER_VERS;
		}

		//Architecture detection
		static const char *const compiler_arch(void)
		{
			#if defined(_M_X64)
				static const char *const COMPILER_ARCH = "x64";
			#elif defined(_M_IX86)
				static const char *const COMPILER_ARCH = "x86";
			#else
				#error Architecture is not supported!
			#endif

			return COMPILER_ARCH;
		}
	
	private:
		//Raw Build date
		static const char *const build_date_raw(void)
		{
			static const char *const RAW_BUILD_DATE = __DATE__;
			return RAW_BUILD_DATE;
		}

		//Raw Build date
		static const char *const build_time_raw(void)
		{
			static const char *const RAW_BUILD_TIME = __TIME__;
			return RAW_BUILD_TIME;
		}

		//Disable construction
		Version(void)           { throw 666; }
		Version(const Version&) { throw 666; }
	};
}

///////////////////////////////////////////////////////////////////////////////