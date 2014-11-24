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

#if _MSC_VER
#define _CRT_RAND_S 1
#endif

#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>

//Qt
#include <QDir>
#include <QReadWriteLock>
#include <QProcess>

//CRT
#include <cstdlib>
#include <ctime>
#include <process.h>

///////////////////////////////////////////////////////////////////////////////
// Random Support
///////////////////////////////////////////////////////////////////////////////

//Robert Jenkins' 96 bit Mix Function
static unsigned int mix_function(const unsigned int x, const unsigned int y, const unsigned int z)
{
	unsigned int a = x;
	unsigned int b = y;
	unsigned int c = z;
	
	a=a-b;  a=a-c;  a=a^(c >> 13);
	b=b-c;  b=b-a;  b=b^(a << 8 ); 
	c=c-a;  c=c-b;  c=c^(b >> 13);
	a=a-b;  a=a-c;  a=a^(c >> 12);
	b=b-c;  b=b-a;  b=b^(a << 16);
	c=c-a;  c=c-b;  c=c^(b >> 5 );
	a=a-b;  a=a-c;  a=a^(c >> 3 );
	b=b-c;  b=b-a;  b=b^(a << 10);
	c=c-a;  c=c-b;  c=c^(b >> 15);

	return a ^ b ^ c;
}

void MUtils::seed_rand(void)
{
	qsrand(mix_function(clock(), time(NULL), _getpid()));
}

quint32 MUtils::next_rand32(void)
{
	quint32 rnd = 0xDEADBEEF;

#ifdef _CRT_RAND_S
	if(rand_s(&rnd) == 0)
	{
		return rnd;
	}
#endif //_CRT_RAND_S

	for(size_t i = 0; i < sizeof(quint32); i++)
	{
		rnd = (rnd << 8) ^ qrand();
	}

	return rnd;
}

quint64 MUtils::next_rand64(void)
{
	return (quint64(next_rand32()) << 32) | quint64(next_rand32());
}

QString MUtils::rand_str(const bool &bLong)
{
	if(!bLong)
	{
		return QString::number(next_rand64(), 16).rightJustified(16, QLatin1Char('0'));
	}
	return QString("%1%2").arg(rand_str(false), rand_str(false));
}

///////////////////////////////////////////////////////////////////////////////
// TEMP FOLDER
///////////////////////////////////////////////////////////////////////////////

static QScopedPointer<QFile>   g_temp_folder_file;
static QScopedPointer<QString> g_temp_folder_path;
static QReadWriteLock          g_temp_folder_lock;

static QString try_create_subfolder(const QString &baseDir, const QString &postfix)
{
	const QString baseDirPath = QDir(baseDir).absolutePath();
	for(int i = 0; i < 32; i++)
	{
		QDir directory(baseDirPath);
		if(directory.mkpath(postfix) && directory.cd(postfix))
		{
			return directory.canonicalPath();
		}
	}
	return QString();
}

static QString try_init_temp_folder(const QString &baseDir)
{
	static const char *TEST_DATA = "Lorem ipsum dolor sit amet, consectetur, adipisci velit!";
	
	QString tempPath = try_create_subfolder(baseDir, MUtils::rand_str());
	if(!tempPath.isEmpty())
	{
		const QByteArray testData = QByteArray(TEST_DATA);
		for(int i = 0; i < 32; i++)
		{
			g_temp_folder_file.reset(new QFile(QString("%1/~%2.lck").arg(tempPath, MUtils::rand_str())));
			if(g_temp_folder_file->open(QIODevice::ReadWrite | QIODevice::Truncate))
			{
				if(g_temp_folder_file->write(testData) >= testData.size())
				{
					return tempPath;
				}
				g_temp_folder_file->remove();
			}
		}
	}

	return QString();
}

