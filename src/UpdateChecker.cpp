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

#include <MUtils/Global.h>
#include <MUtils/UpdateChecker.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Exception.h>

#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include <QHash>
#include <QQueue>

#include "Mirrors.h"

///////////////////////////////////////////////////////////////////////////////
// CONSTANTS
///////////////////////////////////////////////////////////////////////////////

static const char *GLOBALHEADER_ID = "!Update";

static const char *MIRROR_URL_POSTFIX[] = 
{
	"update.ver",
	"update_beta.ver",
	NULL
};

static const int MIN_CONNSCORE = 5;
static const int QUICK_MIRRORS = 3;
static const int MAX_CONN_TIMEOUT = 16000;
static const int DOWNLOAD_TIMEOUT = 30000;

static const int VERSION_INFO_EXPIRES_MONTHS = 6;
static char *USER_AGENT_STR = "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:52.0) Gecko/20100101 Firefox/52.0"; /*use something innocuous*/

////////////////////////////////////////////////////////////
// Utility Macros
////////////////////////////////////////////////////////////

#define CHECK_CANCELLED() do \
{ \
	if(MUTILS_BOOLIFY(m_cancelled)) \
	{ \
		m_success.fetchAndStoreOrdered(0); \
		log("", "Update check has been cancelled by user!"); \
		setProgress(m_maxProgress); \
		setStatus(UpdateStatus_CancelledByUser); \
		return; \
	} \
} \
while(0)

#define LOG_MESSAGE_HELPER(X) do \
{ \
	if (!(X).isNull()) \
	{ \
		emit messageLogged((X)); \
	} \
} \
while(0)

#define STRICMP(X,Y) ((X).compare((Y), Qt::CaseInsensitive) == 0)

////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////

static QQueue<QString> buildRandomList(const char *const *values)
{
	QQueue<QString> list;
	while(*values)
	{
		list.insert(MUtils::next_rand_u32(list.size() + 1), QString::fromLatin1(*(values++)));
	}
	return list;
}

static const QHash<QString, QString> *initEnvVars(const QString &binCurl)
{
	QHash<QString, QString> *const environment = new QHash<QString, QString>();
	const QString tempfolder = QDir::toNativeSeparators(MUtils::temp_folder());
	environment->insert(QLatin1String("CURL_HOME"), tempfolder);
	environment->insert(QLatin1String("GNUPGHOME"), tempfolder);
	const QFileInfo curlFile(binCurl);
	environment->insert(QLatin1String("CURL_CA_BUNDLE"), QDir::toNativeSeparators(curlFile.absoluteDir().absoluteFilePath(QString("%1.crt").arg(curlFile.completeBaseName()))));
	return environment;
}

////////////////////////////////////////////////////////////
// Update Info Class
////////////////////////////////////////////////////////////

MUtils::UpdateCheckerInfo::UpdateCheckerInfo(void)
{
	resetInfo();
}
	
void MUtils::UpdateCheckerInfo::resetInfo(void)
{
	m_buildNo = 0;
	m_buildDate.setDate(1900, 1, 1);
	m_downloadSite.clear();
	m_downloadAddress.clear();
	m_downloadFilename.clear();
	m_downloadFilecode.clear();
	m_downloadChecksum.clear();
}

bool MUtils::UpdateCheckerInfo::isComplete(void)
{
	return (this->m_buildNo > 0) &&
		(this->m_buildDate.year() >= 2010) &&
		(!this->m_downloadSite.isEmpty()) &&
		(!this->m_downloadAddress.isEmpty()) &&
		(!this->m_downloadFilename.isEmpty()) &&
		(!this->m_downloadFilecode.isEmpty()) &&
		(!this->m_downloadChecksum.isEmpty());
}

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

