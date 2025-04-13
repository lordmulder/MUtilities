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

#pragma once

//MUtils
#include <MUtils/Global.h>

///////////////////////////////////////////////////////////////////////////////

namespace MUtils
{
	namespace Sound
	{
		enum beep_t
		{
			BEEP_NFO = 0,
			BEEP_WRN = 1,
			BEEP_ERR = 2
		};

		//Simple beep
		MUTILS_API bool beep(const beep_t &beepType);

		//Play built-in sound by name
		MUTILS_API bool play_sound(const QString &name, const bool &bAsync);

		//Play system sound by name
		MUTILS_API bool play_system_sound(const QString &alias, const bool &bAsync);

		//Play sound from file
		MUTILS_API bool play_sound_file(const QString &library, const unsigned short uiSoundIdx, const bool &bAsync);
	}
}

///////////////////////////////////////////////////////////////////////////////
