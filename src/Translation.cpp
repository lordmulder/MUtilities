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

//MUtils
#include <MUtils/Translation.h>

//Qt
#include <QPair>
#include <QReadWriteLock>
#include <QMap>
#include <QStringList>
#include <QTranslator>
#include <QFileInfo>
#include <QApplication>

//////////////////////////////////////////////////////////////////////////////////
// TYPES
//////////////////////////////////////////////////////////////////////////////////

typedef QPair<QString,QString> translation_info_t;
typedef QPair<quint32,quint32> translation_data_t;

typedef QPair<translation_info_t, translation_data_t> translation_entry_t;
typedef QMap<QString, translation_entry_t>            translation_store_t;

#define MAKE_ENTRY(NAME,PATH,SYSID,CNTRY) \
	qMakePair(qMakePair((NAME),(PATH)),qMakePair((SYSID),(CNTRY)))

//////////////////////////////////////////////////////////////////////////////////
// TRANSLATIONS STORE
//////////////////////////////////////////////////////////////////////////////////

static QReadWriteLock                      g_translation_lock;
static QScopedPointer<translation_store_t> g_translation_data;
static QScopedPointer<QTranslator>         g_translation_inst;

//////////////////////////////////////////////////////////////////////////////////
// CONSTANT
//////////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Translation
	{
		const char *const DEFAULT_LANGID = "en";
	}
}

//////////////////////////////////////////////////////////////////////////////////
// REGISTER TRANSLATION
//////////////////////////////////////////////////////////////////////////////////

bool MUtils::Translation::insert(const QString &langId, const QString &qmFile, const QString &langName, const quint32 &systemId, const quint32 &country)
{
	QWriteLocker writeLockTranslations(&g_translation_lock);

	const QString key = langId.simplified().toLower();
	if(key.isEmpty() || qmFile.isEmpty() || langName.isEmpty() || (systemId < 1))
	{
		return false;
	}

	if(g_translation_data.isNull())
	{
		g_translation_data.reset(new translation_store_t());
	}

	if(g_translation_data->contains(key))
	{
		qWarning("Translation store already contains entry for '%s', going to replace!", MUTILS_UTF8(key));
	}

	g_translation_data->insert(key, MAKE_ENTRY(langName, qmFile, systemId, country));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
// GET TRANSLATION INFO
//////////////////////////////////////////////////////////////////////////////////

int MUtils::Translation::enumerate(QStringList &list)
{
	QReadLocker readLockTranslations(&g_translation_lock);

	if(g_translation_data.isNull())
	{
		list.clear();
		return -1;
	}

	list.swap(g_translation_data->keys());
	return list.count();
}

QString MUtils::Translation::get_name(const QString &langId)
{
	QReadLocker readLockTranslations(&g_translation_lock);

	const QString key = langId.simplified().toLower();
	if(key.isEmpty() || g_translation_data.isNull() || (!g_translation_data->contains(key)))
	{
		return QString();
	}

	return (*g_translation_data)[key].first.first;
}

quint32 MUtils::Translation::get_sysid(const QString &langId)
{
	QReadLocker readLockTranslations(&g_translation_lock);

	const QString key = langId.simplified().toLower();
	if(key.isEmpty() || g_translation_data.isNull() || (!g_translation_data->contains(key)))
	{
		return 0;
	}

	return (*g_translation_data)[key].second.first;
}

quint32 MUtils::Translation::get_country(const QString &langId)
{
	QReadLocker readLockTranslations(&g_translation_lock);
	const QString key = langId.simplified().toLower();
	if(key.isEmpty() || g_translation_data.isNull() || (!g_translation_data->contains(key)))
	{
		return 0;
	}

	return (*g_translation_data)[key].second.second;
}

//////////////////////////////////////////////////////////////////////////////////
// INSTALL TRANSLATION
//////////////////////////////////////////////////////////////////////////////////

bool MUtils::Translation::install_translator(const QString &langId)
{
	QReadLocker readLockTranslations(&g_translation_lock);

	const QString key = langId.simplified().toLower();
	if(key.isEmpty() || g_translation_data.isNull() || (!g_translation_data->contains(key)))
	{
		return false;
	}

	const QString qmFile = (*g_translation_data)[key].first.second;
	readLockTranslations.unlock();
	return install_translator_from_file(qmFile);
}

bool MUtils::Translation::install_translator_from_file(const QString &qmFile)
{
	QWriteLocker writeLock(&g_translation_lock);

	if(g_translation_inst.isNull())
	{
		g_translation_inst.reset(new QTranslator());
	}

	if(qmFile.isEmpty())
	{
		QApplication::removeTranslator(g_translation_inst.data());
		return true;
	}

	const QFileInfo qmFileInfo(qmFile);
	if(!(qmFileInfo.exists() && qmFileInfo.isFile()))
	{
		qWarning("Translation file not found:\n\"%s\"", MUTILS_UTF8(qmFile));
		return false;
	}

	const QString qmPath = QFileInfo(qmFile).canonicalFilePath();
	if(!qmPath.isEmpty())
	{
		QApplication::removeTranslator(g_translation_inst.data());
		if(g_translation_inst->load(qmPath))
		{
			QApplication::installTranslator(g_translation_inst.data());
			return true;
		}
	}

	qWarning("Failed to load translation:\n\"%s\"",  MUTILS_UTF8(qmFile));
	return false;
}
