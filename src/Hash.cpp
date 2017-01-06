///////////////////////////////////////////////////////////////////////////////
// Simple x264 Launcher
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

//MUtils
#include <MUtils/Hash.h>
#include <MUtils/Exception.h>

//Hash Functions
#include "Hash_Keccak.h"
#include "Hash_Blake2.h"

// ==========================================================================
// Abstract Hash Class
// ==========================================================================

bool MUtils::Hash::Hash::update(QFile &file)
{
	while (!file.atEnd())
	{
		const QByteArray data = file.read(4194304i64);
		if (data.isEmpty() || (!update(data)))
		{
			return false;
		}
	}
	return true;
}

// ==========================================================================
// Hash Factory
// ==========================================================================

MUtils::Hash::Hash *MUtils::Hash::create(const quint16 &hashId, const char *const key)
{
	switch (hashId)
	{
	case HASH_KECCAK_224:
		return Keccak::create(Keccak::hb224, key);
	case HASH_KECCAK_256:
		return Keccak::create(Keccak::hb256, key);
	case HASH_KECCAK_384:
		return Keccak::create(Keccak::hb384, key);
	case HASH_KECCAK_512:
		return Keccak::create(Keccak::hb512, key);
	case HASH_BLAKE2_512:
		return new Blake2(key);
	default:
		MUTILS_THROW_FMT("Hash algorithm 0x%02X is unknown!", quint32(hashId));
	}
}
