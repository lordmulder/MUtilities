///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2026 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Win32 API
#ifndef _INC_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif //_INC_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// CRITICAL SECTION
///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Internal
	{
		/*
		 * wrapper for native Win32 critical sections
		 */
		class CriticalSection
		{
		public:
			inline CriticalSection(void)
			{
				InitializeCriticalSection(&m_win32criticalSection);
			}

			inline ~CriticalSection(void)
			{
				DeleteCriticalSection(&m_win32criticalSection);
			}

			inline void enter(void)
			{
				EnterCriticalSection(&m_win32criticalSection);
			}

			inline bool tryEnter(void)
			{
				return TryEnterCriticalSection(&m_win32criticalSection);
			}

			inline void leave(void)
			{
				LeaveCriticalSection(&m_win32criticalSection);
			}

		protected:
			CRITICAL_SECTION m_win32criticalSection;
		};

		/*
		 * RAII-style critical section locker
		 */
		class CSLocker
		{
		public:
			inline CSLocker(CriticalSection &criticalSection)
			:
				m_criticalSection(criticalSection)
			{
				m_criticalSection.enter();
				m_locked.ref();
			}

			inline ~CSLocker(void)
			{
				forceUnlock();
			}

			inline void forceUnlock(void)
			{
				if (m_locked.fetchAndStoreOrdered(0) > 0)
				{
					m_criticalSection.leave();
				}
			}
		protected:
			QAtomicInt m_locked;
			CriticalSection &m_criticalSection;
		};
	}
}
