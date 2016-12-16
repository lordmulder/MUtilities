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
#include <MUtils/OSSupport.h>
#include <MUtils/Version.h>

//Qt
#include <QSet>
#include <QDir>

//CRT
#include <cstdio>

//Utilities
#define ASSERT_QSTR(X,Y) ASSERT_EQ((X).compare(QLatin1String(Y)), 0);

//===========================================================================
// GLOBAL
//===========================================================================

class Global : public ::testing::Test
{
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}

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

//-----------------------------------------------------------------
// Random
//-----------------------------------------------------------------

#define TEST_RANDOM_MAX 99991
#define TEST_RANDOM(X,Y) do \
{ \
	MUtils::seed_rand(); \
	QSet<X> test; \
	int attempts = 0; \
	while(test.count() != TEST_RANDOM_MAX) \
	{ \
		if(++attempts <= 64) \
		{ \
			if(attempts > 1) \
			{ \
				MUtils::OS::sleep_ms(1); \
			} \
			test.clear(); \
			for (size_t i = 0; i < TEST_RANDOM_MAX; ++i)  \
			{  \
				test.insert(MUtils::next_rand_##Y()); \
			} \
		} \
		else \
		{ \
			FAIL(); /*too many attempts!*/ \
		} \
	} \
} \
while(0)

TEST_F(Global, RandomU32)
{
	TEST_RANDOM(quint32, u32);
}

TEST_F(Global, RandomU64)
{
	TEST_RANDOM(quint64, u64);
}

TEST_F(Global, RandomStr)
{
	TEST_RANDOM(QString, str);
}

#undef TEST_RANDOM
#undef RND_LIMIT

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

TEST_F(Global, TrimStringLeft)
{
	TEST_TRIM_STR(left, "", "");
	TEST_TRIM_STR(left, "   ", "");
	TEST_TRIM_STR(left, "!   test   !", "!   test   !");
	TEST_TRIM_STR(left, "   test   ", "test   ");
	TEST_TRIM_STR(left, "   !   test   !   ", "!   test   !   ");
}

TEST_F(Global, TrimStringRight)
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

static const char *const VALID_FILENAME_CHARS = "!#$%&'()+,-.0123456789;=@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz{}~";

TEST_F(Global, CleanFileName)
{
	TEST_CLEAN_FILE(name, "", "");
	TEST_CLEAN_FILE(name, VALID_FILENAME_CHARS, VALID_FILENAME_CHARS);
	TEST_CLEAN_FILE(name, "example.txt", "example.txt");
	TEST_CLEAN_FILE(name, " example.txt", " example.txt");
	TEST_CLEAN_FILE(name, "example.txt ", "example.txt");
	TEST_CLEAN_FILE(name, ".example.txt", ".example.txt");
	TEST_CLEAN_FILE(name, "example.txt.", "example.txt");
	TEST_CLEAN_FILE(name, "foo<>:\"/\\|?*\t\r\nbar", "foo____________bar");
	TEST_CLEAN_FILE(name, "NUL", "___");
	TEST_CLEAN_FILE(name, "xNUL", "xNUL");
	TEST_CLEAN_FILE(name, "NULx", "NULx");
	TEST_CLEAN_FILE(name, "NUL.txt", "___.txt");
	TEST_CLEAN_FILE(name, "NULx.txt", "NULx.txt");
	TEST_CLEAN_FILE(name, "xNUL.txt", "xNUL.txt");
}

TEST_F(Global, CleanFilePath)
{
	TEST_CLEAN_FILE(path, "", "");
	TEST_CLEAN_FILE(path, VALID_FILENAME_CHARS, VALID_FILENAME_CHARS);
	TEST_CLEAN_FILE(path, "c:\\foo\\bar\\example.txt", "c:/foo/bar/example.txt");
	TEST_CLEAN_FILE(path, "c:/foo/bar/example.txt", "c:/foo/bar/example.txt");
	TEST_CLEAN_FILE(path, "foo\\bar\\example.txt", "foo/bar/example.txt");
	TEST_CLEAN_FILE(path, "\\foo\\bar\\example.txt", "/foo/bar/example.txt");
	TEST_CLEAN_FILE(path, "\\\\hostname\\share\\example.txt", "//hostname/share/example.txt");
	TEST_CLEAN_FILE(path, "\\\\?\\c:\\very long path", "//?/c:/very long path");
	TEST_CLEAN_FILE(path, "c:\\foo<>:\"|?*\t\r\nbar\\example.txt", "c:/foo__________bar/example.txt");
	TEST_CLEAN_FILE(path, "c:\\example\\foo<>:\"|?*\t\r\nbar.txt", "c:/example/foo__________bar.txt");
	TEST_CLEAN_FILE(path, "c:\\ foo\\ bar\\ example.txt", "c:/ foo/ bar/ example.txt");
	TEST_CLEAN_FILE(path, "c:\\foo \\bar \\example.txt ", "c:/foo/bar/example.txt");
	TEST_CLEAN_FILE(path, "c:\\foo   bar\\exa   mple.txt", "c:/foo   bar/exa   mple.txt");
	TEST_CLEAN_FILE(path, "c:\\example\\NUL", "c:/example/___");
	TEST_CLEAN_FILE(path, "c:\\example\\xNUL", "c:/example/xNUL");
	TEST_CLEAN_FILE(path, "c:\\example\\NULx", "c:/example/NULx");
	TEST_CLEAN_FILE(path, "c:\\example\\NUL.txt", "c:/example/___.txt");
	TEST_CLEAN_FILE(path, "c:\\example\\xNUL.txt", "c:/example/xNUL.txt");
	TEST_CLEAN_FILE(path, "c:\\example\\NULx.txt", "c:/example/NULx.txt");
}

//-----------------------------------------------------------------
// File Names
//-----------------------------------------------------------------

#define TEST_FILE_NAME(X, Y, ...) \
{ \
	const QString workDir = makeTempFolder(__FUNCTION__); \
	ASSERT_FALSE(workDir.isEmpty()); \
	QSet<QString> test; \
	const QRegExp pattern((Y)); \
	for (int i = 0; i < 997; ++i) \
	{ \
		const QString name = MUtils::make_##X##_file(workDir, __VA_ARGS__); \
		ASSERT_FALSE(name.isEmpty()); \
		ASSERT_FALSE(test.contains(name)); \
		ASSERT_GE(pattern.indexIn(name), 0); \
		test.insert(name); \
		QFile file(name); \
		ASSERT_TRUE(file.open(QIODevice::ReadWrite)); \
		ASSERT_GT(file.write("foo"), 0); \
		file.close(); \
	} \
	for (QSet<QString>::const_iterator iter = test.constBegin(); iter != test.constEnd(); iter++) \
	{ \
		ASSERT_TRUE(QFile::exists(*iter)); \
		QFile::remove(*iter); \
	} \
}

TEST_F(Global, TempFileName)
{
	TEST_FILE_NAME(temp, "/\\w+\\.txt$", "txt", true);
}

TEST_F(Global, UniqFileName)
{
	TEST_FILE_NAME(unique, "/example.\\w+\\.txt$", "example", "txt");
}

#undef TEST_FILE_NAME

//-----------------------------------------------------------------
// Parity
//-----------------------------------------------------------------

TEST_F(Global, Parity)
{
	ASSERT_EQ(MUtils::parity(0x00000000), false);
	ASSERT_EQ(MUtils::parity(0x11111111), false);
	ASSERT_EQ(MUtils::parity(0xFFFFFFFF), false);
	ASSERT_EQ(MUtils::parity(0x00000001), true );
	ASSERT_EQ(MUtils::parity(0x00000010), true );
	ASSERT_EQ(MUtils::parity(0x00000100), true );
	ASSERT_EQ(MUtils::parity(0x00001000), true );
	ASSERT_EQ(MUtils::parity(0x00010000), true );
	ASSERT_EQ(MUtils::parity(0x00100000), true );
	ASSERT_EQ(MUtils::parity(0x01000000), true );
	ASSERT_EQ(MUtils::parity(0x10000000), true );
	ASSERT_EQ(MUtils::parity(0xEFFFFFFF), true);
	ASSERT_EQ(MUtils::parity(0xFEFFFFFF), true);
	ASSERT_EQ(MUtils::parity(0xFFEFFFFF), true);
	ASSERT_EQ(MUtils::parity(0xFFFEFFFF), true);
	ASSERT_EQ(MUtils::parity(0xFFFFEFFF), true);
	ASSERT_EQ(MUtils::parity(0xFFFFFEFF), true);
	ASSERT_EQ(MUtils::parity(0xFFFFFFEF), true);
	ASSERT_EQ(MUtils::parity(0xFFFFFFFE), true);
	ASSERT_EQ(MUtils::parity(0x10101010), false);
	ASSERT_EQ(MUtils::parity(0x01010101), false);
	ASSERT_EQ(MUtils::parity(0xC8A2CC96), false);
	ASSERT_EQ(MUtils::parity(0x504928DD), true );
	ASSERT_EQ(MUtils::parity(0x38BFB9EC), false);
	ASSERT_EQ(MUtils::parity(0x73F42695), true );
	ASSERT_EQ(MUtils::parity(0x9161E326), false);
	ASSERT_EQ(MUtils::parity(0xB1C93AC2), true );
	ASSERT_EQ(MUtils::parity(0xCA4B1193), false);
}

//===========================================================================
// Main function
//===========================================================================

int main(int argc, char **argv)
{
	printf("MuldeR's Utilities for Qt v%u.%02u - Regression Test Suite [%s]\n", MUtils::Version::lib_version_major(), MUtils::Version::lib_version_minor(), MUTILS_DEBUG ? "DEBUG" : "RELEASE");
	printf("Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>. Some rights reserved.\n");
	printf("Built on %s at %s with %s for Win-%s.\n\n", MUTILS_UTF8(MUtils::Version::lib_build_date().toString(Qt::ISODate)), MUTILS_UTF8(MUtils::Version::lib_build_time().toString(Qt::ISODate)), MUtils::Version::compiler_version(), MUtils::Version::compiler_arch());

	printf("This library is free software; you can redistribute it and/or\n");
	printf("modify it under the terms of the GNU Lesser General Public\n");
	printf("License as published by the Free Software Foundation; either\n");
	printf("version 2.1 of the License, or (at your option) any later version.\n\n");

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
