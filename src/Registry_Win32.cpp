///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2023 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Qt
#include <QStringList>

//Win32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

///////////////////////////////////////////////////////////////////////////////
// INTERNAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

#define ENUM2STR(X,Y) do \
{ \
	static const char *_name = #Y; \
	if((X) == (Y)) return _name; \
} \
while(0)

namespace MUtils
{
	namespace Registry
	{
		static HKEY registry_root(const reg_root_t &rootKey)
		{
			switch(rootKey)
			{
				case root_classes: return HKEY_CLASSES_ROOT;  break;
				case root_user:    return HKEY_CURRENT_USER;  break;
				case root_machine: return HKEY_LOCAL_MACHINE; break;
				default: MUTILS_THROW("Unknown root reg value was specified!");
			}
		}

		static DWORD registry_access(const reg_access_t &access)
		{
			switch(access)
			{
				case access_readonly:  return KEY_READ;               break;
				case access_writeonly: return KEY_WRITE;              break;
				case access_readwrite: return KEY_READ | KEY_WRITE;   break;
				case access_enumerate: return KEY_ENUMERATE_SUB_KEYS; break;
				default: MUTILS_THROW("Unknown access value was specified!");
			}
		}

		static DWORD registry_scope(const reg_scope_t &scope)
		{
			switch (scope)
			{
				case scope_default: return 0;               break;
				case scope_wow_x32: return KEY_WOW64_32KEY; break;
				case scope_wow_x64: return KEY_WOW64_64KEY; break;
				default: MUTILS_THROW("Unknown scope value was specified!");
			}
		}

		static const char* reg_root2str(const reg_root_t &rootKey)
		{
			ENUM2STR(rootKey, root_classes);
			ENUM2STR(rootKey, root_user);
			ENUM2STR(rootKey, root_machine);

			static const char *unknown = "<unknown>";
			return unknown;
		}

		static const char* reg_access2str(const reg_access_t &access)
		{
			ENUM2STR(access, access_readonly);
			ENUM2STR(access, access_writeonly);
			ENUM2STR(access, access_readwrite);
			ENUM2STR(access, access_enumerate);

			static const char *unknown = "<unknown>";
			return unknown;
		}
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
				HKEY  m_hKey;
				DWORD m_access;
				bool  m_isOpen;
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
	if(!(p->m_access & (X))) \
	{ \
		MUTILS_THROW("This operation is not support with current access rights!"); \
	} \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// Registry Key Class
///////////////////////////////////////////////////////////////////////////////

MUtils::Registry::RegistryKey::RegistryKey(const reg_root_t &rootKey, const QString &keyName, const reg_access_t &access, const reg_scope_t &scope)
:
	p(new Internal::RegistryKeyPrivate())
{
	p->m_hKey   = NULL;
	p->m_access = registry_access(access) | registry_scope(scope);
	p->m_isOpen = false;

	p->m_isOpen = (RegCreateKeyEx(registry_root(rootKey), MUTILS_WCHR(keyName), 0, NULL, 0, p->m_access, NULL, &p->m_hKey, NULL) == ERROR_SUCCESS);
	if(!p->m_isOpen)
	{
		qWarning("Failed to open registry key \"%s\"! (rootKey: %s, access: %s)", MUTILS_UTF8(keyName), reg_root2str(rootKey), reg_access2str(access));
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
	delete p;
}

inline bool MUtils::Registry::RegistryKey::isOpen(void)
{
	return p->m_isOpen; 
}

bool MUtils::Registry::RegistryKey::value_write(const QString &valueName, const quint32 &value)
{
	CHECK_STATUS(KEY_WRITE);
	return (RegSetValueEx(p->m_hKey, valueName.isEmpty() ? NULL : MUTILS_WCHR(valueName), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(quint32)) == ERROR_SUCCESS);
}

bool MUtils::Registry::RegistryKey::value_write(const QString &valueName, const QString &value)
{
	CHECK_STATUS(KEY_WRITE);
	return (RegSetValueEx(p->m_hKey, valueName.isEmpty() ? NULL : MUTILS_WCHR(valueName), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.utf16()), (value.length() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS);
}

bool MUtils::Registry::RegistryKey::value_read(const QString &valueName, quint32 &value) const
{
	value = 0;
	DWORD size = sizeof(quint32), type = static_cast<DWORD>(-1);
	CHECK_STATUS(KEY_READ);
	return (RegQueryValueEx(p->m_hKey, valueName.isEmpty() ? NULL : MUTILS_WCHR(valueName), 0, &type, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS) && (type == REG_DWORD);
}

bool MUtils::Registry::RegistryKey::value_read(const QString &valueName, QString &value) const
{
	value = QString();
	wchar_t buffer[2048]; DWORD size = sizeof(wchar_t) * 2048, type = static_cast<DWORD>(-1);
	CHECK_STATUS(KEY_READ);
	if((RegQueryValueEx(p->m_hKey, valueName.isEmpty() ? NULL : MUTILS_WCHR(valueName), 0, &type, reinterpret_cast<BYTE*>(&(buffer[0])), &size) == ERROR_SUCCESS) && ((type == REG_SZ) || (type == REG_EXPAND_SZ)))
	{
		value = QString::fromUtf16(reinterpret_cast<const ushort*>(buffer));
		return true;
	}
	return false;
}

bool MUtils::Registry::RegistryKey::enum_values(QStringList &list) const
{
	wchar_t buffer[2048];
	list.clear();
	CHECK_STATUS(KEY_QUERY_VALUE);
	for(DWORD i = 0; i < UINT_MAX; i++)
	{
		DWORD size = 2048;
		const DWORD ret = RegEnumValue(p->m_hKey, i, buffer, &size, NULL, NULL, NULL, NULL);
		if(ret == ERROR_SUCCESS)
		{
			list << QString::fromUtf16(reinterpret_cast<const ushort*>(buffer));
			continue;
		}
		return (ret == ERROR_NO_MORE_ITEMS);
	}
	return false;
}

bool MUtils::Registry::RegistryKey::enum_subkeys(QStringList &list) const
{
	wchar_t buffer[2048];
	list.clear();
	CHECK_STATUS(KEY_ENUMERATE_SUB_KEYS);
	for(DWORD i = 0; i < UINT_MAX; i++)
	{
		DWORD size = 2048;
		const DWORD ret = RegEnumKeyEx(p->m_hKey, i, buffer, &size, NULL, NULL, NULL, NULL);
		if(ret == ERROR_SUCCESS)
		{
			list << QString::fromUtf16(reinterpret_cast<const ushort*>(buffer));
			continue;
		}
		return (ret == ERROR_NO_MORE_ITEMS);
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Write registry value
 */
bool MUtils::Registry::reg_value_write(const reg_root_t &rootKey, const QString &keyName, const QString &valueName, const quint32 &value, const reg_scope_t &scope)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, access_readwrite, scope);
	if(regKey.isOpen())
	{
		success = regKey.value_write(valueName, value);
	}
	return success;
}

/*
 * Write registry value
 */
bool MUtils::Registry::reg_value_write(const reg_root_t &rootKey, const QString &keyName, const QString &valueName, const QString &value, const reg_scope_t &scope)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, access_readwrite, scope);
	if(regKey.isOpen())
	{
		success = regKey.value_write(valueName, value);
	}
	return success;
}

/*
 * Read registry value
 */
bool MUtils::Registry::reg_value_read(const reg_root_t &rootKey, const QString &keyName, const QString &valueName, quint32 &value, const reg_scope_t &scope)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, access_readonly, scope);
	if(regKey.isOpen())
	{
		success = regKey.value_read(valueName, value);
	}
	else
	{
		value = 0;
	}
	return success;
}

/*
 * Read registry value
 */
bool MUtils::Registry::reg_value_read(const reg_root_t &rootKey, const QString &keyName, const QString &valueName, QString &value, const reg_scope_t &scope)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, access_readonly, scope);
	if(regKey.isOpen())
	{
		success = regKey.value_read(valueName, value);
	}
	else
	{
		value = QString();
	}
	return success;
}

