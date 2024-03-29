/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#ifndef _INCLUDE_SOURCEMOD_SYSTEM_LIBRARY_H_
#define _INCLUDE_SOURCEMOD_SYSTEM_LIBRARY_H_

#include <ILibrarySys.h>
#include "sm_platform.h"

using namespace SourceMod;

#if defined PLATFORM_WINDOWS
typedef HMODULE		LibraryHandle;
#elif defined PLATFORM_POSIX
typedef void *		LibraryHandle;
#endif

class CDirectory : public IDirectory
{
public:
	CDirectory(const char *path);
	~CDirectory();
public:
	bool MoreFiles();
	void NextEntry();
	const char *GetEntryName();
	bool IsEntryDirectory();
	bool IsEntryFile();
	bool IsEntryValid();
public:
	bool IsValid();
private:
#if defined PLATFORM_WINDOWS
	HANDLE m_dir;
	WIN32_FIND_DATAA m_fd;
#elif defined PLATFORM_POSIX
	DIR *m_dir;
	struct dirent *ep;
	char m_origpath[PLATFORM_MAX_PATH];
#endif
};

class CLibrary : public ILibrary
{
public:
	CLibrary(LibraryHandle me);
	~CLibrary();
public:
	void CloseLibrary();
	void *GetSymbolAddress(const char *symname);
private:
	LibraryHandle m_lib;
};

class LibrarySystem : public ILibrarySys
{
public:
	ILibrary *OpenLibrary(const char *path, char *error, size_t maxlength);
	IDirectory *OpenDirectory(const char *path);
	void CloseDirectory(IDirectory *dir);
	bool PathExists(const char *path);
	bool IsPathFile(const char *path);
	bool IsPathDirectory(const char *path);
	void GetPlatformError(char *error, size_t maxlength);
	size_t PathFormat(char *buffer, size_t len, const char *fmt, ...);
	const char *GetFileExtension(const char *filename);
	bool CreateFolder(const char *path);
	size_t GetFileFromPath(char *buffer, size_t maxlength, const char *path);
	bool FileTime(const char *path, FileTimeType type, time_t *pTime);
	void GetLoaderError(char *buffer, size_t maxlength);
};

extern LibrarySystem g_LibSys;

#endif //_INCLUDE_SOURCEMOD_SYSTEM_LIBRARY_H_
