///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
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

//MUtils
#include <MUtils/ErrorHandler.h>
#include <MUtils/OSSupport.h>

//Win32 API
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

//CRT
#include <csignal>

///////////////////////////////////////////////////////////////////////////////
// CALLBACK FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

// Invalid parameters handler
static void my_invalid_param_handler(const wchar_t* /*exp*/, const wchar_t* /*fun*/, const wchar_t* /*fil*/, unsigned int, uintptr_t)
{
	MUtils::OS::fatal_exit(L"Invalid parameter handler invoked, application will exit!");
}

// Signal handler
static void my_signal_handler(int signal_num)
{
	signal(signal_num, my_signal_handler);
	MUtils::OS::fatal_exit(L"Signal handler invoked, application will exit!");
}

// Global exception handler
static LONG WINAPI my_exception_handler(struct _EXCEPTION_POINTERS* /*ExceptionInfo*/)
{
	MUtils::OS::fatal_exit(L"Unhandeled exception handler invoked, application will exit!");
	return LONG_MAX;
}

///////////////////////////////////////////////////////////////////////////////
// DEFAULT DLL DIRECTORIES
///////////////////////////////////////////////////////////////////////////////

//Flags
#define MY_LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x200
#define MY_LOAD_LIBRARY_SEARCH_USER_DIRS       0x400
#define MY_LOAD_LIBRARY_SEARCH_SYSTEM32        0x800

#ifdef MUTILS_STATIC_LIB
#define MY_LOAD_LIBRARY_FLAGS (MY_LOAD_LIBRARY_SEARCH_SYSTEM32 | MY_LOAD_LIBRARY_SEARCH_USER_DIRS)
#else
#define MY_LOAD_LIBRARY_FLAGS (MY_LOAD_LIBRARY_SEARCH_SYSTEM32 | MY_LOAD_LIBRARY_SEARCH_USER_DIRS | MY_LOAD_LIBRARY_SEARCH_APPLICATION_DIR)
#endif

static void set_default_dll_directories(void)
{
	typedef BOOL(__stdcall *MySetDefaultDllDirectories)(const DWORD DirectoryFlags);
	if (const HMODULE kernel32 = GetModuleHandleW(L"kernel32"))
	{
		if (const MySetDefaultDllDirectories pSetDefaultDllDirectories = (MySetDefaultDllDirectories)GetProcAddress(kernel32, "SetDefaultDllDirectories"))
		{
			pSetDefaultDllDirectories(MY_LOAD_LIBRARY_FLAGS);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// SETUP ERROR HANDLERS
///////////////////////////////////////////////////////////////////////////////

void MUtils::ErrorHandler::initialize(void)
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	SetUnhandledExceptionFilter(my_exception_handler);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	_set_invalid_parameter_handler(my_invalid_param_handler);

	/*to prevent DLL pre-loading attacks*/
	set_default_dll_directories();
	SetDllDirectoryW(L"");

	static const int signal_num[6] = { SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM };

	for(size_t i = 0; i < 6; i++)
	{
		signal(signal_num[i], my_signal_handler);
	}
}

///////////////////////////////////////////////////////////////////////////////
