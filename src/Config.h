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

#ifndef MUTILS_INC_CONFIG
#error Please do *not* include CONFIG.H directly!
#endif

///////////////////////////////////////////////////////////////////////////////
// MUtilities Version Info
///////////////////////////////////////////////////////////////////////////////

#define VER_MUTILS_MAJOR					1
#define VER_MUTILS_MINOR_HI					1
#define VER_MUTILS_MINOR_LO					7
#define VER_MUTILS_PATCH					0

///////////////////////////////////////////////////////////////////////////////
// Helper macros (aka: having fun with the C pre-processor)
///////////////////////////////////////////////////////////////////////////////

#define VER_MUTILS_STR_HLP1(X)			#X
#define VER_MUTILS_STR_HLP2(W,X,Y,Z)	VER_MUTILS_STR_HLP1(v##W.X##Y-Z)
#define VER_MUTILS_STR_HLP3(W,X,Y,Z)	VER_MUTILS_STR_HLP2(W,X,Y,Z)
#define VER_MUTILS_STR					VER_MUTILS_STR_HLP3(VER_MUTILS_MAJOR,VER_MUTILS_MINOR_HI,VER_MUTILS_MINOR_LO,VER_MUTILS_PATCH)
