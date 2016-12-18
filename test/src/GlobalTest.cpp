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

#include "MUtilsTest.h"

//MUtils
#include <MUtils/OSSupport.h>

//Qt
#include <QSet>

//===========================================================================
// TESTBED CLASS
//===========================================================================

class GlobalTest : public Testbed
{
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

//===========================================================================
// TEST METHODS
//===========================================================================

//-----------------------------------------------------------------
// Random
//-----------------------------------------------------------------

#define TEST_RANDOM_MAX 29989
#define TEST_RANDOM(X,Y) do \
{ \
	QSet<X> test; \
	for (size_t i = 0; i < TEST_RANDOM_MAX; ++i)  \
	{  \
		test.insert(MUtils::next_rand_##Y()); \
	} \
	ASSERT_EQ(test.count(), TEST_RANDOM_MAX); \
} \
while(0)

TEST_F(GlobalTest, RandomU32)
{
	TEST_RANDOM(quint32, u32);
}

TEST_F(GlobalTest, RandomU64)
{
	TEST_RANDOM(quint64, u64);
}

TEST_F(GlobalTest, RandomStr)
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

TEST_F(GlobalTest, TrimStringLeft)
{
	TEST_TRIM_STR(left, "", "");
	TEST_TRIM_STR(left, "   ", "");
	TEST_TRIM_STR(left, "!   test   !", "!   test   !");
	TEST_TRIM_STR(left, "   test   ", "test   ");
	TEST_TRIM_STR(left, "   !   test   !   ", "!   test   !   ");
}

TEST_F(GlobalTest, TrimStringRight)
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

TEST_F(GlobalTest, CleanFileName)
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

TEST_F(GlobalTest, CleanFilePath)
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
		ASSERT_GE(file.write(TEST_STRING), strlen(TEST_STRING)); \
		file.close(); \
	} \
	for (QSet<QString>::const_iterator iter = test.constBegin(); iter != test.constEnd(); iter++) \
	{ \
		ASSERT_TRUE(QFile::exists(*iter)); \
		QFile::remove(*iter); \
	} \
}

TEST_F(GlobalTest, TempFileName)
{
	TEST_FILE_NAME(temp, "/\\w+\\.txt$", "txt", true);
}

TEST_F(GlobalTest, UniqFileName)
{
	TEST_FILE_NAME(unique, "/example.\\w+\\.txt$", "example", "txt");
}

#undef TEST_FILE_NAME

//-----------------------------------------------------------------
// Parity
//-----------------------------------------------------------------

TEST_F(GlobalTest, Parity)
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
	ASSERT_EQ(MUtils::parity(0xEFFFFFFF), true );
	ASSERT_EQ(MUtils::parity(0xFEFFFFFF), true );
	ASSERT_EQ(MUtils::parity(0xFFEFFFFF), true );
	ASSERT_EQ(MUtils::parity(0xFFFEFFFF), true );
	ASSERT_EQ(MUtils::parity(0xFFFFEFFF), true );
	ASSERT_EQ(MUtils::parity(0xFFFFFEFF), true );
	ASSERT_EQ(MUtils::parity(0xFFFFFFEF), true );
	ASSERT_EQ(MUtils::parity(0xFFFFFFFE), true );
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

//-----------------------------------------------------------------
// Remove File/Dirrectory
//-----------------------------------------------------------------

#define MAKE_TEST_FILE(X) do \
{ \
	ASSERT_TRUE((X).open(QIODevice::ReadWrite)); \
	ASSERT_GE((X).write(TEST_STRING), strlen(TEST_STRING)); \
	(X).setPermissions(QFile::ReadOwner|QFile::ExeOwner|QFile::ReadGroup|QFile::ExeGroup|QFile::ReadOther|QFile::ExeOther); \
} \
while(0)

#define MAKE_SUB_DIR(X,Y) do \
{ \
	ASSERT_TRUE((X).mkpath((Y))); \
	ASSERT_TRUE((X).cd((Y))); \
} \
while (0)

TEST_F(GlobalTest, RemoveFile)
{
	const QString workDir = makeTempFolder(__FUNCTION__);
	ASSERT_FALSE(workDir.isEmpty());
	const QString fileName = QString("%1/example.txt").arg(workDir);
	QFile test(fileName);
	MAKE_TEST_FILE(test);
	ASSERT_FALSE(MUtils::remove_file(fileName));
	test.close();
	ASSERT_TRUE(QFileInfo(fileName).exists());
	ASSERT_TRUE(MUtils::remove_file(fileName));
	ASSERT_FALSE(QFileInfo(fileName).exists());
}

