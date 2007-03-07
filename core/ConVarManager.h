/**
* ===============================================================
* SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
* ===============================================================
*
* This file is not open source and may not be copied without explicit
* written permission of AlliedModders LLC.  This file may not be redistributed 
* in whole or significant part.
* For information, see LICENSE.txt or http://www.sourcemod.net/license.php
*
* Version: $Id$
*/

#ifndef _INCLUDE_SOURCEMOD_CONVARMANAGER_H_
#define _INCLUDE_SOURCEMOD_CONVARMANAGER_H_

#include "sm_globals.h"
#include "sourcemm_api.h"
#include "sm_trie.h"
#include <sh_list.h>
#include <IPluginSys.h>
#include <IForwardSys.h>
#include <IHandleSys.h>
#include <IRootConsoleMenu.h>

using namespace SourceHook;

/**
 * Holds SourceMod-specific information about a convar
 */
struct ConVarInfo
{
	Handle_t handle;					/**< Handle to convar */
	bool sourceMod;						/**< Determines whether or not convar was created by a SourceMod plugin */
	IChangeableForward *changeForward;	/**< Forward associated with convar */
	FnChangeCallback origCallback;		/**< The original callback function */
};

class ConVarManager :
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IPluginsListener,
	public IRootConsoleCommand
{
public:
	ConVarManager();
	~ConVarManager();
public: // SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: // IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void *object);
public: // IPluginsListener
	void OnPluginUnloaded(IPlugin *plugin);
public: //IRootConsoleCommand
	void OnRootConsoleCommand(const char *command, unsigned int argcount);
public:
	/**
	 * Get the 'ConVar' handle type ID.
	 */
	inline HandleType_t GetHandleType()
	{
		return m_ConVarType;
	}

	/**
	 * Get the convar lookup trie.
	 */
	inline Trie *GetConVarCache()
	{
		return m_ConVarCache;
	}
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
private:
	/**
	 * Adds a convar to a plugin's list.
	 */
	static void AddConVarToPluginList(IPluginContext *pContext, const ConVar *pConVar);

	/**
	 * Static callback that Valve's ConVar class executes when the convar's value changes.
	 */
	static void OnConVarChanged(ConVar *pConVar, const char *oldValue);
private:
	HandleType_t m_ConVarType;
	List<ConVarInfo *> m_ConVars;
	Trie *m_ConVarCache;
};

extern ConVarManager g_ConVarManager;

#endif // _INCLUDE_SOURCEMOD_CONVARMANAGER_H_