MUtils::UpdateChecker::UpdateChecker(const QString &binCurl, const QString &binGnuPG, const QString &binKeys, const QString &applicationId, const quint32 &installedBuildNo, const bool betaUpdates, const bool testMode)
:
	m_updateInfo(new UpdateCheckerInfo()),
	m_binaryCurl(binCurl),
	m_binaryGnuPG(binGnuPG),
	m_binaryKeys(binKeys),
	m_environment(initEnvVars(binCurl)),
	m_applicationId(applicationId),
	m_installedBuildNo(installedBuildNo),
	m_betaUpdates(betaUpdates),
	m_testMode(testMode),
	m_maxProgress(MIN_CONNSCORE + 5)
{
	m_status = UpdateStatus_NotStartedYet;
	m_progress = 0;

	if(m_binaryCurl.isEmpty() || m_binaryGnuPG.isEmpty() || m_binaryKeys.isEmpty())
	{
		MUTILS_THROW("Tools not initialized correctly!");
	}
}

MUtils::UpdateChecker::~UpdateChecker(void)
{
}

////////////////////////////////////////////////////////////
// Public slots
////////////////////////////////////////////////////////////

void MUtils::UpdateChecker::start(Priority priority)
{
	m_success.fetchAndStoreOrdered(0);
	m_cancelled.fetchAndStoreOrdered(0);
	QThread::start(priority);
}

////////////////////////////////////////////////////////////
// Protected functions
////////////////////////////////////////////////////////////

void MUtils::UpdateChecker::run(void)
{
	qDebug("Update checker thread started!");
	MUTILS_EXCEPTION_HANDLER(m_testMode ? testMirrorsList() : checkForUpdates());
	qDebug("Update checker thread completed.");
}

void MUtils::UpdateChecker::checkForUpdates(void)
{
	// ----- Initialization ----- //

	m_updateInfo->resetInfo();
	setProgress(0);

	// ----- Test Internet Connection ----- //

	log("Checking your Internet connection...", "");
	setStatus(UpdateStatus_CheckingConnection);

	const int networkStatus = OS::network_status();
	if(networkStatus == OS::NETWORK_TYPE_NON)
	{
		if (!MUtils::OS::arguments().contains("ignore-network-status"))
		{
			log("Operating system reports that the computer is currently offline !!!");
			setProgress(m_maxProgress);
			setStatus(UpdateStatus_ErrorNoConnection);
			return;
		}
	}
	
	msleep(333);
	setProgress(1);

	// ----- Test Known Hosts Connectivity ----- //

	int connectionScore = 0;
	QQueue<QString> mirrorList = buildRandomList(known_hosts);

	for(int connectionTimeout = 1000; connectionTimeout <= MAX_CONN_TIMEOUT; connectionTimeout *= 2)
	{
		QElapsedTimer elapsedTimer;
		elapsedTimer.start();
		const int globalTimeout = 2 * MIN_CONNSCORE * connectionTimeout, count = mirrorList.count();
		for(int i = 0; i < count; ++i)
		{
			Q_ASSERT(!mirrorList.isEmpty());
			const QString hostName = mirrorList.dequeue();
			if (tryContactHost(hostName, connectionTimeout))
			{
				setProgress(1 + (++connectionScore));
				if (connectionScore >= MIN_CONNSCORE)
				{
					goto endLoop; /*success*/
				}
			}
			else
			{
				mirrorList.enqueue(hostName);
				if(elapsedTimer.hasExpired(globalTimeout))
				{
					break; /*timer expired*/
				}
			}
			CHECK_CANCELLED();
		}
	}

endLoop:
	if(connectionScore < MIN_CONNSCORE)
	{
		log("", "Connectivity test has failed: Internet connection appears to be broken!");
		setProgress(m_maxProgress);
		setStatus(UpdateStatus_ErrorConnectionTestFailed);
		return;
	}

	// ----- Fetch Update Info From Server ----- //

	log("----", "", "Internet connection is operational, checking for updates online...");
	setStatus(UpdateStatus_FetchingUpdates);

	int mirrorCount = 0;
	mirrorList = buildRandomList(update_mirrors);

	while(!mirrorList.isEmpty())
	{
		const QString currentMirror = mirrorList.takeFirst();
		const bool isQuick = (mirrorCount++ < QUICK_MIRRORS);
		if(tryUpdateMirror(m_updateInfo.data(), currentMirror, isQuick))
		{
			m_success.ref(); /*success*/
			break;
		}
		if (isQuick)
		{
			mirrorList.append(currentMirror); /*re-schedule*/
		}
		CHECK_CANCELLED();
		msleep(1);
	}

	msleep(333);
	setProgress(MIN_CONNSCORE + 5);

	// ----- Generate final result ----- //

	if(MUTILS_BOOLIFY(m_success))
	{
		if(m_updateInfo->m_buildNo > m_installedBuildNo)
		{
			setStatus(UpdateStatus_CompletedUpdateAvailable);
		}
		else if(m_updateInfo->m_buildNo == m_installedBuildNo)
		{
			setStatus(UpdateStatus_CompletedNoUpdates);
		}
		else
		{
			setStatus(UpdateStatus_CompletedNewVersionOlder);
		}
	}
	else
	{
		setStatus(UpdateStatus_ErrorFetchUpdateInfo);
	}
}

