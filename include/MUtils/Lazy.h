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
#include <QThread>
#include <QAtomicPointer>
#include <QAtomicInt>

//CRT
#include <functional>

namespace MUtils
{
	/**
	* \brief Lazy initialization template class
	*
	* The lazy-initialized value of type T can be obtained from a `Lazy<T>` instance by using the `operator*()`. Initialization of the value happens when the `operator*()` is called for the very first time, by invoking the `initializer` lambda-function that was passed to the constructor. The return value of the `initializer` lambda-function is then stored internally, so that any subsequent call to the `operator*()` *immediately* returns the previously created value.
	*
	* **Note on thread-saftey:** This class is thread-safe in the sense that all calls to `operator*()` on the same `Lazy<T>` instance, regardless from which thread, are guaranteed to return the exactly same value/object. The *first* thread trying to access the value will invoke the `initializer` lambda-function; concurrent threads may need to busy-wait until the initialization is completed. The `initializer` lambda-function is invoked at most once.
	*/
	template<typename T> class Lazy
	{
	public:
		Lazy(std::function<T*(void)> &&initializer) : m_initializer(initializer) { }

		T& operator*(void)
		{
			return (*getValue());
		}

		T* operator->(void)
		{
			return getValue();
		}

		bool initialized()
		{
			return (m_state > 1);
		}

		~Lazy(void)
		{
			if(T *const value = m_value)
			{
				delete value;
			}
		}

	protected:
		__forceinline T* getValue()
		{
			T *value;
			while (!(value = m_value))
			{
				if (m_state.testAndSetOrdered(0, 1))
				{
					if (value = m_initializer())
					{
						m_value.fetchAndStoreOrdered(value);
						m_state.fetchAndStoreOrdered(2);
						break; /*success*/
					}
					m_state.fetchAndStoreOrdered(0);
					MUTILS_THROW("Initializer returned NULL pointer!");
				}
				QThread::yieldCurrentThread();
			}
			return value;
		}

	private:
		QAtomicPointer<T> m_value;
		QAtomicInt m_state;
		const std::function<T*(void)> m_initializer;
	};
}
