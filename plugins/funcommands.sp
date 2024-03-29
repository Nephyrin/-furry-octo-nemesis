/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Fun Commands Plugin
 * Implements basic punishment commands.
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
#include <sdktools>
#undef REQUIRE_PLUGIN
#include <adminmenu>

public Plugin:myinfo =
{
	name = "Fun Commands",
	author = "AlliedModders LLC",
	description = "Fun Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

// -------------------------------------------------------------------------------
// Set any of these to 0 and recompile to completely disable those commands
// -------------------------------------------------------------------------------
#define BEACON		1
#define TIMEBOMB	1
#define FIRE		1
#define ICE			1
#define GRAVITY		1
#define BLIND		1
#define NOCLIP		1
#define DRUG		1
#define TELEPORT	1
#define HEALTH		1
#define COLORS		1
#define CASH		1
// -------------------------------------------------------------------------------

new Handle:hTopMenu = INVALID_HANDLE;

// Sounds
#define SOUND_BLIP		"buttons/blip1.wav"
#define SOUND_BEEP		"buttons/button17.wav"
#define SOUND_FINAL		"weapons/cguard/charging.wav"
#define SOUND_BOOM		"weapons/explode3.wav"
#define SOUND_FREEZE	"physics/glass/glass_impact_bullet4.wav"

// Following are model indexes for temp entities
new g_BeamSprite;
new g_BeamSprite2;
new g_HaloSprite;
new g_GlowSprite;
new g_ExplosionSprite;

// Basic color arrays for temp entities
new redColor[4] = {255, 75, 75, 255};
new orangeColor[4] = {255, 128, 0, 255};
new greenColor[4] = {75, 255, 75, 255};
new blueColor[4] = {75, 75, 255, 255};
new whiteColor[4] = {255, 255, 255, 255};
new greyColor[4] = {128, 128, 128, 255};

// UserMessageId for Fade.
new UserMsg:g_FadeUserMsgId;

// Include various commands and supporting functions
#if BEACON
#include "funcommands/beacon.sp"
#endif
#if TIMEBOMB
#include "funcommands/timebomb.sp"
#endif
#if FIRE
#include "funcommands/fire.sp"
#endif
#if ICE
#include "funcommands/ice.sp"
#endif
#if GRAVITY
#include "funcommands/gravity.sp"
#endif
#if BLIND
#include "funcommands/blind.sp"
#endif
#if NOCLIP
#include "funcommands/noclip.sp"
#endif
#if DRUG
#include "funcommands/drug.sp"
#endif

public OnPluginStart()
{
	if (FindPluginByFile("basefuncommands.smx") != INVALID_HANDLE)
	{
		ThrowError("This plugin replaces basefuncommands.  You cannot run both at once.");
	}
	
	LoadTranslations("common.phrases");
	LoadTranslations("basefuncommands.phrases");

	g_FadeUserMsgId = GetUserMessageId("Fade");
	
	RegAdminCmd("sm_play", Command_Play, ADMFLAG_GENERIC, "sm_play <#userid|name> <filename>");

	HookEvent("round_end", Event_RoundEnd, EventHookMode_PostNoCopy);	

	#if BEACON
	SetupBeacon();		// sm_beacon
	#endif
	
	#if TIMEBOMB
	SetupTimeBomb();	// sm_timebomb
	#endif
	
	#if FIRE
	SetupFire();		// sm_burn and sm_firebomb
	#endif
	
	#if ICE
	SetupIce();			// sm_freeze and sm_freezebomb
	#endif
	
	#if GRAVITY
	SetupGravity();		// sm_gravity
	#endif
	
	#if BLIND
	SetupBlind();		// sm_blind
	#endif
	
	#if NOCLIP
	SetupNoClip();		// sm_noclip
	#endif
	
	#if DRUG
	SetupDrugs();		// sm_drug
	#endif
	
	AutoExecConfig(true, "funcommands");
	
	/* Account for late loading */
	new Handle:topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != INVALID_HANDLE))
	{
		OnAdminMenuReady(topmenu);
	}
}

public OnMapStart()
{
	PrecacheSound(SOUND_BLIP, true);
	PrecacheSound(SOUND_BEEP, true);
	PrecacheSound(SOUND_FINAL, true);
	PrecacheSound(SOUND_BOOM, true);
	PrecacheSound(SOUND_FREEZE, true);

	g_BeamSprite = PrecacheModel("materials/sprites/laser.vmt");
	g_BeamSprite2 = PrecacheModel("sprites/bluelight1.vmt");
	g_HaloSprite = PrecacheModel("materials/sprites/halo01.vmt");	
	g_GlowSprite = PrecacheModel("sprites/blueglow2.vmt");
	g_ExplosionSprite = PrecacheModel("sprites/sprite_fire01.vmt");
}

public OnMapEnd()
{
	#if BEACON
	KillAllBeacons();
	#endif
	#if TIMEBOMB
	KillAllTimeBombs();
	#endif
	#if FIRE
	KillAllFireBombs();
	#endif
	#if ICE
	KillAllFreezes();
	#endif
	#if DRUG
	KillAllDrugs();
	#endif
}

