/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
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

#if defined _sdktools_voice_included
  #endinput
#endif
#define _sdktools_voice_included

/**
 * @section voice flags.
 */
#define VOICE_NORMAL		0	/**< Allow the client to listen and speak normally. */
#define VOICE_MUTED			1	/**< Mutes the client from speaking to everyone. */
#define VOICE_SPEAKALL		2	/**< Allow the client to speak to everyone. */
#define VOICE_LISTENALL		4	/**< Allow the client to listen to everyone. */
#define VOICE_TEAM			8	/**< Allow the client to always speak to team, even when dead. */
#define VOICE_LISTENTEAM	16	/**< Allow the client to always hear teammates, including dead ones. */

#define LISTEN_DEFAULT		0	/**< Default action (flags or game) is taken */
#define LISTEN_NO			1	/**< Receiver cannot hear sender */
#define LISTEN_YES			2	/**< Receiver can hear sender */
 
/**
 * @endsection
 */

/**
 * Set the client listening flags.
 *
 * @param client		The client index
 * @param flags			The voice flags
 * @noreturn
 */
native SetClientListeningFlags(client, flags);

/**
 * Retrieve the client current listening flags.
 *
 * @param client		The client index
 * @return				The current voice flags
 */
native GetClientListeningFlags(client);

/**
 * Set the receiver's ability to listen to the sender.
 *
 * @param receiver		The listener client index.
 * @param sender		The sender client index.
 * @param control		A LISTEN_ constant describing the voice relationship.
 * @noreturn
 * @error				If either client index is invalid or not connected.
 */
native bool:SetClientListening(receiver, sender, control);

/**
 * Retrieves if a receiver can listen to the sender.
 *
 * @param receiver		The listener client index.
 * @param sender		The sender client index.
 * @return				A LISTEN_ constant describing the voice relationship.
 * @error				If either client index is invalid or not connected.
 */
native bool:GetClientListening(receiver, sender);
