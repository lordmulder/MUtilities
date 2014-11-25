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

//StdLib
#include <stdexcept>

//Qt
#include <QString>
#include <QFile>

///////////////////////////////////////////////////////////////////////////////
// Directory Locker
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Internal
	{
		class DirLockException : public std::runtime_error
		{
		public:
			DirLockException(const char *const message)
			:
				std::runtime_error(message)
			{
			}
		};

		class DirLock
		{
		public:
			DirLock(const QString dirPath)
			:
				m_dirPath(dirPath)
			{
				if(m_dirPath.isEmpty())
				{
					throw DirLockException("Path must not be empty!");
				}
				const QByteArray testData = QByteArray(TEST_DATA);
				bool okay = false;
				for(int i = 0; i < 32; i++)
				{
					m_lockFile.reset(new QFile(QString("%1/~%2.lck").arg(m_dirPath, MUtils::rand_str())));
					if(m_lockFile->open(QIODevice::WriteOnly | QIODevice::Truncate))
					{
						if(m_lockFile->write(testData) >= testData.size())
						{
							okay = true;
							break;
						}
						m_lockFile->remove();
					}
				}
				if(!okay)
				{
					throw DirLockException("Locking has failed!");
				}
			}

			~DirLock(void)
			{
				if(!m_lockFile.isNull())
				{
					m_lockFile->remove();
				}
				for(int i = 0; i < 8; i++)
				{
					if(MUtils::remove_directory(m_dirPath))
					{
						break;
					}
				}
			}

			inline const QString &path(void) const
			{
				return m_dirPath;
			}

		private:
			static const char *const TEST_DATA;
			const QString m_dirPath;
			QScopedPointer<QFile> m_lockFile;
		};

		const char *const DirLock::TEST_DATA = "7QtDxPWotHGQYv1xHyQHFKjTB5u5VYKHE20NMDAgLRYoy16CZN7mEYijbjCJpORnoBbtHCzqyy1a6BLCMMiTUZpLzgjg4qnN505egUBqk3wMhPsYjFpkng9i37mWd1iF";
	}
}
