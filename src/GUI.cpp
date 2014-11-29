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

#include <MUtils/GUI.h>

//Qt
#include <QApplication>
#include <QWidget>

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
// BROADCAST
///////////////////////////////////////////////////////////////////////////////

void MUtils::GUI::force_quit(void)
{
	qApp->closeAllWindows();
	qApp->quit();
}

///////////////////////////////////////////////////////////////////////////////