const QString &MUtils::temp_folder(void)
{
	QReadLocker readLock(&g_temp_folder_lock);

	//Already initialized?
	if((!g_temp_folder_path.isNull()) && (!g_temp_folder_path->isEmpty()))
	{
		return (*g_temp_folder_path.data());
	}

	//Obtain the write lock to initilaize
	readLock.unlock();
	QWriteLocker writeLock(&g_temp_folder_lock);
	
	//Still uninitilaized?
	if((!g_temp_folder_path.isNull()) && (!g_temp_folder_path->isEmpty()))
	{
		return (*g_temp_folder_path.data());
	}

	//Try the %TMP% or %TEMP% directory first
	QString tempPath = try_init_temp_folder(QDir::tempPath());
	if(!tempPath.isEmpty())
	{
		g_temp_folder_path.reset(new QString(tempPath));
		return (*g_temp_folder_path.data());
	}

	qWarning("%%TEMP%% directory not found -> trying fallback mode now!");
	static const OS::known_folder_t FOLDER_ID[2] = { OS::FOLDER_LOCALAPPDATA, OS::FOLDER_SYSTROOT_DIR };
	for(size_t id = 0; id < 2; id++)
	{
		const QString &knownFolder = OS::known_folder(FOLDER_ID[id]);
		if(!knownFolder.isEmpty())
		{
			const QString tempRoot = try_create_subfolder(knownFolder, QLatin1String("TEMP"));
			if(!tempRoot.isEmpty())
			{
				tempPath = try_init_temp_folder(tempRoot);
				if(!tempPath.isEmpty())
				{
					g_temp_folder_path.reset(new QString(tempPath));
					return (*g_temp_folder_path.data());
				}
			}
		}
	}

	qFatal("Temporary directory could not be initialized !!!");
	return (*((QString*)NULL));
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS UTILS
///////////////////////////////////////////////////////////////////////////////

void MUtils::init_process(QProcess &process, const QString &wokringDir, const bool bReplaceTempDir)
{
	//Environment variable names
	static const char *const s_envvar_names_temp[] =
	{
		"TEMP", "TMP", "TMPDIR", "HOME", "USERPROFILE", "HOMEPATH", NULL
	};
	static const char *const s_envvar_names_remove[] =
	{
		"WGETRC", "SYSTEM_WGETRC", "HTTP_PROXY", "FTP_PROXY", "NO_PROXY", "GNUPGHOME", "LC_ALL", "LC_COLLATE", "LC_CTYPE", "LC_MESSAGES", "LC_MONETARY", "LC_NUMERIC", "LC_TIME", "LANG", NULL
	};

	//Initialize environment
	QProcessEnvironment env = process.processEnvironment();
	if(env.isEmpty()) env = QProcessEnvironment::systemEnvironment();

	//Clean a number of enviroment variables that might affect our tools
	for(size_t i = 0; s_envvar_names_remove[i]; i++)
	{
		env.remove(QString::fromLatin1(s_envvar_names_remove[i]));
		env.remove(QString::fromLatin1(s_envvar_names_remove[i]).toLower());
	}

	const QString tempDir = QDir::toNativeSeparators(temp_folder());

	//Replace TEMP directory in environment
	if(bReplaceTempDir)
	{
		for(size_t i = 0; s_envvar_names_temp[i]; i++)
		{
			env.insert(s_envvar_names_temp[i], tempDir);
		}
	}

	//Setup PATH variable
	const QString path = env.value("PATH", QString()).trimmed();
	env.insert("PATH", path.isEmpty() ? tempDir : QString("%1;%2").arg(tempDir, path));
	
	//Setup QPorcess object
	process.setWorkingDirectory(wokringDir);
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.setProcessEnvironment(env);
}

///////////////////////////////////////////////////////////////////////////////
// LIB VERSION
///////////////////////////////////////////////////////////////////////////////

const char* MUtils::mutils_build_date(void)
{
	static const char *const BUILD_DATE = __DATE__;
	return BUILD_DATE;
}

const char* MUtils::mutils_build_time(void)
{
	static const char *const BUILD_TIME = __TIME__;
	return BUILD_TIME;
}

///////////////////////////////////////////////////////////////////////////////
// SELF-TEST
///////////////////////////////////////////////////////////////////////////////

int MUtils::Internal::selfTest(const char *const date, const bool debug)
{
	if(strcmp(date, __DATE__) || (MUTILS_DEBUG != debug))
	{
		MUtils::OS::system_message_err(L"MUtils", L"FATAL ERROR: MUtils library version mismatch detected!");
		abort();
	}
	return 0;
}
