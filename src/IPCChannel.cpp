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

//MUtils
#include <MUtils/IPCChannel.h>
#include <MUtils/Exception.h>

//Qt includes
#include <QRegExp>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QWriteLocker>

///////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	static const size_t IPC_SLOTS = 128;
	static const size_t MAX_MESSAGE_LEN = 4096;

	typedef struct
	{
		unsigned int command;
		unsigned int reserved_1;
		unsigned int reserved_2;
		char parameter[MAX_MESSAGE_LEN];
	}
	ipc_data_t;

	typedef struct
	{
		unsigned int pos_wr;
		unsigned int pos_rd;
		ipc_data_t data[IPC_SLOTS];
	}
	ipc_t;
}

///////////////////////////////////////////////////////////////////////////////
// UTILITIES
///////////////////////////////////////////////////////////////////////////////

static inline QString ESCAPE(QString str)
{
	return str.replace(QRegExp("[^A-Za-z0-9_]"), "_").toLower();
}

static QString MAKE_ID(const QString &applicationId, const QString &channelId, const QString &itemId)
{
	return QString("ipc://mutilities.muldersoft.com:37402/%1/%2/%3").arg(ESCAPE(applicationId), ESCAPE(channelId), ESCAPE(itemId));
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
		volatile bool initialized;
		QScopedPointer<QSharedMemory> sharedmem;
		QScopedPointer<QSystemSemaphore> semaphore_rd;
		QScopedPointer<QSystemSemaphore> semaphore_wr;
		QReadWriteLock lock;
	};
}

///////////////////////////////////////////////////////////////////////////////
// CONSTRUCTOR & DESTRUCTOR
///////////////////////////////////////////////////////////////////////////////

MUtils::IPCChannel::IPCChannel(const QString &applicationId, const QString &channelId)
:
	p(new IPCChannel_Private()),
	m_applicationId(applicationId),
	m_channelId(channelId)
{
	p->initialized = false;
}

MUtils::IPCChannel::~IPCChannel(void)
{
	if(p->initialized)
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
	
	if(p->initialized)
	{
		return IPC_RET_ALREADY_INITIALIZED;
	}

	p->sharedmem.reset(new QSharedMemory(MAKE_ID(m_applicationId, m_channelId, "sharedmem"), NULL));
	p->semaphore_rd.reset(new QSystemSemaphore(MAKE_ID(m_applicationId, m_channelId, "semaphore_rd"), 0));
	p->semaphore_wr.reset(new QSystemSemaphore(MAKE_ID(m_applicationId, m_channelId, "semaphore_wr"), 0));

	if(p->semaphore_rd->error() != QSystemSemaphore::NoError)
	{
		const QString errorMessage = p->semaphore_rd->errorString();
		qWarning("Failed to create system smaphore: %s", MUTILS_UTF8(errorMessage));
		return IPC_RET_FAILURE;
	}

	if(p->semaphore_wr->error() != QSystemSemaphore::NoError)
	{
		const QString errorMessage = p->semaphore_wr->errorString();
		qWarning("Failed to create system smaphore: %s", MUTILS_UTF8(errorMessage));
		return IPC_RET_FAILURE;
	}
	
	if(!p->sharedmem->create(sizeof(ipc_t)))
	{
		if(p->sharedmem->error() == QSharedMemory::AlreadyExists)
		{
			if(!p->sharedmem->attach())
			{
				const QString errorMessage = p->sharedmem->errorString();
				qWarning("Failed to attach to shared memory: %s", MUTILS_UTF8(errorMessage));
				return IPC_RET_FAILURE;
			}
			if(p->sharedmem->error() != QSharedMemory::NoError)
			{
				const QString errorMessage = p->sharedmem->errorString();
				qWarning("Failed to attach to shared memory: %s", MUTILS_UTF8(errorMessage));
				return IPC_RET_FAILURE;
			}
			p->initialized = true;
			return IPC_RET_SUCCESS_SLAVE;
		}
		else
		{
			const QString errorMessage = p->sharedmem->errorString();
			qWarning("Failed to create shared memory: %s", MUTILS_UTF8(errorMessage));
			return IPC_RET_FAILURE;
		}
	}
	
	if(p->sharedmem->error() != QSharedMemory::NoError)
	{
		const QString errorMessage = p->sharedmem->errorString();
		qWarning("Failed to create shared memory: %s", MUTILS_UTF8(errorMessage));
		return IPC_RET_FAILURE;
	}

	if(void *const data = p->sharedmem->data())
	{
		memset(data, 0, sizeof(ipc_t));
	}

	if(!p->semaphore_wr->release(IPC_SLOTS))
	{
		const QString errorMessage = p->semaphore_wr->errorString();
		qWarning("Failed to release system semaphore: %s", MUTILS_UTF8(errorMessage));
		return IPC_RET_FAILURE;
	}
	
	p->initialized = true;
	return IPC_RET_SUCCESS_MASTER;
}

