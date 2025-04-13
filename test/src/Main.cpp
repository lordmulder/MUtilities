///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <cstdlib>

//===========================================================================
// Message Handler
//===========================================================================

static FILE* g_logFile = NULL;

static void initialize_mutils_log_file(const int argc, const wchar_t *const *const argv)
{
	wchar_t basePath[_MAX_PATH], logFilePath[_MAX_PATH];
	char gtestOutputPath[_MAX_PATH + 16];

	wcscpy_s(basePath, _MAX_PATH, L"MUtilsTest");
	if ((argc > 0) && argv[0] && argv[0][0])
	{
		wcsncpy_s(basePath, _MAX_PATH, argv[0], _TRUNCATE);
	}

	_snwprintf_s(logFilePath, _MAX_PATH, _TRUNCATE, L"%s.log", basePath);
	if (_wfopen_s(&g_logFile, logFilePath, L"w"))
	{
		fprintf(stderr, "Failed to open log file for writing!\n");
		g_logFile = NULL;
	}

	_snprintf_s(gtestOutputPath, _MAX_PATH + 16, _TRUNCATE, "xml:%S.xml", basePath);
	if (gtestOutputPath[0] && (!strchr(gtestOutputPath, '?')))
	{
		::testing::GTEST_FLAG(output) = std::string(gtestOutputPath);
	}
}

static bool get_time_stamp(char *const buffer, const size_t buff_size)
{
	const time_t time_stamp = time(NULL);
	struct tm tm_info;
	if(localtime_s(&tm_info, &time_stamp))
	{
		buffer[0] = L'\0';
		return false;
	}
	const size_t ret = strftime(buffer, buff_size, "%Y-%m-%d %H:%M:%S", &tm_info);
	return (ret > 0) && (ret < buff_size);
}

static void qt_message_handler(QtMsgType type, const char *const msg)
{
#if defined(MUTILS_DEBUG) && MUTILS_DEBUG
	if (msg && msg[0])
	{
		fprintf(stderr, "%s\n", msg);
	}
#endif //MUTILS_DEBUG
	if (g_logFile && (!ferror(g_logFile)))
	{
		char time_buffer[32];
		if (get_time_stamp(time_buffer, 32))
		{
			fprintf(g_logFile, "[%s] %s\n", time_buffer, msg);
		}
		else
		{
			fprintf(g_logFile, "%s\n", msg);
		}
	}
}

//===========================================================================
// Main function
//===========================================================================

int wmain(int argc, wchar_t **argv)
{
	printf("MuldeR's Utilities for Qt v%u.%02u - Regression Test Suite [%s]\n", MUtils::Version::lib_version_major(), MUtils::Version::lib_version_minor(), MUTILS_DEBUG ? "DEBUG" : "RELEASE");
	printf("Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>. Some rights reserved.\n");
	printf("Built on %s at %s with %s for Win-%s.\n\n", MUTILS_UTF8(MUtils::Version::lib_build_date().toString(Qt::ISODate)), MUTILS_UTF8(MUtils::Version::lib_build_time().toString(Qt::ISODate)), MUtils::Version::compiler_version(), MUtils::Version::compiler_arch());

	printf("This library is free software; you can redistribute it and/or\n");
	printf("modify it under the terms of the GNU Lesser General Public\n");
	printf("License as published by the Free Software Foundation; either\n");
	printf("version 2.1 of the License, or (at your option) any later version.\n\n");

	initialize_mutils_log_file(argc, argv);
	qInstallMsgHandler(qt_message_handler);
	::testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
