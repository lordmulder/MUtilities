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

//Win32 API
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

//MUtils
#include <MUtils/CPUFeatures.h>
#include <MUtils/OSSupport.h>
#include "Utils_Win32.h"

#define MY_CPUID(X,Y) __cpuid(((int*)(X)), ((int)(Y)))
#define CHECK_VENDOR(X,Y,Z) (_stricmp((X), (Y)) ? 0U : (Z));
#define CHECK_FLAG(X,Y,Z) (((X) & (Y)) ? (Z) : 0U)

MUtils::CPUFetaures::cpu_info_t MUtils::CPUFetaures::detect(void)
{
	const OS::ArgumentMap &args = OS::arguments();
	typedef BOOL (WINAPI *IsWow64ProcessFun)(__in HANDLE hProcess, __out PBOOL Wow64Process);
	static const quint32 FLAGS_X64 = (FLAG_MMX | FLAG_SSE | FLAG_SSE2);

	cpu_info_t  features;
	SYSTEM_INFO systemInfo;
	uint32_t    cpuInfo[4];

	//Initialize variables to zero
	memset(&features,   0, sizeof(cpu_info_t));
	memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
	memset(cpuInfo,     0, sizeof(cpuInfo));

	//Detect the CPU identifier string
	MY_CPUID(&cpuInfo[0], 0);
	const uint32_t max_basic_cap = cpuInfo[0];
	memcpy(&features.idstr[0U * sizeof(uint32_t)], &cpuInfo[1], sizeof(uint32_t));
	memcpy(&features.idstr[1U * sizeof(uint32_t)], &cpuInfo[3], sizeof(uint32_t));
	memcpy(&features.idstr[2U * sizeof(uint32_t)], &cpuInfo[2], sizeof(uint32_t));
	features.idstr[3U * sizeof(uint32_t)] = '\0';
	features.vendor |= CHECK_VENDOR(features.idstr, "GenuineIntel", VENDOR_INTEL);
	features.vendor |= CHECK_VENDOR(features.idstr, "AuthenticAMD", VENDOR_AMD);

	//Detect the CPU model and feature flags
	if(max_basic_cap >= 1)
	{
		MY_CPUID(&cpuInfo[0], 1);
		features.features |= CHECK_FLAG(cpuInfo[3], 0x00008000, FLAG_CMOV);
		features.features |= CHECK_FLAG(cpuInfo[3], 0x00800000, FLAG_MMX);
		features.features |= CHECK_FLAG(cpuInfo[3], 0x02000000, FLAG_SSE);
		features.features |= CHECK_FLAG(cpuInfo[3], 0x04000000, FLAG_SSE2);
		features.features |= CHECK_FLAG(cpuInfo[2], 0x00000001, FLAG_SSE3);
		features.features |= CHECK_FLAG(cpuInfo[2], 0x00000200, FLAG_SSSE3);
		features.features |= CHECK_FLAG(cpuInfo[2], 0x00080000, FLAG_SSE41);
		features.features |= CHECK_FLAG(cpuInfo[2], 0x00100000, FLAG_SSE42);

		//Check for AVX
		if ((cpuInfo[2] & 0x18000000) == 0x18000000)
		{
			if((_xgetbv(0) & 0x6ui64) == 0x6ui64) /*AVX requires OS support!*/
			{
				features.features |= FLAG_AVX;
				features.features |= CHECK_FLAG(cpuInfo[2], 0x00001000, FLAG_FMA3);
			}
		}

		//Compute the CPU stepping, model and family
		features.stepping = cpuInfo[0] & 0xf;
		features.model    = ((cpuInfo[0] >> 4) & 0xf) + (((cpuInfo[0] >> 16) & 0xf) << 4);
		features.family   = ((cpuInfo[0] >> 8) & 0xf) + ((cpuInfo[0] >> 20) & 0xff);
	}

	//Detect extended feature flags
	if (max_basic_cap >= 7)
	{
		MY_CPUID(&cpuInfo[1], 7);
		if (features.features & FLAG_AVX)
		{
			features.features |= CHECK_FLAG(cpuInfo[2], 0x00000020, FLAG_AVX2);
		}
	}

	//Read the CPU "brand" string
	if (max_basic_cap > 0)
	{
		MY_CPUID(&cpuInfo[0], 0x80000000);
		const uint32_t max_extended_cap = qBound(0x80000000, cpuInfo[0], 0x80000004);
		if (max_extended_cap >= 0x80000001)
		{
			MY_CPUID(&cpuInfo[0], 0x80000001);
			features.features |= CHECK_FLAG(cpuInfo[2], 0x00000020, FLAG_LZCNT);
			for (uint32_t i = 0x80000002; i <= max_extended_cap; ++i)
			{
				MY_CPUID(&cpuInfo[0], i);
				memcpy(&features.brand[(i - 0x80000002) * sizeof(cpuInfo)], &cpuInfo[0], sizeof(cpuInfo));
			}
			features.brand[sizeof(features.brand) - 1] = '\0';
		}
	}

	//Detect 64-Bit processors
#if (!(defined(_M_X64) || defined(_M_IA64)))
	const IsWow64ProcessFun isWow64ProcessPtr = MUtils::Win32Utils::resolve<IsWow64ProcessFun>(QLatin1String("kernel32"), QLatin1String("IsWow64Process"));
	if(isWow64ProcessPtr)
	{
		BOOL x64flag = FALSE;
		if(isWow64ProcessPtr(GetCurrentProcess(), &x64flag))
		{
			if (x64flag)
			{
				features.x64 = true;
				features.features |= FLAGS_X64; /*x86_64 implies SSE2*/
			}
		}
	}
#else
	features.x64 = true;
	features.features |= FLAGS_X64;
#endif

	//Make sure that (at least) the MMX flag has been set!
	if (!(features.features & FLAG_MMX))
	{
		qWarning("Warning: CPU does not seem to support MMX. Take care!\n");
		features.features = 0;
	}

	//Count the number of available(!) CPU cores
	DWORD_PTR procAffinity, sysAffinity;
	if(GetProcessAffinityMask(GetCurrentProcess(), &procAffinity, &sysAffinity))
	{
		for(DWORD_PTR mask = 1; mask; mask <<= 1)
		{
			features.count += ((sysAffinity & mask) ? (1) : (0));
		}
	}
	if(features.count < 1)
	{
		GetNativeSystemInfo(&systemInfo);
		features.count = qBound(1UL, systemInfo.dwNumberOfProcessors, 64UL);
	}

	//Apply manual CPU overwrites
	bool userFlag = false;
	if (args.contains(QLatin1String("cpu-no-simd")))   { userFlag = true; features.features = 0U; }
	if (args.contains(QLatin1String("cpu-no-vendor"))) { userFlag = true; features.vendor   = 0U; }
	if (args.contains(QLatin1String("cpu-no-x64")))    { userFlag = true; features.x64      = 0U; }
	if(userFlag)
	{
		qWarning("CPU flags overwritten by user-defined parameters. Take care!\n");
	}

	return features;
}