void MUtils::UpdateChecker::testMirrorsList(void)
{
	QQueue<QString> mirrorList;
	for(int i = 0; update_mirrors[i]; i++)
	{
		mirrorList.enqueue(QString::fromLatin1(update_mirrors[i]));
	}

	// ----- Test update mirrors ----- //

	qDebug("\n[Mirror Sites]");
	log("Testing all known mirror sites...", "", "---");

	UpdateCheckerInfo updateInfo;
	while (!mirrorList.isEmpty())
	{
		const QString currentMirror = mirrorList.dequeue();
		bool success = false;
		qDebug("Testing: %s", MUTILS_L1STR(currentMirror));
		log("", "Testing mirror:", currentMirror, "");
		for (quint8 attempt = 0; attempt < 3; ++attempt)
		{
			updateInfo.resetInfo();
			if (tryUpdateMirror(&updateInfo, currentMirror, (!attempt)))
			{
				success = true;
				break;
			}
		}
		if (!success)
		{
			qWarning("\nUpdate mirror seems to be unavailable:\n%s\n", MUTILS_L1STR(currentMirror));
		}
		log("", "---");
	}

	// ----- Test known hosts ----- //

	mirrorList.clear();
	for (int i = 0; known_hosts[i]; i++)
	{
		mirrorList.enqueue(QString::fromLatin1(known_hosts[i]));
	}

	qDebug("\n[Known Hosts]");
	log("Testing all known hosts...", "", "---");

	while(!mirrorList.isEmpty())
	{
		const QString currentHost = mirrorList.dequeue();
		qDebug("Testing: %s", MUTILS_L1STR(currentHost));
		log(QLatin1String(""), "Testing host:", currentHost, "");
		if (!tryContactHost(currentHost, DOWNLOAD_TIMEOUT))
		{
			qWarning("\nConnectivity test FAILED on the following host:\n%s\n", MUTILS_L1STR(currentHost));
		}
		log("---");
	}
}

////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////

void MUtils::UpdateChecker::setStatus(const int status)
{
	if(m_status != status)
	{
		m_status = status;
		emit statusChanged(status);
	}
}

void MUtils::UpdateChecker::setProgress(const int progress)
{
	const int value = qBound(0, progress, m_maxProgress);
	if(m_progress != value)
	{
		emit progressChanged(m_progress = value);
	}
}

void MUtils::UpdateChecker::log(const QString &str1, const QString &str2, const QString &str3, const QString &str4)
{
	LOG_MESSAGE_HELPER(str1);
	LOG_MESSAGE_HELPER(str2);
	LOG_MESSAGE_HELPER(str3);
	LOG_MESSAGE_HELPER(str4);
}

