///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <MUtils/Version.h>
#include "Internal.h"

//Internal
#include "DirLocker.h"
#include "3rd_party/strnatcmp/include/strnatcmp.h"

//Qt
#include <QDir>
#include <QReadWriteLock>
#include <QProcess>
#include <QTextCodec>
#include <QPair>
#include <QHash>
#include <QListIterator>
#include <QMutex>
#include <QThreadStorage>

//CRT
#include <cstdlib>
#include <ctime>
#include <process.h>

//VLD
#ifdef _MSC_VER
#include <vld.h>
#endif

//Global
const QString MUtils::Internal::g_empty;

///////////////////////////////////////////////////////////////////////////////
// Random Support
///////////////////////////////////////////////////////////////////////////////

#ifndef _CRT_RAND_S
#define rand_s(X) (true)
#endif

//Per-thread init flag
static QThreadStorage<bool> g_srand_flag;

//32-Bit wrapper for qrand()
#define QRAND() ((static_cast<quint32>(qrand()) & 0xFFFF) | (static_cast<quint32>(qrand()) << 16U))

//Robert Jenkins' 96 bit Mix Function
static quint32 mix_function(quint32 a, quint32 b, quint32 c)
{
	a=a-b;  a=a-c;  a=a^(c >> 13);
	b=b-c;  b=b-a;  b=b^(a <<  8); 
	c=c-a;  c=c-b;  c=c^(b >> 13);
	a=a-b;  a=a-c;  a=a^(c >> 12);
	b=b-c;  b=b-a;  b=b^(a << 16);
	c=c-a;  c=c-b;  c=c^(b >>  5);
	a=a-b;  a=a-c;  a=a^(c >>  3);
	b=b-c;  b=b-a;  b=b^(a << 10);
	c=c-a;  c=c-b;  c=c^(b >> 15);

	return a ^ b ^ c;
}

static void seed_rand(void)
{
	QDateTime build(MUtils::Version::lib_build_date(), MUtils::Version::lib_build_time());
	const quint32 seed_0 = mix_function(MUtils::OS::process_id(), MUtils::OS::thread_id(), build.toMSecsSinceEpoch());
	qsrand(mix_function(clock(), time(NULL), seed_0));
}

static quint32 rand_fallback(void)
{
	Q_ASSERT(RAND_MAX >= 0x7FFF);

	if (!(g_srand_flag.hasLocalData() && g_srand_flag.localData()))
	{
		seed_rand();
		g_srand_flag.setLocalData(true);
	}

	quint32 rnd_val = mix_function(0x32288EA3, clock(), time(NULL));

	for (size_t i = 0; i < 42; i++)
	{
		rnd_val = mix_function(rnd_val, QRAND(), QRAND());
		rnd_val = mix_function(QRAND(), rnd_val, QRAND());
		rnd_val = mix_function(QRAND(), QRAND(), rnd_val);
	}

	return rnd_val;
}

quint32 MUtils::next_rand_u32(void)
{
	quint32 rnd;
	if (rand_s(&rnd))
	{
		return rand_fallback();
	}
	return rnd;
}

quint32 MUtils::next_rand_u32(const quint32 max)
{
	static const uint32_t DIV_LUT[64] =
	{
		0xFFFFFFFF, 0xFFFFFFFF, 0x80000000, 0x55555556, 0x40000000, 0x33333334, 0x2AAAAAAB, 0x24924925,
		0x20000000, 0x1C71C71D, 0x1999999A, 0x1745D175, 0x15555556, 0x13B13B14, 0x12492493, 0x11111112,
		0x10000000, 0x0F0F0F10, 0x0E38E38F, 0x0D79435F, 0x0CCCCCCD, 0x0C30C30D, 0x0BA2E8BB, 0x0B21642D,
		0x0AAAAAAB, 0x0A3D70A4, 0x09D89D8A, 0x097B425F, 0x0924924A, 0x08D3DCB1, 0x08888889, 0x08421085,
		0x08000000, 0x07C1F07D, 0x07878788, 0x07507508, 0x071C71C8, 0x06EB3E46, 0x06BCA1B0, 0x06906907,
		0x06666667, 0x063E7064, 0x06186187, 0x05F417D1, 0x05D1745E, 0x05B05B06, 0x0590B217, 0x0572620B,
		0x05555556, 0x0539782A, 0x051EB852, 0x05050506, 0x04EC4EC5, 0x04D4873F, 0x04BDA130, 0x04A7904B,
		0x04924925, 0x047DC120, 0x0469EE59, 0x0456C798, 0x04444445, 0x04325C54, 0x04210843, 0x04104105
	};
	return (max < 64) ? (next_rand_u32() / DIV_LUT[max]) : (next_rand_u32() / (UINT32_MAX / max + 1U));
}


