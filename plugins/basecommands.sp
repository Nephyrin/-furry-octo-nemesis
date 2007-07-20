/**
 * basecommands.sp
 * Implements basic admin commands.
 * This file is part of SourceMod, Copyright (C) 2004-2007 AlliedModders LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Version: $Id$
 */

#pragma semicolon 1

#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Basic Commands",
	author = "AlliedModders LLC",
	description = "Basic Admin Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

new Handle:hBanForward = INVALID_HANDLE;
new Handle:hAddBanForward = INVALID_HANDLE;
new Handle:hBanRemoved = INVALID_HANDLE;

public OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("plugin.basecommands");
	RegAdminCmd("sm_kick", Command_Kick, ADMFLAG_KICK, "sm_kick <#userid|name> [reason]");
	RegAdminCmd("sm_map", Command_Map, ADMFLAG_CHANGEMAP, "sm_map <map>");
	RegAdminCmd("sm_rcon", Command_Rcon, ADMFLAG_RCON, "sm_rcon <args>");
	RegAdminCmd("sm_cvar", Command_Cvar, ADMFLAG_CONVARS, "sm_cvar <cvar> [value]");
	RegAdminCmd("sm_execcfg", Command_ExecCfg, ADMFLAG_CONFIG, "sm_execcfg <filename>");
	RegAdminCmd("sm_who", Command_Who, ADMFLAG_GENERIC, "sm_who [#userid|name]");
	RegAdminCmd("sm_ban", Command_Ban, ADMFLAG_BAN, "sm_ban <#userid|name> <minutes|0> [reason]");
	RegAdminCmd("sm_unban", Command_Unban, ADMFLAG_UNBAN, "sm_unban <steamid>");
	RegAdminCmd("sm_addban", Command_AddBan, ADMFLAG_RCON, "sm_addban <time> <steamid> [reason]");
	RegAdminCmd("sm_banip", Command_BanIp, ADMFLAG_RCON, "sm_banip <time> <ip> [reason]");
	
	hBanForward = CreateGlobalForward("OnClientBanned", ET_Hook, Param_Cell, Param_Cell, Param_Cell, Param_String);
	hAddBanForward = CreateGlobalForward("OnBanAdded", ET_Hook, Param_Cell, Param_String, Param_Cell, Param_String);
	hBanRemoved = CreateGlobalForward("OnBanRemoved", ET_Hook, Param_Cell, Param_String);
}

public Action:Command_BanIp(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_banip <time> <ip> [reason]");
		return Plugin_Handled;
	}
	
	decl String:arg[50], String:time[20];
	GetCmdArg(1, time, sizeof(time));
	GetCmdArg(2, arg, sizeof(arg));
	
	new minutes = StringToInt(time);
	
	decl String:reason[128];
	if (args >= 3)
	{
		GetCmdArg(3, reason, sizeof(reason));
	}
	
	new Action:act = Plugin_Continue;
	Call_StartForward(hAddBanForward);
	Call_PushCell(client);
	Call_PushString(arg);
	Call_PushCell(minutes);
	Call_PushString(reason);
	Call_Finish(act);
	
	if (act < Plugin_Handled)
	{
		LogMessage("\"%L\" added ban (minutes \"%d\") (ip \"%s\") (reason \"%s\")", client, minutes, arg, reason);
		ReplyToCommand(client, "[SM] %t", "Ban added");
	}
	
	if (act < Plugin_Stop)
	{
		ServerCommand("banip %d %s", minutes, arg);
		ServerCommand("writeip");
	}
	
	return Plugin_Handled;
}

public Action:Command_AddBan(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_addban <time> <steamid> [reason]");
		return Plugin_Handled;
	}
	
	decl String:arg[50], String:time[20];
	GetCmdArg(1, time, sizeof(time));
	GetCmdArg(2, arg, sizeof(arg));
	
	new minutes = StringToInt(time);
	
	decl String:reason[128];
	if (args >= 3)
	{
		GetCmdArg(3, reason, sizeof(reason));
	}
	
	new Action:act = Plugin_Continue;
	Call_StartForward(hAddBanForward);
	Call_PushCell(client);
	Call_PushString(arg);
	Call_PushCell(minutes);
	Call_PushString(reason);
	Call_Finish(act);
	
	if (act < Plugin_Handled)
	{
		LogMessage("\"%L\" added ban (minutes \"%d\") (id \"%s\") (reason \"%s\")", client, minutes, arg, reason);
		ReplyToCommand(client, "[SM] %t", "Ban added");
	}
	
	if (act < Plugin_Stop)
	{
		ServerCommand("banid %d %s", minutes, arg);
		ServerCommand("writeid");
	}
	
	return Plugin_Handled;
}

