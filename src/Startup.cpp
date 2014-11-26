///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <MUtils/Startup.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Terminal.h>
#include <MUtils/ErrorHandler.h>
#include <MUtils/Exception.h>

//Qt
#include <QtGlobal>

///////////////////////////////////////////////////////////////////////////////
// MESSAGE HANDLER
///////////////////////////////////////////////////////////////////////////////

static void qt_message_handler(QtMsgType type, const char *msg)
{
	if((!msg) || (!(msg[0])))
	{
		return;
	}

	MUtils::Terminal::write(type, msg);

	if((type == QtCriticalMsg) || (type == QtFatalMsg))
	{
		MUtils::OS::fatal_exit(MUTILS_WCHR(QString::fromUtf8(msg)));
	}
}

///////////////////////////////////////////////////////////////////////////////
// STARTUP FUNCTION
///////////////////////////////////////////////////////////////////////////////

static int startup_main(int &argc, char **argv, MUtils::Startup::main_function_t *const entry_point)
{
	qInstallMsgHandler(qt_message_handler);
	MUtils::Terminal::setup(argc, argv, MUTILS_DEBUG);
	return entry_point(argc, argv);
}

static int startup_helper(int &argc, char **argv, MUtils::Startup::main_function_t *const entry_point)
{
	int iResult = -1;
	try
	{
		iResult = startup_main(argc, argv, entry_point);
	}
	catch(const std::exception &error)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nException error:\n%s\n", error.what());
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}
	catch(...)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nUnknown exception error!\n");
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}
	return iResult;
}

int MUtils::Startup::startup(int &argc, char **argv, main_function_t *const entry_point)
{
	int iResult = -1;
#if(MUTILS_DEBUG)
	iResult = startup_main(argc, argv, entry_point);
#else //MUTILS_DEBUG
#ifdef _MSC_VER
	__try
	{
		MUtils::ErrorHandler::initialize();
		iResult = startup_helper(argc, argv, entry_point);
	}
	__except(1)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nUnhandeled structured exception error!\n");
		MUtils::OS::fatal_exit(L"Unhandeled structured exception error, application will exit!");
	}
#else //_MSCVER
	MUtils::ErrorHandler::initialize();
	iResult = startup_helper(argc, argv, entry_point);
#endif //_MSCVER
#endif //MUTILS_DEBUG
	return iResult;
}

///////////////////////////////////////////////////////////////////////////////
