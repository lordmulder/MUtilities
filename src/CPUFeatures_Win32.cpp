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

//Win32 API
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

//MUtils
#include <MUtils/CPUFeatures.h>
#include <MUtils/OSSupport.h>

//Qt
#include <QLibrary>

MUtils::CPUFetaures::cpu_info_t MUtils::CPUFetaures::detect(void)
{
	const OS::ArgumentMap &args = OS::arguments();
	typedef BOOL (WINAPI *IsWow64ProcessFun)(__in HANDLE hProcess, __out PBOOL Wow64Process);

	cpu_info_t features;
	SYSTEM_INFO systemInfo;
	int CPUInfo[4] = {-1};
	char CPUIdentificationString[0x40];
	char CPUBrandString[0x40];

	memset(&features, 0, sizeof(cpu_info_t));
	memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
	memset(CPUIdentificationString, 0, sizeof(CPUIdentificationString));
	memset(CPUBrandString, 0, sizeof(CPUBrandString));
	
	__cpuid(CPUInfo, 0);
	memcpy(CPUIdentificationString, &CPUInfo[1], sizeof(int));
	memcpy(CPUIdentificationString + 4, &CPUInfo[3], sizeof(int));
	memcpy(CPUIdentificationString + 8, &CPUInfo[2], sizeof(int));
	features.intel = (_stricmp(CPUIdentificationString, "GenuineIntel") == 0);
	strncpy_s(features.vendor, 0x40, CPUIdentificationString, _TRUNCATE);

	if(CPUInfo[0] >= 1)
	{
		__cpuid(CPUInfo, 1);
		if(CPUInfo[3] & 0x00800000) features.features |= FLAG_MMX;
		if(CPUInfo[3] & 0x02000000) features.features |= FLAG_SSE;
		if(CPUInfo[3] & 0x04000000) features.features |= FLAG_SSE2;
		if(CPUInfo[2] & 0x00000001) features.features |= FLAG_SSE3;
		if(CPUInfo[2] & 0x00000200) features.features |= FLAG_SSSE3;
		if(CPUInfo[2] & 0x00080000) features.features |= FLAG_SSE4;
		if(CPUInfo[2] & 0x00100000) features.features |= FLAG_SSE42;
		features.stepping = CPUInfo[0] & 0xf;
		features.model    = ((CPUInfo[0] >> 4) & 0xf) + (((CPUInfo[0] >> 16) & 0xf) << 4);
		features.family   = ((CPUInfo[0] >> 8) & 0xf) + ((CPUInfo[0] >> 20) & 0xff);
	}

	__cpuid(CPUInfo, 0x80000000);
	int nExIds = qMax<int>(qMin<int>(CPUInfo[0], 0x80000004), 0x80000000);

	for(int i = 0x80000002; i <= nExIds; ++i)
	{
		__cpuid(CPUInfo, i);
		switch(i)
		{
		case 0x80000002:
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
			break;
		case 0x80000003:
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
			break;
		case 0x80000004:
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
			break;
		}
	}

	strncpy_s(features.brand, 0x40, CPUBrandString, _TRUNCATE);

	if(strlen(features.brand)  < 1) strncpy_s(features.brand,  0x40, "Unknown", _TRUNCATE);
	if(strlen(features.vendor) < 1) strncpy_s(features.vendor, 0x40, "Unknown", _TRUNCATE);

#if (!(defined(_M_X64) || defined(_M_IA64)))
	QLibrary Kernel32Lib("kernel32.dll");
	if(IsWow64ProcessFun IsWow64ProcessPtr = (IsWow64ProcessFun) Kernel32Lib.resolve("IsWow64Process"))
	{
		BOOL x64flag = FALSE;
		if(IsWow64ProcessPtr(GetCurrentProcess(), &x64flag))
		{
			features.x64 = (x64flag == TRUE);
		}
	}
#else
	features.x64 = true;
#endif

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

	bool flag = false;
	if(args.contains("force-cpu-no-64bit")) { flag = true; features.x64 = false; }
	if(args.contains("force-cpu-no-sse"  )) { flag = true; features.features &= (~(FLAG_SSE | FLAG_SSE2 | FLAG_SSE3 | FLAG_SSSE3 | FLAG_SSE4 | FLAG_SSE42)); }
	if(args.contains("force-cpu-no-intel")) { flag = true; features.intel = false; }

	if(flag)
	{
		qWarning("CPU flags overwritten by user-defined parameters. Take care!\n");
	}

	return features;
}
