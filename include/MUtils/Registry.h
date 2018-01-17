///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <MUtils/Global.h>

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Registry
	{
		//Regsitry root
		typedef enum
		{
			root_classes = 0,
			root_user    = 1,
			root_machine = 2,
		}
		reg_root_t;

		//Regsitry access
		typedef enum
		{
			access_readonly  = 0,
			access_writeonly = 1,
			access_readwrite = 2,
			access_enumerate = 3
		}
		reg_access_t;

		//Regsitry scope
		typedef enum
		{
			scope_default = 0,
			scope_wow_x32 = 1,
			scope_wow_x64 = 2
		}
		reg_scope_t;

		//Forward declaration
		namespace Internal
		{
			class RegistryKeyPrivate;
		}

		//Registry key class
		class MUTILS_API RegistryKey
		{
		public:
			RegistryKey(const reg_root_t &rootKey, const QString &keyName, const reg_access_t &access, const reg_scope_t &scope = scope_default);
			~RegistryKey(void);

			inline bool isOpen(void);

			bool value_write(const QString &valueName, const quint32 &value);
			bool value_write(const QString &valueName, const QString &value);

			bool value_read(const QString &valueName, quint32 &value) const;
			bool value_read(const QString &valueName, QString &value) const;

			bool enum_values (QStringList &list) const;
			bool enum_subkeys(QStringList &list) const;

		private:
			Internal::RegistryKeyPrivate *const p;
		};

		//Regsitry functions
		MUTILS_API bool reg_value_write (const reg_root_t &rootKey, const QString &keyName, const QString &valueName, const quint32 &value,           const reg_scope_t &scope = scope_default);
		MUTILS_API bool reg_value_write (const reg_root_t &rootKey, const QString &keyName, const QString &valueName, const QString &value,           const reg_scope_t &scope = scope_default);
		MUTILS_API bool reg_value_read  (const reg_root_t &rootKey, const QString &keyName, const QString &valueName, quint32       &value,           const reg_scope_t &scope = scope_default);
		MUTILS_API bool reg_value_read  (const reg_root_t &rootKey, const QString &keyName, const QString &valueName, QString       &value,           const reg_scope_t &scope = scope_default);
		MUTILS_API bool reg_key_exists  (const reg_root_t &rootKey, const QString &keyName,                                                           const reg_scope_t &scope = scope_default);
		MUTILS_API bool reg_key_delete  (const reg_root_t &rootKey, const QString &keyName, const bool &recusrive = true, const bool &ascend = false, const reg_scope_t &scope = scope_default);
		MUTILS_API bool reg_enum_values (const reg_root_t &rootKey, const QString &keyName, QStringList &list,                                        const reg_scope_t &scope = scope_default);
		MUTILS_API bool	reg_enum_subkeys(const reg_root_t &rootKey, const QString &keyName, QStringList &list,                                        const reg_scope_t &scope = scope_default);
	}
}

///////////////////////////////////////////////////////////////////////////////
