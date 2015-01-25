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

#pragma once

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QtGlobal>

namespace MUtils
{
	class MUTILS_API IPCChannel_Private;

	class MUTILS_API IPCChannel
	{
	public:
		static const size_t MAX_MESSAGE_LEN = 4096;

		typedef enum
		{
			RET_SUCCESS_MASTER = 0,
			RET_SUCCESS_SLAVE = 1,
			RET_ALREADY_INITIALIZED = 2,
			RET_FAILURE = 3
		}
		ipc_result_t;

		IPCChannel(const QString &applicationId, const quint32 &versionNo, const QString &channelId);
		~IPCChannel(void);

		int initialize(void);

		bool send(const quint32 &command, const quint32 &flags, const char *const message);
		bool read(quint32 &command, quint32 &flags, char *const message, const size_t &buffSize);

	private:
		IPCChannel(const IPCChannel&) : p(NULL), m_appVersionNo(-1) { throw "Constructor is disabled!"; }
		IPCChannel &operator=(const IPCChannel&) { throw "Assignment operator is disabled!"; return *this; }

		const QString m_applicationId;
		const QString m_channelId;
		const unsigned int m_appVersionNo;
		const QByteArray m_headerStr;

		IPCChannel_Private *const p;
	};
}
