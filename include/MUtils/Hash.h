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

#pragma once

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QByteArray>
#include <QFile>

namespace MUtils
{
	namespace Hash
	{
		static const quint16 HASH_BLAKE2_512 = 0x0000U;
		static const quint16 HASH_KECCAK_224 = 0x0100U;
		static const quint16 HASH_KECCAK_256 = 0x0101U;
		static const quint16 HASH_KECCAK_384 = 0x0102U;
		static const quint16 HASH_KECCAK_512 = 0x0103U;

		class MUTILS_API Hash
		{
		public:
			virtual ~Hash(void) {};

			bool update(const quint8 *const data, const quint32 len) { return process(data, len); }
			bool update(const QByteArray &data) { return process(((const quint8*)data.constData()), ((quint32)data.length())); }
			bool update(QFile &file);

			QByteArray digest(const bool bAsHex = true) { return bAsHex ? finalize().toHex() : finalize(); }

		protected:
			Hash(const char* key = NULL) {/*nothing to do*/};
			virtual bool process(const quint8 *const data, const quint32 len) = 0;
			virtual QByteArray finalize(void) = 0;

		private:
			Hash &operator=(const Hash&) { throw "Disabled"; }
			Hash(const Hash&)            { throw "Disabled"; }
		};

		MUTILS_API Hash *create(const quint16 &hashId, const char *const key = NULL);
	}
}
