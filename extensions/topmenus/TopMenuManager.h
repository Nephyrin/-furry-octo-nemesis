/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod TopMenus Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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


#ifndef _INCLUDE_SOURCEMOD_TOP_MENU_MANAGER_IMPL_H_
#define _INCLUDE_SOURCEMOD_TOP_MENU_MANAGER_IMPL_H_

#include <ITopMenus.h>
#include <IPlayerHelpers.h>
#include <IPluginSys.h>
#include <sh_list.h>

using namespace SourceMod;
using namespace SourceHook;

class TopMenu;

class TopMenuManager : 
	public ITopMenuManager,
	public IClientListener,
	public IPluginsListener
{
public:
	const char *GetInterfaceName();
	unsigned int GetInterfaceVersion();
	ITopMenu *CreateTopMenu(ITopMenuObjectCallbacks *callbacks);
	void DestroyTopMenu(ITopMenu *topmenu);
public:
	void OnClientConnected(int client);
	void OnClientDisconnected(int client);
	void OnServerActivated(int max_clients);
	void OnPluginUnloaded(IPlugin *plugin);
private:
	List<TopMenu *> m_TopMenus;
};

extern TopMenuManager g_TopMenus;

#endif //_INCLUDE_SOURCEMOD_TOP_MENU_MANAGER_IMPL_H_
