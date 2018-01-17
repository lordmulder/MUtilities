///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Win32 API
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

//MUtils
#include <MUtils/GUI.h>
#include <MUtils/OSSupport.h>

//Internal
#include "Utils_Win32.h"

//Qt
#include <QIcon>
#include <QApplication>
#include <QWidget>
#include <QReadWriteLock>
#include <Dwmapi.h>

///////////////////////////////////////////////////////////////////////////////
// THEME SUPPORT
///////////////////////////////////////////////////////////////////////////////

static QReadWriteLock g_themes_lock;
static int g_themes_initialized = 0;

typedef int (WINAPI *IsAppThemedFunction)(void);

bool MUtils::GUI::themes_enabled(void)
{
	QReadLocker readLock(&g_themes_lock);

	if(g_themes_initialized != 0)
	{
		return (g_themes_initialized > 0);
	}

	readLock.unlock();
	QWriteLocker writeLock(&g_themes_lock);

	if(g_themes_initialized != 0)
	{
		return (g_themes_initialized > 0);
	}

	const MUtils::OS::Version::os_version_t &osVersion = MUtils::OS::os_version();
	if(osVersion >= MUtils::OS::Version::WINDOWS_WINXP)
	{
		const IsAppThemedFunction isAppThemedPtr = MUtils::Win32Utils::resolve<IsAppThemedFunction>(QLatin1String("UxTheme"), QLatin1String("IsAppThemed"));
		if(isAppThemedPtr)
		{
			g_themes_initialized = isAppThemedPtr() ? 1 : (-1);
			if(g_themes_initialized < 0)
			{
				qWarning("Theme support is disabled for this process!");
			}
		}
	}

	return (g_themes_initialized > 0);
}

///////////////////////////////////////////////////////////////////////////////
// SYSTEM MENU
///////////////////////////////////////////////////////////////////////////////

bool MUtils::GUI::sysmenu_append(const QWidget *win, const unsigned int identifier, const QString &text)
{
	bool ok = false;
	
	if(HMENU hMenu = GetSystemMenu(reinterpret_cast<HWND>(win->winId()), FALSE))
	{
		ok = (AppendMenuW(hMenu, MF_SEPARATOR, 0, 0) == TRUE);
		ok = (AppendMenuW(hMenu, MF_STRING, identifier, MUTILS_WCHR(text)) == TRUE);
	}

	return ok;
}

bool MUtils::GUI::sysmenu_update(const QWidget *win, const unsigned int identifier, const QString &text)
{
	bool ok = false;
	
	if(HMENU hMenu = ::GetSystemMenu(reinterpret_cast<HWND>(win->winId()), FALSE))
	{
		ok = (ModifyMenu(hMenu, identifier, MF_STRING | MF_BYCOMMAND, identifier, MUTILS_WCHR(text)) == TRUE);
	}
	return ok;
}

bool MUtils::GUI::sysmenu_check_msg(void *const message, const unsigned int &identifier)
{
	return (((MSG*)message)->message == WM_SYSCOMMAND) && ((((MSG*)message)->wParam & 0xFFF0) == identifier);
}

///////////////////////////////////////////////////////////////////////////////
// CLOSE BUTTON
///////////////////////////////////////////////////////////////////////////////

bool MUtils::GUI::enable_close_button(const QWidget *win, const bool &bEnable)
{
	bool ok = false;

	if(HMENU hMenu = GetSystemMenu(reinterpret_cast<HWND>(win->winId()), FALSE))
	{
		ok = (EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED)) == TRUE);
	}

	return ok;
}

///////////////////////////////////////////////////////////////////////////////
// BRING WINDOW TO FRONT
///////////////////////////////////////////////////////////////////////////////

static BOOL CALLBACK bring_process_to_front_helper(HWND hwnd, LPARAM lParam)
{
	DWORD processId = *reinterpret_cast<WORD*>(lParam);
	DWORD windowProcessId = NULL;
	GetWindowThreadProcessId(hwnd, &windowProcessId);
	if(windowProcessId == processId)
	{
		SwitchToThisWindow(hwnd, TRUE);
		SetForegroundWindow(hwnd);
		return FALSE;
	}

	return TRUE;
}

bool MUtils::GUI::bring_to_front(const QWidget *window)
{
	bool ret = false;
	
	if(window)
	{
		for(int i = 0; (i < 5) && (!ret); i++)
		{
			ret = (SetForegroundWindow(reinterpret_cast<HWND>(window->winId())) != FALSE);
			SwitchToThisWindow(reinterpret_cast<HWND>(window->winId()), TRUE);
		}
		LockSetForegroundWindow(LSFW_LOCK);
	}

	return ret;
}

