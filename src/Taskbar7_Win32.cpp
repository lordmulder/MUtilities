///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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

//MUtils
#include <MUtils/Taskbar7.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Exception.h>

//Internal
#include "Utils_Win32.h"

//Qt
#include <QWidget>
#include <QIcon>
#if QT_VERSION > QT_VERSION_CHECK(5,0,0)
#include <QtWinExtras>
#endif

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <ShObjIdl.h>

///////////////////////////////////////////////////////////////////////////////
// UNTILITIES
///////////////////////////////////////////////////////////////////////////////

#define INITIALIZE_TASKBAR() do \
{ \
	if(!p->supported) \
	{ \
		return false; \
	} \
	if(!(p->initialized || initialize())) \
	{ \
		qWarning("Taskbar initialization failed!"); \
		return false; \
	} \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// PRIVATE DATA
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	class Taskbar7_Private
	{
		friend class Taskbar7;

	protected:
		Taskbar7_Private(void)
		{
			taskbarList = NULL;
			supported   = false;
			initialized = false;
		}

		ITaskbarList3 *taskbarList;
		volatile bool supported;
		volatile bool initialized;
	};
}

///////////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR & DESTRUCTOR
///////////////////////////////////////////////////////////////////////////////

MUtils::Taskbar7::Taskbar7(QWidget *const window)
:
	p(new Taskbar7_Private()),
	m_window(window)
{
	if(!m_window)
	{
		MUTILS_THROW("Taskbar7: Window pointer must not be NULL!");
	}
	if(!(p->supported = (OS::os_version() >= OS::Version::WINDOWS_WIN70)))
	{
		qWarning("Taskbar7: Taskbar progress not supported on this platform.");
	}
}

MUtils::Taskbar7::~Taskbar7(void)
{
	if(p->taskbarList)
	{
		p->taskbarList->Release();
		p->taskbarList = NULL;
	}

	delete p;
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC INTERFACE
///////////////////////////////////////////////////////////////////////////////

bool MUtils::Taskbar7::setTaskbarState(const taskbarState_t &state)
{
	INITIALIZE_TASKBAR();
	HRESULT result = HRESULT(-1);

	switch(state)
	{
	case TASKBAR_STATE_NONE:
		result = p->taskbarList->SetProgressState(reinterpret_cast<HWND>(m_window->winId()), TBPF_NOPROGRESS);
		break;
	case TASKBAR_STATE_NORMAL:
		result = p->taskbarList->SetProgressState(reinterpret_cast<HWND>(m_window->winId()), TBPF_NORMAL);
		break;
	case TASKBAR_STATE_INTERMEDIATE:
		result = p->taskbarList->SetProgressState(reinterpret_cast<HWND>(m_window->winId()), TBPF_INDETERMINATE);
		break;
	case TASKBAR_STATE_PAUSED:
		result = p->taskbarList->SetProgressState(reinterpret_cast<HWND>(m_window->winId()), TBPF_ERROR);
		break;
	case TASKBAR_STATE_ERROR:
		result = p->taskbarList->SetProgressState(reinterpret_cast<HWND>(m_window->winId()), TBPF_PAUSED);
		break;
	default:
		MUTILS_THROW("Taskbar7: Invalid taskbar state specified!");
	}

	return SUCCEEDED(result);
}

bool MUtils::Taskbar7::setTaskbarProgress(const quint64 &currentValue, const quint64 &maximumValue)
{
	INITIALIZE_TASKBAR();
	const HRESULT result = p->taskbarList->SetProgressValue(reinterpret_cast<HWND>(m_window->winId()), currentValue, maximumValue);
	return SUCCEEDED(result);
}

bool MUtils::Taskbar7::setOverlayIcon(const QIcon *const icon, const QString &info)
{
	INITIALIZE_TASKBAR();
	HRESULT result = HRESULT(-1);
	if(icon)
	{
		if(const HICON hIcon = (HICON)MUtils::Win32Utils::qicon_to_hicon(icon, 16, 16))
		{
			result = p->taskbarList->SetOverlayIcon(reinterpret_cast<HWND>(m_window->winId()), hIcon, MUTILS_WCHR(info));
			DestroyIcon(hIcon);
		}
	}
	else
	{
		result = p->taskbarList->SetOverlayIcon(reinterpret_cast<HWND>(m_window->winId()), NULL, MUTILS_WCHR(info));
	}
	return SUCCEEDED(result);
}

///////////////////////////////////////////////////////////////////////////////
// INTERNAL
///////////////////////////////////////////////////////////////////////////////

bool MUtils::Taskbar7::initialize(void)
{
	while(!p->taskbarList)
	{
		ITaskbarList3 *ptbl = NULL;
		const HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));
		if(!SUCCEEDED(hr))
		{
			qWarning("ITaskbarList3 could not be created!");
			return false;
		}
		p->taskbarList = ptbl;
	}

	while(!p->initialized)
	{
		bool okay = false;
		for(int i = 0; i < 8; i++)
		{
			if(SUCCEEDED(p->taskbarList->HrInit()))
			{
				okay = true;
				break;
			}
			Sleep(1);
		}
		if(!okay)
		{
			qWarning("ITaskbarList3::HrInit() has failed!");
			return false;
		}
		p->initialized = true;
	}

	return true;
}
