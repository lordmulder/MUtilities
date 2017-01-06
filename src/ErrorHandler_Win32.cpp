///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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
static void my_invalid_param_handler(const wchar_t* exp, const wchar_t* fun, const wchar_t* fil, unsigned int, uintptr_t)
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
static LONG WINAPI my_exception_handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	MUtils::OS::fatal_exit(L"Unhandeled exception handler invoked, application will exit!");
	return LONG_MAX;
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
	SetDllDirectoryW(L""); /*don'tload DLL from "current" directory*/
	
	static const int signal_num[6] = { SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM };

	for(size_t i = 0; i < 6; i++)
	{
		signal(signal_num[i], my_signal_handler);
	}

}

///////////////////////////////////////////////////////////////////////////////
