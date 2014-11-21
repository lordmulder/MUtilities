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

#include <MUtils/Version.h>

#include <MUtils/Global.h>
#include <MUtils/Exception.h>

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

const QDate MUtils::Version::build_date(const char *const raw_date)
{
	int date[3] = {0, 0, 0};
	char temp_m[4], temp_d[3], temp_y[5];

	temp_m[0] = raw_date[0x0];
	temp_m[1] = raw_date[0x1];
	temp_m[2] = raw_date[0x2];
	temp_m[3] = 0x00;

	temp_d[0] = raw_date[0x4];
	temp_d[1] = raw_date[0x5];
	temp_d[2] = 0x00;

	temp_y[0] = raw_date[0x7];
	temp_y[1] = raw_date[0x8];
	temp_y[2] = raw_date[0x9];
	temp_y[3] = raw_date[0xA];
	temp_y[4] = 0x00;
		
	date[0] = atoi(temp_y);
	date[1] = month2int(temp_m);
	date[2] = atoi(temp_d);

	if(!((date[0] > 0) && (date[1] > 0) && (date[2] > 0)))
	{
		MUTILS_THROW("Internal error: Date format could not be recognized!");
	}
	
	return QDate(date[0], date[1], date[2]);
}

///////////////////////////////////////////////////////////////////////////////