quint64 MUtils::next_rand_u64(void)
{
	return (quint64(next_rand_u32()) << 32) | quint64(next_rand_u32());
}

QString MUtils::next_rand_str(const bool &bLong)
{
	if (bLong)
	{
		return next_rand_str(false) + next_rand_str(false);
	}
	else
	{
		return QString::number(next_rand_u64(), 16).rightJustified(16, QLatin1Char('0'));
	}
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
	return make_temp_file(QDir(basePath), extension, placeholder);
}

QString MUtils::make_temp_file(const QDir &basePath, const QString &extension, const bool placeholder)
{
	if (extension.isEmpty())
	{
		qWarning("Cannot generate temp file name with invalid parameters!");
		return QString();
	}

	for(int i = 0; i < 4096; i++)
	{
		const QString tempFileName = basePath.absoluteFilePath(QString("%1.%2").arg(next_rand_str(), extension));
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

QString MUtils::make_unique_file(const QString &basePath, const QString &baseName, const QString &extension, const bool fancy, const bool placeholder)
{
	return make_unique_file(QDir(basePath), baseName, extension, fancy, placeholder);
}

QString MUtils::make_unique_file(const QDir &basePath, const QString &baseName, const QString &extension, const bool fancy, const bool placeholder)
{
	if (baseName.isEmpty() || extension.isEmpty())
	{
		qWarning("Cannot generate unique file name with invalid parameters!");
		return QString();
	}

	quint32 n = fancy ? 2 : 0;
	QString fileName = fancy ? basePath.absoluteFilePath(QString("%1.%2").arg(baseName, extension)) : QString();
	while (fileName.isEmpty() || QFileInfo(fileName).exists())
	{
		if (n <= quint32(USHRT_MAX))
		{
			if (fancy)
			{
				fileName = basePath.absoluteFilePath(QString("%1 (%2).%3").arg(baseName, QString::number(n++), extension));
			}
			else
			{
				fileName = basePath.absoluteFilePath(QString("%1.%2.%3").arg(baseName, QString::number(n++, 16).rightJustified(4, QLatin1Char('0')), extension));
			}
		}
		else
		{
			qWarning("Failed to generate unique file name!");
			return QString();
		}
	}

	if (placeholder && (!fileName.isEmpty()))
	{
		QFile placeholderFile(fileName);
		if (placeholderFile.open(QIODevice::WriteOnly))
		{
			placeholderFile.close();
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
	const QString tempPath = try_create_subfolder(baseDir, MUtils::next_rand_str());
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
	static const OS::known_folder_t FOLDER_ID[2] = { OS::FOLDER_APPDATA_LOCA, OS::FOLDER_SYSROOT };
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

	for(size_t round = 0; round < 13; ++round)
	{
		if (round > 0)
		{
			MUtils::OS::sleep_ms(round);
			fileInfo.refresh();
		}
		if (fileInfo.exists())
		{
			QFile file(fileName);
			if (round > 0)
			{
				file.setPermissions(FILE_PERMISSIONS_NONE);
			}
			file.remove();
			fileInfo.refresh();
		}
		if (!fileInfo.exists())
		{
			return true; /*success*/
		}
	}

	qWarning("Could not delete \"%s\"", MUTILS_UTF8(fileName));
	return false;
}

bool MUtils::remove_directory(const QString &folderPath, const bool &recursive)
{
	const QDir folder(folderPath);

	if(recursive && folder.exists())
	{
		const QFileInfoList entryList = folder.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
		for(QFileInfoList::ConstIterator iter = entryList.constBegin(); iter != entryList.constEnd(); iter++)
		{
			if(iter->isDir())
			{
				remove_directory(iter->canonicalFilePath(), true);
			}
			else
			{
				remove_file(iter->canonicalFilePath());
			}
		}
	}

	for(size_t round = 0; round < 13; ++round)
	{
		if(round > 0)
		{
			MUtils::OS::sleep_ms(round);
			folder.refresh();
		}
		if (folder.exists())
		{
			QDir parent = folder;
			if (parent.cdUp())
			{
				if (round > 0)
				{
					QFile::setPermissions(folder.absolutePath(), FILE_PERMISSIONS_NONE);
				}
				parent.rmdir(folder.dirName());
				folder.refresh();
			}
		}
		if (!folder.exists())
		{
			return true; /*success*/
		}
	}
	
	qWarning("Could not rmdir \"%s\"", MUTILS_UTF8(folderPath));
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// PROCESS UTILS
///////////////////////////////////////////////////////////////////////////////

static void prependToPath(QProcessEnvironment &env, const QString &value)
{
	static const QLatin1String PATH("PATH");
	const QString path = env.value(PATH, QString()).trimmed();
	env.insert(PATH, path.isEmpty() ? value : QString("%1;%2").arg(value, path));
}

void MUtils::init_process(QProcess &process, const QString &wokringDir, const bool bReplaceTempDir, const QStringList *const extraPaths, const QHash<QString, QString> *const extraEnv)
{
	//Environment variable names
	static const char *const ENVVAR_NAMES_TMP[] =
	{
		"TEMP", "TMP", "TMPDIR", "HOME", "USERPROFILE", "HOMEPATH", NULL
	};
	static const char *const ENVVAR_NAMES_SYS[] =
	{
		"WINDIR", "SYSTEMROOT", NULL
	};
	static const char *const ENVVAR_NAMES_DEL[] =
	{
		"HTTP_PROXY", "FTP_PROXY", "NO_PROXY", "HOME", "LC_ALL", "LC_COLLATE", "LC_CTYPE",
		"LC_MESSAGES", "LC_MONETARY", "LC_NUMERIC", "LC_TIME", "LANG", NULL
	};

	//Initialize environment
	QProcessEnvironment env = process.processEnvironment();
	if (env.isEmpty())
	{
		env = QProcessEnvironment::systemEnvironment();
	}

	//Clean enviroment variables that might affect our tools
	for(const char *const *ptr = ENVVAR_NAMES_DEL; *ptr; ++ptr)
	{
		env.remove(QString::fromLatin1(*ptr));
	}

	//Set up system root directory
	const QString sysRoot = QDir::toNativeSeparators(OS::known_folder(OS::FOLDER_SYSROOT));
	if (!sysRoot.isEmpty())
	{
		for (const char *const *ptr = ENVVAR_NAMES_SYS; *ptr; ++ptr)
		{
			env.insert(QString::fromLatin1(*ptr), sysRoot);
		}
	}

	//Replace TEMP directory in environment
	const QString tempDir = QDir::toNativeSeparators(temp_folder());
	if(bReplaceTempDir)
	{
		for (const char *const *ptr = ENVVAR_NAMES_TMP; *ptr; ++ptr)
		{
			env.insert(QString::fromLatin1(*ptr), tempDir);
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
	
	//Setup environment
	if (extraEnv && (!extraEnv->isEmpty()))
	{
		for (QHash<QString, QString>::ConstIterator iter = extraEnv->constBegin(); iter != extraEnv->constEnd(); iter++)
		{
			env.insert(iter.key(), iter.value());
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

static QMutex                                              g_clean_file_name_mutex;
static QScopedPointer<const QList<QPair<QRegExp,QString>>> g_clean_file_name_regex;

static void clean_file_name_make_pretty(QString &str)
{
	static const struct { const char *p; const char *r; } PATTERN[] =
	{
		{ "^\\s*\"([^\"]*)\"\\s*$",    "\\1"                         },  //Remove straight double quotes around the whole string
		{ "\"([^\"]*)\"",              "\xE2\x80\x9C\\1\xE2\x80\x9D" },  //Replace remaining pairs of straight double quotes with opening/closing double quote
		{ "^[\\\\/:]+([^\\\\/:]+.*)$", "\\1"                         },  //Remove leading slash, backslash and colon characters
		{ "^(.*[^\\\\/:]+)[\\\\/:]+$", "\\1"                         },  //Remove trailing slash, backslash and colon characters
		{ "(\\s*[\\\\/:]\\s*)+",       " - "                         },  //Replace any slash, backslash or colon character that appears in the middle
		{ NULL, NULL }
	};

	QMutexLocker locker(&g_clean_file_name_mutex);

	if (g_clean_file_name_regex.isNull())
	{
		QScopedPointer<QList<QPair<QRegExp, QString>>> list(new QList<QPair<QRegExp, QString>>());
		for (size_t i = 0; PATTERN[i].p; ++i)
		{
			list->append(qMakePair(QRegExp(QString::fromUtf8(PATTERN[i].p), Qt::CaseInsensitive), PATTERN[i].r ? QString::fromUtf8(PATTERN[i].r) : QString()));
		}
		g_clean_file_name_regex.reset(list.take());
	}

	bool keepOnGoing = !str.isEmpty();
	while(keepOnGoing)
	{
		const QString prev = str;
		keepOnGoing = false;
		for (QList<QPair<QRegExp, QString>>::ConstIterator iter = g_clean_file_name_regex->constBegin(); iter != g_clean_file_name_regex->constEnd(); ++iter)
		{
			str.replace(iter->first, iter->second);
			if (str.compare(prev))
			{
				str = str.simplified();
				keepOnGoing = !str.isEmpty();
				break;
			}
		}
	}
}

QString MUtils::clean_file_name(const QString &name, const bool &pretty)
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
	if (pretty)
	{
		clean_file_name_make_pretty(result);
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

QString MUtils::clean_file_path(const QString &path, const bool &pretty)
{
	const QPair<QString, QString> prefix = clean_file_path_get_prefix(path);

	QStringList parts = prefix.second.split(QLatin1Char('/'), QString::SkipEmptyParts);
	for(int i = 0; i < parts.count(); i++)
	{
		if((i == 0) && (parts[i].length() == 2) && parts[i][0].isLetter() && (parts[i][1] == QLatin1Char(':')))
		{
			continue; //handle case "c:\"
		}
		parts[i] = MUtils::clean_file_name(parts[i], pretty);
	}

	const QString cleanPath = parts.join(QLatin1String("/"));
	return prefix.first.isEmpty() ? cleanPath : prefix.first + cleanPath;
}

///////////////////////////////////////////////////////////////////////////////
// PARENT PATH
///////////////////////////////////////////////////////////////////////////////

static void remove_trailing_separators(QString &pathStr)
{
	while (pathStr.endsWith('/'))
	{
		pathStr.chop(1);
	}
}

MUTILS_API QString MUtils::parent_path(const QString &path)
{
	QString parentPath(QDir::fromNativeSeparators(path));
	remove_trailing_separators(parentPath);
	const int pos = parentPath.lastIndexOf(QLatin1Char('/'));
	if (pos >= 0)
	{
		parentPath.truncate(pos);
		remove_trailing_separators(parentPath);
		if (parentPath.isEmpty())
		{
			return QLatin1String("/");
		}
		if ((parentPath.length() == 2) && parentPath.at(0).isLetter() && (parentPath.at(1) == QLatin1Char(':')))
		{
			parentPath += QLatin1Char('/');
		}
		return parentPath;
	}
	return QString();
}

///////////////////////////////////////////////////////////////////////////////
// REGULAR EXPESSION HELPER
///////////////////////////////////////////////////////////////////////////////

bool MUtils::regexp_parse_uint32(const QRegExp &regexp, quint32 &value)
{
	return regexp_parse_uint32(regexp, &value, 1U, 1U);
}

bool MUtils::regexp_parse_int32(const QRegExp &regexp, qint32 &value)
{
	return regexp_parse_int32(regexp, &value, 1U, 1U);
}

bool MUtils::regexp_parse_uint32(const QRegExp &regexp, quint32 &value, const size_t &offset)
{
	return regexp_parse_uint32(regexp, &value, offset, 1U);
}

bool MUtils::regexp_parse_int32(const QRegExp &regexp, qint32 &value, const size_t &offset)
{
	return regexp_parse_int32(regexp, &value, offset, 1U);
}

bool MUtils::regexp_parse_uint32(const QRegExp &regexp, quint32 *values, const size_t &count)
{
	return regexp_parse_uint32(regexp, values, 1U, count);
}

bool MUtils::regexp_parse_int32(const QRegExp &regexp, qint32 *values, const size_t &count)
{
	return regexp_parse_int32(regexp, values, 1U, count);
}

bool MUtils::regexp_parse_uint32(const QRegExp &regexp, quint32 *values, const size_t &offset, const size_t &count)
{
	const QStringList caps = regexp.capturedTexts();

	if (caps.isEmpty() || (quint32(caps.count()) <= count))
	{
		return false;
	}

	for (size_t i = 0; i < count; i++)
	{
		bool ok = false;
		values[i] = caps[offset+i].toUInt(&ok);
		if (!ok)
		{
			return false;
		}
	}

	return true;
}

bool MUtils::regexp_parse_int32(const QRegExp &regexp, qint32 *values, const size_t &offset, const size_t &count)
{
	const QStringList caps = regexp.capturedTexts();

	if (caps.isEmpty() || (quint32(caps.count()) <= count))
	{
		return false;
	}

	for (size_t i = 0; i < count; i++)
	{
		bool ok = false;
		values[i] = caps[offset+i].toInt(&ok);
		if (!ok)
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
// FP MATH SUPPORT
///////////////////////////////////////////////////////////////////////////////

MUtils::fp_parts_t MUtils::break_fp(const double value)
{
	fp_parts_t result = { };
	if (_finite(value))
	{
		result.parts[1] = modf(value, &result.parts[0]);
	}
	else
	{
		result.parts[0] = std::numeric_limits<double>::quiet_NaN();
		result.parts[1] = std::numeric_limits<double>::quiet_NaN();
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
// INITIALIZER
///////////////////////////////////////////////////////////////////////////////

unsigned int MUtils::Internal::MUTILS_INITIALIZER(const unsigned int interfaceId)
{
	if(interfaceId != ((unsigned int)MUTILS_INTERFACE))
	{
		OS::system_message_err(L"MUtils", L"ERROR: MUtils library initialization has failed!");
		for(;;) _exit((int)0xC0000142);
	}

	volatile unsigned int _result = MUTILS_INTERFACE;
	return _result;
}