/*
 * Enumerate value names
 */
bool MUtils::Registry::reg_enum_values(const reg_root_t &rootKey, const QString &keyName, QStringList &values, const reg_scope_t &scope)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, access_readonly, scope);
	if(regKey.isOpen())
	{
		success = regKey.enum_values(values);
	}
	else
	{
		values.clear();
	}
	return success;
}

/*
 * Enumerate subkey names
 */
bool MUtils::Registry::reg_enum_subkeys(const reg_root_t &rootKey, const QString &keyName, QStringList &subkeys, const reg_scope_t &scope)
{
	bool success = false;
	RegistryKey regKey(rootKey, keyName, access_enumerate, scope);
	if(regKey.isOpen())
	{
		success = regKey.enum_subkeys(subkeys);
	}
	else
	{
		subkeys.clear();
	}
	return success;
}

/*
 * Check registry key existence
 */
bool MUtils::Registry::reg_key_exists(const reg_root_t &rootKey, const QString &keyName, const reg_scope_t &scope)
{
	HKEY hKey = NULL;
	if(RegOpenKeyEx(registry_root(rootKey), MUTILS_WCHR(keyName), 0, STANDARD_RIGHTS_READ | registry_scope(scope), &hKey) == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return true;
	}
	return false;
}

/*
 * Delete registry key
 */
bool MUtils::Registry::reg_key_delete(const reg_root_t &rootKey, const QString &keyName, const bool &recusrive, const bool &ascend, const reg_scope_t &scope)
{
	bool okay = false;

	if (scope != scope_default)
	{
		MUTILS_THROW("Scope option not currently supported by reg_key_delete() function!");
	}

	if(recusrive)
	{
		okay = (SHDeleteKey(registry_root(rootKey), MUTILS_WCHR(keyName)) == ERROR_SUCCESS);
	}
	else
	{
		okay = (RegDeleteKey(registry_root(rootKey), MUTILS_WCHR(keyName)) == ERROR_SUCCESS);
	}

	if(ascend && okay)
	{
		const int pos = qMax(keyName.lastIndexOf(QLatin1Char('/')), keyName.lastIndexOf(QLatin1Char('\\')));
		if(pos > 0)
		{
			reg_key_delete(rootKey, keyName.left(pos), false, true);
		}
	}

	return okay;
}
