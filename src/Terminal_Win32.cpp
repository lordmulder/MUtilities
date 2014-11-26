///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#include <MUtils/Terminal.h>

//Internal
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include "CriticalSection_Win32.h"

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

//Qt
#include <QFile>
#include <QStringList>

//CRT
#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <ctime>

//Lock
static MUtils::Internal::CriticalSection g_terminal_lock;

#ifdef _MSC_VER
#define stricmp(X,Y) _stricmp((X),(Y))
#endif

///////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static void make_timestamp(char *timestamp, const size_t &buffsize)
{
	time_t rawtime;
	struct tm timeinfo;

	time(&rawtime);
	if(localtime_s(&timeinfo, &rawtime) == 0)
	{
		strftime(timestamp, 32, "%H:%M:%S", &timeinfo);
	}
	else
	{
		timestamp[0] = '\0';
	}
}

static const char *clean_str(char *str)
{
	//Clean
	char *ptr = &str[0];
	while((*ptr) && isspace(*ptr))
	{
		*(ptr++) = 0x20;
	}

	//Trim left
	while((*str) && isspace(*str))
	{
		str++;
	}

	//Trim right
	size_t pos = strlen(str);
	while(pos > 0)
	{
		if(isspace(str[--pos]))
		{
			str[pos] = '\0';
			continue;
		}
		break;
	}

	return str;
}

///////////////////////////////////////////////////////////////////////////////
// TERMINAL SETUP
///////////////////////////////////////////////////////////////////////////////

static bool g_terminal_attached = false;
static QScopedPointer<std::filebuf> g_filebufStdOut;
static QScopedPointer<std::filebuf> g_filebufStdErr;
static QScopedPointer<QFile> g_log_file;