TEST_F(GlobalTest, Directory)
{
	const QString workDir = makeTempFolder(__FUNCTION__);
	ASSERT_FALSE(workDir.isEmpty());
	static const char *const DIR_NAMES[] = { "foo", "bar", NULL };
	for (size_t i = 0; DIR_NAMES[i]; i++)
	{
		QDir dir(workDir);
		MAKE_SUB_DIR(dir, QLatin1String(DIR_NAMES[i]));
		for (size_t j = 0; DIR_NAMES[j]; j++)
		{
			QDir subdir(dir);
			MAKE_SUB_DIR(subdir, QLatin1String(DIR_NAMES[j]));
			QFile test(subdir.filePath("example.txt"));
			MAKE_TEST_FILE(test);
			test.close();
		}
	}
	for (size_t i = 0; DIR_NAMES[i]; i++)
	{
		QDir dir(QString("%1/%2").arg(workDir, QLatin1String(DIR_NAMES[i])));
		ASSERT_TRUE(dir.exists());
		ASSERT_FALSE(MUtils::remove_directory(dir.absolutePath(), false));
		dir.refresh();
		ASSERT_TRUE(dir.exists());
		ASSERT_TRUE(MUtils::remove_directory(dir.absolutePath(), true));
		dir.refresh();
		ASSERT_FALSE(dir.exists());
	}
}

#undef MAKE_TEST_FILE
#undef MAKE_SUB_DIR

//-----------------------------------------------------------------
// Natural String Sort
//-----------------------------------------------------------------

TEST_F(GlobalTest, NaturalStrSort)
{
	static const char *const TEST[] =
	{
		"z0.txt",   "z1.txt",   "z2.txt",   "z3.txt",   "z4.txt",   "z5.txt",   "z6.txt",   "z7.txt",   "z8.txt",   "z9.txt",
		"z10.txt",  "z11.txt",  "z12.txt",  "z13.txt",  "z14.txt",  "z15.txt",  "z16.txt",  "z17.txt",  "z18.txt",  "z19.txt",
		"z100.txt", "z101.txt", "z102.txt", "z103.txt", "z104.txt", "z105.txt", "z106.txt", "z107.txt", "z108.txt", "z109.txt",
		NULL
	};

	QStringList test;
	for (size_t i = 0; TEST[i]; i++)
	{
		test << QLatin1String(TEST[i]);
	}

	qsrand(time(NULL));
	for (size_t q = 0; q < 97; q++)
	{
		for (size_t k = 0; k < 997; k++)
		{
			const size_t len = size_t(test.count());
			for (size_t i = 0; i < len; i++)
			{
				test.swap(i, qrand() % len);
			}
		}
		MUtils::natural_string_sort(test, true);
		for (size_t i = 0; TEST[i]; i++)
		{
			ASSERT_QSTR(test[i], TEST[i]);
		}
	}
}

//-----------------------------------------------------------------
// RegExp Parser
//-----------------------------------------------------------------

#define TEST_REGEX_U32(X,Y,Z,...) do \
{ \
	const QRegExp test(QLatin1String((X))); \
	ASSERT_GE(test.indexIn(QLatin1String((Y))), 0); \
	quint32 result[(Z)]; \
	ASSERT_TRUE(MUtils::regexp_parse_uint32(test, result, (Z))); \
	const quint32 expected[] = { __VA_ARGS__ }; \
	for(size_t i = 0; i < (Z); i++) \
	{ \
		ASSERT_EQ(result[i], expected[i]); \
	} \
} \
while(0)

TEST_F(GlobalTest, ParseRegExp)
{
	TEST_REGEX_U32("(\\d+)", "42", 1, 42);
	TEST_REGEX_U32("(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)", "4 8 15 16 23 42", 6, 4, 8, 15, 16, 23, 42);
	TEST_REGEX_U32("x264\\s+(\\d+)\\.(\\d+)\\.(\\d+)\\s+\\w+", "x264 0.148.2744 b97ae06", 3, 0, 148, 2744);
	TEST_REGEX_U32("HEVC\\s+encoder\\s+version\\s+(\\d+)\\.(\\d+)\\+(\\d+)-\\w+", "HEVC encoder version 2.1+70-78e1e1354a25", 3, 2, 1, 70);
}

#undef TEST_REGEX_U32