public Action:Command_Unban(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_unban <steamid>");
		return Plugin_Handled;
	}
	
	decl String:arg[50];
	new start=0;
	decl String:new_arg[50];
	GetCmdArgString(arg, sizeof(arg));
	
	if (strncmp(arg, "STEAM_0:", 8) == 0)
	{
		start = 8;
	} else if (strncmp(arg, "0:1:", 4) == 0 || strncmp(arg, "0:0:", 4) == 0)
	{
		start = 2;
	}
	
	Format(new_arg, sizeof(new_arg), "STEAM_0:%s", arg[start]);
	
	new Action:act = Plugin_Continue;
	Call_StartForward(hBanRemoved);
	Call_PushCell(client);
	Call_PushString(new_arg);
	Call_Finish(act);
	
	if (act < Plugin_Handled)
	{
		LogMessage("\"%L\" removed ban (filter \"%s\")", client, arg[start]);
		ReplyToCommand(client, "[SM] %t", "Removed bans matching", arg);
	}
	
	if (act < Plugin_Stop)
	{
		ServerCommand("removeid \"%s\"", arg[start]);
		ServerCommand("writeid");
	}
	
	return Plugin_Handled;
}

public Action:Command_Ban(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_ban <#userid|name> <minutes|0> [reason]");
		return Plugin_Handled;
	}
	
	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new clients[2];
	new numClients = SearchForClients(arg, clients, 2);
	
	if (numClients == 0)
	{
		ReplyToCommand(client, "[SM] %t", "No matching client");
		return Plugin_Handled;
	} else if (numClients > 1) {
		ReplyToCommand(client, "[SM] %t", "More than one client matches", arg);
		return Plugin_Handled;
	} else if (!CanUserTarget(client, clients[0])) {
		ReplyToCommand(client, "[SM] %t", "Unable to target");
		return Plugin_Handled;
	} else if (IsFakeClient(clients[0])) {
		ReplyToCommand(client, "[SM] %t", "Cannot target bot");
		return Plugin_Handled;
	}
	
	decl String:s_time[12];
	GetCmdArg(2, s_time, sizeof(s_time));
	
	new time = StringToInt(s_time);
	
	decl String:reason[128];
	if (args >= 3)
	{
		GetCmdArg(3, reason, sizeof(reason));
	} else {
		reason[0] = '\0';
	}
	
	new userid = GetClientUserId(clients[0]);
	GetClientName(clients[0], arg, sizeof(arg));
	
	/* Fire the ban forward */
	new Action:act = Plugin_Continue;
	Call_StartForward(hBanForward);
	Call_PushCell(client);
	Call_PushCell(clients[0]);
	Call_PushCell(time);
	Call_PushString(reason);
	Call_Finish(act);
	
	if (act < Plugin_Handled)
	{
		if (!time)
		{
			if (reason[0] == '\0')
			{
				ShowActivity(client, "%t", "Permabanned player", arg);
			} else {
				ShowActivity(client, "%t", "Permabanned player reason", arg, reason);
			}
		} else {
			if (reason[0] == '\0')
			{
				ShowActivity(client, "%t", "Banned player", arg, time);
			} else {
				ShowActivity(client, "%t", "Banned player reason", arg, time, reason);
			}
		}
		LogMessage("\"%L\" banned \"%L\" (minutes \"%d\") (reason \"%s\")", client, clients[0], time, reason);
	}

	if (act < Plugin_Stop)
	{
		if (reason[0] == '\0')
		{
			strcopy(reason, sizeof(reason), "Banned");
		}
		
		ServerCommand("banid %d %d", time, userid);
		KickClient(clients[0], "%s", reason);
		
		if (time == 0)
		{
			ServerCommand("writeid");
		}
	}
	
	return Plugin_Handled;
}

