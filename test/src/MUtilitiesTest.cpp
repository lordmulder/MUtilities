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
// GLOBAL
//===========================================================================

//-----------------------------------------------------------------
// Trim String
//-----------------------------------------------------------------

#define TEST_TRIM_STR(X,Y,Z) do \
{ \
	{ \
		QString test((Y)); \
		MUtils::trim_##X(test); \
		ASSERT_QSTR(test, (Z)); \
	} \
	{ \
		const QString test((Y)); \
		ASSERT_QSTR(MUtils::trim_##X(test), (Z)); \
	} \
} \
while(0)

TEST(Global, TrimStringLeft)
{
	TEST_TRIM_STR(left, "", "");
	TEST_TRIM_STR(left, "   ", "");
	TEST_TRIM_STR(left, "!   test   !", "!   test   !");
	TEST_TRIM_STR(left, "   test   ", "test   ");
	TEST_TRIM_STR(left, "   !   test   !   ", "!   test   !   ");
}

TEST(Global, TrimStringRight)
{
	TEST_TRIM_STR(right, "", "");
	TEST_TRIM_STR(right, "   ", "");
	TEST_TRIM_STR(right, "!   test   !", "!   test   !");
	TEST_TRIM_STR(right, "   test   ", "   test");
	TEST_TRIM_STR(right, "   !   test   !   ", "   !   test   !");
}

#undef TEST_TRIM_STR

//-----------------------------------------------------------------
// Clean File Path
//-----------------------------------------------------------------

#define TEST_CLEAN_FILE(X,Y,Z) do \
{ \
	ASSERT_QSTR(MUtils::clean_file_##X((Y)), (Z)); \
} \
while(0)

TEST(Global, CleanFileName)
{
	static const char *const VALID_CHARS = "!#$%&'()+,-.0123456789;=@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{}~";
	TEST_CLEAN_FILE(name, "", "");
	TEST_CLEAN_FILE(name, VALID_CHARS, VALID_CHARS);
	TEST_CLEAN_FILE(name, "example.txt", "example.txt");
	TEST_CLEAN_FILE(name, " example.txt", " example.txt");
	TEST_CLEAN_FILE(name, "example.txt ", "example.txt");
	TEST_CLEAN_FILE(name, ".example.txt", ".example.txt");
	TEST_CLEAN_FILE(name, "example.txt.", "example.txt");
	TEST_CLEAN_FILE(name, "foo<>:\"/\\|?*\t\r\n.bar", "foo____________.bar");
	TEST_CLEAN_FILE(name, "NUL", "___");
	TEST_CLEAN_FILE(name, "NUL.txt", "___.txt");
	TEST_CLEAN_FILE(name, "NULx.txt", "NULx.txt");
	TEST_CLEAN_FILE(name, "xNUL.txt", "xNUL.txt");
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
