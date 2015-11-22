///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Utils_Win32.h"

//Win32 API
#ifndef _INC_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif //_INC_WINDOWS

//Qt
#include <QIcon>
#include <QPair>
#include <QReadWriteLock>
#include <QLibrary>
#include <QHash>

///////////////////////////////////////////////////////////////////////////////
// QICON TO HICON
///////////////////////////////////////////////////////////////////////////////

uintptr_t MUtils::Win32Utils::qicon_to_hicon(const QIcon &icon, const int w, const int h)
{
	if(!icon.isNull())
	{
		QPixmap pixmap = icon.pixmap(w, h);
		if(!pixmap.isNull())
		{
			return (uintptr_t) pixmap.toWinHICON();
		}
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// RESOLVE FUNCTION
///////////////////////////////////////////////////////////////////////////////

typedef QHash<QString, uintptr_t> FunctionMap;
typedef QPair<QSharedPointer<QLibrary>, FunctionMap> LibraryItem;

static QReadWriteLock              g_resolve_lock;
static QHash<QString, LibraryItem> g_resolve_libs;

uintptr_t MUtils::Win32Utils::resolve_helper(const QString &libraryName, const QString &functionName)
{
	QReadLocker rdLock(&g_resolve_lock);

	//Fuction already loaded?
	const QString libNameLower = libraryName.toLower();
	if (g_resolve_libs.contains(libNameLower))
	{
		LibraryItem &lib = g_resolve_libs[libNameLower];
		if (lib.second.contains(functionName))
		{
			qWarning("TEST: Function already there!");
			return lib.second[functionName];
		}
	}

	//Accquire write access!
	rdLock.unlock();
	QWriteLocker wrLock(&g_resolve_lock);

	//Load library
	while (!g_resolve_libs.contains(libNameLower))
	{
		qWarning("TEST: Library not there -> going to load now!");
		QSharedPointer<QLibrary> lib(new QLibrary(libNameLower));
		if (!(lib->isLoaded() || lib->load()))
		{
			qWarning("Failed to load library: \"%s\"", MUTILS_UTF8(libNameLower));
			return NULL;
		}
		g_resolve_libs.insert(libNameLower, qMakePair(lib, FunctionMap()));
	}

	//Lookup the function
	LibraryItem &lib = g_resolve_libs[libNameLower];
	while (!lib.second.contains(functionName))
	{
		qWarning("TEST: Function not there -> going to resolve now!");
		void *const ptr = lib.first->resolve(functionName.toLatin1().constData());
		if (!ptr)
		{
			lib.second.insert(functionName, NULL);
			qWarning("Failed to resolve function: \"%s\"", MUTILS_UTF8(functionName));
			return NULL;
		}
		qWarning("TEST: Function resolved to 0x%p", ptr);
		lib.second.insert(functionName, reinterpret_cast<uintptr_t>(ptr));
	}

	//Return function pointer
	return lib.second[functionName];
}
