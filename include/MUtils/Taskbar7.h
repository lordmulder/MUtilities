///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2023 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <MUtils/Global.h>

class QWidget;
class QIcon;

namespace MUtils
{
	class MUTILS_API Taskbar7_Private;

	class MUTILS_API Taskbar7
	{
	public:
		//Taskbar states
		typedef enum
		{
			TASKBAR_STATE_NONE         = 0,
			TASKBAR_STATE_NORMAL       = 1,
			TASKBAR_STATE_INTERMEDIATE = 2,
			TASKBAR_STATE_PAUSED       = 3,
			TASKBAR_STATE_ERROR        = 4
		}
		taskbarState_t;
		
		//Constructor
		Taskbar7(QWidget *const window);
		~Taskbar7(void);
		
		//Public interface
		bool setTaskbarState(const taskbarState_t &state);
		bool setTaskbarProgress(const quint64 &currentValue, const quint64 &maximumValue);
		bool setOverlayIcon(const QIcon *const icon, const QString &info = QString());
		
	private:
		Taskbar7_Private *const p;
		QWidget *const m_window;

		inline bool initialize(void);
	};
}
