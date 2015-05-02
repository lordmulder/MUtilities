///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <MUtils/Registry.h>
#include <MUtils/Exception.h>

//Win32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

///////////////////////////////////////////////////////////////////////////////

static HKEY registry_root(const int &rootKey)
{
	switch(rootKey)
	{
		case MUtils::Registry::root_classes: return HKEY_CLASSES_ROOT;  break;
		case MUtils::Registry::root_user:    return HKEY_CURRENT_USER;  break;
		case MUtils::Registry::root_machine: return HKEY_LOCAL_MACHINE; break;
		default: MUTILS_THROW("Unknown root reg value was specified!");
	}
}

///////////////////////////////////////////////////////////////////////////////
// RegistryKeyPrivate Key Class
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Registry
	{
		namespace Internal
		{
			class RegistryKeyPrivate
			{
				friend class MUtils::Registry::RegistryKey;

			private:
				HKEY m_hKey;
				bool m_readOnly;
				bool m_isOpen;
			};
		}
	}
}

#define CHECK_STATUS(X) do \
{ \
	if(!p->m_isOpen) \
	{ \
		MUTILS_THROW("Cannot read from or write to a key is not currently open!"); \
	} \
	if(p->m_readOnly != (X)) \
	{ \
		MUTILS_THROW("Cannot write to read-only key or read from write-only key!"); \
	} \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// Registry Key Class
///////////////////////////////////////////////////////////////////////////////

MUtils::Registry::RegistryKey::RegistryKey(const int &rootKey, const QString &keyName, const bool &readOnly)
:
	p(new Internal::RegistryKeyPrivate())
{
	p->m_hKey = NULL;
	p->m_readOnly = readOnly;
	p->m_isOpen = false;

	p->m_isOpen = (RegCreateKeyEx(registry_root(rootKey), MUTILS_WCHR(keyName), 0, NULL, 0, p->m_readOnly ? KEY_READ : KEY_WRITE, NULL, &p->m_hKey, NULL) == ERROR_SUCCESS);
	if(!p->m_isOpen)
	{
		qWarning("Failed to open registry key!");
	}
}

MUtils::Registry::RegistryKey::~RegistryKey(void)
{
	if(p->m_isOpen)
	{
		CloseHandle(p->m_hKey);
		p->m_hKey = NULL;
		p->m_isOpen = false;
	}
}

inline bool MUtils::Registry::RegistryKey::isOpen(void)
{
	return p->m_isOpen; 
}

bool MUtils::Registry::RegistryKey::value_write(const QString &valueName, const quint32 &value)
{
	CHECK_STATUS(false);
	return (RegSetValueEx(p->m_hKey, valueName.isEmpty() ? NULL : MUTILS_WCHR(valueName), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(quint32)) == ERROR_SUCCESS);
}

bool MUtils::Registry::RegistryKey::value_write(const QString &valueName, const QString &value)
{
	CHECK_STATUS(false);
	return (RegSetValueEx(p->m_hKey, valueName.isEmpty() ? NULL : MUTILS_WCHR(valueName), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.utf16()), (value.length() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS);
}

bool MUtils::Registry::RegistryKey::value_read(const QString &valueName, quint32 &value) const
{
	DWORD size = sizeof(quint32), type = -1;
	CHECK_STATUS(false);
	return (RegQueryValueEx(p->m_hKey, valueName.isEmpty() ? NULL : MUTILS_WCHR(valueName), 0, &type, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS) && (type == REG_DWORD);
}

bool MUtils::Registry::RegistryKey::value_read(const QString &valueName, QString &value) const
{
	wchar_t buffer[2048];
	DWORD size = sizeof(wchar_t) * 2048, type = -1;
	CHECK_STATUS(false);
	if((RegQueryValueEx(p->m_hKey, valueName.isEmpty() ? NULL : MUTILS_WCHR(valueName), 0, &type, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS) && ((type == REG_SZ) || (type == REG_EXPAND_SZ)))
	{
		value = QString::fromUtf16(reinterpret_cast<const ushort*>(buffer));
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Write registry value
 */
bool MUtils::Registry::reg_value_write(const int &rootKey, const QString &keyName, const QString &valueName, const quint32 &value)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, false);
	if(regKey.isOpen())
	{
		success = regKey.value_write(valueName, value);
	}
	return success;
}

/*
 * Write registry value
 */
bool MUtils::Registry::reg_value_write(const int &rootKey, const QString &keyName, const QString &valueName, const QString &value)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, false);
	if(regKey.isOpen())
	{
		success = regKey.value_write(valueName, value);
	}
	return success;
}

/*
 * Read registry value
 */
bool MUtils::Registry::reg_value_read(const int &rootKey, const QString &keyName, const QString &valueName, quint32 &value)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, true);
	if(regKey.isOpen())
	{
		success = regKey.value_read(valueName, value);
	}
	return success;
}

/*
 * Read registry value
 */
bool MUtils::Registry::reg_value_read(const int &rootKey, const QString &keyName, const QString &valueName, QString &value)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, true);
	if(regKey.isOpen())
	{
		success = regKey.value_read(valueName, value);
	}
	return success;
}

/*
 * Delete registry key
 */
bool MUtils::Registry::reg_key_delete(const int &rootKey, const QString &keyName)
{
	return (SHDeleteKey(registry_root(rootKey), MUTILS_WCHR(keyName)) == ERROR_SUCCESS);
}