#define FLAG_STRINGS		14
new String:g_FlagNames[FLAG_STRINGS][20] =
{
	"reservation",
	"admin",
	"kick",
	"ban",
	"unban",
	"slay",
	"map",
	"cvars",
	"cfg",
	"chat",
	"vote",
	"pass",
	"rcon",
	"cheat"
};

FlagsToString(String:buffer[], maxlength, flags)
{
	decl String:joins[FLAG_STRINGS][20];
	new total;
	
	for (new i=0; i<FLAG_STRINGS; i++)
	{
		if (flags & (1<<i))
		{
			strcopy(joins[total++], 20, g_FlagNames[i]);
		}
	}
	
	ImplodeStrings(joins, total, ", ", buffer, maxlength);
}

public Action:Command_Who(client, args)
{
	if (args < 1)
	{
		/* Display header */
		decl String:t_access[16], String:t_name[16];
		Format(t_access, sizeof(t_access), "%t", "Access", client);
		Format(t_name, sizeof(t_name), "%t", "Name", client);
		
		PrintToConsole(client, "%-24.23s %s", t_name, t_access);
		
		/* List all players */
		new maxClients = GetMaxClients();
		decl String:flagstring[255];
				
		for (new i=1; i<=maxClients; i++)
		{
			if (!IsClientInGame(i))
			{
				continue;
			}
			new flags = GetUserFlagBits(i);
			if (flags == 0)
			{
				strcopy(flagstring, sizeof(flagstring), "none");
			} else if (flags & ADMFLAG_ROOT) {
				strcopy(flagstring, sizeof(flagstring), "root");
			} else {
				FlagsToString(flagstring, sizeof(flagstring), flags);
			}
			decl String:name[65];
			GetClientName(i, name, sizeof(name));
			PrintToConsole(client, "%d. %-24.23s %s", i, name, flagstring);
		}
		
		if (GetCmdReplySource() == SM_REPLY_TO_CHAT)
		{
			ReplyToCommand(client, "[SM] %t", "See console for output");
		}
		
		return Plugin_Handled;
	}
	
	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new clients[2];
	new numClients = SearchForClients(arg, clients, 2);
	
	if (numClients == 0)
	{
		ReplyToCommand(client, "[SM] %t", "No matching client");
		return Plugin_Handled;
	} else if (numClients > 1) {
		ReplyToCommand(client, "[SM] %t", "More than one client matches", arg);
		return Plugin_Handled;
	}
	
	new flags = GetUserFlagBits(clients[0]);
	decl String:flagstring[255];
	if (flags == 0)
	{
		strcopy(flagstring, sizeof(flagstring), "none");
	} else if (flags & ADMFLAG_ROOT) {
		strcopy(flagstring, sizeof(flagstring), "root");
	} else {
		FlagsToString(flagstring, sizeof(flagstring), flags);
	}
	
	ReplyToCommand(client, "[SM] %t: %s", "Access", flagstring);
	
	return Plugin_Handled;
}

public Action:Command_ExecCfg(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_execcfg <filename>");
		return Plugin_Handled;
	}
	
	new String:path[64] = "cfg/";
	GetCmdArg(1, path[4], sizeof(path)-4);
	
	if (!FileExists(path))
	{
		ReplyToCommand(client, "[SM] %t", "Config not found", path[4]);
		return Plugin_Handled;
	}
	
	ShowActivity(client, "%t", "Executed config", path[4]);
	
	LogMessage("\"%L\" executed config (file \"%s\")", client, path[4]);
		
	ServerCommand("exec \"%s\"", path[4]);
	
	return Plugin_Handled;
}

