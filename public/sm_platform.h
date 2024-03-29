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

#ifndef _INCLUDE_SOURCEMOD_PLATFORM_H_
#define _INCLUDE_SOURCEMOD_PLATFORM_H_

/**
 * @file sm_platform.h
 * @brief Contains platform-specific macros for abstraction.
 */

#if defined WIN32 || defined WIN64
#define PLATFORM_WINDOWS
#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined snprintf
#define snprintf _snprintf
#endif
#if !defined stat
#define stat _stat
#endif
#define strcasecmp strcmpi
#define strncasecmp strnicmp
#include <windows.h>
#include <direct.h>
#define PLATFORM_LIB_EXT		"dll"
#define PLATFORM_MAX_PATH		MAX_PATH
#define PLATFORM_SEP_CHAR		'\\'
#define PLATFORM_SEP_ALTCHAR	'/'
#define PLATFORM_EXTERN_C		extern "C" __declspec(dllexport)
#if defined _MSC_VER && _MSC_VER >= 1400
#define SUBPLATFORM_SECURECRT
#endif
#elif defined __linux__
#define PLATFORM_LINUX
#define PLATFORM_POSIX
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#define PLATFORM_MAX_PATH		PATH_MAX
#define PLATFORM_LIB_EXT		"so"
#define PLATFORM_SEP_CHAR		'/'
#define PLATFORM_SEP_ALTCHAR	'\\'
#define PLATFORM_EXTERN_C		extern "C" __attribute__((visibility("default")))
#endif

#if !defined SOURCEMOD_BUILD
#define SOURCEMOD_BUILD
#endif

#endif //_INCLUDE_SOURCEMOD_PLATFORM_H_
