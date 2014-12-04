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

#pragma once

//MUtils
#include <MUtils/Global.h>

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

		//Broadcast message
		MUTILS_API bool broadcast(int eventType, const bool &onlyToVisible);

		MUTILS_API bool set_window_icon(QWidget *window, const QIcon &icon, const bool bIsBigIcon);

		//Force quit application
		MUTILS_API void force_quit(void);
	}
}

///////////////////////////////////////////////////////////////////////////////
