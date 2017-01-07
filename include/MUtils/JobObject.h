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
* @brief This file contains function for creating and managing job objects
*
* Each instance of MUtils::JobObject represents a job object. Call MUtils::JobObject::addProcessToJob() to add another sub-process to the job object. Call MUtils::JobObject::terminateJob() to terminate *all* sub-processes that currently belong to the job object at once.
* 
* Note that all sub-processes that belong to the job object will be terminated when *this* process exits, gracefully or due to a crash. All sub-process belonging to a job object also are terminated when the corresponding MUtils::JobObject instance is destroyed.
*/

#pragma once

#include <MUtils/Global.h>

class QProcess;

namespace MUtils
{
	/**
	* @brief This class represents a job object
	*
	* Call addProcessToJob() to add another sub-process to this job object. Call terminateJob() to terminate all sub-processes that belong to this job object. Note that all sub-processes that belong to this job object will also be terminated when *this* process exits, gracefully or due to a crash.
	*
	* Also, when the JobObject instance is destroyed, all sub-process that belong to its corresponding job object and that are still running will be terminated!
	*/
	class MUTILS_API JobObject
	{
	public:
		/**
		* \brief Create a new JobObject instance
		*
		* Creating a new JobObject instance automatically creates a new job object on the system-level. Check isObjectCreated() to test whether the job object was successfully created or not.
		*/
		JobObject(void);
		
		/**
		* \brief Destroys the JobObject instance
		*
		* If the job object still has any running sub-processes left when the corresponding JobObject instance is destroyed, these sub-process are terminated!
		*/
		~JobObject(void);

		/**
		* \brief Test whether job object was created successfully
		*
		* The job object will be created automatically when a new JobObject instance is created. However, the constructor has **no** to tell whether the job object was created successfully on the system-level. Call this function to test whether the job object has been created.
		*
		* \return The function returns `true`, if and only if a job object was successfully created; otherwise it returns `false`.
		*/
		bool isObjectCreated(void);
		
		/**
		* \brief Add a process to the job object
		*
		* This function adds a another sub-process to the job object that is represented by this JobObject instance. Job object limitations apply to the sub-process a
		*
		* \param process A read-only pointer to the [QProcess](http://doc.qt.io/qt-4.8/qprocess.html) object that represents the sub-process to be added to the job object. The sub-process must be in the "running" state; otherwise the function will fail.
		*
		* \return The function returns `true`, if and only if the process was successfully added to the job object; otherwise it returns `false`.
		*/
		bool addProcessToJob(const QProcess *const process);
		
		/**
		* \brief Terminate all sub-processes of the job object
		*
		* This function immediately terminates *all* running sub-processes that belong to the job object represented by this JobObject instance at once.
		*
		* \param exitCode The exit code to be set for the sub-process when they are terminated.
		*
		* \return The function returns `true`, if the sub-processes were destroyed successfully, even if there were no running sub-process left; otherwise it returns `false`.
		*/
		bool terminateJob(const quint32 &exitCode);

	private:
		uintptr_t m_jobPtr;
		MUTILS_NO_COPY(JobObject)
	};
}
