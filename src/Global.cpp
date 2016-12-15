///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <QPair>
#include <QListIterator>
#include <QMutex>

//CRT
#include <cstdlib>
#include <ctime>
#include <process.h>

//VLD
#ifdef _MSC_VER
#include <vld.h>
#endif

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
// STRING UTILITY FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static QScopedPointer<QRegExp> g_str_trim_rx_r;
static QScopedPointer<QRegExp> g_str_trim_rx_l;
static QMutex                  g_str_trim_lock;

static QString& trim_helper(QString &str, QScopedPointer<QRegExp> &regex, const char *const pattern)
{
	QMutexLocker lock(&g_str_trim_lock);
	if (regex.isNull())
	{
		regex.reset(new QRegExp(QLatin1String(pattern)));
	}
	str.remove(*regex.data());
	return str;
}

QString& MUtils::trim_right(QString &str)
{
	static const char *const TRIM_RIGHT = "\\s+$";
	return trim_helper(str, g_str_trim_rx_r, TRIM_RIGHT);
}

QString& MUtils::trim_left(QString &str)
{
	static const char *const TRIM_LEFT = "^\\s+";
	return trim_helper(str, g_str_trim_rx_l, TRIM_LEFT);
}

QString MUtils::trim_right(const QString &str)
{
	QString temp(str);
	return trim_right(temp);
}

QString MUtils::trim_left(const QString &str)
{
	QString temp(str);
	return trim_left(temp);
}

///////////////////////////////////////////////////////////////////////////////
// GENERATE FILE NAME
///////////////////////////////////////////////////////////////////////////////

QString MUtils::make_temp_file(const QString &basePath, const QString &extension, const bool placeholder)
{
	for(int i = 0; i < 4096; i++)
	{
		const QString tempFileName = QString("%1/%2.%3").arg(basePath, rand_str(), extension);
		if(!QFileInfo(tempFileName).exists())
		{
			if(placeholder)
			{
				QFile file(tempFileName);
				if(file.open(QFile::ReadWrite))
				{
					file.close();
					return tempFileName;
				}
			}
			else
			{
				return tempFileName;
			}
		}
	}

	qWarning("Failed to generate temp file name!");
	return QString();
}

QString MUtils::make_unique_file(const QString &basePath, const QString &baseName, const QString &extension, const bool fancy)
{
	quint32 n = fancy ? 2 : 0;
	QString fileName = fancy ? QString("%1/%2.%3").arg(basePath, baseName, extension) : QString();
	while (fileName.isEmpty() || QFileInfo(fileName).exists())
	{
		if (n <= quint32(USHRT_MAX))
		{
			if (fancy)
			{
				fileName = QString("%1/%2 (%3).%4").arg(basePath, baseName, QString::number(n++), extension);
			}
			else
			{
				fileName = QString("%1/%2.%3.%4").arg(basePath, baseName, QString::number(n++, 16).rightJustified(4, QLatin1Char('0')), extension);
			}
		}
		else
		{
			qWarning("Failed to generate unique file name!");
			return QString();
		}
	}
	return fileName;
}

///////////////////////////////////////////////////////////////////////////////
// COMPUTE PARITY
///////////////////////////////////////////////////////////////////////////////

/*
 * Compute parity in parallel
 * http://www.graphics.stanford.edu/~seander/bithacks.html#ParityParallel
 */