public Action:Command_Cvar(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_cvar <cvar> [value]");
		return Plugin_Handled;
	}
	
	decl String:cvarname[33];
	GetCmdArg(1, cvarname, sizeof(cvarname));
	
	new Handle:hndl = FindConVar(cvarname);
	if (hndl == INVALID_HANDLE)
	{
		ReplyToCommand(client, "[SM] %t", "Unable to find cvar", cvarname);
		return Plugin_Handled;
	}
	
	new bool:allowed = false;
	if (GetConVarFlags(hndl) & FCVAR_PROTECTED)
	{
		/* If they're root, allow anything */
		if ((GetUserFlagBits(client) & ADMFLAG_ROOT) == ADMFLAG_ROOT)
		{
			allowed = true;
		}
		/* If they're not root, and getting sv_password, see if they have ADMFLAG_PASSWORD */
		else if (StrEqual(cvarname, "sv_password") && (GetUserFlagBits(client) & ADMFLAG_PASSWORD))
		{
			allowed = true;
		}
	} 
	/* Do a check for the cheat cvar */
	else if (StrEqual(cvarname, "sv_cheats"))
	{
		if (GetUserFlagBits(client) & ADMFLAG_CHEATS
			|| GetUserFlagBits(client) & ADMFLAG_ROOT)
		{
			allowed = true;
		}
	}
	/* If we drop down to here, it was a normal cvar. */
	else
	{
		allowed = true;
	}
	
	if (!allowed)
	{
		ReplyToCommand(client, "[SM] %t", "No access to cvar");
		return Plugin_Handled;
	}
	
	decl String:value[255];
	if (args < 2)
	{
		GetConVarString(hndl, value, sizeof(value));
		
		ReplyToCommand(client, "[SM] %t", "Value of cvar", cvarname, value);
		return Plugin_Handled;
	}
	
	GetCmdArg(2, value, sizeof(value));
	
	if ((GetConVarFlags(hndl) & FCVAR_PROTECTED) != FCVAR_PROTECTED)
	{
		ShowActivity(client, "%t", "Cvar changed", cvarname, value);
	} else {
		ReplyToCommand(client, "[SM] %t", "Cvar changed", cvarname, value);
	}
	
	LogMessage("\"%L\" changed cvar (cvar \"%s\") (value \"%s\")", client, cvarname, value);
	
	SetConVarString(hndl, value, true);
		
	return Plugin_Handled;
}

public Action:Command_Rcon(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_rcon <args>");
		return Plugin_Handled;
	}
	
	decl String:argstring[255];
	GetCmdArgString(argstring, sizeof(argstring));
	
	LogMessage("\"%L\" console command (cmdline \"%s\")", client, argstring);
	
	ServerCommand("%s", argstring);
	
	return Plugin_Handled;
}

public Action:Command_Map(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_map <map>");
		return Plugin_Handled;
	}
	
	decl String:map[64];
	GetCmdArg(1, map, sizeof(map));
	
	if (!IsMapValid(map))
	{
		ReplyToCommand(client, "[SM] %t", "Map was not found", map);
		return Plugin_Handled;
	}
	
	ShowActivity(client, "%t", "Changing map", map);
	
	LogMessage("\"%L\" changed map to \"%s\"", client, map);
	
	new Handle:dp;
	CreateDataTimer(3.0, Timer_ChangeMap, dp);
	WritePackString(dp, map);
	
	return Plugin_Handled;
}

public Action:Timer_ChangeMap(Handle:timer, Handle:dp)
{
	decl String:map[65];
	
	ResetPack(dp);
	ReadPackString(dp, map, sizeof(map));
	
	ServerCommand("changelevel \"%s\"", map);
	
	return Plugin_Stop;
}

public Action:Command_Kick(client, args)
{
	if (args < 1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_kick <#userid|name> [reason]");
		return Plugin_Handled;
	}
	
	decl String:arg[65];
	GetCmdArg(1, arg, sizeof(arg));
	
	new clients[2];
	new numClients = SearchForClients(arg, clients, 2);
	
	if (numClients == 0)
	{
		ReplyToCommand(client, "[SM] %t", "No matching client");
		return Plugin_Handled;
	} else if (numClients > 1) {
		ReplyToCommand(client, "[SM] %t", "More than one client matches", arg);
		return Plugin_Handled;
	} else if (!CanUserTarget(client, clients[0])) {
		ReplyToCommand(client, "[SM] %t", "Unable to target");
		return Plugin_Handled;
	}
	
	decl String:name[65];
	
	GetClientName(clients[0], name, sizeof(name));
	
	decl String:reason[256];
	if (args < 2)
	{
		/* Safely null terminate */
		reason[0] = '\0';
	} else {
		GetCmdArg(2, reason, sizeof(reason));
	}
	
	ShowActivity(client, "%t", "Kicked player", name);
	
	LogMessage("\"%L\" kicked \"%L\" (reason \"%s\")", client, clients[0], reason);
	
	KickClient(clients[0], "%s", reason);
	
	return Plugin_Handled;
}
