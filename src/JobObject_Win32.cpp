///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2019 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

//Internal
#include <MUtils/JobObject.h>

//Qt
#include <QProcess>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>

//Utilities
#define PTR2HANDLE(X) reinterpret_cast<HANDLE>((X))
#define HANDLE2PTR(X) reinterpret_cast<uintptr_t>((X))

MUtils::JobObject::JobObject(void)
:
	m_jobPtr(NULL)
{
	const HANDLE jobObject = CreateJobObject(NULL, NULL);
	if((jobObject != NULL) && (jobObject != INVALID_HANDLE_VALUE))
	{
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobExtendedLimitInfo;
		memset(&jobExtendedLimitInfo, 0, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
		jobExtendedLimitInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
		if(SetInformationJobObject(jobObject, JobObjectExtendedLimitInformation, &jobExtendedLimitInfo, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)))
		{
			m_jobPtr = HANDLE2PTR(jobObject);
		}
		else
		{
			qWarning("Failed to set up job object limit information!");
			CloseHandle(jobObject);
		}
	}
	else
	{
		qWarning("Failed to create the job object!");
	}
}

MUtils::JobObject::~JobObject(void)
{
	if(m_jobPtr)
	{
		CloseHandle(PTR2HANDLE(m_jobPtr));
		m_jobPtr = NULL;
	}
}

bool MUtils::JobObject::isObjectCreated(void)
{
	return (bool) m_jobPtr;
}

bool MUtils::JobObject::addProcessToJob(const QProcess *const process)
{
	if(!m_jobPtr)
	{
		qWarning("Cannot assign process to job: No job bject available!");
		return false;
	}

	if(const Q_PID pid = process->pid())
	{
		DWORD exitCode;
		if(!GetExitCodeProcess(pid->hProcess, &exitCode))
		{
			qWarning("Cannot assign process to job: Failed to query process status!");
			return false;
		}
		if(exitCode != STILL_ACTIVE)
		{
			qWarning("Cannot assign process to job: Process is not running anymore!");
			return false;
		}
		if(!AssignProcessToJobObject(PTR2HANDLE(m_jobPtr), pid->hProcess))
		{
			qWarning("Failed to assign process to job object!");
			return false;
		}
		return true;
	}
	else
	{
		qWarning("Cannot assign process to job: Process handle not available!");
		return false;
	}
}

bool MUtils::JobObject::terminateJob(const quint32 &exitCode)
{
	if(m_jobPtr)
	{
		if(TerminateJobObject(PTR2HANDLE(m_jobPtr), exitCode))
		{
			return true;
		}
		else
		{
			qWarning("Failed to terminate job object!");
			return false;
		}
	}
	else
	{
		qWarning("Cannot assign process to job: No job bject available!");
		return false;
	}
}