bool MUtils::UpdateChecker::tryUpdateMirror(UpdateCheckerInfo *updateInfo, const QString &url, const bool &quick)
{
	bool success = false;
	log("", "Trying update mirror:", url, "");

	if (quick)
	{
		setProgress(MIN_CONNSCORE + 1);
		if (!tryContactHost(QUrl(url).host(), (MAX_CONN_TIMEOUT / 8)))
		{
			log("", "Mirror is too slow, skipping!");
			return false;
		}
	}

	const QString randPart = next_rand_str();
	const QString outFileVers = QString("%1/%2.ver").arg(temp_folder(), randPart);
	const QString outFileSign = QString("%1/%2.sig").arg(temp_folder(), randPart);

	if (!getUpdateInfo(url, outFileVers, outFileSign))
	{
		log("", "Oops: Download of update information has failed!");
		goto cleanUp;
	}

	log("Download completed, verifying signature:", "");
	setProgress(MIN_CONNSCORE + 4);
	if (!checkSignature(outFileVers, outFileSign))
	{
		log("", "Bad signature detected, take care !!!");
		goto cleanUp;
	}

	log("", "Signature is valid, parsing update information:", "");
	success = parseVersionInfo(outFileVers, updateInfo);

cleanUp:
	QFile::remove(outFileVers);
	QFile::remove(outFileSign);
	return success;
}

bool MUtils::UpdateChecker::getUpdateInfo(const QString &url, const QString &outFileVers, const QString &outFileSign)
{
	log("Downloading update information:", "");
	setProgress(MIN_CONNSCORE + 2);
	if(getFile(QUrl(QString("%1%2").arg(url, MIRROR_URL_POSTFIX[m_betaUpdates ? 1 : 0])), outFileVers))
	{
		if (!m_cancelled)
		{
			log( "Downloading signature file:", "");
			setProgress(MIN_CONNSCORE + 3);
			if (getFile(QUrl(QString("%1%2.sig2").arg(url, MIRROR_URL_POSTFIX[m_betaUpdates ? 1 : 0])), outFileSign))
			{
				return true; /*completed*/
			}
		}
	}
	return false;
}

//----------------------------------------------------------
// PARSE UPDATE INFO
//----------------------------------------------------------

#define _CHECK_HEADER(ID,NAME) \
	if (STRICMP(name, (NAME))) \
	{ \
		sectionId = (ID); \
		continue; \
	}

#define _PARSE_TEXT(OUT,KEY) \
	if (STRICMP(key, (KEY))) \
	{ \
		(OUT) = val; \
		break; \
	}

#define _PARSE_UINT(OUT,KEY) \
	if (STRICMP(key, (KEY))) \
	{ \
		bool _ok = false; \
		const unsigned int _tmp = val.toUInt(&_ok); \
		if (_ok) \
		{ \
			(OUT) = _tmp; \
			break; \
		} \
	}

#define _PARSE_DATE(OUT,KEY) \
	if (STRICMP(key, (KEY))) \
	{ \
		const QDate _tmp = QDate::fromString(val, Qt::ISODate); \
		if (_tmp.isValid()) \
		{ \
			(OUT) = _tmp; \
			break; \
		} \
	}

