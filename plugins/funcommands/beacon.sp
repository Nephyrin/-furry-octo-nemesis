/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides beacon functionality
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

new Handle:g_BeaconTimers[MAXPLAYERS+1];

new Handle:g_BeaconRadius = INVALID_HANDLE;

SetupBeacon()
{
	RegAdminCmd("sm_beacon", Command_Beacon, ADMFLAG_SLAY, "sm_beacon <#userid|name> [0/1]");
	
	g_BeaconRadius = CreateConVar("sm_beacon_radius", "375", "Sets the radius for beacon's light rings.", 0, true, 50.0, true, 1500.0);
}

CreateBeacon(client)
{
	g_BeaconTimers[client] = CreateTimer(2.0, Timer_Beacon, client, TIMER_REPEAT);	
}

KillBeacon(client)
{
	KillTimer(g_BeaconTimers[client]);
	g_BeaconTimers[client] = INVALID_HANDLE;
}

KillAllBeacons()
{
	new maxclients = GetMaxClients();
	for (new i = 1; i <= maxclients; i++)
	{
		if (g_BeaconTimers[i] != INVALID_HANDLE)
		{
			KillBeacon(i);
		}
	}
}

PerformBeacon(client, target, toggle)
{
	switch (toggle)
	{
		case (2):
		{
			if (g_BeaconTimers[target] == INVALID_HANDLE)
			{
				CreateBeacon(target);
				LogAction(client, target, "\"%L\" set a beacon on \"%L\"", client, target);
			}
			else
			{
				KillBeacon(target);
				LogAction(client, target, "\"%L\" removed a beacon on \"%L\"", client, target);
			}			
		}

		case (1):
		{
			if (g_BeaconTimers[target] == INVALID_HANDLE)
			{
				CreateBeacon(target);
				LogAction(client, target, "\"%L\" set a beacon on \"%L\"", client, target);
			}			
		}
		
		case (0):
		{
			if (g_BeaconTimers[target] != INVALID_HANDLE)
			{
				KillBeacon(target);
				LogAction(client, target, "\"%L\" removed a beacon on \"%L\"", client, target);
			}			
		}
	}
}

public Action:Timer_Beacon(Handle:timer, any:client)
{
	if (!IsClientInGame(client) || !IsPlayerAlive(client))
	{
		KillBeacon(client);		

		return Plugin_Handled;
	}
	
	new team = GetClientTeam(client);

	new Float:vec[3];
	GetClientAbsOrigin(client, vec);
	vec[2] += 10;

	TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_BeaconRadius), g_BeamSprite, g_HaloSprite, 0, 15, 0.5, 5.0, 0.0, greyColor, 10, 0);
	TE_SendToAll();
	
	if (team == 2)
	{
		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_BeaconRadius), g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 10.0, 0.5, redColor, 10, 0);
	}
	else if (team == 3)
	{
		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_BeaconRadius), g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 10.0, 0.5, blueColor, 10, 0);
	}
	else
	{
		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_BeaconRadius), g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 10.0, 0.5, greenColor, 10, 0);
	}
	
	TE_SendToAll();
		
	GetClientEyePosition(client, vec);
	EmitAmbientSound(SOUND_BLIP, vec, client, SNDLEVEL_RAIDSIREN);	
		
	return Plugin_Handled;
}

public AdminMenu_Beacon(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Beacon player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayBeaconMenu(param);
	}
}

DisplayBeaconMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Beacon);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Beacon player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, true, true);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public MenuHandler_Beacon(Handle:menu, MenuAction:action, param1, param2)
{
	if (action == MenuAction_End)
	{
		CloseHandle(menu);
	}
	else if (action == MenuAction_Cancel)
	{
		if (param2 == MenuCancel_ExitBack && hTopMenu != INVALID_HANDLE)
		{
			DisplayTopMenu(hTopMenu, param1, TopMenuPosition_LastCategory);
		}
	}
	else if (action == MenuAction_Select)
	{
		decl String:info[32];
		new userid, target;
		
		GetMenuItem(menu, param2, info, sizeof(info));
		userid = StringToInt(info);

		if ((target = GetClientOfUserId(userid)) == 0)
		{
			PrintToChat(param1, "[SM] %t", "Player no longer available");
		}
		else if (!CanUserTarget(param1, target))
		{
			PrintToChat(param1, "[SM] %t", "Unable to target");
		}
		else
		{
			new String:name[32];
			GetClientName(target, name, sizeof(name));
			
			PerformBeacon(param1, target, 2);
			ShowActivity2(param1, "[SM] ", "%t", "Toggled beacon on target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayBeaconMenu(param1);
		}
	}
}

public Action:Command_Beacon(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_beacon <#userid|name> [0/1]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new toggle = 2;
	if (args > 1)
	{
		decl String:arg2[2];
		GetCmdArg(2, arg2, sizeof(arg2));
		if (arg2[0])
		{
			toggle = 1;
		}
		else
		{
			toggle = 0;
		}
	}

	decl String:target_name[MAX_TARGET_LENGTH];
	decl target_list[MAXPLAYERS], target_count, bool:tn_is_ml;
	
	if ((target_count = ProcessTargetString(
			arg,
			client,
			target_list,
			MAXPLAYERS,
			COMMAND_FILTER_ALIVE,
			target_name,
			sizeof(target_name),
			tn_is_ml)) <= 0)
	{
		ReplyToTargetError(client, target_count);
		return Plugin_Handled;
	}
	
	for (new i = 0; i < target_count; i++)
	{
		PerformBeacon(client, target_list[i], toggle);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled beacon on target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled beacon on target", "_s", target_name);
	}
	
	return Plugin_Handled;
}
