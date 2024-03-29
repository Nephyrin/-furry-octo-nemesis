/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SQLite Extension
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "SqDatabase.h"
#include "SqQuery.h"

SqDatabase::SqDatabase(sqlite3 *sq3, bool persistent) : 
	m_sq3(sq3), m_refcount(1), m_pFullLock(NULL), m_Persistent(persistent)
{
	m_pRefLock = threader->MakeMutex();
}

SqDatabase::~SqDatabase()
{
	m_pRefLock->DestroyThis();
	if (m_pFullLock)
	{
		m_pFullLock->DestroyThis();
	}
	sqlite3_close(m_sq3);
}

void SqDatabase::IncReferenceCount()
{
	m_pRefLock->Lock();
	m_refcount++;
	m_pRefLock->Unlock();
}

bool SqDatabase::Close()
{
	m_pRefLock->Lock();
	if (m_refcount > 1)
	{
		m_refcount--;
		m_pRefLock->Unlock();
		return false;
	}
	m_pRefLock->Unlock();

	if (m_Persistent)
	{
		g_SqDriver.RemovePersistent(this);
	}

	delete this;

	return true;
}

const char *SqDatabase::GetError(int *errorCode/* =NULL */)
{
	return sqlite3_errmsg(m_sq3);
}

bool SqDatabase::LockForFullAtomicOperation()
{
	if (!m_pFullLock)
	{
		m_pFullLock = threader->MakeMutex();
		if (!m_pFullLock)
		{
			return false;
		}
	}

	m_pFullLock->Lock();

	return true;
}

void SqDatabase::UnlockFromFullAtomicOperation()
{
	if (m_pFullLock)
	{
		m_pFullLock->Unlock();
	}
}

IDBDriver *SqDatabase::GetDriver()
{
	return &g_SqDriver;
}

bool SqDatabase::QuoteString(const char *str, char buffer[], size_t maxlen, size_t *newSize)
{
	char *res = sqlite3_snprintf(static_cast<int>(maxlen), buffer, "%q", str);

	if (res != NULL && newSize != NULL)
	{
		*newSize = strlen(buffer);
	}

	return (res != NULL);
}

unsigned int SqDatabase::GetInsertID()
{
	return (unsigned int)sqlite3_last_insert_rowid(m_sq3);
}

unsigned int SqDatabase::GetAffectedRows()
{
	return (unsigned int)sqlite3_changes(m_sq3);
}

bool SqDatabase::DoSimpleQuery(const char *query)
{
	IQuery *pQuery = DoQuery(query);
	if (!pQuery)
	{
		return false;
	}
	pQuery->Destroy();
	return true;
}

/* this sounds like daiquiri.. i'm tired. */
IQuery *SqDatabase::DoQuery(const char *query)
{
	IPreparedQuery *pQuery = PrepareQuery(query, NULL, 0, NULL);
	if (!pQuery)
	{
		return NULL;
	}
	if (!pQuery->Execute())
	{
		pQuery->Destroy();
		return NULL;
	}
	return pQuery;
}

bool SqDatabase::DoSimpleQueryEx(const char *query, size_t len)
{
	IQuery *pQuery = DoQueryEx(query, len);
	if (!pQuery)
	{
		return false;
	}
	pQuery->Destroy();
	return true;
}

IQuery *SqDatabase::DoQueryEx(const char *query, size_t len)
{
	IPreparedQuery *pQuery = PrepareQueryEx(query, len, NULL, 0, NULL);
	if (!pQuery)
	{
		return NULL;
	}
	if (!pQuery->Execute())
	{
		pQuery->Destroy();
		return NULL;
	}
	return pQuery;
}

IPreparedQuery *SqDatabase::PrepareQuery(const char *query, 
										 char *error, 
										 size_t maxlength, 
										 int *errCode/* =NULL */)
{
	sqlite3_stmt *stmt = NULL;
	if ((m_LastErrorCode = sqlite3_prepare_v2(m_sq3, query, -1, &stmt, NULL)) != SQLITE_OK
		|| !stmt)
	{
		const char *msg;
		if (m_LastErrorCode != SQLITE_OK)
		{
			msg = sqlite3_errmsg(m_sq3);
		} else {
			msg = "Invalid query string";
			m_LastErrorCode = SQLITE_ERROR;
		}
		if (error)
		{
			strncopy(error, msg, maxlength);
		}
		m_LastError.assign(msg);
		return NULL;
	}
	return new SqQuery(this, stmt);
}

IPreparedQuery *SqDatabase::PrepareQueryEx(const char *query, 
										   size_t len, 
										   char *error, 
										   size_t maxlength, 
										   int *errCode/* =NULL */)
{
	sqlite3_stmt *stmt = NULL;
	if ((m_LastErrorCode = sqlite3_prepare_v2(m_sq3, query, len, &stmt, NULL)) != SQLITE_OK
		|| !stmt)
	{
		const char *msg;
		if (m_LastErrorCode != SQLITE_OK)
		{
			msg = sqlite3_errmsg(m_sq3);
		} else {
			msg = "Invalid query string";
			m_LastErrorCode = SQLITE_ERROR;
		}
		if (error)
		{
			strncopy(error, msg, maxlength);
		}
		m_LastError.assign(msg);
		return NULL;
	}
	return new SqQuery(this, stmt);
}

sqlite3 *SqDatabase::GetDb()
{
	return m_sq3;
}