bool MUtils::parity(quint32 value)
{
	value ^= value >> 16;
	value ^= value >> 8;
	value ^= value >> 4;
	value &= 0xf;
	return ((0x6996 >> value) & 1) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// TEMP FOLDER
///////////////////////////////////////////////////////////////////////////////

static QScopedPointer<MUtils::Internal::DirLock> g_temp_folder_file;
static QReadWriteLock                            g_temp_folder_lock;

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

static bool temp_folder_cleanup_helper(const QString &tempPath)
{
	size_t delay = 1;
	static const size_t MAX_DELAY = 8192;
	forever
	{
		QDir::setCurrent(QDir::rootPath());
		if(MUtils::remove_directory(tempPath, true))
		{
			return true;
		}
		else
		{
			if(delay > MAX_DELAY)
			{
				return false;
			}
			MUtils::OS::sleep_ms(delay);
			delay *= 2;
		}
	}
}

static void temp_folder_cleaup(void)
{
	QWriteLocker writeLock(&g_temp_folder_lock);

	//Clean the directory
	while(!g_temp_folder_file.isNull())
	{
		const QString tempPath = g_temp_folder_file->getPath();
		g_temp_folder_file.reset(NULL);
		if(!temp_folder_cleanup_helper(tempPath))
		{
			MUtils::OS::system_message_wrn(L"Temp Cleaner", L"Warning: Not all temporary files could be removed!");
		}
	}
}

const QString &MUtils::temp_folder(void)
{
	QReadLocker readLock(&g_temp_folder_lock);

	//Already initialized?
	if(!g_temp_folder_file.isNull())
	{
		return g_temp_folder_file->getPath();
	}

	//Obtain the write lock to initilaize
	readLock.unlock();
	QWriteLocker writeLock(&g_temp_folder_lock);
	
	//Still uninitilaized?
	if(!g_temp_folder_file.isNull())
	{
		return g_temp_folder_file->getPath();
	}

	//Try the %TMP% or %TEMP% directory first
	if(MUtils::Internal::DirLock *lockFile = try_init_temp_folder(QDir::tempPath()))
	{
		g_temp_folder_file.reset(lockFile);
		atexit(temp_folder_cleaup);
		return lockFile->getPath();
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
					atexit(temp_folder_cleaup);
					return lockFile->getPath();
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

static const QFile::Permissions FILE_PERMISSIONS_NONE = QFile::ReadOther | QFile::WriteOther;

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
		file.setPermissions(FILE_PERMISSIONS_NONE);
		if((!(fileInfo.exists() && fileInfo.isFile())) || file.remove())
		{
			return true;
		}
		fileInfo.refresh();
	}

	qWarning("Could not delete \"%s\"", MUTILS_UTF8(fileName));
	return false;
}

static bool remove_directory_helper(const QDir &folder)
{
	if(!folder.exists())
	{
		return true;
	}
	const QString dirName = folder.dirName();
	if(!dirName.isEmpty())
	{
		QDir parent(folder);
		if(parent.cdUp())
		{
			QFile::setPermissions(folder.absolutePath(), FILE_PERMISSIONS_NONE);
			if(parent.rmdir(dirName))
			{
				return true;
			}
		}
	}
	return false;
}

bool MUtils::remove_directory(const QString &folderPath, const bool &recursive)
{
	QDir folder(folderPath);
	if(!folder.exists())
	{
		return true;
	}

	if(recursive)
	{
		const QFileInfoList entryList = folder.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
		for(QFileInfoList::ConstIterator iter = entryList.constBegin(); iter != entryList.constEnd(); iter++)
		{
			if(iter->isDir())
			{
				remove_directory(iter->canonicalFilePath(), true);
			}
			else if(iter->isFile())
			{
				remove_file(iter->canonicalFilePath());
			}
		}
	}

	for(int i = 0; i < 32; i++)
	{
		if(remove_directory_helper(folder))
		{
			return true;
		}
		folder.refresh();
	}
	
	qWarning("Could not rmdir \"%s\"", MUTILS_UTF8(folderPath));
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS UTILS
///////////////////////////////////////////////////////////////////////////////

static void prependToPath(QProcessEnvironment &env, const QString &value)
{
	const QLatin1String PATH = QLatin1String("PATH");
	const QString path = env.value(PATH, QString()).trimmed();
	env.insert(PATH, path.isEmpty() ? value : QString("%1;%2").arg(value, path));
}

void MUtils::init_process(QProcess &process, const QString &wokringDir, const bool bReplaceTempDir, const QStringList *const extraPaths)
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
	prependToPath(env, tempDir);
	if (extraPaths && (!extraPaths->isEmpty()))
	{
		QListIterator<QString> iter(*extraPaths);
		iter.toBack();
		while (iter.hasPrevious())
		{
			prependToPath(env, QDir::toNativeSeparators(iter.previous()));
		}
	}
	
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

QString MUtils::clean_file_name(const QString &name)
{
	static const QLatin1Char REPLACEMENT_CHAR('_');
	static const char FILENAME_ILLEGAL_CHARS[] = "<>:\"/\\|?*";
	static const char *const FILENAME_RESERVED_NAMES[] =
	{
		"CON", "PRN", "AUX", "NUL",
		"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
		"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9", NULL
	};

	QString result(name);
	if (result.contains(QLatin1Char('"')))
	{
		QRegExp quoted("\"(.+)\"");
		quoted.setMinimal(true);
		result.replace(quoted, "``\\1´´");
	}

	for(QString::Iterator iter = result.begin(); iter != result.end(); iter++)
	{
		if (iter->category() == QChar::Other_Control)
		{
			*iter = REPLACEMENT_CHAR;
		}
	}
		
	for(size_t i = 0; FILENAME_ILLEGAL_CHARS[i]; i++)
	{
		result.replace(QLatin1Char(FILENAME_ILLEGAL_CHARS[i]), REPLACEMENT_CHAR);
	}
	
	trim_right(result);
	while (result.endsWith(QLatin1Char('.')))
	{
		result.chop(1);
		trim_right(result);
	}

	for (size_t i = 0; FILENAME_RESERVED_NAMES[i]; i++)
	{
		const QString reserved = QString::fromLatin1(FILENAME_RESERVED_NAMES[i]);
		if ((!result.compare(reserved, Qt::CaseInsensitive)) || result.startsWith(reserved + QLatin1Char('.'), Qt::CaseInsensitive))
		{
			result.replace(0, reserved.length(), QString().leftJustified(reserved.length(), REPLACEMENT_CHAR));
		}
	}

	return result;
}

static QPair<QString,QString> clean_file_path_get_prefix(const QString path)
{
	static const char *const PREFIXES[] =
	{
		"//?/", "//", "/", NULL
	};
	const QString posixPath = QDir::fromNativeSeparators(path.trimmed());
	for (int i = 0; PREFIXES[i]; i++)
	{
		const QString prefix = QString::fromLatin1(PREFIXES[i]);
		if (posixPath.startsWith(prefix))
		{
			return qMakePair(prefix, posixPath.mid(prefix.length()));
		}
	}
	return qMakePair(QString(), posixPath);
}

QString MUtils::clean_file_path(const QString &path)
{
	const QPair<QString, QString> prefix = clean_file_path_get_prefix(path);

	QStringList parts = prefix.second.split(QLatin1Char('/'), QString::SkipEmptyParts);
	for(int i = 0; i < parts.count(); i++)
	{
		if((i == 0) && (parts[i].length() == 2) && parts[i][0].isLetter() && (parts[i][1] == QLatin1Char(':')))
		{
			continue; //handle case "c:\"
		}
		parts[i] = MUtils::clean_file_name(parts[i]);
	}

	const QString cleanPath = parts.join(QLatin1String("/"));
	return prefix.first.isEmpty() ? cleanPath : prefix.first + cleanPath;
}

///////////////////////////////////////////////////////////////////////////////
// REGULAR EXPESSION HELPER
///////////////////////////////////////////////////////////////////////////////

bool MUtils::regexp_parse_uint32(const QRegExp &regexp, quint32 &value)
{
	return regexp_parse_uint32(regexp, &value, 1);
}

bool MUtils::regexp_parse_uint32(const QRegExp &regexp, quint32 *values, const size_t &count)
{
	const QStringList caps = regexp.capturedTexts();
	
	if(caps.isEmpty() || (quint32(caps.count()) <= count))
	{
		return false;
	}

	for(size_t i = 0; i < count; i++)
	{
		bool ok = false;
		values[i] = caps[i+1].toUInt(&ok);
		if(!ok)
		{
			return false;
		}
	}

	return true;
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
	static const char *const MY_BUILD_KEY = __DATE__ "@" __TIME__;

	if(strncmp(buildKey, MY_BUILD_KEY, 13) || (MY_DEBUG_FLAG != debug))
	{
		MUtils::OS::system_message_err(L"MUtils", L"FATAL ERROR: MUtils library version mismatch detected!");
		MUtils::OS::system_message_wrn(L"MUtils", L"Perform a clean(!) re-install of the application to fix the problem!");
		abort();
	}
	return 0;
}