public Action:Event_RoundEnd(Handle:event,const String:name[],bool:dontBroadcast)
{
	#if BEACON
	KillAllBeacons();
	#endif
	#if TIMEBOMB
	KillAllTimeBombs();
	#endif
	#if FIRE
	KillAllFireBombs();
	#endif
	#if ICE
	KillAllFreezes();
	#endif
	#if DRUG
	KillAllDrugs();
	#endif
	
	return Plugin_Handled;
}

public OnAdminMenuReady(Handle:topmenu)
{
	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Find the "Player Commands" category */
	new TopMenuObject:player_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_PLAYERCOMMANDS);

	if (player_commands != INVALID_TOPMENUOBJECT)
	{
		#if BEACON
		AddToTopMenu(hTopMenu,
			"sm_beacon",
			TopMenuObject_Item,
			AdminMenu_Beacon,
			player_commands,
			"sm_beacon",
			ADMFLAG_SLAY);
		#endif
		
		#if TIMEBOMB
		AddToTopMenu(hTopMenu,
			"sm_timebomb",
			TopMenuObject_Item,
			AdminMenu_TimeBomb,
			player_commands,
			"sm_timebomb",
			ADMFLAG_SLAY);
		#endif
		
		#if FIRE
		AddToTopMenu(hTopMenu,
			"sm_burn",
			TopMenuObject_Item,
			AdminMenu_Burn,
			player_commands,
			"sm_burn",
			ADMFLAG_SLAY);
			
		AddToTopMenu(hTopMenu,
			"sm_firebomb",
			TopMenuObject_Item,
			AdminMenu_FireBomb,
			player_commands,
			"sm_firebomb",
			ADMFLAG_SLAY);
		#endif
		
		#if ICE
		AddToTopMenu(hTopMenu,
			"sm_freeze",
			TopMenuObject_Item,
			AdminMenu_Freeze,
			player_commands,
			"sm_freeze",
			ADMFLAG_SLAY);
			
		AddToTopMenu(hTopMenu,
			"sm_freezebomb",
			TopMenuObject_Item,
			AdminMenu_FreezeBomb,
			player_commands,
			"sm_freezebomb",
			ADMFLAG_SLAY);
		#endif
			
		#if GRAVITY
		AddToTopMenu(hTopMenu,
			"sm_gravity",
			TopMenuObject_Item,
			AdminMenu_Gravity,
			player_commands,
			"sm_gravity",
			ADMFLAG_SLAY);
		#endif
			
		#if BLIND
		AddToTopMenu(hTopMenu,
			"sm_blind",
			TopMenuObject_Item,
			AdminMenu_Blind,
			player_commands,
			"sm_blind",
			ADMFLAG_SLAY);
		#endif
		
		#if NOCLIP
		AddToTopMenu(hTopMenu,
			"sm_noclip",
			TopMenuObject_Item,
			AdminMenu_NoClip,
			player_commands,
			"sm_noclip",
			ADMFLAG_SLAY);
		#endif
		
		#if DRUG
		AddToTopMenu(hTopMenu,
			"sm_drug",
			TopMenuObject_Item,
			AdminMenu_Drug,
			player_commands,
			"sm_drug",
			ADMFLAG_SLAY);
		#endif		
	}
}

public Action:Command_Play(client, args)
{
	if (args < 2)
	{
		ReplyToCommand(client, "[SM] Usage: sm_play <#userid|name> <filename>");
	}

	new String:Arguments[PLATFORM_MAX_PATH + 65];
	GetCmdArgString(Arguments, sizeof(Arguments));

 	decl String:Arg[65];
	new len = BreakString(Arguments, Arg, sizeof(Arg));

	/* Make sure it does not go out of bound by doing "sm_play user  "*/
	if (len == -1)
	{
		ReplyToCommand(client, "[SM] Usage: sm_play <#userid|name> <filename>");
		return Plugin_Handled;
	}

	/* Incase they put quotes and white spaces after the quotes */
	if (Arguments[len] == '"')
	{
		len++;
		new FileLen = TrimString(Arguments[len]) + len;

		if (Arguments[FileLen - 1] == '"')
		{
			Arguments[FileLen - 1] = '\0';
		}
	}
	
	decl String:target_name[MAX_TARGET_LENGTH];
	decl target_list[MAXPLAYERS], target_count, bool:tn_is_ml;
	
	if ((target_count = ProcessTargetString(
			Arg,
			client,
			target_list,
			MAXPLAYERS,
			COMMAND_FILTER_NO_BOTS,
			target_name,
			sizeof(target_name),
			tn_is_ml)) <= 0)
	{
		ReplyToTargetError(client, target_count);
		return Plugin_Handled;
	}
	
	for (new i = 0; i < target_count; i++)
	{
		ClientCommand(target_list[i], "playgamesound \"%s\"", Arguments[len]);
		LogAction(client, target_list[i], "\"%L\" played sound on \"%L\" (file \"%s\")", client, target_list[i], Arguments[len]);
	}
	
	if (tn_is_ml)
	{
		ShowActivity2(client, "[SM] ", "%t", "Played sound to target", target_name);
	}
	else
	{
		ShowActivity2(client, "[SM] ", "%t", "Played sound to target", "_s", target_name);
	}

	return Plugin_Handled;
}
