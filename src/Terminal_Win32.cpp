///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2021 LoRd_MuldeR <MuldeR2@GMX.de>
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

//CRT
#include <iostream>
#include <fstream>
#include <ctime>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>

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
static QAtomicInt g_terminal_attached;

//Terminal output buffer
static const size_t BUFF_SIZE = 8192;
static char g_conOutBuff[BUFF_SIZE] = { '\0' };

//Buffer objects
static QScopedPointer<std::filebuf> g_fileBuf_stdout;
static QScopedPointer<std::filebuf> g_fileBuf_stderr;

//The log file
static QScopedPointer<QFile> g_terminal_log_file;

//Terminal icon
static HICON g_terminal_icon = NULL;

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

static inline size_t clean_string(char *const str)
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

static inline void set_hicon(HICON *const ptr, const HICON val)
{
	if (*ptr)
	{
		DestroyIcon(*ptr);
	}
	*ptr = val;
}

///////////////////////////////////////////////////////////////////////////////
// TERMINAL SETUP
///////////////////////////////////////////////////////////////////////////////

static inline std::filebuf *terminal_connect(FILE *const fs, std::ostream &os)
{
	std::filebuf *result = NULL;
	FILE *temp;
	if (freopen_s(&temp, "CONOUT$", "wb", fs) == 0)
	{
		os.rdbuf(result = new std::filebuf(temp));
	}
	return result;
}

static void terminal_shutdown(void)
{
	MUtils::Internal::CSLocker lock(g_terminal_lock);

	if (g_terminal_attached.fetchAndStoreOrdered(0) > 0)
	{
		g_fileBuf_stdout.reset();
		g_fileBuf_stderr.reset();
		FILE *temp[2];
		if(stdout) freopen_s(&temp[0], "NUL", "wb", stdout);
		if(stderr) freopen_s(&temp[1], "NUL", "wb", stderr);
		FreeConsole();
		set_hicon(&g_terminal_icon, NULL);
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
					static const char MARKER[3] = { '\xEF', '\xBB', '\xBF' };
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
		if(!g_terminal_attached.fetchAndStoreOrdered(1))
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
			}
			else
			{
				g_terminal_attached.fetchAndStoreOrdered(0); /*failed*/
			}
		}

		if(MUTILS_BOOLIFY(g_terminal_attached))
		{
			g_fileBuf_stdout.reset(terminal_connect(stdout, std::cout));
			g_fileBuf_stderr.reset(terminal_connect(stderr, std::cerr));

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

static void set_terminal_color(FILE *const fp, const WORD &attributes)
{
	if(_isatty(_fileno(fp)))
	{
		const HANDLE hConsole = (HANDLE)(_get_osfhandle(_fileno(fp)));
		if (VALID_HANLDE(hConsole))
		{
			SetConsoleTextAttribute(hConsole, attributes);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// WRITE TO TERMINAL
///////////////////////////////////////////////////////////////////////////////

static const char *const FORMAT = "[%c][%s] %s\r\n";
static const char *const GURU_MEDITATION = "\n\nGURU MEDITATION !!!\n\n";

static void write_to_logfile(QFile *const file, const int &type, const char *const message)
{
	int len = -1;

	if (null_or_whitespace(message))
	{
		return; /*don't write empty message to log file*/
	}

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
		if (clean_string(g_conOutBuff) > 0)
		{
			file->write(g_conOutBuff);
			file->flush();
		}
	}
}

static void write_to_debugger(const int &type, const char *const message)
{
	int len = -1;

	if (null_or_whitespace(message))
	{
		return; /*don't send empty message to debugger*/
	}

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
		if (clean_string(g_conOutBuff) > 0)
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
		set_terminal_color(stderr, COLOR_RED);
		fprintf(stderr, GURU_MEDITATION);
		fprintf(stderr, "%s\n", message);
		break;
	case QtWarningMsg:
		set_terminal_color(stderr, COLOR_YELLOW);
		fprintf(stderr, "%s\n", message);
		break;
	default:
		set_terminal_color(stderr, COLOR_WHITE);
		fprintf(stderr, "%s\n", message);
		break;
	}

	fflush(stderr);
}

void MUtils::Terminal::write(const int &type, const char *const message)
{
	MUtils::Internal::CSLocker lock(g_terminal_lock);

	if(MUTILS_BOOLIFY(g_terminal_attached))
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

	if(MUTILS_BOOLIFY(g_terminal_attached) && (!(icon.isNull() || MUtils::OS::running_on_wine())))
	{
		if(const HICON hIcon = (HICON) MUtils::Win32Utils::qicon_to_hicon(&icon, 16, 16))
		{
			typedef BOOL(__stdcall *SetConsoleIconFun)(HICON);
			bool success = false;
			if (const SetConsoleIconFun pSetConsoleIconFun = MUtils::Win32Utils::resolve<SetConsoleIconFun>(QLatin1String("kernel32"), QLatin1String("SetConsoleIcon")))
			{
				const DWORD before = GetLastError();
				if (pSetConsoleIconFun(hIcon))
				{
					success = true;
				}
				else
				{
					const DWORD error = GetLastError();
					qWarning("SetConsoleIcon() has failed! [Error: 0x%08X]", error);
				}
			}
			if (!success)
			{
				const HWND hwndConsole = GetConsoleWindow();
				if ((hwndConsole != NULL) && (hwndConsole != INVALID_HANDLE_VALUE))
				{
					SendMessage(hwndConsole, WM_SETICON, ICON_SMALL, LPARAM(hIcon));
					success = true;
				}
			}
			if (success)
			{
				set_hicon(&g_terminal_icon, hIcon);
			}
		}
	}
}
