///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <QScopedPointer>
#include <QAtomicPointer>

namespace MUtils
{
	/**
	* \brief Lazy initialization template class
	*
	* In order to create your own "lazy" initializer, inherit from the `Lazy<T>` class an implement the create() function. The lazy-initialized value can be obtained from a `Lazy<T>` instance by using the `operator*()`. Initialization of the value happens when the `operator*()` is called for the very first time, by invoking the concrete create() function. The return value of create() is then stored internally, so that any subsequent call to the `operator*()` immediately returns the previously created value.
	*
	* **Note on thread-saftey:** This class is thread-safe in the sense that all calls to `operator*()` on the same `Lazy<T>` instance, regardless from which thread, are guaranteed to return the exactly same value/object. Still, if the value has *not* been initialized yet **and** if multiple threads happen to call `operator*()` at the same time, then the concrete create() function *may* be invoked more than once (concurrently and by different threads). In that case, all but one return value of create() are discarded, and all threads eventually receive the same value/object.
	*/
	template<typename T> class Lazy
	{
	public:
		T& operator*(void)
		{
			while (!m_data)
			{
				if (T *const initializer = create())
				{
					if (!m_data.testAndSetOrdered(NULL, initializer))
					{
						delete initializer;
					}
				}
				else
				{
					MUTILS_THROW("Initializer function returned NULL!");
				}
			}
			return *m_data;
		}

	protected:
		virtual T *create() = 0;

	private:
		QAtomicPointer<T> m_data;
	};
}
