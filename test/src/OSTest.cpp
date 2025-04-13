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

#include "MUtilsTest.h"

//MUtils
#include <MUtils/OSSupport.h>

//Qt
#include <QSet>
#include <QVector>

//Win32
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

//===========================================================================
// TESTBED CLASS
//===========================================================================

class OSTest : public Testbed
{
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

//===========================================================================
// TEST METHODS
//===========================================================================


static QString expandEnvVars(const wchar_t *text)
{
#ifdef _WINDOWS_
	const DWORD size = ExpandEnvironmentStringsW(text, NULL, 0);
	QVector<wchar_t> buffer(size + 1);
	const DWORD result = ExpandEnvironmentStringsW(text, buffer.data(), buffer.size());
	if ((result > 0U) || (result <= size))
	{
		return MUTILS_QSTR(buffer.data());
	}
	return QString();
#else
	#error "Function expandEnvVars() not implemented!"
#endif
}

//-----------------------------------------------------------------
// KnownFolder
//-----------------------------------------------------------------

#define TEST_KNOWN_FOLDER(X,Y) do \
{ \
	const QString path = MUtils::OS::known_folder((MUtils::OS::##X)); \
	/*_putws(MUTILS_WCHR(path));*/ \
	ASSERT_FALSE(path.isEmpty()); \
	const QString expected = QDir::fromNativeSeparators(expandEnvVars((Y))); \
	ASSERT_STRCASEEQ(MUTILS_UTF8(path), MUTILS_UTF8(expected)); \
} \
while(0)

TEST_F(OSTest, KnownFolder01) { TEST_KNOWN_FOLDER(FOLDER_PROFILE_USER,  L"%USERPROFILE%");           }
TEST_F(OSTest, KnownFolder02) { TEST_KNOWN_FOLDER(FOLDER_PROFILE_PUBL,  L"%PUBLIC%");                }
TEST_F(OSTest, KnownFolder03) { TEST_KNOWN_FOLDER(FOLDER_APPDATA_ROAM,  L"%APPDATA%");               }
TEST_F(OSTest, KnownFolder04) { TEST_KNOWN_FOLDER(FOLDER_APPDATA_LOCA,  L"%LOCALAPPDATA%");          }
TEST_F(OSTest, KnownFolder05) { TEST_KNOWN_FOLDER(FOLDER_DOCS_USER,     L"%USERPROFILE%/Documents"); }
TEST_F(OSTest, KnownFolder06) { TEST_KNOWN_FOLDER(FOLDER_DOCS_PUBL,     L"%PUBLIC%/Documents");      }
TEST_F(OSTest, KnownFolder07) { TEST_KNOWN_FOLDER(FOLDER_DESKTOP_USER,  L"%USERPROFILE%/Desktop");   }
TEST_F(OSTest, KnownFolder08) { TEST_KNOWN_FOLDER(FOLDER_DESKTOP_PUBL,  L"%PUBLIC%/Desktop");        }
TEST_F(OSTest, KnownFolder09) { TEST_KNOWN_FOLDER(FOLDER_PICTURES_USER, L"%USERPROFILE%/Pictures");  }
TEST_F(OSTest, KnownFolder10) { TEST_KNOWN_FOLDER(FOLDER_PICTURES_PUBL, L"%PUBLIC%/Pictures");       }
TEST_F(OSTest, KnownFolder11) { TEST_KNOWN_FOLDER(FOLDER_MUSIC_USER,    L"%USERPROFILE%/Music");     }
TEST_F(OSTest, KnownFolder12) { TEST_KNOWN_FOLDER(FOLDER_MUSIC_PUBL,    L"%PUBLIC%/Music");          }
TEST_F(OSTest, KnownFolder13) { TEST_KNOWN_FOLDER(FOLDER_VIDEO_USER,    L"%USERPROFILE%/Videos");    }
TEST_F(OSTest, KnownFolder14) { TEST_KNOWN_FOLDER(FOLDER_VIDEO_PUBL,    L"%PUBLIC%/Videos");         }
#ifdef _WIN64
TEST_F(OSTest, KnownFolder15) { TEST_KNOWN_FOLDER(FOLDER_PROGRAMS_DEF,  L"%ProgramFiles%"); }
TEST_F(OSTest, KnownFolder17) { TEST_KNOWN_FOLDER(FOLDER_PROGRAMS_X64,  L"%ProgramW6432%");          }
#else
TEST_F(OSTest, KnownFolder15) { TEST_KNOWN_FOLDER(FOLDER_PROGRAMS_DEF,  L"%ProgramFiles(x86)%");     }
#endif
TEST_F(OSTest, KnownFolder16) { TEST_KNOWN_FOLDER(FOLDER_PROGRAMS_X86,  L"%ProgramFiles(x86)%");     }
TEST_F(OSTest, KnownFolder18) { TEST_KNOWN_FOLDER(FOLDER_SYSROOT,       L"%SystemRoot%");            }
TEST_F(OSTest, KnownFolder19) { TEST_KNOWN_FOLDER(FOLDER_SYSTEM_DEF,    L"%SystemRoot%/System32");   }
TEST_F(OSTest, KnownFolder20) { TEST_KNOWN_FOLDER(FOLDER_SYSTEM_X86,    L"%SystemRoot%/SysWOW64");   }

#undef TEST_KNOWN_FOLDER
