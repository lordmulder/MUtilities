///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2021 LoRd_MuldeR <MuldeR2@GMX.de>
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

#pragma once

#include <stdexcept>

#define MUTILS_PRINT_ERROR(FORMAT, ...) do \
{ \
	fflush(stdout); \
	fprintf(stderr, (FORMAT), __VA_ARGS__); \
	fflush(stderr); \
} \
while(0)

#define MUTILS_EXCEPTION_HANDLER(COMMAND) do \
{ \
	try \
	{ \
		do { COMMAND; } while(0); \
	} \
	catch(const std::exception &error) \
	{ \
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nException error:\n%s\n", error.what()); \
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!"); \
	} \
	catch(...) \
	{ \
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nUnknown exception error!\n"); \
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!"); \
	} \
} \
while(0)

#define MUTILS_THROW(MESSAGE) do \
{ \
	throw std::runtime_error((MESSAGE)); \
} \
while(0)

#define MUTILS_THROW_FMT(MESSAGE, ...) do \
{ \
	char _message[256]; \
	_snprintf_s(_message, 256, _TRUNCATE, (MESSAGE), __VA_ARGS__); \
	throw std::runtime_error(_message); \
} \
while(0)
