///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

//Internal
#include <MUtils/Terminal.h>
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include "Utils_Win32.h"
#include "CriticalSection_Win32.h"

//Qt
#include <QFile>
#include <QStringList>
#include <QIcon>
#include <QLibrary>

//CRT
#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <ctime>

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
// TERMINAL VARIABLES
///////////////////////////////////////////////////////////////////////////////

//Critical section
static MUtils::Internal::CriticalSection g_terminal_lock;

//Is terminal attached?
static volatile bool g_terminal_attached = false;

//Terminal replacement streams
static QScopedPointer<std::filebuf> g_terminal_filebuf_out;
static QScopedPointer<std::filebuf> g_terminal_filebuf_err;

//The log file
static QScopedPointer<QFile> g_terminal_log_file;

///////////////////////////////////////////////////////////////////////////////
// TERMINAL UTILS
///////////////////////////////////////////////////////////////////////////////

static inline void terminal_connect(FILE *const fp, std::ostream &os, QScopedPointer<std::filebuf> &buff)
{
	FILE *temp = NULL;
	if (freopen_s(&temp, "CONOUT$", "wb", fp) == 0)
	{
		setvbuf(temp, NULL, _IONBF, 0);
		buff.reset(new std::filebuf(temp));
		os.rdbuf(buff.data());
	}
}
static inline void terminal_restore(void)
{
	MUtils::Internal::CSLocker lock(g_terminal_lock);

	if(g_terminal_attached)
	{
		g_terminal_filebuf_out.reset();
		g_terminal_filebuf_err.reset();
		FreeConsole();
		g_terminal_attached = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// TERMINAL SETUP
///////////////////////////////////////////////////////////////////////////////

void MUtils::Terminal::setup(int &argc, char **argv, const char* const appName, const bool forceEnabled)
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
				g_terminal_log_file.reset(new QFile(MUTILS_QSTR(logfile)));
				if(g_terminal_log_file->open(QIODevice::WriteOnly))
				{
					static const char MARKER[3] = { char(0xEF), char(0xBB), char(0xBF) };
					g_terminal_log_file->write(MARKER, 3);
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
				SetConsoleOutputCP(CP_UTF8);
				SetConsoleCtrlHandler(NULL, TRUE);
				if(appName && appName[0])
				{
					char title[128];
					_snprintf_s(title, 128, _TRUNCATE, "%s | Debug Console", appName);
					SetConsoleTitleA(title);
				}
				g_terminal_attached = true;
			}
		}

		if(g_terminal_attached)
		{
			//-------------------------------------------------------------------
			//See: http://support.microsoft.com/default.aspx?scid=kb;en-us;105305
			//-------------------------------------------------------------------
			terminal_connect(stdout, std::cout, g_terminal_filebuf_out);
			terminal_connect(stderr, std::cerr, g_terminal_filebuf_err);
			atexit(terminal_restore);

			const HWND hwndConsole = GetConsoleWindow();
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

	if(!g_terminal_log_file.isNull())
	{
		write_logfile_helper(g_terminal_log_file.data(), type, message);
	}
}

///////////////////////////////////////////////////////////////////////////////
// TERMINAL ICON
///////////////////////////////////////////////////////////////////////////////

void MUtils::Terminal::set_icon(const QIcon &icon)
{
	MUtils::Internal::CSLocker lock(g_terminal_lock);

	if(g_terminal_attached && (!(icon.isNull() || MUtils::OS::running_on_wine())))
	{
		QLibrary kernel32("kernel32.dll");
		if(kernel32.load())
		{
			typedef DWORD (__stdcall *SetConsoleIconFun)(HICON);
			if(SetConsoleIconFun SetConsoleIconPtr = (SetConsoleIconFun) kernel32.resolve("SetConsoleIcon"))
			{
				if(HICON hIcon = qicon_to_hicon(icon, 16, 16))
				{
					SetConsoleIconPtr(hIcon);
					DestroyIcon(hIcon);
				}
			}
		}
	}
}
