///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2026 LoRd_MuldeR <MuldeR2@GMX.de>
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

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QColor>

//Forward Declaration
class QIcon;
class QWidget;

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace GUI
	{
		typedef enum
		{
			USER_EVENT = 1000,           /*QEvent::User*/
			USER_EVENT_QUERYENDSESSION = USER_EVENT + 666,
			USER_EVENT_ENDSESSION      = USER_EVENT + 667
		}
		user_events_t;
		
		typedef enum
		{
			SYSCOLOR_TEXT       = 1,
			SYSCOLOR_BACKGROUND = 2,
			SYSCOLOR_CAPTION    = 3
		}
		system_color_t;

		//Broadcast message
		MUTILS_API bool broadcast(int eventType, const bool &onlyToVisible);

		//Window icon
		MUTILS_API bool set_window_icon(QWidget *const window, const QIcon &icon, const bool bIsBigIcon);

		//Theme support
		MUTILS_API bool themes_enabled(void);

		//DPI information
		MUTILS_API double dpi_scale(void);
		MUTILS_API bool scale_widget(QWidget *const widget, const bool recenter = true);
		MUTILS_API bool center_widget(QWidget *const widget);

		//System menu
		MUTILS_API bool sysmenu_append(const QWidget *const win, const unsigned int identifier, const QString &text);
		MUTILS_API bool sysmenu_update(const QWidget *const win, const unsigned int identifier, const QString &text);
		MUTILS_API bool sysmenu_check_msg(void *const message, const unsigned int &identifier);

		//Close button
		MUTILS_API bool enable_close_button(const QWidget *const win, const bool &bEnable);

		//Bring to front
		MUTILS_API bool bring_to_front(const QWidget *const window);
		MUTILS_API bool bring_to_front(const unsigned long pid);

		//Sheet of glass
		MUTILS_API bool sheet_of_glass(QWidget *const window);

		//System colors
		MUTILS_API QColor system_color(const int &color_id);

		//Blink window
		MUTILS_API void blink_window(QWidget *const poWindow, const unsigned int &count = 10, const unsigned int &delay = 150);

		//Force quit application
		MUTILS_API void force_quit(void);
	}
}

///////////////////////////////////////////////////////////////////////////////
