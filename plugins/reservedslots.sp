/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Reserved Slots Plugin
 * Provides basic reserved slots.
 *
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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

#pragma semicolon 1

#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Reserved Slots",
	author = "AlliedModders LLC",
	description = "Provides basic reserved slots",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

/* Maximum number of clients that can connect to server */
new g_MaxClients;

/* Handles to convars used by plugin */
new Handle:sm_reserved_slots;
new Handle:sm_hide_slots;
new Handle:sv_visiblemaxplayers;
new Handle:sm_reserve_type;

public OnPluginStart()
{
	LoadTranslations("reservedslots.phrases");
	
	sm_reserved_slots = CreateConVar("sm_reserved_slots", "0", "Number of reserved player slots", 0, true, 0.0);
	sm_hide_slots = CreateConVar("sm_hide_slots", "0", "If set to 1, reserved slots will hidden (subtracted from the max slot count)", 0, true, 0.0, true, 1.0);
	sv_visiblemaxplayers = FindConVar("sv_visiblemaxplayers");
	sm_reserve_type = CreateConVar("sm_reserve_type", "0", "Method of reserving slots", 0, true, 0.0, true, 1.0);
	
	HookConVarChange(sm_reserved_slots, SlotsChanged);
	HookConVarChange(sm_hide_slots, SlotsChanged);
}

public OnPluginEnd()
{
	/* 	If the plugin has been unloaded, reset visiblemaxplayers. In the case of the server shutting down this effect will not be visible */
	SetConVarInt(sv_visiblemaxplayers, g_MaxClients);
}

public OnMapStart()
{
	g_MaxClients = GetMaxClients();
	
	if (GetConVarBool(sm_hide_slots))
	{		
		SetVisibleMaxSlots(GetClientCount(false), g_MaxClients - GetConVarInt(sm_reserved_slots));
	}
}

public OnConfigsExecuted()
{
	if (GetConVarBool(sm_hide_slots))
	{
		SetVisibleMaxSlots(GetClientCount(false), g_MaxClients - GetConVarInt(sm_reserved_slots));
	}	
}

public Action:OnTimedKick(Handle:timer, any:client)
{	
	if (!client || !IsClientInGame(client))
	{
		return Plugin_Handled;
	}
	
	KickClient(client, "%T", "Slot reserved", client);
	
	if (GetConVarBool(sm_hide_slots))
	{				
		SetVisibleMaxSlots(GetClientCount(false), g_MaxClients - GetConVarInt(sm_reserved_slots));
	}
	
	return Plugin_Handled;
}

public OnClientPostAdminCheck(client)
{
	new reserved = GetConVarInt(sm_reserved_slots);

	if (reserved > 0)
	{
		new clients = GetClientCount(false);
		new limit = g_MaxClients - reserved;
		new flags = GetUserFlagBits(client);
		
		new type = GetConVarInt(sm_reserve_type);
		
		if (type == 0)
		{
			if (clients <= limit || IsFakeClient(client) || flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION)
			{
				if (GetConVarBool(sm_hide_slots))
				{
					SetVisibleMaxSlots(clients, limit);
				}
				
				return;
			}
			
			/* Kick player because there are no public slots left */
			CreateTimer(0.1, OnTimedKick, client);
		}
		else
		{		
			if (clients > limit)
			{	
				if (flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION)
				{
					new target = SelectKickClient();
						
					if (target)
					{
						/* Kick public player to free the reserved slot again */
						CreateTimer(0.1, OnTimedKick, target);
					}
				}
				else
				{				
					/* Kick player because there are no public slots left */
					CreateTimer(0.1, OnTimedKick, client);
				}
			}
		}
	}
}

public OnClientDisconnect_Post(client)
{
	if (GetConVarBool(sm_hide_slots))
	{		
		SetVisibleMaxSlots(GetClientCount(false), g_MaxClients - GetConVarInt(sm_reserved_slots));
	}
}

public SlotsChanged(Handle:convar, const String:oldValue[], const String:newValue[])
{
	/* Reserved slots or hidden slots have been disabled - reset sv_visiblemaxplayers */
	if (StringToInt(newValue) == 0)
	{
		SetConVarInt(sv_visiblemaxplayers, g_MaxClients);
	}
}

SetVisibleMaxSlots(clients, limit)
{
	new num = clients;
	
	if (clients == g_MaxClients)
	{
		num = g_MaxClients;
	} else if (clients < limit) {
		num = limit;
	}
	
	SetConVarInt(sv_visiblemaxplayers, num);
}

SelectKickClient()
{
	new Float:highestLatency;
	new highestLatencyId;
	
	new Float:highestSpecLatency;
	new highestSpecLatencyId;
	
	new bool:specFound;
	
	new Float:latency;
	
	for (new i=1; i<=g_MaxClients; i++)
	{	
		if (!IsClientConnected(i))
		{
			continue;
		}
	
		new flags = GetUserFlagBits(i);
		
		if (IsFakeClient(i) || flags & ADMFLAG_ROOT || flags & ADMFLAG_RESERVATION || CheckCommandAccess(i, "sm_reskick_immunity", ADMFLAG_RESERVATION, false))
		{
			continue;
		}
		
		latency = 0.0;
			
		if (IsClientInGame(i))
		{		
			latency = GetClientAvgLatency(i, NetFlow_Outgoing);
			
			LogMessage("Latency : %f",latency);
			
			if (IsClientObserver(i))
			{			
				specFound = true;
				
				if (latency > highestSpecLatency)
				{
					highestSpecLatency = latency;
					highestSpecLatencyId = i;
				}
			}
		}
		
		if (latency >= highestLatency)
		{
			highestLatency = latency;
			highestLatencyId = i;
		}
	}
	
	if (specFound)
	{
		return highestSpecLatencyId;
	}
	
	return highestLatencyId;
}
