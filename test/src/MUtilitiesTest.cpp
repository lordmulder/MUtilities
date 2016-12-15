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

//Google Test
#include <gtest/gtest.h>

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Version.h>

//CRT
#include <cstdio>

//Utilities
#define ASSERT_QSTR(X,Y) ASSERT_EQ((X).compare(QLatin1String(Y)), 0);

//===========================================================================
// Trim String
//===========================================================================

TEST(Global, TrimStringLeft)
{
	{
		QString test("");
		MUtils::trim_left(test);
		ASSERT_QSTR(test, "");
	}
	{
		QString test("   ");
		MUtils::trim_left(test);
		ASSERT_QSTR(test, "");
	}
	{
		QString test("!   test   !");
		MUtils::trim_left(test);
		ASSERT_QSTR(test, "!   test   !");
	}
	{
		QString test("   test   ");
		MUtils::trim_left(test);
		ASSERT_QSTR(test, "test   ");
	}
	{
		QString test("   !   test   !   ");
		MUtils::trim_left(test);
		ASSERT_QSTR(test, "!   test   !   ");
	}
	{
		const QString test("   test   ");
		ASSERT_QSTR(MUtils::trim_left(test), "test   ");
	}
}

TEST(Global, TrimStringRight)
{
	{
		QString test("");
		MUtils::trim_right(test);
		ASSERT_QSTR(test, "");
	}
	{
		QString test("   ");
		MUtils::trim_right(test);
		ASSERT_QSTR(test, "");
	}
	{
		QString test("!   test   !");
		MUtils::trim_right(test);
		ASSERT_QSTR(test, "!   test   !");
	}
	{
		QString test("   test   ");
		MUtils::trim_right(test);
		ASSERT_QSTR(test, "   test");
	}
	{
		QString test("   !   test   !   ");
		MUtils::trim_right(test);
		ASSERT_QSTR(test, "   !   test   !");
	}
	{
		const QString test("   test   ");
		ASSERT_QSTR(MUtils::trim_right(test), "   test");
	}
}

//===========================================================================
// Main function
//===========================================================================

int main(int argc, char **argv)
{
	printf("MuldeR's Utilities for Qt - Test Suite\n");
	printf("Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>\n\n");

	printf("Using MUtils v%u.%02u, built %s at %s, %s [%s]\n\n",
		MUtils::Version::lib_version_major(), MUtils::Version::lib_version_minor(),
		MUTILS_UTF8(MUtils::Version::lib_build_date().toString(Qt::ISODate)), MUTILS_UTF8(MUtils::Version::lib_build_time().toString(Qt::ISODate)),
		MUtils::Version::compiler_version(), MUtils::Version::compiler_arch()
	);

	printf("This library is free software; you can redistribute it and/or\n");
	printf("modify it under the terms of the GNU Lesser General Public\n");
	printf("License as published by the Free Software Foundation; either\n");
	printf("version 2.1 of the License, or (at your option) any later version.\n\n");

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
