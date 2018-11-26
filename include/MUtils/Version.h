///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
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
	class Version
	{
	public:
		//Get Library Version Numbers
		MUTILS_API static const quint32 &lib_version_major(void);
		MUTILS_API static const quint32 &lib_version_minor(void);

		//Get Library Build Date/Time
		MUTILS_API static const QDate lib_build_date(void);
		MUTILS_API static const QTime lib_build_time(void);

		//Get Application Build Date/Time
		MUTILS_API static const QDate app_build_date(const char *const date_str = build_date_raw());
		MUTILS_API static const QTime app_build_time(const char *const time_str = build_time_raw());

		//Compiler detection
		static const char *const compiler_version(void)
		{
			static const char *const COMPILER_VERS =
				#if defined(__INTEL_COMPILER)
					#if (__INTEL_COMPILER >= 1500)
						"ICL 15." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
					#elif (__INTEL_COMPILER >= 1400)
						"ICL 14." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
					#elif (__INTEL_COMPILER >= 1300)
						"ICL 13." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
					#elif (__INTEL_COMPILER >= 1200)
						"ICL 12." MUTILS_MAKE_STRING(__INTEL_COMPILER_BUILD_DATE);
					#elif (__INTEL_COMPILER >= 1100)
						"ICL 11.x";
					#elif (__INTEL_COMPILER >= 1000)
						"ICL 10.x";
					#else
						#error Compiler is not supported!
					#endif
				#elif defined(_MSC_VER)
					#if (_MSC_VER == 1916)
						#if((_MSC_FULL_VER >= 191627024) && (_MSC_FULL_VER <= 191627024))
							"MSVC 2017.9";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1915)
						#if((_MSC_FULL_VER >= 191526726) && (_MSC_FULL_VER <= 191526732))
							"MSVC 2017.8";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1914)
						#if((_MSC_FULL_VER >= 191426430) && (_MSC_FULL_VER <= 191426433))
							"MSVC 2017.7";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1913)
						#if((_MSC_FULL_VER >= 191326128) && (_MSC_FULL_VER <= 191326132))
							"MSVC 2017.6";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1912)
						#if((_MSC_FULL_VER >= 191225830) && (_MSC_FULL_VER <= 191225835))
							"MSVC 2017.5";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1911)
						#if((_MSC_FULL_VER >= 191125542) && (_MSC_FULL_VER <= 191125547))
							"MSVC 2017.4";
						#elif((_MSC_FULL_VER >= 191125506) && (_MSC_FULL_VER <= 191125508))
							"MSVC 2017.3";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1910)
						#if ((_MSC_FULL_VER >= 191025017) && (_MSC_FULL_VER <= 191025019))
							"MSVC 2017.2";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1900)
						#if (_MSC_FULL_VER == 190023026)
							"MSVC 2015";
						#elif (_MSC_FULL_VER == 190023506)
							"MSVC 2015.1";
						#elif (_MSC_FULL_VER == 190023918)
							"MSVC 2015.2";
						#elif (_MSC_FULL_VER == 190024210) || (_MSC_FULL_VER == 190024215)
							"MSVC 2015.3";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1800)
						#if (_MSC_FULL_VER == 180021005)
							"MSVC 2013";
						#elif (_MSC_FULL_VER == 180030501)
							"MSVC 2013.2";
						#elif (_MSC_FULL_VER == 180030723)
							"MSVC 2013.3";
						#elif (_MSC_FULL_VER == 180031101)
							"MSVC 2013.4";
						#elif (_MSC_FULL_VER == 180040629)
							"MSVC 2013.5";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1700)
						#if (_MSC_FULL_VER == 170050727)
							"MSVC 2012";
						#elif (_MSC_FULL_VER == 170051106)
							"MSVC 2012.1";
						#elif (_MSC_FULL_VER == 170060315)
							"MSVC 2012.2";
						#elif (_MSC_FULL_VER == 170060610)
							"MSVC 2012.3";
						#elif (_MSC_FULL_VER == 170061030)
							"MSVC 2012.4";
						#else
							#error Compiler version is not supported yet!
						#endif
					#elif (_MSC_VER == 1600)
						#if (_MSC_FULL_VER >= 160040219)
							"MSVC 2010-SP1";
						#else
							"MSVC 2010";
						#endif
					#elif (_MSC_VER == 1500)
						#if (_MSC_FULL_VER >= 150030729)
							"MSVC 2008-SP1";
						#else
							"MSVC 2008";
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
			static const char *const COMPILER_ARCH =
				#if defined(_M_X64)
					"x64";
				#elif defined(_M_IX86)
					"x86";
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

		//Raw Build time
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
