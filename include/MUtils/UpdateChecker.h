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

#pragma once

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QThread>
#include <QDate>

class QUrl;

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	class MUTILS_API UpdateCheckerInfo
	{
		friend class UpdateChecker;

	public:
		UpdateCheckerInfo(void);
		void resetInfo(void);
		bool isComplete(void);

		const quint32 &getBuildNo(void)          const { return m_buildNo;          }
		const QDate   &getBuildDate(void)        const { return m_buildDate;        }
		const QString &getDownloadSite(void)     const { return m_downloadSite;     }
		const QString &getDownloadAddress(void)  const { return m_downloadAddress;  }
		const QString &getDownloadFilename(void) const { return m_downloadFilename; }
		const QString &getDownloadFilecode(void) const { return m_downloadFilecode; }
		const QString &getDownloadChecksum(void) const { return m_downloadChecksum; }

	private:
		quint32 m_buildNo;
		QDate m_buildDate;
		QString m_downloadSite;
		QString m_downloadAddress;
		QString m_downloadFilename;
		QString m_downloadFilecode;
		QString m_downloadChecksum;
	};

	// ----------------------------------------------------------------

	class MUTILS_API UpdateChecker : public QThread
	{
		Q_OBJECT

	public:
		enum
		{
			UpdateStatus_NotStartedYet             = 0,
			UpdateStatus_CheckingConnection        = 1,
			UpdateStatus_FetchingUpdates           = 2,
			UpdateStatus_CompletedUpdateAvailable  = 3,
			UpdateStatus_CompletedNoUpdates        = 4,
			UpdateStatus_CompletedNewVersionOlder  = 5,
			UpdateStatus_ErrorNoConnection         = 6,
			UpdateStatus_ErrorConnectionTestFailed = 7,
			UpdateStatus_ErrorFetchUpdateInfo      = 8,
			UpdateStatus_CancelledByUser           = 9
		}
		update_status_t;

		UpdateChecker(const QString &binCurl, const QString &binVerify, const QString &applicationId, const quint32 &installedBuildNo, const bool betaUpdates, const bool testMode = false);
		~UpdateChecker(void);

		const int  getUpdateStatus(void)             const { return m_status; }
		const bool getSuccess(void)                  const { return m_success; };
		const int  getMaximumProgress(void)          const { return m_maxProgress; };
		const int  getCurrentProgress(void)          const { return m_progress; };
		const UpdateCheckerInfo *getUpdateInfo(void) const { return m_updateInfo.data(); }

		bool cancel(void) { return m_cancelled.ref(); }

	public slots:
		void start(Priority = InheritPriority);

	protected:
		void run(void);
		void checkForUpdates(void);
		void testMirrorsList(void);

	signals:
		void statusChanged(const int status);
		void progressChanged(const int progress);
		void messageLogged(const QString &text);

	private:
		const int m_maxProgress;
		QScopedPointer<UpdateCheckerInfo> m_updateInfo;
	
		const bool m_betaUpdates;
		const bool m_testMode;

		const QString m_applicationId;
		const quint32 m_installedBuildNo;

		const QString m_binaryCurl;
		const QString m_binaryVerify;

		const QScopedPointer<const QHash<QString, QString>> m_environment;

		QAtomicInt m_success;
		QAtomicInt m_cancelled;

		int m_status;
		int m_progress;

		inline void setStatus(const int status);
		inline void setProgress(const int progress);
		inline void log(const QString &str1, const QString &str2 = QString(), const QString &str3 = QString(), const QString &str4 = QString());

		bool getUpdateInfo(const QString &url, const QString &outFileVers, const QString &outFileSign);
		bool tryContactHost(const QString &hostname, const int &timeoutMsec);
		bool parseVersionInfo(const QString &file, UpdateCheckerInfo *const updateInfo);

		bool getFile(const QUrl &url, const QString &outFile, const unsigned int maxRedir = 8U);
		bool checkSignature(const QString &file, const QString &signature);
		bool tryUpdateMirror(UpdateCheckerInfo *updateInfo, const QString &url, const bool &quick);

		bool execCurl(const QStringList &args, const QString &workingDir, const int timeout);
		int execProcess(const QString &programFile, const QStringList &args, const QString &workingDir, const int timeout);
	};
}
