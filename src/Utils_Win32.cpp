///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <ObjIdl.h>  // required by QWinMime in QtWinExtras
#endif //_INC_WINDOWS

//Qt
#include <QIcon>
#include <QPair>
#include <QReadWriteLock>
#include <QLibrary>
#include <QHash>
#if QT_VERSION > QT_VERSION_CHECK(5,0,0)
#include <QtWinExtras>
#endif

//Qt5 support
#if QT_VERSION > QT_VERSION_CHECK(5,0,0)
#define PIXMAP2HICON(X) QtWin::toHICON((X))
#else
#define PIXMAP2HICON(X) (X).toWinHICON()
#endif

///////////////////////////////////////////////////////////////////////////////
// QICON TO HICON
///////////////////////////////////////////////////////////////////////////////

uintptr_t MUtils::Win32Utils::qicon_to_hicon(const QIcon *const icon, const int w, const int h)
{
	if(!icon->isNull())
	{
		QPixmap pixmap = icon->pixmap(w, h);
		if(!pixmap.isNull())
		{
			return (uintptr_t) PIXMAP2HICON(pixmap);
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

const uintptr_t &MUtils::Win32Utils::resolve_helper(const QString &libraryName, const QString &functionName)
{
	const QString libraryNameFolded = libraryName.toCaseFolded().trimmed();
	const QString functionIdTrimmed = functionName.trimmed();

	//Fuction already loaded?
	QReadLocker rdLock(&g_resolve_lock);
	if (g_resolve_libs.contains(libraryNameFolded))
	{
		LibraryItem &lib = g_resolve_libs[libraryNameFolded];
		if (lib.second.contains(functionIdTrimmed))
		{
			return lib.second[functionIdTrimmed];
		}
	}

	//Accquire write access!
	rdLock.unlock();
	QWriteLocker wrLock(&g_resolve_lock);

	//Load library
	if (!g_resolve_libs.contains(libraryNameFolded))
	{
		QSharedPointer<QLibrary> lib(new QLibrary(libraryNameFolded));
		if (!(lib->isLoaded() || lib->load()))
		{
			qWarning("Failed to load dynamic library: %s", MUTILS_UTF8(libraryNameFolded));
			lib.clear();
		}
		g_resolve_libs.insert(libraryNameFolded, qMakePair(lib, FunctionMap()));
	}

	//Is library available?
	LibraryItem &lib = g_resolve_libs[libraryNameFolded];
	if (lib.first.isNull() || (!lib.first->isLoaded()))
	{
		static const uintptr_t null = NULL;
		return null; /*library unavailable*/
	}

	//Lookup the function
	if (!lib.second.contains(functionIdTrimmed))
	{
		void *const ptr = lib.first->resolve(functionIdTrimmed.toLatin1().constData());
		if (!ptr)
		{
			qWarning("Failed to resolve function: %s::%s", MUTILS_UTF8(libraryNameFolded), MUTILS_UTF8(functionIdTrimmed));
		}
		lib.second.insert(functionIdTrimmed, reinterpret_cast<uintptr_t>(ptr));
	}

	//Return function pointer
	return lib.second[functionIdTrimmed];
}