bool MUtils::UpdateChecker::parseVersionInfo(const QString &file, UpdateCheckerInfo *const updateInfo)
{
	updateInfo->resetInfo();

	QFile data(file);
	if(!data.open(QIODevice::ReadOnly))
	{
		qWarning("Cannot open update info file for reading!");
		return false;
	}

	QDate updateInfoDate;
	int sectionId = 0;
	QRegExp regex_sec("^\\[(.+)\\]$"), regex_val("^([^=]+)=(.+)$");

	while(!data.atEnd())
	{
		QString line = QString::fromLatin1(data.readLine()).simplified();
		if (regex_sec.indexIn(line) >= 0)
		{
			sectionId = 0; /*unknown section*/
			const QString name = regex_sec.cap(1).trimmed();
			log(QString("Sec: [%1]").arg(name));
			_CHECK_HEADER(1, GLOBALHEADER_ID)
			_CHECK_HEADER(2, m_applicationId)
			continue;
		}
		if (regex_val.indexIn(line) >= 0)
		{
			const QString key = regex_val.cap(1).trimmed();
			const QString val = regex_val.cap(2).trimmed();
			log(QString("Val: \"%1\" = \"%2\"").arg(key, val));
			switch (sectionId)
			{
			case 1:
				_PARSE_DATE(updateInfoDate, "TimestampCreated")
				break;
			case 2:
				_PARSE_UINT(updateInfo->m_buildNo,          "BuildNo")
				_PARSE_DATE(updateInfo->m_buildDate,        "BuildDate")
				_PARSE_TEXT(updateInfo->m_downloadSite,     "DownloadSite")
				_PARSE_TEXT(updateInfo->m_downloadAddress,  "DownloadAddress")
				_PARSE_TEXT(updateInfo->m_downloadFilename, "DownloadFilename")
				_PARSE_TEXT(updateInfo->m_downloadFilecode, "DownloadFilecode")
				_PARSE_TEXT(updateInfo->m_downloadChecksum, "DownloadChecksum")
				break;
			}
		}
	}

	if (!updateInfo->isComplete())
	{
		log("", "WARNING: Update information is incomplete!");
		goto failure;
	}

	if(updateInfoDate.isValid())
	{
		const QDate expiredDate = updateInfoDate.addMonths(VERSION_INFO_EXPIRES_MONTHS);
		if (expiredDate < OS::current_date())
		{
			log("", QString("WARNING: Update information has expired at %1!").arg(expiredDate.toString(Qt::ISODate)));
			goto failure;
		}
	}
	else
	{
		log("", "WARNING: Timestamp is missing from update information header!");
		goto failure;
	}

	log("", "Success: Update information is complete.");
	return true; /*success*/

failure:
	updateInfo->resetInfo();
	return false;
}

//----------------------------------------------------------
// EXTERNAL TOOLS
//----------------------------------------------------------

bool MUtils::UpdateChecker::getFile(const QUrl &url, const QString &outFile, const unsigned int maxRedir)
{
	QFileInfo output(outFile);
	output.setCaching(false);

	if (output.exists())
	{
		QFile::remove(output.canonicalFilePath());
		if (output.exists())
		{
			qWarning("Existing output file could not be found!");
			return false;
		}
	}
	
	QStringList args(QLatin1String("-vsSNqfL"));
	args << "-m" << QString::number(DOWNLOAD_TIMEOUT / 1000);
	args << "--max-redirs" << QString::number(maxRedir);
	args << "-A" << USER_AGENT_STR;
	args << "-e" << QString("%1://%2/;auto").arg(url.scheme(), url.host());
	args << "-o" << output.fileName() << url.toString();

	return execCurl(args, output.absolutePath(), DOWNLOAD_TIMEOUT);
}

bool MUtils::UpdateChecker::tryContactHost(const QString &hostname, const int &timeoutMsec)
{
	log(QString("Connecting to host: %1").arg(hostname), "");

	QStringList args(QLatin1String("-vsSNqkI"));
	args << "-m" << QString::number(qMax(1, timeoutMsec / 1000));
	args << "-A" << USER_AGENT_STR;
	args << "-o" << OS::null_device() << QString("http://%1/").arg(hostname);
	
	return execCurl(args, temp_folder(), timeoutMsec);
}

