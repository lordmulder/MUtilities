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

#define MUTILS_INC_CONFIG 1

#include <MUtils/Version.h>

//Internal
#include <MUtils/Global.h>
#include <MUtils/Exception.h>
#include "Config.h"

#ifdef _MSC_VER
#define _snscanf(X, Y, Z, ...) _snscanf_s((X), (Y), (Z), __VA_ARGS__)
#endif

///////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static const char *g_months_lut[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static int month2int(const char *str)
{
	int ret = 0;

	for(int j = 0; j < 12; j++)
	{
		if(!_strcmpi(str, g_months_lut[j]))
		{
			ret = j+1;
			break;
		}
	}

	return ret;
}

static const QDate decode_date_str(const char *const date_str)
{
	bool ok = true;
	int date[3] = {0, 0, 0};
	char month_s[4];

	ok = ok && (_snscanf(&date_str[0x0], 3, "%s", &month_s) == 1);
	ok = ok && ((date[1] = month2int(month_s)) > 0);
	ok = ok && (_snscanf(&date_str[0x4], 2, "%d", &date[2]) == 1);
	ok = ok && (_snscanf(&date_str[0x7], 4, "%d", &date[0]) == 1);

	if(!ok)
	{
		MUTILS_THROW("Internal error: Date format could not be recognized!");
	}
	
	//qWarning("MUtils::Version::build_date: y=%d, m=%d, d=%d", date[0], date[1], date[2]);
	return QDate(date[0], date[1], date[2]);
}

static const QTime decode_time_str(const char *const time_str)
{
	bool ok = true;
	int time[3] = {0, 0, 0};

	ok = ok && (_snscanf(&time_str[0x0], 2, "%d", &time[0]) == 1);
	ok = ok && (_snscanf(&time_str[0x3], 2, "%d", &time[1]) == 1);
	ok = ok && (_snscanf(&time_str[0x6], 2, "%d", &time[2]) == 1);

	if(!ok)
	{
		MUTILS_THROW("Internal error: Time format could not be recognized!");
	}

	//qWarning("MUtils::Version::build_date: h=%d, m=%d, s=%d", time[0], time[1], time[2]);
	return QTime(time[0], time[1], time[2]);
}

///////////////////////////////////////////////////////////////////////////////
// LIB VERSION
///////////////////////////////////////////////////////////////////////////////

const QDate MUtils::Version::lib_build_date(void)
{
	static const char *const BUILD_DATE = __DATE__;
	return decode_date_str(BUILD_DATE);
}

const QTime MUtils::Version::lib_build_time(void)
{
	static const char *const BUILD_TIME = __TIME__;
	return decode_time_str(BUILD_TIME);
}

const quint32 &MUtils::Version::lib_version_major(void)
{
	static const quint32 VERSION_MAJOR = VER_MUTILS_MAJOR;
	return VERSION_MAJOR;
}

const quint32 &MUtils::Version::lib_version_minor(void)
{
	static const quint32 VERSION_MINOR = (10 * VER_MUTILS_MINOR_HI) + VER_MUTILS_MINOR_LO;
	return VERSION_MINOR;
}


///////////////////////////////////////////////////////////////////////////////
// APP VERSION
///////////////////////////////////////////////////////////////////////////////

const QDate MUtils::Version::app_build_date(const char *const date_str)
{
	return decode_date_str(date_str);
}

const QTime MUtils::Version::app_build_time(const char *const time_str)
{
	return decode_time_str(time_str);
}

///////////////////////////////////////////////////////////////////////////////
