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

/**
* @file
* @brief This file contains function for detecting information about the CPU
*
* Call the MUtils::CPUFetaures::detect() to detect information about the processor, which will be returned in a `MUtils::CPUFetaures::cpu_info_t` struct.
*/

#pragma once

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QStringList>

namespace MUtils
{
	/**
	* \brief This namespace contains functions and constants for detecting CPU information
	*
	* Call the detect() to detect information about the processor, which will be returned in a `cpu_info_t` struct.
	*/
	namespace CPUFetaures
	{
		// CPU vendor flag
		static const quint8 VENDOR_INTEL = 0x01U;	///< \brief CPU vendor flag \details Indicates that the CPU's vendor is *Intel*
		static const quint8 VENDOR_AMD = 0x02U;	///< \brief CPU vendor flag \details Indicates that the CPU's vendor is *AMD*

		// CPU feature flag
		static const quint32 FLAG_CMOV  = 0x001U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *CMOV* instruction
		static const quint32 FLAG_MMX   = 0x002U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *MMX* instruction set extension
		static const quint32 FLAG_SSE   = 0x004U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *SSE* instruction set extension
		static const quint32 FLAG_SSE2  = 0x008U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *SSE2* instruction set extension
		static const quint32 FLAG_SSE3  = 0x010U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *SSE3* instruction set extension
		static const quint32 FLAG_SSSE3 = 0x020U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *SSSE3* instruction set extension
		static const quint32 FLAG_SSE41 = 0x040U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *SSE4.1* instruction set extension
		static const quint32 FLAG_SSE42 = 0x080U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *SSE4.2* instruction set extension
		static const quint32 FLAG_AVX   = 0x100U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *AVX* instruction set extension
		static const quint32 FLAG_AVX2  = 0x200U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *AVX2* instruction set extension
		static const quint32 FLAG_FMA3  = 0x400U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *FMA3* instruction
		static const quint32 FLAG_LZCNT = 0x800U;	///< \brief CPU feature flag \details Indicates that the CPU supports the *LZCNT* instruction

		/**
		* \brief Struct to hold information about the CPU. See `_cpu_info_t` for details!
		*/
		typedef struct _cpu_info_t
		{
			quint32 family;		///< CPU *family* indicator, which specifies the processor "generation" to which the CPU belongs
			quint32 model;		///< CPU *model* indicator, which is used to distinguish processor "variants" within a generation
			quint32 stepping;	///< CPU *stepping* indicator, which is used to distinguish "revisions" of a certain processor model
			quint32 count;		///< The number of available (logical) processors
			quint32 features;	///< CPU *feature* flags, indicating suppoprt for extended instruction sets; all flags are OR-combined
			bool x64;			///< Indicates that the processor and the operating system support 64-Bit (AMD64/EM64T)
			quint8 vendor;		///< CPU *vendor* flag; might be zero, if vendor is unknown
			char idstr[13];		///< CPU *identifier* string, exactly 12 characters (e.g. "GenuineIntel" or "AuthenticAMD")
			char brand[48];		///< CPU *brand* string, up to 48 characters (e.g. "Intel(R) Core(TM) i7-6700K CPU @ 4.00GHz")
		}
		cpu_info_t;

		/**
		* \brief Detect processor information
		*
		* Detects information about the CPU on which the application is running, including CPU vendor, identifier string, feature flags (MMX, SSE, AVX, etc) as well as the CPU core count.
		*
		* \return The function returns a `cpu_info_t` struct containing the detected information about the CPU.
		*/
		MUTILS_API cpu_info_t detect(void);
	}
}
