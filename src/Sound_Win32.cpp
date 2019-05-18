///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2019 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Win32 API
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include <MMSystem.h>

//MUtils
#include <MUtils/Sound.h>
#include <MUtils/OSSupport.h>

//Qt
#include <QReadWriteLock>
#include <QHash>
#include <QResource>
#include <QFileInfo>
#include <QDir>

///////////////////////////////////////////////////////////////////////////////
// BEEP
///////////////////////////////////////////////////////////////////////////////

bool MUtils::Sound::beep(const MUtils::Sound::beep_t &beepType)
{
	switch(beepType)
	{
		case BEEP_NFO: return MessageBeep(MB_ICONASTERISK)    == TRUE; break;
		case BEEP_WRN: return MessageBeep(MB_ICONEXCLAMATION) == TRUE; break;
		case BEEP_ERR: return MessageBeep(MB_ICONHAND)        == TRUE; break;
		default: return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// PLAY SOUND
///////////////////////////////////////////////////////////////////////////////

typedef QHash<const QString, const unsigned char*> SoundDB;

static QReadWriteLock g_sound_lock;
static QScopedPointer<SoundDB> g_sound_db;

static const unsigned char *get_sound_from_cache(const QString &name)
{
	//Try to look-up the sound in the cache first
	QReadLocker readLock(&g_sound_lock);
	if((!g_sound_db.isNull()) && g_sound_db->contains(name))
	{
		return g_sound_db->value(name);
	}

	//Get the write lock now
	readLock.unlock();
	QWriteLocker writeLock(&g_sound_lock);

	//Is sound still not in cache?
	if((!g_sound_db.isNull()) && g_sound_db->contains(name))
	{
		return g_sound_db->value(name);
	}

	//If data not found in cache, try to load from resource!
	QResource resource(QString(":/sounds/%1.wav").arg(name));
	if(resource.isValid())
	{
		if(const unsigned char *data = resource.data())
		{
			if(g_sound_db.isNull())
			{
				g_sound_db.reset(new SoundDB());
			}
			g_sound_db->insert(name, data);
			return data;
		}
	}

	qWarning("Sound effect \"%s\" not found!", MUTILS_UTF8(name));
	return NULL;
}

bool MUtils::Sound::play_sound(const QString &name, const bool &bAsync)
{
	if(!name.isEmpty())
	{
		if(const unsigned char *data = get_sound_from_cache(name))
		{
			return PlaySound(LPCWSTR(data), NULL, (SND_MEMORY | (bAsync ? SND_ASYNC : SND_SYNC))) != FALSE;
		}
	}
	
	return false;
}

bool MUtils::Sound::play_system_sound(const QString &alias, const bool &bAsync)
{
	return PlaySound(MUTILS_WCHR(alias), GetModuleHandle(NULL), (SND_ALIAS | (bAsync ? SND_ASYNC : SND_SYNC))) != FALSE;
}

bool MUtils::Sound::play_sound_file(const QString &library, const unsigned short uiSoundIdx, const bool &bAsync)
{
	bool result = false;

	QFileInfo libraryFile(library);
	if(!libraryFile.isAbsolute())
	{
		const QString &systemDir = MUtils::OS::known_folder(MUtils::OS::FOLDER_SYSTEM_DEF);
		if(!systemDir.isEmpty())
		{
			libraryFile.setFile(QDir(systemDir), libraryFile.fileName());
		}
	}

	if(libraryFile.exists() && libraryFile.isFile())
	{
		if(const HMODULE module = GetModuleHandleW(MUTILS_WCHR(QDir::toNativeSeparators(libraryFile.canonicalFilePath()))))
		{
			result = (PlaySound(MAKEINTRESOURCE(uiSoundIdx), module, (SND_RESOURCE | (bAsync ? SND_ASYNC : SND_SYNC))) != FALSE);
		}
		else if(const HMODULE module = LoadLibraryW(MUTILS_WCHR(QDir::toNativeSeparators(libraryFile.canonicalFilePath()))))
		{
			result = (PlaySound(MAKEINTRESOURCE(uiSoundIdx), module, (SND_RESOURCE | (bAsync ? SND_ASYNC : SND_SYNC))) != FALSE);
			FreeLibrary(module);
		}
	}
	else
	{
		qWarning("PlaySound: File \"%s\" could not be found!", MUTILS_UTF8(libraryFile.absoluteFilePath()));
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
