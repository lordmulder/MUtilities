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
#include <ctime>
#include <stdarg.h>

#ifdef _MSC_VER
#define stricmp(X,Y) _stricmp((X),(Y))
#endif

#define VALID_HANLDE(X) (((X) != NULL) && ((X) != INVALID_HANDLE_VALUE))

///////////////////////////////////////////////////////////////////////////////
// TERMINAL VARIABLES
///////////////////////////////////////////////////////////////////////////////

//Critical section
static MUtils::Internal::CriticalSection g_terminal_lock;

//Is terminal attached?
static volatile bool g_terminal_attached = false;

//Terminal output handle
static HANDLE g_hConOut = NULL;

//Terminal output buffer
static const size_t BUFF_SIZE = 8192;
static char g_conOutBuff[BUFF_SIZE] = { '\0' };

//The log file
static QScopedPointer<QFile> g_terminal_log_file;

///////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static inline void make_timestamp(char *timestamp, const size_t &buffsize)
{
	time_t rawtime;
	struct tm timeinfo;

	time(&rawtime);
	if(localtime_s(&timeinfo, &rawtime) == 0)
	{
		strftime(timestamp, buffsize, "%H:%M:%S", &timeinfo);
	}
	else
	{
		timestamp[0] = '\0';
	}
}

static inline bool null_or_whitespace(const char *const str)
{
	if (str)
	{
		size_t pos = 0;
		while (str[pos])
		{
			if (!(isspace(str[pos]) || iscntrl(str[pos])))
			{
				return false;
			}
			pos++;
		}
	}
	return true;
}

static inline size_t clean_str(char *const str)
{
	bool space_flag = true;
	size_t src = 0, out = 0;

	while (str[src])
	{
		if (isspace(str[src]) || iscntrl(str[src])) /*replace any space-sequence with a single space character*/
		{
			src++;
			if (!space_flag)
			{
				space_flag = true;
				str[out++] = 0x20;
			}
		}
		else /*otherwise we'll just copy over the current character*/
		{
			if (src != out)
			{
				str[out] = str[src];
			}
			space_flag = false;
			out++; src++;
		}
	}

	if (space_flag && (out > 0)) /*trim trailing space, if any*/
	{
		out--;
	}

	str[out] = NULL;
	return out;
}

///////////////////////////////////////////////////////////////////////////////
// TERMINAL SETUP
///////////////////////////////////////////////////////////////////////////////

static void terminal_shutdown(void)
{
	MUtils::Internal::CSLocker lock(g_terminal_lock);

	if (g_terminal_attached)
	{
		if (VALID_HANLDE(g_hConOut))
		{
			CloseHandle(g_hConOut);
			g_hConOut = NULL;
		}
		g_terminal_attached = false;
	}
}

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
			g_hConOut = CreateFileA("CONOUT$", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
			atexit(terminal_shutdown);

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

static void set_terminal_color(const WORD &attributes)
{
	if(VALID_HANLDE(g_hConOut))
	{
		SetConsoleTextAttribute(g_hConOut, attributes);
	}
}

///////////////////////////////////////////////////////////////////////////////
// WRITE TO TERMINAL
///////////////////////////////////////////////////////////////////////////////

static const char *const FORMAT = "[%c][%s] %s\r\n";
static const char *const GURU_MEDITATION = "\n\nGURU MEDITATION !!!\n\n";

static inline int terminal_fprintf(const char *const text, ...)
{
	if (VALID_HANLDE(g_hConOut))
	{
		va_list ap;
		va_start(ap, text);
		const int len = _vsnprintf_s(g_conOutBuff, BUFF_SIZE, _TRUNCATE, text, ap);
		va_end(ap);

		if (len > 0)
		{
			DWORD written;
			WriteFile(g_hConOut, g_conOutBuff, DWORD(len), &written, NULL);
			return len;
		}
	}

	return -1;
}

static void write_to_logfile(QFile *const file, const int &type, const char *const message)
{
	if (null_or_whitespace(message))
	{
		return; /*don't write empty message to log file*/
	}

	int len = -1;

	static char timestamp[32];
	make_timestamp(timestamp, 32);

	switch(type)
	{
	case QtCriticalMsg:
	case QtFatalMsg:
		len = _snprintf_s(g_conOutBuff, BUFF_SIZE, _TRUNCATE, FORMAT, 'C', timestamp, message);
		break;
	case QtWarningMsg:
		len = _snprintf_s(g_conOutBuff, BUFF_SIZE, _TRUNCATE, FORMAT, 'W', timestamp, message);
		break;
	default:
		len = _snprintf_s(g_conOutBuff, BUFF_SIZE, _TRUNCATE, FORMAT, 'I', timestamp, message);
		break;
	}

	if (len > 0)
	{
		if (clean_str(g_conOutBuff) > 0)
		{
			file->write(g_conOutBuff);
			file->flush();
		}
	}
}

static void write_to_debugger(const int &type, const char *const message)
{
	if (null_or_whitespace(message))
	{
		return; /*don't send empty message to debugger*/
	}

	int len = -1;

	static char timestamp[32];
	make_timestamp(timestamp, 32);

	switch(type)
	{
	case QtCriticalMsg:
	case QtFatalMsg:
		len = _snprintf_s(g_conOutBuff, BUFF_SIZE, _TRUNCATE, FORMAT, 'C', timestamp, message);
		break;
	case QtWarningMsg:
		len = _snprintf_s(g_conOutBuff, BUFF_SIZE, _TRUNCATE, FORMAT, 'W', timestamp, message);
		break;
	default:
		len = _snprintf_s(g_conOutBuff, BUFF_SIZE, _TRUNCATE, FORMAT, 'I', timestamp, message);
		break;
	}

	if (len > 0)
	{
		if (clean_str(g_conOutBuff) > 0)
		{
			OutputDebugStringA(g_conOutBuff);
		}
	}
}

static void write_to_terminal(const int &type, const char *const message)
{
	switch(type)
	{
	case QtCriticalMsg:
	case QtFatalMsg:
		set_terminal_color(COLOR_RED);
		terminal_fprintf(GURU_MEDITATION);
		terminal_fprintf("%s\n", message);
		break;
	case QtWarningMsg:
		set_terminal_color(COLOR_YELLOW);
		terminal_fprintf("%s\n", message);
		break;
	default:
		set_terminal_color(COLOR_WHITE);
		terminal_fprintf("%s\n", message);
		break;
	}
}

void MUtils::Terminal::write(const int &type, const char *const message)
{
	MUtils::Internal::CSLocker lock(g_terminal_lock);

	if(g_terminal_attached)
	{
		write_to_terminal(type, message);
	}
	else
	{
		write_to_debugger(type, message);
	}

	if(!g_terminal_log_file.isNull())
	{
		write_to_logfile(g_terminal_log_file.data(), type, message);
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
