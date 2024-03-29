/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basefuncommands Plugin
 * Provides FireBomb functionality
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

new Handle:g_FireBombTimers[MAXPLAYERS+1];
new g_FireBombTracker[MAXPLAYERS+1];

new Handle:g_BurnDuration = INVALID_HANDLE;
new Handle:g_FireBombTicks = INVALID_HANDLE;
new Handle:g_FireBombRadius = INVALID_HANDLE;
new Handle:g_FireBombMode = INVALID_HANDLE;

SetupFire()
{
	RegAdminCmd("sm_burn", Command_Burn, ADMFLAG_SLAY, "sm_burn <#userid|name> [time]");
	RegAdminCmd("sm_firebomb", Command_FireBomb, ADMFLAG_SLAY, "sm_firebomb <#userid|name> [0/1]");	
	
	g_BurnDuration = CreateConVar("sm_burn_duration", "20", "Sets the default duration of sm_burn and firebomb victims.", 0, true, 0.5, true, 20.0);
	g_FireBombTicks = CreateConVar("sm_firebomb_ticks", "10", "Sets how long the FireBomb fuse is.", 0, true, 5.0, true, 120.0);
	g_FireBombRadius = CreateConVar("sm_firebomb_radius", "600", "Sets the bomb blast radius.", 0, true, 50.0, true, 3000.0);
	g_FireBombMode = CreateConVar("sm_firebomb_mode", "0", "Who is targetted by the FireBomb? 0 = Target only, 1 = Target's team, 2 = Everyone", 0, true, 0.0, true, 2.0);
}

CreateFireBomb(client)
{
	g_FireBombTimers[client] = CreateTimer(1.0, Timer_FireBomb, client, TIMER_REPEAT);
	g_FireBombTracker[client] = GetConVarInt(g_FireBombTicks);
}

KillFireBomb(client)
{
	KillTimer(g_FireBombTimers[client]);
	g_FireBombTimers[client] = INVALID_HANDLE;
}

KillAllFireBombs()
{
	new maxclients = GetMaxClients();
	for (new i = 1; i <= maxclients; i++)
	{
		if (g_FireBombTimers[i] != INVALID_HANDLE)
		{
			KillFireBomb(i);
		}
	}
}

PerformBurn(client, target, Float:seconds)
{
	LogAction(client, target, "\"%L\" ignited \"%L\" (seconds \"%f\")", client, target, seconds);
	IgniteEntity(target, seconds);
}

PerformFireBomb(client, target, toggle)
{
	switch (toggle)
	{
		case (2):
		{
			if (g_FireBombTimers[target] == INVALID_HANDLE)
			{
				CreateFireBomb(target);
				LogAction(client, target, "\"%L\" set a FireBomb on \"%L\"", client, target);
			}
			else
			{
				KillFireBomb(target);
				SetEntityRenderColor(client, 255, 255, 255, 255);
				LogAction(client, target, "\"%L\" removed a FireBomb on \"%L\"", client, target);
			}			
		}

		case (1):
		{
			if (g_FireBombTimers[target] == INVALID_HANDLE)
			{
				CreateFireBomb(target);
				LogAction(client, target, "\"%L\" set a FireBomb on \"%L\"", client, target);
			}			
		}
		
		case (0):
		{
			if (g_FireBombTimers[target] != INVALID_HANDLE)
			{
				KillFireBomb(target);
				SetEntityRenderColor(client, 255, 255, 255, 255);
				LogAction(client, target, "\"%L\" removed a FireBomb on \"%L\"", client, target);
			}			
		}
	}
}