void MUtils::Terminal::setup(int &argc, char **argv, const bool forceEnabled)
{
	MUtils::Internal::CSLocker lock(g_terminal_lock);
	bool enableConsole = (MUTILS_DEBUG) || forceEnabled;

	if(_environ)
	{
		wchar_t *logfile = NULL; size_t logfile_len = 0;
		if(!_wdupenv_s(&logfile, &logfile_len, L"MUTILS_LOGFILE"))
		{
			if(logfile && (logfile_len > 0))
			{
				g_log_file.reset(new QFile(MUTILS_QSTR(logfile)));
				if(g_log_file->open(QIODevice::WriteOnly))
				{
					static const char MARKER[3] = { char(0xEF), char(0xBB), char(0xBF) };
					g_log_file->write(MARKER, 3);
				}
				free(logfile);
			}
		}
	}

	if(!MUTILS_DEBUG)
	{
		for(int i = 0; i < argc; i++)
		{
			if(!stricmp(argv[i], "--console"))
			{
				enableConsole = true;
			}
			else if(!stricmp(argv[i], "--no-console"))
			{
				enableConsole = false;
			}
		}
	}

	if(enableConsole)
	{
		if(!g_terminal_attached)
		{
			if(AllocConsole() != FALSE)
			{
				SetConsoleCtrlHandler(NULL, TRUE);
				SetConsoleTitle(L"LameXP - Audio Encoder Front-End | Debug Console");
				SetConsoleOutputCP(CP_UTF8);
				g_terminal_attached = true;
			}
		}
		
		if(g_terminal_attached)
		{
			//-------------------------------------------------------------------
			//See: http://support.microsoft.com/default.aspx?scid=kb;en-us;105305
			//-------------------------------------------------------------------
			const int flags = _O_WRONLY | _O_U8TEXT;
			const int hCrtStdOut = _open_osfhandle((intptr_t) GetStdHandle(STD_OUTPUT_HANDLE), flags);
			const int hCrtStdErr = _open_osfhandle((intptr_t) GetStdHandle(STD_ERROR_HANDLE ), flags);
			FILE *const hfStdOut = (hCrtStdOut >= 0) ? _fdopen(hCrtStdOut, "wb") : NULL;
			FILE *const hfStdErr = (hCrtStdErr >= 0) ? _fdopen(hCrtStdErr, "wb") : NULL;
			if(hfStdOut)
			{
				*stdout = *hfStdOut;
				g_filebufStdOut.reset(new std::filebuf(hfStdOut));
				std::cout.rdbuf(g_filebufStdOut.data());
			}
			if(hfStdErr)
			{
				*stderr = *hfStdErr;
				g_filebufStdErr.reset(new std::filebuf(hfStdErr));
				std::cerr.rdbuf(g_filebufStdErr.data());
			}
		}

		HWND hwndConsole = GetConsoleWindow();
		if((hwndConsole != NULL) && (hwndConsole != INVALID_HANDLE_VALUE))
		{
			HMENU hMenu = GetSystemMenu(hwndConsole, 0);
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
			RemoveMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);

			SetWindowPos (hwndConsole, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
			SetWindowLong(hwndConsole, GWL_STYLE, GetWindowLong(hwndConsole, GWL_STYLE) & (~WS_MAXIMIZEBOX) & (~WS_MINIMIZEBOX));
			SetWindowPos (hwndConsole, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// TERMINAL COLORS
///////////////////////////////////////////////////////////////////////////////

//Colors
static const WORD COLOR_RED    = FOREGROUND_RED | FOREGROUND_INTENSITY;
static const WORD COLOR_YELLOW = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
static const WORD COLOR_WHITE  = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
static const WORD COLOR_DEFAULT= FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;

static void set_terminal_color(FILE* file, const WORD &attributes)
{
	const HANDLE hConsole = (HANDLE)(_get_osfhandle(_fileno(file)));
	if((hConsole != NULL) && (hConsole != INVALID_HANDLE_VALUE))
	{
		SetConsoleTextAttribute(hConsole, attributes);
	}
}

///////////////////////////////////////////////////////////////////////////////
// WRITE TO TERMINAL
///////////////////////////////////////////////////////////////////////////////

static const char *const FORMAT = "[%c][%s] %s\r\n";
static const char *const GURU_MEDITATION = "\n\nGURU MEDITATION !!!\n\n";

static void write_logfile_helper(QFile *const file, const int &type, const char *const message)
{
	static char input[1024], output[1024], timestamp[32];
	make_timestamp(timestamp, 32);
	strncpy_s(input, 1024, message, _TRUNCATE);
	const char *const temp = clean_str(input);

	switch(type)
	{
	case QtCriticalMsg:
	case QtFatalMsg:
		_snprintf_s(output, 1024, FORMAT, 'C', timestamp, temp);
		break;
	case QtWarningMsg:
		_snprintf_s(output, 1024, FORMAT, 'W', timestamp, temp);
		break;
	default:
		_snprintf_s(output, 1024, FORMAT, 'I', timestamp, temp);
		break;
	}

	file->write(output);
	file->flush();
}

static void write_debugger_helper(const int &type, const char *const message)
{
	static char input[1024], output[1024], timestamp[32];
	make_timestamp(timestamp, 32);
	strncpy_s(input, 1024, message, _TRUNCATE);
	const char *const temp = clean_str(input);

	switch(type)
	{
	case QtCriticalMsg:
	case QtFatalMsg:
		_snprintf_s(output, 1024, FORMAT, 'C', timestamp, temp);
		break;
	case QtWarningMsg:
		_snprintf_s(output, 1024, FORMAT, 'W', timestamp, temp);
		break;
	default:
		_snprintf_s(output, 1024, FORMAT, 'I', timestamp, temp);
		break;
	}

	OutputDebugStringA(output);
}

static void write_terminal_helper(const int &type, const char *const message)
{
	if(_isatty(_fileno(stderr)))
	{
		UINT oldOutputCP = GetConsoleOutputCP();
		if(oldOutputCP != CP_UTF8) SetConsoleOutputCP(CP_UTF8);

		switch(type)
		{
		case QtCriticalMsg:
		case QtFatalMsg:
			set_terminal_color(stderr, COLOR_RED);
			fprintf(stderr, GURU_MEDITATION);
			fprintf(stderr, "%s\n", message);
			fflush(stderr);
			break;
		case QtWarningMsg:
			set_terminal_color(stderr, COLOR_YELLOW);
			fprintf(stderr, "%s\n", message);
			fflush(stderr);
			break;
		default:
			set_terminal_color(stderr, COLOR_WHITE);
			fprintf(stderr, "%s\n", message);
			fflush(stderr);
			break;
		}
	
		set_terminal_color(stderr, COLOR_DEFAULT);
		if(oldOutputCP != CP_UTF8)
		{
			SetConsoleOutputCP(oldOutputCP);
		}
	}
}

void MUtils::Terminal::write(const int &type, const char *const message)
{
	MUtils::Internal::CSLocker lock(g_terminal_lock);

	if(g_terminal_attached)
	{
		write_terminal_helper(type, message);
	}
	else
	{
		write_debugger_helper(type, message);
	}

	if(!g_log_file.isNull())
	{
		write_logfile_helper(g_log_file.data(), type, message);
	}
}
