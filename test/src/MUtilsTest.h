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

//Google Test
#include <gtest/gtest.h>

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QDir>

//CRT
#include <cstdio>

//Utilities
#define ASSERT_QSTR(X,Y) ASSERT_EQ((X).compare(QLatin1String(Y)), 0);

//Test data
static const char *const TEST_STRING = "Franz jagt im komplett verwahrlosten Taxi quer durch Bayern.";

//===========================================================================
// TESTBED CLASS
//===========================================================================

class Testbed : public ::testing::Test
{
protected:
	virtual void SetUp() = 0;
	virtual void TearDown() = 0;

	QString makeTempFolder(const char *const suffix)
	{
		const QString tempPath = MUtils::temp_folder();
		if (!tempPath.isEmpty())
		{
			const QString tempSuffix(QString(suffix).simplified().replace(QRegExp("[^\\w]+"), "_"));
			QDir tempDir(tempPath);
			if (tempDir.mkpath(tempSuffix) && tempDir.cd(tempSuffix))
			{
				return tempDir.absolutePath();
			}
		}
		return QString();
	}
};