public Action:Timer_FireBomb(Handle:timer, any:client)
{
	if (!IsClientInGame(client) || !IsPlayerAlive(client))
	{
		KillFireBomb(client);		

		return Plugin_Handled;
	}
	
	g_FireBombTracker[client]--;
	
	new Float:vec[3];
	GetClientEyePosition(client, vec);
	
	if (g_FireBombTracker[client] > 0)
	{
		new color;
		
		if (g_FireBombTracker[client] > 1)
		{
			color = RoundToFloor(g_FireBombTracker[client] * (255.0 / GetConVarFloat(g_FireBombTicks)));
			EmitAmbientSound(SOUND_BEEP, vec, client, SNDLEVEL_RAIDSIREN);	
		}
		else
		{
			color = 0;
			EmitAmbientSound(SOUND_FINAL, vec, client, SNDLEVEL_RAIDSIREN);
		}
		
		SetEntityRenderColor(client, 255, color, color, 255);

		decl String:name[64];
		GetClientName(client, name, sizeof(name));
		PrintCenterTextAll("%t", "Till Explodes", name, g_FireBombTracker[client]);		
		
		GetClientAbsOrigin(client, vec);
		vec[2] += 10;

		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_FireBombRadius) / 3.0, g_BeamSprite, g_HaloSprite, 0, 15, 0.5, 5.0, 0.0, greyColor, 10, 0);
		TE_SendToAll();
		TE_SetupBeamRingPoint(vec, 10.0, GetConVarFloat(g_FireBombRadius) / 3.0, g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 10.0, 0.5, whiteColor, 10, 0);
		TE_SendToAll();
	}
	else
	{
		TE_SetupExplosion(vec, g_ExplosionSprite, 0.1, 1, 0, GetConVarInt(g_FireBombRadius), 5000);
		TE_SendToAll();
		
		GetClientAbsOrigin(client, vec);
		vec[2] += 10;
		TE_SetupBeamRingPoint(vec, 50.0, GetConVarFloat(g_FireBombRadius), g_BeamSprite, g_HaloSprite, 0, 10, 0.5, 30.0, 1.5, orangeColor, 5, 0);
		TE_SendToAll();
		vec[2] += 15;
		TE_SetupBeamRingPoint(vec, 40.0, GetConVarFloat(g_FireBombRadius), g_BeamSprite, g_HaloSprite, 0, 10, 0.6, 30.0, 1.5, orangeColor, 5, 0);
		TE_SendToAll();	
		vec[2] += 15;
		TE_SetupBeamRingPoint(vec, 30.0, GetConVarFloat(g_FireBombRadius), g_BeamSprite, g_HaloSprite, 0, 10, 0.7, 30.0, 1.5, orangeColor, 5, 0);
		TE_SendToAll();
		vec[2] += 15;
		TE_SetupBeamRingPoint(vec, 20.0, GetConVarFloat(g_FireBombRadius), g_BeamSprite, g_HaloSprite, 0, 10, 0.8, 30.0, 1.5, orangeColor, 5, 0);
		TE_SendToAll();		
		
		EmitAmbientSound(SOUND_BOOM, vec, client, SNDLEVEL_RAIDSIREN);

		IgniteEntity(client, GetConVarFloat(g_BurnDuration));
		KillFireBomb(client);
		SetEntityRenderColor(client, 255, 255, 255, 255);
		
		if (GetConVarInt(g_FireBombMode) > 0)
		{
			new teamOnly = ((GetConVarInt(g_FireBombMode) == 1) ? true : false);
			new maxClients = GetMaxClients();
			
			for (new i = 1; i < maxClients; i++)
			{
				if (!IsClientInGame(i) || !IsPlayerAlive(i) || i == client)
				{
					continue;
				}
				
				if (teamOnly && GetClientTeam(i) != GetClientTeam(client))
				{
					continue;
				}
				
				new Float:pos[3];
				GetClientAbsOrigin(i, pos);
				
				new Float:distance = GetVectorDistance(vec, pos);
				
				if (distance > GetConVarFloat(g_FireBombRadius))
				{
					continue;
				}
				
				new Float:duration = GetConVarFloat(g_BurnDuration);
				duration *= (GetConVarFloat(g_FireBombRadius) - distance) / GetConVarFloat(g_FireBombRadius);

				IgniteEntity(i, duration);
			}		
		}
	}
			
	return Plugin_Handled;
}

public AdminMenu_Burn(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "Burn player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayBurnMenu(param);
	}
}

public AdminMenu_FireBomb(Handle:topmenu, 
					  TopMenuAction:action,
					  TopMenuObject:object_id,
					  param,
					  String:buffer[],
					  maxlength)
{
	if (action == TopMenuAction_DisplayOption)
	{
		Format(buffer, maxlength, "%T", "FireBomb player", param);
	}
	else if (action == TopMenuAction_SelectOption)
	{
		DisplayFireBombMenu(param);
	}
}

DisplayBurnMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_Burn);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "Burn player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, true, true);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

DisplayFireBombMenu(client)
{
	new Handle:menu = CreateMenu(MenuHandler_FireBomb);
	
	decl String:title[100];
	Format(title, sizeof(title), "%T:", "FireBomb player", client);
	SetMenuTitle(menu, title);
	SetMenuExitBackButton(menu, true);
	
	AddTargetsToMenu(menu, client, true, true);
	
	DisplayMenu(menu, client, MENU_TIME_FOREVER);
}

public MenuHandler_Burn(Handle:menu, MenuAction:action, param1, param2)
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
			PerformBurn(param1, target, 20.0);
			ShowActivity2(param1, "[SM] ", "%t", "Set target on fire", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayBurnMenu(param1);
		}
	}
}

public MenuHandler_FireBomb(Handle:menu, MenuAction:action, param1, param2)
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
			
			PerformFireBomb(param1, target, 2);
			ShowActivity2(param1, "[SM] ", "%t", "Toggled FireBomb on target", "_s", name);
		}
		
		/* Re-draw the menu if they're still valid */
		if (IsClientInGame(param1) && !IsClientInKickQueue(param1))
		{
			DisplayFireBombMenu(param1);
		}
	}
}

public Action:Command_Burn(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_burn <#userid|name> [time]");
		return Plugin_Handled;
	}

	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));

	new Float:seconds = GetConVarFloat(g_BurnDuration);
	
	if (args > 1)
	{
		decl String:time[20];
		GetCmdArg(2, time, sizeof(time));
		if (StringToFloatEx(time, seconds) == 0)
		{
			ReplyToCommand(client, "[SM] %t", "Invalid Amount");
			return Plugin_Handled;
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
		PerformBurn(client, target_list[i], seconds);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Set target on fire", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Set target on fire", "_s", target_name);
	}

	return Plugin_Handled;
}

public Action:Command_FireBomb(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_firebomb <#userid|name> [0/1]");
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
		PerformFireBomb(client, target_list[i], toggle);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled FireBomb on target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Toggled FireBomb on target", "_s", target_name);
	}
	
	return Plugin_Handled;
}






