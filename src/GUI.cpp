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

//MUtils
#include <MUtils/GUI.h>
#include <MUtils/OSSupport.h>

//Internal
#include "Utils_Win32.h"

//Qt
#include <QIcon>
#include <QApplication>
#include <QWidget>
#include <QMutex>

//Win32 API
#ifndef _INC_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif //_INC_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// BROADCAST
///////////////////////////////////////////////////////////////////////////////

bool MUtils::GUI::broadcast(int eventType, const bool &onlyToVisible)
{
	if(QApplication *app = dynamic_cast<QApplication*>(QApplication::instance()))
	{
		qDebug("Broadcasting %d", eventType);
		
		bool allOk = true;
		QEvent poEvent(static_cast<QEvent::Type>(eventType));
		QWidgetList list = app->topLevelWidgets();

		while(!list.isEmpty())
		{
			QWidget *widget = list.takeFirst();
			if(!onlyToVisible || widget->isVisible())
			{
				if(!app->sendEvent(widget, &poEvent))
				{
					allOk = false;
				}
			}
		}

		qDebug("Broadcast %d done (%s)", eventType, (allOk ? "OK" : "Stopped"));
		return allOk;
	}
	else
	{
		qWarning("Broadcast failed, could not get QApplication instance!");
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// WINDOW ICON
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace GUI
	{
		namespace Internal
		{
			class WindowIconHelper : public QObject
			{
			public:
				WindowIconHelper(QWidget *const parent, const HICON hIcon, const bool &bIsBigIcon)
				:
					QObject(parent),
					m_hIcon(hIcon)
				{
					SendMessage(reinterpret_cast<HWND>(parent->winId()), WM_SETICON, (bIsBigIcon ? ICON_BIG : ICON_SMALL), LPARAM(hIcon));
				}

				virtual ~WindowIconHelper(void)
				{
					if(m_hIcon)
					{
						DestroyIcon(m_hIcon);
					}
				}

			private:
				const HICON m_hIcon;
			};
		}
	}
}

bool MUtils::GUI::set_window_icon(QWidget *const window, const QIcon &icon, const bool bIsBigIcon)
{
	if((!icon.isNull()) && window->winId())
	{
		const int extend = (bIsBigIcon ? 32 : 16);
		if(const HICON hIcon = (HICON) MUtils::Win32Utils::qicon_to_hicon(&icon, extend, extend))
		{
			new Internal::WindowIconHelper(window, hIcon, bIsBigIcon); /*will be free'd using QObject parent mechanism*/
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// BLINK WINDOW
///////////////////////////////////////////////////////////////////////////////

static QMutex g_blinkMutex;

void MUtils::GUI::blink_window(QWidget *const poWindow, const unsigned int &count, const unsigned int &delay)
{
	const double maxOpac = 1.0;
	const double minOpac = 0.3;
	const double delOpac = 0.1;

	if(!g_blinkMutex.tryLock())
	{
		qWarning("Blinking is already in progress, skipping!");
		return;
	}
	
	try
	{
		const int steps = static_cast<int>(ceil(maxOpac - minOpac) / delOpac);
		const int sleep = static_cast<int>(floor(static_cast<double>(delay) / static_cast<double>(steps)));
		const double opacity = poWindow->windowOpacity();
	
		for(unsigned int i = 0; i < count; i++)
		{
			for(double x = maxOpac; x >= minOpac; x -= delOpac)
			{
				poWindow->setWindowOpacity(x);
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				MUtils::OS::sleep_ms(sleep);
			}

			for(double x = minOpac; x <= maxOpac; x += delOpac)
			{
				poWindow->setWindowOpacity(x);
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				MUtils::OS::sleep_ms(sleep);
			}
		}

		poWindow->setWindowOpacity(opacity);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	catch(...)
	{
		qWarning("Exception error while blinking!");
	}

	g_blinkMutex.unlock();
}

///////////////////////////////////////////////////////////////////////////////
// FORCE QUIT
///////////////////////////////////////////////////////////////////////////////

void MUtils::GUI::force_quit(void)
{
	qApp->closeAllWindows();
	qApp->quit();
}

///////////////////////////////////////////////////////////////////////////////