///////////////////////////////////////////////////////////////////////////////
// SEND MESSAGE
///////////////////////////////////////////////////////////////////////////////

bool MUtils::IPCChannel::send(const unsigned int &command, const char *const message)
{
	bool success = false;
	QReadLocker readLock(&p->lock);

	if(!p->initialized)
	{
		MUTILS_THROW("Shared memory for IPC not initialized yet.");
	}

	ipc_data_t ipc_data;
	memset(&ipc_data, 0, sizeof(ipc_data_t));
	ipc_data.command = command;
	
	if(message)
	{
		strncpy_s(ipc_data.parameter, MAX_MESSAGE_LEN, message, _TRUNCATE);
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

	if(ipc_t *const ptr = reinterpret_cast<ipc_t*>(p->sharedmem->data()))
	{
		success = true;
		memcpy(&ptr->data[ptr->pos_wr], &ipc_data, sizeof(ipc_data_t));
		ptr->pos_wr = (ptr->pos_wr + 1) % IPC_SLOTS;
	}
	else
	{
		qWarning("Shared memory pointer is NULL -> unable to write data!");
	}

	if(!p->sharedmem->unlock())
	{
		const QString errorMessage = p->sharedmem->errorString();
		qWarning("Failed to unlock shared memory: %s", MUTILS_UTF8(errorMessage));
		return false;
	}

	if(!p->semaphore_rd->release())
	{
		const QString errorMessage = p->semaphore_rd->errorString();
		qWarning("Failed to acquire release semaphore: %s", MUTILS_UTF8(errorMessage));
		return false;
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// READ MESSAGE
///////////////////////////////////////////////////////////////////////////////

bool MUtils::IPCChannel::read(unsigned int &command, char *const message, const size_t &buffSize)
{
	bool success = false;
	QReadLocker readLock(&p->lock);
	
	command = 0;
	if(message && (buffSize > 0))
	{
		message[0] = '\0';
	}
	
	if(!p->initialized)
	{
		MUTILS_THROW("Shared memory for IPC not initialized yet.");
	}

	ipc_data_t ipc_data;
	memset(&ipc_data, 0, sizeof(ipc_data_t));

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

	if(ipc_t *const ptr = reinterpret_cast<ipc_t*>(p->sharedmem->data()))
	{
			success = true;
		memcpy(&ipc_data, &ptr->data[ptr->pos_rd], sizeof(ipc_data_t));
		ptr->pos_rd = (ptr->pos_rd + 1) % IPC_SLOTS;

		if(!(ipc_data.reserved_1 || ipc_data.reserved_2))
		{
			command = ipc_data.command;
			strncpy_s(message, buffSize, ipc_data.parameter, _TRUNCATE);
		}
		else
		{
			qWarning("Malformed IPC message, will be ignored");
		}
	}
	else
	{
		qWarning("Shared memory pointer is NULL -> unable to write data!");
	}

	if(!p->sharedmem->unlock())
	{
		const QString errorMessage = p->sharedmem->errorString();
		qWarning("Failed to unlock shared memory: %s", MUTILS_UTF8(errorMessage));
		return false;
	}

	if(!p->semaphore_wr->release())
	{
		const QString errorMessage = p->semaphore_wr->errorString();
		qWarning("Failed to acquire release semaphore: %s", MUTILS_UTF8(errorMessage));
		return false;
	}

	return success;
}
