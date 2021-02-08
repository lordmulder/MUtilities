///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2021 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <MUtils/IPCChannel.h>
#include <MUtils/Exception.h>

//Internal
#include "3rd_party/adler32/include/adler32.h"

//Qt includes
#include <QRegExp>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QMutex>
#include <QWriteLocker>
#include <QCryptographicHash>
#include <QStringList>
//CRT
#include <cassert>

///////////////////////////////////////////////////////////////////////////////
// UTILITIES
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Internal
	{
		static const quint32 ADLER_SEED = 0x5D90C356;

		template<class T>
		static inline void UPDATE_CHECKSUM(T &data)
		{
			data.checksum = Internal::adler32(ADLER_SEED, &data.payload, sizeof(data.payload));
		}

		template<class T>
		static inline bool VERIFY_CHECKSUM(const T &data)
		{
			return (data.checksum == Internal::adler32(ADLER_SEED, &data.payload, sizeof(data.payload)));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Internal
	{
		static const size_t HDR_LEN = 40;
		static const size_t IPC_SLOTS = 128;

		typedef struct
		{
			quint64 counter;
			quint32 pos_wr;
			quint32 pos_rd;
		}
		ipc_status_data_t;

		typedef struct
		{
			ipc_status_data_t payload;
			quint32           checksum;
		}
		ipc_status_t;

		typedef struct
		{
			char    values[MUtils::IPCChannel::MAX_PARAM_CNT][MUtils::IPCChannel::MAX_PARAM_LEN];
			quint32 count;
		}
		ipc_msg_data_params_t;

		typedef struct
		{
			quint32               command_id;
			quint32               flags;
			ipc_msg_data_params_t params;
			quint64               timestamp;
		}
		ipc_msg_data_t;

		typedef struct
		{
			ipc_msg_data_t payload;
			quint32        checksum;
		}
		ipc_msg_t;

		typedef struct
		{
			char         header[HDR_LEN];
			ipc_status_t status;
			ipc_msg_t    data[IPC_SLOTS];
		}
		ipc_t;
	}
}

///////////////////////////////////////////////////////////////////////////////
// UTILITIES
///////////////////////////////////////////////////////////////////////////////

static QScopedPointer<QRegExp> g_escape_regexp;
static QMutex g_escape_lock;

#define ESCAPE(STR) (QString((STR)).replace(*g_escape_regexp, QLatin1String("_")).toLower())

static QString MAKE_ID(const QString &applicationId, const unsigned int &appVersionNo, const QString &channelId, const QString &itemId)
{
	QMutexLocker locker(&g_escape_lock);

	if(g_escape_regexp.isNull())
	{
		g_escape_regexp.reset(new QRegExp(QLatin1String("[^A-Za-z0-9_\\-]")));
	}

	return QString("com.muldersoft.mutilities.ipc.%1.r%2.%3.%4").arg(ESCAPE(applicationId), QString::number(appVersionNo, 16).toUpper(), ESCAPE(channelId), ESCAPE(itemId));
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE DATA
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	class IPCChannel_Private
	{
		friend class IPCChannel;

	protected:
		QAtomicInt initialized;
		QScopedPointer<QSharedMemory> sharedmem;
		QScopedPointer<QSystemSemaphore> semaphore_rd;
		QScopedPointer<QSystemSemaphore> semaphore_wr;
		QReadWriteLock lock;
	};
}

///////////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR & DESTRUCTOR
///////////////////////////////////////////////////////////////////////////////

MUtils::IPCChannel::IPCChannel(const QString &applicationId, const quint32 &appVersionNo, const QString &channelId)
:
	p(new IPCChannel_Private()),
	m_applicationId(applicationId),
	m_channelId(channelId),
	m_appVersionNo(appVersionNo),
	m_headerStr(QCryptographicHash::hash(MAKE_ID(applicationId, appVersionNo, channelId, "header").toLatin1(), QCryptographicHash::Sha1).toHex())
{
	if(m_headerStr.length() != Internal::HDR_LEN)
	{
		MUTILS_THROW("Invalid header length has been detected!");
	}
}

MUtils::IPCChannel::~IPCChannel(void)
{
	if(MUTILS_BOOLIFY(p->initialized))
	{
		if(p->sharedmem->isAttached())
		{
			p->sharedmem->detach();
		}
	}

	delete p;
}

///////////////////////////////////////////////////////////////////////////////
// INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

int MUtils::IPCChannel::initialize(void)
{
	QWriteLocker writeLock(&p->lock);
	
	if(MUTILS_BOOLIFY(p->initialized))
	{
		return RET_ALREADY_INITIALIZED;
	}

	p->sharedmem.   reset(new QSharedMemory   (MAKE_ID(m_applicationId, m_appVersionNo, m_channelId, "sharedmem"), 0));
	p->semaphore_rd.reset(new QSystemSemaphore(MAKE_ID(m_applicationId, m_appVersionNo, m_channelId, "semaph_rd"), 0));
	p->semaphore_wr.reset(new QSystemSemaphore(MAKE_ID(m_applicationId, m_appVersionNo, m_channelId, "semaph_wr"), 0));

	if(p->semaphore_rd->error() != QSystemSemaphore::NoError)
	{
		const QString errorMessage = p->semaphore_rd->errorString();
		qWarning("Failed to create system smaphore: %s", MUTILS_UTF8(errorMessage));
		return RET_FAILURE;
	}

	if(p->semaphore_wr->error() != QSystemSemaphore::NoError)
	{
		const QString errorMessage = p->semaphore_wr->errorString();
		qWarning("Failed to create system smaphore: %s", MUTILS_UTF8(errorMessage));
		return RET_FAILURE;
	}
	
	if(!p->sharedmem->create(sizeof(Internal::ipc_t)))
	{
		if(p->sharedmem->error() == QSharedMemory::AlreadyExists)
		{
			if(!p->sharedmem->attach())
			{
				const QString errorMessage = p->sharedmem->errorString();
				qWarning("Failed to attach to shared memory: %s", MUTILS_UTF8(errorMessage));
				return RET_FAILURE;
			}
			if(p->sharedmem->error() != QSharedMemory::NoError)
			{
				const QString errorMessage = p->sharedmem->errorString();
				qWarning("Failed to attach to shared memory: %s", MUTILS_UTF8(errorMessage));
				return RET_FAILURE;
			}
			if(p->sharedmem->size() < sizeof(Internal::ipc_t))
			{
				qWarning("Failed to attach to shared memory: Size verification has failed!");
				return RET_FAILURE;
			}
			if(Internal::ipc_t *const ptr = reinterpret_cast<Internal::ipc_t*>(p->sharedmem->data()))
			{
				if(memcmp(&ptr->header[0], m_headerStr.constData(), Internal::HDR_LEN) != 0)
				{
					qWarning("Failed to attach to shared memory: Header verification has failed!");
					return RET_FAILURE;
				}
			}
			else
			{
				const QString errorMessage = p->sharedmem->errorString();
				qWarning("Failed to access shared memory: %s", MUTILS_UTF8(errorMessage));
				return RET_FAILURE;
			}
			p->initialized.ref();
			return RET_SUCCESS_SLAVE;
		}
		else
		{
			const QString errorMessage = p->sharedmem->errorString();
			qWarning("Failed to create shared memory: %s", MUTILS_UTF8(errorMessage));
			return RET_FAILURE;
		}
	}
	
	if(p->sharedmem->error() != QSharedMemory::NoError)
	{
		const QString errorMessage = p->sharedmem->errorString();
		qWarning("Failed to create shared memory: %s", MUTILS_UTF8(errorMessage));
		return RET_FAILURE;
	}

	if(Internal::ipc_t *const ptr = reinterpret_cast<Internal::ipc_t*>(p->sharedmem->data()))
	{
		memset(ptr, 0, sizeof(Internal::ipc_t));
		memcpy(&ptr->header[0], m_headerStr.constData(), Internal::HDR_LEN);
		UPDATE_CHECKSUM(ptr->status);
	}
	else
	{
		const QString errorMessage = p->sharedmem->errorString();
		qWarning("Failed to access shared memory: %s", MUTILS_UTF8(errorMessage));
		return RET_FAILURE;
	}

	if(!p->semaphore_wr->release(Internal::IPC_SLOTS))
	{
		const QString errorMessage = p->semaphore_wr->errorString();
		qWarning("Failed to release system semaphore: %s", MUTILS_UTF8(errorMessage));
		return RET_FAILURE;
	}
	
	//qDebug("IPC KEY #1: %s", MUTILS_UTF8(p->sharedmem->key()));
	//qDebug("IPC KEY #2: %s", MUTILS_UTF8(p->semaphore_rd->key()));
	//qDebug("IPC KEY #3: %s", MUTILS_UTF8(p->semaphore_wr->key()));

	p->initialized.ref();
	return RET_SUCCESS_MASTER;
}

///////////////////////////////////////////////////////////////////////////////
// SEND MESSAGE
///////////////////////////////////////////////////////////////////////////////

bool MUtils::IPCChannel::send(const quint32 &command, const quint32 &flags, const QStringList &params)
{
	bool success = false;
	QReadLocker readLock(&p->lock);

	if(!p->initialized)
	{
		MUTILS_THROW("Shared memory for IPC not initialized yet.");
	}

	if(!p->semaphore_wr->acquire())
	{
		const QString errorMessage = p->semaphore_wr->errorString();
		qWarning("Failed to acquire system semaphore: %s", MUTILS_UTF8(errorMessage));
		return false;
	}

	if(!p->sharedmem->lock())
	{
		const QString errorMessage = p->sharedmem->errorString();
		qWarning("Failed to lock shared memory: %s", MUTILS_UTF8(errorMessage));
		return false;
	}

	if(Internal::ipc_t *const ptr = reinterpret_cast<Internal::ipc_t*>(p->sharedmem->data()))
	{
		if(VERIFY_CHECKSUM(ptr->status))
		{
			Internal::ipc_msg_t ipc_msg;
			memset(&ipc_msg, 0, sizeof(Internal::ipc_msg_t));

			ipc_msg.payload.command_id = command;
			ipc_msg.payload.flags = flags;
			if(!params.isEmpty())
			{
				const quint32 param_count = qMin(MAX_PARAM_CNT, (quint32)params.count());
				for(quint32 i = 0; i < param_count; i++)
				{
					strncpy_s(ipc_msg.payload.params.values[i], MAX_PARAM_LEN, MUTILS_UTF8(params[i].trimmed()), _TRUNCATE);
				}
				ipc_msg.payload.params.count = param_count;
			}
			ipc_msg.payload.timestamp = ptr->status.payload.counter++;
			UPDATE_CHECKSUM(ipc_msg);

			memcpy(&ptr->data[ptr->status.payload.pos_wr], &ipc_msg, sizeof(Internal::ipc_msg_t));
			ptr->status.payload.pos_wr = (ptr->status.payload.pos_wr + 1) % Internal::IPC_SLOTS;
			UPDATE_CHECKSUM(ptr->status);

			success = true;
		}
		else
		{
			qWarning("Corrupted IPC status detected -> skipping!");
		}
	}
	else
	{
		qWarning("Shared memory pointer is NULL -> unable to write data!");
	}

	if(!p->sharedmem->unlock())
	{
		const QString errorMessage = p->sharedmem->errorString();
		qFatal("Failed to unlock shared memory: %s", MUTILS_UTF8(errorMessage));
	}

	if(!p->semaphore_rd->release())
	{
		const QString errorMessage = p->semaphore_rd->errorString();
		qWarning("Failed to release system semaphore: %s", MUTILS_UTF8(errorMessage));
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// READ MESSAGE
///////////////////////////////////////////////////////////////////////////////

bool MUtils::IPCChannel::read(quint32 &command, quint32 &flags, QStringList &params)
{
	bool success = false;
	QReadLocker readLock(&p->lock);
	command = 0;
	params.clear();

	if(!p->initialized)
	{
		MUTILS_THROW("Shared memory for IPC not initialized yet.");
	}

	Internal::ipc_msg_t ipc_msg;
	memset(&ipc_msg, 0, sizeof(Internal::ipc_msg_t));

	if(!p->semaphore_rd->acquire())
	{
		const QString errorMessage = p->semaphore_rd->errorString();
		qWarning("Failed to acquire system semaphore: %s", MUTILS_UTF8(errorMessage));
		return false;
	}

	if(!p->sharedmem->lock())
	{
		const QString errorMessage = p->sharedmem->errorString();
		qWarning("Failed to lock shared memory: %s", MUTILS_UTF8(errorMessage));
		return false;
	}

	if(Internal::ipc_t *const ptr = reinterpret_cast<Internal::ipc_t*>(p->sharedmem->data()))
	{
		if(VERIFY_CHECKSUM(ptr->status))
		{
			memcpy(&ipc_msg, &ptr->data[ptr->status.payload.pos_rd], sizeof(Internal::ipc_msg_t));
			ptr->status.payload.pos_rd = (ptr->status.payload.pos_rd + 1) % Internal::IPC_SLOTS;
			UPDATE_CHECKSUM(ptr->status);

			if(VERIFY_CHECKSUM(ipc_msg) || (ipc_msg.payload.timestamp < ptr->status.payload.counter))
			{
				command = ipc_msg.payload.command_id;
				flags = ipc_msg.payload.flags;
				const quint32 param_count = qMin(ipc_msg.payload.params.count, MAX_PARAM_CNT);
				char temp[MAX_PARAM_LEN];
				for(quint32 i = 0; i < param_count; i++)
				{
					strncpy_s(temp, MAX_PARAM_LEN, ipc_msg.payload.params.values[i], _TRUNCATE);
					params.append(QString::fromUtf8(temp));
				}
				success = true;
			}
			else
			{
				qWarning("Malformed or corrupted IPC message, will be ignored!");
			}
		}
		else
		{
			qWarning("Corrupted IPC status detected -> skipping!");
		}
	}
	else
	{
		qWarning("Shared memory pointer is NULL -> unable to read data!");
	}

	if(!p->sharedmem->unlock())
	{
		const QString errorMessage = p->sharedmem->errorString();
		qFatal("Failed to unlock shared memory: %s", MUTILS_UTF8(errorMessage));
	}

	if(!p->semaphore_wr->release())
	{
		const QString errorMessage = p->semaphore_wr->errorString();
		qWarning("Failed to release system semaphore: %s", MUTILS_UTF8(errorMessage));
	}

	return success;
}
