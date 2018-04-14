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
* @brief This file contains a template class for lazy initialization
*/

#pragma once

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Exception.h>

//Qt
#include <QAtomicPointer>

//CRT
#include <functional>

namespace MUtils
{
	/**
	* \brief Lazy initialization template class
	*
	* The lazy-initialized value of type T can be obtained from a `Lazy<T>` instance by using the `operator*()`. Initialization of the value happens when the `operator*()` is called for the very first time, by invoking the `initializer` lambda-function that was passed to the constructor. The return value of the `initializer` lambda-function is then stored internally, so that any subsequent call to the `operator*()` *immediately* returns the previously created value.
	*
	* **Note on thread-saftey:** This class is thread-safe in the sense that all calls to `operator*()` on the same `Lazy<T>` instance, regardless from which thread, are guaranteed to return the exactly same value/object. Still, if the value has *not* been initialized yet **and** if multiple threads happen to call `operator*()` at the same time, then the `initializer` lambda-function *may* be invoked more than once (concurrently and by different threads). In that case, all but one return value of the `initializer` lambda-function are discarded, and all threads eventually obtain the same value/object.
	*/
	template<typename T> class Lazy
	{
	public:
		Lazy(std::function<T*(void)> &&initializer) : m_initializer(initializer) { }

		T& operator*(void)
		{
			while (!m_value)
			{
				if (T *const value = m_initializer())
				{
					if (!m_value.testAndSetOrdered(NULL, value))
					{
						delete value; /*too late*/
					}
				}
				else
				{
					MUTILS_THROW("Initializer returned NULL!");
				}
			}
			return *m_value;
		}

		~Lazy(void)
		{
			if(T *const value = m_value.fetchAndStoreOrdered(NULL))
			{
				delete value;
			}
		}

	private:
		QAtomicPointer<T> m_value;
		const std::function<T*(void)> m_initializer;
	};
}
