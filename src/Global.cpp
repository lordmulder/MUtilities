///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
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

#if _MSC_VER
#define _CRT_RAND_S 1
#endif

#include <MUtils/Global.h>

//CRT
#include <cstdlib>
#include <ctime>
#include <process.h>

///////////////////////////////////////////////////////////////////////////////
// Random Support
///////////////////////////////////////////////////////////////////////////////

//Robert Jenkins' 96 bit Mix Function
static unsigned int mix_function(const unsigned int x, const unsigned int y, const unsigned int z)
{
	unsigned int a = x;
	unsigned int b = y;
	unsigned int c = z;
	
	a=a-b;  a=a-c;  a=a^(c >> 13);
	b=b-c;  b=b-a;  b=b^(a << 8 ); 
	c=c-a;  c=c-b;  c=c^(b >> 13);
	a=a-b;  a=a-c;  a=a^(c >> 12);
	b=b-c;  b=b-a;  b=b^(a << 16);
	c=c-a;  c=c-b;  c=c^(b >> 5 );
	a=a-b;  a=a-c;  a=a^(c >> 3 );
	b=b-c;  b=b-a;  b=b^(a << 10);
	c=c-a;  c=c-b;  c=c^(b >> 15);

	return a ^ b ^ c;
}

void MUtils::seed_rand(void)
{
	qsrand(mix_function(clock(), time(NULL), _getpid()));
}

quint32 MUtils::next_rand32(void)
{
	quint32 rnd = 0xDEADBEEF;

#ifdef _CRT_RAND_S
	if(rand_s(&rnd) == 0)
	{
		return rnd;
	}
#endif //_CRT_RAND_S

	for(size_t i = 0; i < sizeof(quint32); i++)
	{
		rnd = (rnd << 8) ^ qrand();
	}

	return rnd;
}

quint64 MUtils::next_rand64(void)
{
	return (quint64(next_rand32()) << 32) | quint64(next_rand32());
}

QString MUtils::rand_str(const bool &bLong)
{
	if(!bLong)
	{
		return QString::number(next_rand64(), 16).rightJustified(16, QLatin1Char('0'));
	}
	return QString("%1%2").arg(rand_str(false), rand_str(false));
}
