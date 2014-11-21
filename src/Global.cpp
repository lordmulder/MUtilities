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

//Qt
#include <QDir>
#include <QReadWriteLock>

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

static QReadWriteLock g_temp_folder_lock;
static QFile *g_temp_folder_file;
static QString *g_temp_folder_path = NULL;

#define INIT_TEMP_FOLDER_RAND(OUT_PTR, FILE_PTR, BASE_DIR) do \
{ \
	for(int _i = 0; _i < 128; _i++) \
	{ \
		const QString _randDir = QString("%1/%2").arg((BASE_DIR), rand_str()); \
		if(!QDir(_randDir).exists()) \
		{ \
			*(OUT_PTR) = try_init_folder(_randDir, (FILE_PTR)); \
			if(!(OUT_PTR)->isEmpty()) break; \
		} \
	} \
} \
while(0)

static QString try_init_folder(const QString &folderPath, QFile *&lockFile)
{
	static const char *TEST_DATA = "Lorem ipsum dolor sit amet, consectetur, adipisci velit!";
	
	bool success = false;

	const QFileInfo folderInfo(folderPath);
	const QDir folderDir(folderInfo.absoluteFilePath());

	//Remove existing lock file
	if(lockFile)
	{
		lockFile->remove();
		MUTILS_DELETE(lockFile);
	}

	//Create folder, if it does *not* exist yet
	if(!folderDir.exists())
	{
		for(int i = 0; i < 16; i++)
		{
			if(folderDir.mkpath(".")) break;
		}
	}

	//Make sure folder exists now *and* is writable
	if(folderDir.exists())
	{
		const QByteArray testData = QByteArray(TEST_DATA);
		for(int i = 0; i < 32; i++)
		{
			lockFile = new QFile(folderDir.absoluteFilePath(QString("~%1.tmp").arg(MUtils::rand_str())));
			if(lockFile->open(QIODevice::ReadWrite | QIODevice::Truncate))
			{
				if(lockFile->write(testData) >= testData.size())
				{
					success = true;
					break;
				}
				lockFile->remove();
				MUTILS_DELETE(lockFile);
			}
		}
	}

	return (success ? folderDir.canonicalPath() : QString());
}

const QString &MUtils::temp_folder(void)
{
	QReadLocker readLock(&g_temp_folder_lock);

	//Already initialized?
	if(g_temp_folder_path && (!g_temp_folder_path->isEmpty()))
	{
		return (*g_temp_folder_path);
	}

	//Obtain the write lock to initilaize
	readLock.unlock();
	QWriteLocker writeLock(&g_temp_folder_lock);
	
	//Still uninitilaized?
	if(g_temp_folder_path && (!g_temp_folder_path->isEmpty()))
	{
		return (*g_temp_folder_path);
	}

	//Create the string, if not done yet
	if(!g_temp_folder_path)
	{
		g_temp_folder_path = new QString();
	}
	
	g_temp_folder_path->clear();

	//Try the %TMP% or %TEMP% directory first
	QString tempPath = try_init_folder(QDir::temp().absolutePath(), g_temp_folder_file);
	if(!tempPath.isEmpty())
	{
		INIT_TEMP_FOLDER_RAND(g_temp_folder_path, g_temp_folder_file, tempPath);
	}

	//Otherwise create TEMP folder in %LOCALAPPDATA% or %SYSTEMROOT%
	if(g_temp_folder_path->isEmpty())
	{
		qWarning("%%TEMP%% directory not found -> trying fallback mode now!");
		static const lamexp_known_folder_t folderId[2] = { lamexp_folder_localappdata, lamexp_folder_systroot_dir };
		for(size_t id = 0; (g_lamexp_temp_folder.path->isEmpty() && (id < 2)); id++)
		{
			const QString &knownFolder = lamexp_known_folder(folderId[id]);
			if(!knownFolder.isEmpty())
			{
				tempPath = try_init_folder(QString("%1/Temp").arg(knownFolder));
				if(!tempPath.isEmpty())
				{
					INIT_TEMP_FOLDER_RAND(g_temp_folder_path, g_temp_folder_file, tempPath);
				}
			}
		}
	}

	//Failed to create TEMP folder?
	if(g_temp_folder_path->isEmpty())
	{
		qFatal("Temporary directory could not be initialized !!!");
	}
	
	return (*g_temp_folder_path);
}
