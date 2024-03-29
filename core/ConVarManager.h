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

#ifndef _INCLUDE_SOURCEMOD_CONVARMANAGER_H_
#define _INCLUDE_SOURCEMOD_CONVARMANAGER_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include <sh_list.h>
#include <IPluginSys.h>
#include <IForwardSys.h>
#include <IHandleSys.h>
#include <IRootConsoleMenu.h>
#include <compat_wrappers.h>
#include "concmd_cleaner.h"

using namespace SourceHook;

class IConVarChangeListener
{
public:
	virtual void OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue) =0;
};

/**
 * Holds SourceMod-specific information about a convar
 */
struct ConVarInfo
{
	Handle_t handle;					/**< Handle to self */
	bool sourceMod;						/**< Determines whether or not convar was created by a SourceMod plugin */
	IChangeableForward *pChangeForward;	/**< Forward associated with convar */
	ConVar *pVar;						/**< The actual convar */
	List<IConVarChangeListener *> changeListeners;
};

/**
 * Holds information about a client convar query
 */
struct ConVarQuery
{
	QueryCvarCookie_t cookie;			/**< Cookie that identifies query */
	IPluginFunction *pCallback;			/**< Function that will be called when query is finished */
	cell_t value;						/**< Optional value passed to query function */
};

class ConVarManager :
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IPluginsListener,
	public IRootConsoleCommand,
	public IConCommandTracker
{
public:
	ConVarManager();
	~ConVarManager();
public: // SMGlobalClass
	void OnSourceModStartup(bool late);
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
	void OnSourceModVSPReceived();
public: // IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize);
public: // IPluginsListener
	void OnPluginUnloaded(IPlugin *plugin);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *cmdname, const CCommand &command);
public: //IConCommandTracker
	void OnUnlinkConCommandBase(ConCommandBase *pBase, const char *name, bool is_read_safe);
public:
	/**
	 * Create a convar and return a handle to it.
	 */
	Handle_t CreateConVar(IPluginContext *pContext, const char *name, const char *defaultVal,
	                      const char *description, int flags, bool hasMin, float min, bool hasMax, float max);

	/**
	 * Searches for a convar and returns a handle to it
	 */
	Handle_t FindConVar(const char* name);

	/**
	 * Add a function to call when the specified convar changes.
	 */
	void HookConVarChange(ConVar *pConVar, IPluginFunction *pFunction);

	/**
	 * Remove a function from the forward that will be called when the specified convar changes.
	 */
	void UnhookConVarChange(ConVar *pConVar, IPluginFunction *pFunction);

	void AddConVarChangeListener(const char *name, IConVarChangeListener *pListener);
	void RemoveConVarChangeListener(const char *name, IConVarChangeListener *pListener);

	/**
	 * Starts a query to find the value of a client convar.
	 */
	QueryCvarCookie_t QueryClientConVar(edict_t *pPlayer, const char *name, IPluginFunction *pCallback,
	                                    Handle_t hndl);

	bool IsQueryingSupported();

	HandleError ReadConVarHandle(Handle_t hndl, ConVar **pVar);

private:
	/**
	 * Adds a convar to a plugin's list.
	 */
	static void AddConVarToPluginList(IPluginContext *pContext, const ConVar *pConVar);

	/**
	 * Static callback that Valve's ConVar object executes when the convar's value changes.
	 */
#if defined ORANGEBOX_BUILD
	static void OnConVarChanged(ConVar *pConVar, const char *oldValue, float flOldValue);
#else
	static void OnConVarChanged(ConVar *pConVar, const char *oldValue);
#endif

	/**
	 * Callback for when StartQueryCvarValue() has finished.
	 */
	void OnQueryCvarValueFinished(QueryCvarCookie_t cookie, edict_t *pPlayer, EQueryCvarValueStatus result,
	                              const char *cvarName, const char *cvarValue);
private:
	HandleType_t m_ConVarType;
	List<ConVarInfo *> m_ConVars;
	List<ConVarQuery> m_ConVarQueries;
	bool m_bIsDLLQueryHooked;
	bool m_bIsVSPQueryHooked;
};

extern ConVarManager g_ConVarManager;

#endif // _INCLUDE_SOURCEMOD_CONVARMANAGER_H_