bool MUtils::GUI::bring_to_front(const unsigned long pid)
{
	return EnumWindows(bring_process_to_front_helper, reinterpret_cast<LPARAM>(&pid)) == TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// SHEET OF GLASS EFFECT
///////////////////////////////////////////////////////////////////////////////

typedef	HRESULT (__stdcall *DwmIsCompositionEnabledFun)      (BOOL *bEnabled);
typedef HRESULT (__stdcall *DwmExtendFrameIntoClientAreaFun) (HWND hWnd, const MARGINS* pMarInset);
typedef HRESULT (__stdcall *DwmEnableBlurBehindWindowFun)    (HWND hWnd, const DWM_BLURBEHIND* pBlurBehind);

bool MUtils::GUI::sheet_of_glass(QWidget *const window)
{
	const DwmIsCompositionEnabledFun      dwmIsCompositionEnabledFun      = MUtils::Win32Utils::resolve<DwmIsCompositionEnabledFun>     (QLatin1String("dwmapi"), QLatin1String("DwmIsCompositionEnabled")     );
	const DwmExtendFrameIntoClientAreaFun dwmExtendFrameIntoClientAreaFun = MUtils::Win32Utils::resolve<DwmExtendFrameIntoClientAreaFun>(QLatin1String("dwmapi"), QLatin1String("DwmExtendFrameIntoClientArea"));
	const DwmEnableBlurBehindWindowFun    dwmEnableBlurBehindWindowFun    = MUtils::Win32Utils::resolve<DwmEnableBlurBehindWindowFun>   (QLatin1String("dwmapi"), QLatin1String("DwmEnableBlurBehindWindow")   );

	//Required functions available?
	BOOL bCompositionEnabled = FALSE;
	if(dwmIsCompositionEnabledFun && dwmExtendFrameIntoClientAreaFun && dwmEnableBlurBehindWindowFun)
	{
		//Check if composition is currently enabled
		if(HRESULT hr = dwmIsCompositionEnabledFun(&bCompositionEnabled))
		{
			qWarning("DwmIsCompositionEnabled function has failed! (error %d)", hr);
			return false;
		}
	}
	
	//All functions available *and* composition enabled?
	if(!bCompositionEnabled)
	{
		return false;
	}

	//Enable the "sheet of glass" effect on this window
	MARGINS margins = {-1, -1, -1, -1};
	if(HRESULT hr = dwmExtendFrameIntoClientAreaFun(reinterpret_cast<HWND>(window->winId()), &margins))
	{
		qWarning("DwmExtendFrameIntoClientArea function has failed! (error %d)", hr);
		return false;
	}

	//Create and populate the Blur Behind structure
	DWM_BLURBEHIND bb;
	memset(&bb, 0, sizeof(DWM_BLURBEHIND));
	bb.fEnable = TRUE;
	bb.dwFlags = DWM_BB_ENABLE;
	if(HRESULT hr = dwmEnableBlurBehindWindowFun(reinterpret_cast<HWND>(window->winId()), &bb))
	{
		qWarning("DwmEnableBlurBehindWindow function has failed! (error %d)", hr);
		return false;
	}

	//Required for Qt
	window->setAutoFillBackground(false);
	window->setAttribute(Qt::WA_TranslucentBackground);
	window->setAttribute(Qt::WA_NoSystemBackground);

	return true;
}

bool MUtils::GUI::sheet_of_glass_update(QWidget *const window)
{
	const DwmIsCompositionEnabledFun      dwmIsCompositionEnabledFun      = MUtils::Win32Utils::resolve<DwmIsCompositionEnabledFun>     (QLatin1String("dwmapi"), QLatin1String("DwmIsCompositionEnabled")     );
	const DwmExtendFrameIntoClientAreaFun dwmExtendFrameIntoClientAreaFun = MUtils::Win32Utils::resolve<DwmExtendFrameIntoClientAreaFun>(QLatin1String("dwmapi"), QLatin1String("DwmExtendFrameIntoClientArea"));
	const DwmEnableBlurBehindWindowFun    dwmEnableBlurBehindWindowFun    = MUtils::Win32Utils::resolve<DwmEnableBlurBehindWindowFun>   (QLatin1String("dwmapi"), QLatin1String("DwmEnableBlurBehindWindow")   );
	
	//Required functions available?
	BOOL bCompositionEnabled = FALSE;
	if(dwmIsCompositionEnabledFun && dwmExtendFrameIntoClientAreaFun && dwmEnableBlurBehindWindowFun)
	{
		//Check if composition is currently enabled
		if(HRESULT hr = dwmIsCompositionEnabledFun(&bCompositionEnabled))
		{
			qWarning("DwmIsCompositionEnabled function has failed! (error %d)", hr);
			return false;
		}
	}
	
	//All functions available *and* composition enabled?
	if(!bCompositionEnabled)
	{
		return false;
	}

	//Create and populate the Blur Behind structure
	DWM_BLURBEHIND bb;
	memset(&bb, 0, sizeof(DWM_BLURBEHIND));
	bb.fEnable = TRUE;
	bb.dwFlags = DWM_BB_ENABLE;
	if(HRESULT hr = dwmEnableBlurBehindWindowFun(reinterpret_cast<HWND>(window->winId()), &bb))
	{
		qWarning("DwmEnableBlurBehindWindow function has failed! (error %d)", hr);
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// SYSTEM COLORS
///////////////////////////////////////////////////////////////////////////////

QColor MUtils::GUI::system_color(const int &color_id)
{
	int nIndex = -1;

	switch(color_id)
	{
	case SYSCOLOR_TEXT:
		nIndex = COLOR_WINDOWTEXT;		/*Text in windows*/
		break;
	case SYSCOLOR_BACKGROUND:
		nIndex = COLOR_WINDOW;			/*Window background*/
		break;
	case SYSCOLOR_CAPTION:
		nIndex = COLOR_CAPTIONTEXT;		/*Text in caption, size box, and scroll bar arrow box*/
		break;
	default:
		qWarning("Unknown system color id (%d) specified!", color_id);
		nIndex = COLOR_WINDOWTEXT;
	}
	
	const DWORD rgb = GetSysColor(nIndex);
	QColor color(GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
	return color;
}
