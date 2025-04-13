///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
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

/**
* @file
* @brief This file contains function for cryptographic hash computation
*
* Call the MUtils::Hash::create() function to create an instance of the desired hash function. All Hash functions implement the MUtils::Hash::Hash interface.
*/

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
		static const quint16 HASH_BLAKE2_512 = 0x0000U;	///< \brief Hash algorithm identifier \details Use [BLAKE2](https://blake2.net/) hash algorithm, with a length of 512-Bit.
		static const quint16 HASH_KECCAK_224 = 0x0100U;	///< \brief Hash algorithm identifier \details Use [Keccak](http://keccak.noekeon.org/) (SHA-3) hash algorithm, with a length of 224-Bit.
		static const quint16 HASH_KECCAK_256 = 0x0101U;	///< \brief Hash algorithm identifier \details Use [Keccak](http://keccak.noekeon.org/) (SHA-3) hash algorithm, with a length of 256-Bit.
		static const quint16 HASH_KECCAK_384 = 0x0102U;	///< \brief Hash algorithm identifier \details Use [Keccak](http://keccak.noekeon.org/) (SHA-3) hash algorithm, with a length of 384-Bit.
		static const quint16 HASH_KECCAK_512 = 0x0103U;	///< \brief Hash algorithm identifier \details Use [Keccak](http://keccak.noekeon.org/) (SHA-3) hash algorithm, with a length of 512-Bit.

		/**
		* \brief This abstract class specifies the generic interface for all support hash algorithms.
		*
		* In order to compute a hash value (digest) call the the Hash::update() function repeatedly until all input data (i.e. the complete "message") has been processed. Then call the Hash::digest() function to retrieve the final hash value.
		*
		* All overloads of the Hash::update() function may be called in an interleaved fashion as needed.
		*
		* This class is **not** thread-safe, i.e. do **not** call the *same* Hash instance from difference threads, unless serialization is ensured (e.g. by means of a Mutex). It is safe, however, to call different *different* Hash instances from difference threads concurrently.
		*/
		class MUTILS_API Hash
		{
		public:
			virtual ~Hash(void) {};

			/**
			* \brief Process the next chunk of input data
			*
			* Updates the internal state of the hash function by processing the next chunk of input that. Can be called repeatedly, until until all input data has been processed.
			*
			* \param data A read-only pointer to the memory buffer holding the input data to be processed.
			*
			* \param len The length of the input data, in bytes. The `data` parameter must be pointing to a memory buffer that is at least `len` bytes in size.
			*
			* \return The function returns `true`, if the input data was processed successfully; otherwise it returns `false`.
			*/
			bool update(const quint8 *const data, const quint32 len) { return process(data, len); }

			/**
			* \brief Process the next chunk of input data
			*
			* Updates the internal state of the hash function by processing the next chunk of input that. Can be called repeatedly, until until all input data has been processed.
			*
			* \param data A read-only reference to a QByteArray object holding the input data to be processed. All bytes in the QByteArray object will be processed.
			*
			* \return The function returns `true`, if the input data was processed successfully; otherwise it returns `false`.
			*/
			bool update(const QByteArray &data) { return process(((const quint8*)data.constData()), ((quint32)data.length())); }

			/**
			* \brief Process the next chunk of input data
			*
			* Updates the internal state of the hash function by processing the next chunk of input that. Can be called repeatedly, until until all input data has been processed.
			*
			* \param data A reference to a QFile object. The QFile object must be open and readable. All data from the current file position to the end of the file will be processed.
			*
			* \return The function returns `true`, if all data in the file was processed successfully; otherwise (e.g. in case of file read errors) it returns `false`.
			*/
			bool update(QFile &file);

			/**
			* \brief Retrieve the hash value
			*
			* This function is used to retrieve the final hash value (digest), after all input data has been processed successfully.
			*
			* \param bAsHex If set to `true`, the function returns the hash value as a Hexadecimal-encoded ASCII string; if set to `false`, the function returns the hash value as "raw" bytes.
			*
			* \return The function returns a QByteArray object holding the final hash value (digest). The format depends on the `bAsHex` parameter.
			*/
			QByteArray digest(const bool bAsHex = true) { return bAsHex ? finalize().toHex() : finalize(); }

		protected:
			Hash(const char* /*key*/ = NULL) {/*nothing to do*/};
			virtual bool process(const quint8 *const data, const quint32 len) = 0;
			virtual QByteArray finalize(void) = 0;

		private:
			MUTILS_NO_COPY(Hash);
		};

		/**
		* \brief Create instance of a hash function
		*
		* This function is used to create a new instance of the desired hash function. All Hash functions implement the MUtils::Hash::Hash interface. The caller is responsible for destroying the returned MUtils::Hash::Hash object.
		*
		* \param hashId Specifies the desired hash function. This must be a valid hash algorithm identifier, as defined in the `Hash.h` header file.
		*
		* \param key Specifies on optional key that is used to "seed" the hash function. If a key is given, it must be a NULL-terminated string of appropriate length. If set to `NULL`, the optional seeding is skipped.
		*
		* \return Returns a pointer to a new MUtils::Hash::Hash object that implements the desired hash function. The function throws if an invalid algorithm identifier was specified!
		*/
		MUTILS_API Hash *create(const quint16 &hashId, const char *const key = NULL);
	}
}