bool MUtils::UpdateChecker::checkSignature(const QString &file, const QString &signature)
{
	if (QFileInfo(file).absolutePath().compare(QFileInfo(signature).absolutePath(), Qt::CaseInsensitive) != 0)
	{
		qWarning("CheckSignature: File and signature should be in same folder!");
		return false;
	}

	QString keyRingPath(m_binaryKeys);
	bool removeKeyring = false;
	if (QFileInfo(file).absolutePath().compare(QFileInfo(m_binaryKeys).absolutePath(), Qt::CaseInsensitive) != 0)
	{
		keyRingPath = make_temp_file(QFileInfo(file).absolutePath(), "gpg");
		removeKeyring = true;
		if (!QFile::copy(m_binaryKeys, keyRingPath))
		{
			qWarning("CheckSignature: Failed to copy the key-ring file!");
			return false;
		}
	}

	QStringList args;
	args << QStringList() << "--homedir" << ".";
	args << "--keyring" << QFileInfo(keyRingPath).fileName();
	args << QFileInfo(signature).fileName();
	args << QFileInfo(file).fileName();

	const int exitCode = execProcess(m_binaryGnuPG, args, QFileInfo(file).absolutePath(), DOWNLOAD_TIMEOUT);
	if (exitCode != INT_MAX)
	{
		log(QString().sprintf("Exited with code %d", exitCode));
	}

	if (removeKeyring)
	{
		remove_file(keyRingPath);
	}

	return (exitCode == 0); /*completed*/
}

bool MUtils::UpdateChecker::execCurl(const QStringList &args, const QString &workingDir, const int timeout)
{
	const int exitCode = execProcess(m_binaryCurl, args, workingDir, timeout + (timeout / 2));
	if (exitCode != INT_MAX)
	{
		switch (exitCode)
		{
			case  0: log(QLatin1String("DONE: Transfer completed successfully."), "");                     break;
			case  6: log(QLatin1String("ERROR: Remote host could not be resolved!"), "");                  break;
			case  7: log(QLatin1String("ERROR: Connection to remote host could not be established!"), ""); break;
			case 22: log(QLatin1String("ERROR: Requested URL was not found or returned an error!"), "");   break;
			case 28: log(QLatin1String("ERROR: Operation timed out !!!"), "");                             break;
			default: log(QString().sprintf("ERROR: Terminated with unknown code %d", exitCode), "");       break;
		}
	}

	return (exitCode == 0); /*completed*/
}

int MUtils::UpdateChecker::execProcess(const QString &programFile, const QStringList &args, const QString &workingDir, const int timeout)
{
	QProcess process;
	init_process(process, workingDir, true, NULL, m_environment.data());

	QEventLoop loop;
	connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(readyRead()), &loop, SLOT(quit()));

	QTimer timer;
	timer.setSingleShot(true);
	connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	process.start(programFile, args);
	if (!process.waitForStarted())
	{
		log("PROCESS FAILED TO START !!!", "");
		qWarning("WARNING: %s process could not be created!", MUTILS_UTF8(QFileInfo(programFile).fileName()));
		return INT_MAX; /*failed to start*/
	}

	bool bAborted = false;
	timer.start(qMax(timeout, 1500));

	while (process.state() != QProcess::NotRunning)
	{
		loop.exec();
		while (process.canReadLine())
		{
			const QString line = QString::fromLatin1(process.readLine()).simplified();
			if (line.length() > 1)
			{
				log(line);
			}
		}
		const bool bCancelled = MUTILS_BOOLIFY(m_cancelled);
		if (bAborted = (bCancelled || ((!timer.isActive()) && (!process.waitForFinished(125)))))
		{
			log(bCancelled ? "CANCELLED BY USER !!!" : "PROCESS TIMEOUT !!!", "");
			qWarning("WARNING: %s process %s!", MUTILS_UTF8(QFileInfo(programFile).fileName()), bCancelled ? "cancelled" : "timed out");
			break; /*abort process*/
		}
	}

	timer.stop();
	timer.disconnect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	if (bAborted)
	{
		process.kill();
		process.waitForFinished(-1);
	}

	while (process.canReadLine())
	{
		const QString line = QString::fromLatin1(process.readLine()).simplified();
		if (line.length() > 1)
		{
			log(line);
		}
	}

	return bAborted ? INT_MAX : process.exitCode();
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

/*NONE*/

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
