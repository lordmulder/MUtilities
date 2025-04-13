///////////////////////////////////////////////////////////////////////////////
// Simple x264 Launcher
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
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

/*
   BLAKE2 reference source code package - reference C implementations

   Written in 2012 by Samuel Neves <sneves@dei.uc.pt>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/

#include "Hash_Blake2.h"

//MUtils
#include <MUtils/Exception.h>

//Internal
#include "3rd_party/blake2/include/blake2.h"

#include <malloc.h>
#include <string.h>
#include <stdexcept>

static const size_t HASH_SIZE = 64;

class MUtils::Hash::Blake2_Context
{
	friend Blake2;

	Blake2_Context(void)
	{
		if(!(state = (MUtils::Hash::Internal::Blake2Impl::blake2b_state*) _aligned_malloc(sizeof(MUtils::Hash::Internal::Blake2Impl::blake2b_state), HASH_SIZE)))
		{
			MUTILS_THROW("Aligend malloc has failed!");
		}
		memset(state, 0, sizeof(MUtils::Hash::Internal::Blake2Impl::blake2b_state));
	}

	~Blake2_Context(void)
	{
		memset(state, 0, sizeof(MUtils::Hash::Internal::Blake2Impl::blake2b_state));
		_aligned_free(state);
	}

private:
	MUtils::Hash::Internal::Blake2Impl::blake2b_state *state;
};

MUtils::Hash::Blake2::Blake2(const char *const key)
:
	m_context(new Blake2_Context()),
	m_finalized(false)
{
	if(key && key[0])
	{
		blake2b_init_key(m_context->state, HASH_SIZE, key, (uint8_t)strlen(key));
	}
	else
	{
		blake2b_init(m_context->state, HASH_SIZE);
	}
}

MUtils::Hash::Blake2::~Blake2(void)
{
	delete m_context;
}

bool MUtils::Hash::Blake2::process(const quint8 *const data, const quint32 len)
{
	if(m_finalized)
	{
		MUTILS_THROW("BLAKE2 was already finalized!");
	}

	if(data && (len > 0))
	{
		if(blake2b_update(m_context->state, data, len) != 0)
		{
			MUTILS_THROW("BLAKE2 internal error!");
		}
	}

	return true;
}

QByteArray MUtils::Hash::Blake2::finalize(void)
{
	QByteArray result(HASH_SIZE, '\0');
	if(blake2b_final(m_context->state, (uint8_t*) result.data(), result.size()) != 0)
	{
		MUTILS_THROW("BLAKE2 internal error!");
	}
	m_finalized = true;
	return result;
}
