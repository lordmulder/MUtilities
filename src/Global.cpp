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

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>

//Internal
#include "DirLocker.h"
#include "3rd_party/strnatcmp/include/strnatcmp.h"

//Qt
#include <QDir>
#include <QReadWriteLock>
#include <QProcess>
#include <QTextCodec>

//CRT
#include <cstdlib>
#include <ctime>
#include <process.h>

//VLD
#include <vld.h>

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

static QScopedPointer<MUtils::Internal::DirLock> g_temp_folder_file;
static QReadWriteLock g_temp_folder_lock;

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

static MUtils::Internal::DirLock *try_init_temp_folder(const QString &baseDir)
{
	const QString tempPath = try_create_subfolder(baseDir, MUtils::rand_str());
	if(!tempPath.isEmpty())
	{
		for(int i = 0; i < 32; i++)
		{
			MUtils::Internal::DirLock *lockFile = NULL;
			try
			{
				lockFile = new MUtils::Internal::DirLock(tempPath);
				return lockFile;
			}
			catch(MUtils::Internal::DirLockException&)
			{
				/*ignore error and try again*/
			}
		}
	}
	return NULL;
}

const QString &MUtils::temp_folder(void)
{
	QReadLocker readLock(&g_temp_folder_lock);

	//Already initialized?
	if(!g_temp_folder_file.isNull())
	{
		return g_temp_folder_file->path();
	}

	//Obtain the write lock to initilaize
	readLock.unlock();
	QWriteLocker writeLock(&g_temp_folder_lock);
	
	//Still uninitilaized?
	if(!g_temp_folder_file.isNull())
	{
		return g_temp_folder_file->path();
	}

	//Try the %TMP% or %TEMP% directory first
	if(MUtils::Internal::DirLock *lockFile = try_init_temp_folder(QDir::tempPath()))
	{
		g_temp_folder_file.reset(lockFile);
		return lockFile->path();
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
				if(MUtils::Internal::DirLock *lockFile = try_init_temp_folder(tempRoot))
				{
					g_temp_folder_file.reset(lockFile);
					return lockFile->path();
				}
			}
		}
	}

	qFatal("Temporary directory could not be initialized !!!");
	return (*((QString*)NULL));
}

///////////////////////////////////////////////////////////////////////////////
// REMOVE DIRECTORY / FILE
///////////////////////////////////////////////////////////////////////////////

bool MUtils::remove_file(const QString &fileName)
{
	QFileInfo fileInfo(fileName);
	if(!(fileInfo.exists() && fileInfo.isFile()))
	{
		return true;
	}

	for(int i = 0; i < 32; i++)
	{
		QFile file(fileName);
		file.setPermissions(QFile::ReadOther | QFile::WriteOther);
		if(file.remove())
		{
			return true;
		}
	}

	qWarning("Could not delete \"%s\"", MUTILS_UTF8(fileName));
	return false;
}

bool MUtils::remove_directory(const QString &folderPath)
{
	QDir folder(folderPath);
	if(!folder.exists())
	{
		return true;
	}

	const QFileInfoList entryList = folder.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
	for(int i = 0; i < entryList.count(); i++)
	{
		if(entryList.at(i).isDir())
		{
			remove_directory(entryList.at(i).canonicalFilePath());
		}
		else
		{
			remove_file(entryList.at(i).canonicalFilePath());
		}
	}

	for(int i = 0; i < 32; i++)
	{
		if(folder.rmdir("."))
		{
			return true;
		}
	}
	
	qWarning("Could not rmdir \"%s\"", MUTILS_UTF8(folderPath));
	return false;
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
// NATURAL ORDER STRING COMPARISON
///////////////////////////////////////////////////////////////////////////////

static bool natural_string_sort_helper(const QString &str1, const QString &str2)
{
	return (MUtils::Internal::NaturalSort::strnatcmp(MUTILS_WCHR(str1), MUTILS_WCHR(str2)) < 0);
}

static bool natural_string_sort_helper_fold_case(const QString &str1, const QString &str2)
{
	return (MUtils::Internal::NaturalSort::strnatcasecmp(MUTILS_WCHR(str1), MUTILS_WCHR(str2)) < 0);
}

void MUtils::natural_string_sort(QStringList &list, const bool bIgnoreCase)
{
	qSort(list.begin(), list.end(), bIgnoreCase ? natural_string_sort_helper_fold_case : natural_string_sort_helper);
}

///////////////////////////////////////////////////////////////////////////////
// CLEAN FILE PATH
///////////////////////////////////////////////////////////////////////////////

static const struct
{
	const char *const search;
	const char *const replace;
}
CLEAN_FILE_NAME[] =
{
	{ "\\",  "-"  },
	{ " / ", ", " },
	{ "/",   ","  },
	{ ":",   "-"  },
	{ "*",   "x"  },
	{ "?",   "!"  },
	{ "<",   "["  },
	{ ">",   "]"  },
	{ "|",   "!"  },
	{ "\"",  "'"  },
	{ NULL,  NULL }
};

QString MUtils::clean_file_name(const QString &name)
{
	QString str = name.simplified();

	for(size_t i = 0; CLEAN_FILE_NAME[i].search; i++) 
	{
		str.replace(CLEAN_FILE_NAME[i].search, CLEAN_FILE_NAME[i].replace);
	}
	
	QRegExp regExp("\"(.+)\"");
	regExp.setMinimal(true);
	str.replace(regExp, "`\\1´");
	
	return str.simplified();
}

QString MUtils::clean_file_path(const QString &path)
{
	QStringList parts = path.simplified().replace("\\", "/").split("/", QString::SkipEmptyParts);

	for(int i = 0; i < parts.count(); i++)
	{
		parts[i] = MUtils::clean_file_name(parts[i]);
	}

	return parts.join("/");
}

///////////////////////////////////////////////////////////////////////////////
// AVAILABLE CODEPAGES
///////////////////////////////////////////////////////////////////////////////

QStringList MUtils::available_codepages(const bool &noAliases)
{
	QStringList codecList;
	QList<QByteArray> availableCodecs = QTextCodec::availableCodecs();

	while(!availableCodecs.isEmpty())
	{
		const QByteArray current = availableCodecs.takeFirst();
		if(!current.toLower().startsWith("system"))
		{
			codecList << QString::fromLatin1(current.constData(), current.size());
			if(noAliases)
			{
				if(QTextCodec *const currentCodec = QTextCodec::codecForName(current.constData()))
				{
					const QList<QByteArray> aliases = currentCodec->aliases();
					for(QList<QByteArray>::ConstIterator iter = aliases.constBegin(); iter != aliases.constEnd(); iter++)
					{
						availableCodecs.removeAll(*iter);
					}
				}
			}
		}
	}

	return codecList;
}

///////////////////////////////////////////////////////////////////////////////
// SELF-TEST
///////////////////////////////////////////////////////////////////////////////

int MUtils::Internal::selfTest(const char *const buildKey, const bool debug)
{
	static const bool MY_DEBUG_FLAG = MUTILS_DEBUG;
	static const char *const MY_BUILD_KEY = __DATE__"@"__TIME__;

	if(strncmp(buildKey, MY_BUILD_KEY, 14) || (MY_DEBUG_FLAG != debug))
	{
		MUtils::OS::system_message_err(L"MUtils", L"FATAL ERROR: MUtils library version mismatch detected!");
		MUtils::OS::system_message_wrn(L"MUtils", L"Please re-build the complete solution in order to fix this issue!");
		abort();
	}
	return 0;
}
