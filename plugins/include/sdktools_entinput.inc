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

#if defined _sdktools_entinput_included
  #endinput
#endif
#define _sdktools_entinput_included

/**
 * Invokes a named input method on an entity.  
 *
 * After completion (successful or not), the current global variant is re-initalized.
 *
 * @param dest		Destination entity index.
 * @param input		Input action.
 * @param activator	Entity index which initiated the sequence of actions (-1 for a NULL entity).
 * @param caller	Entity index from which this event is sent (-1 for a NULL entity).
 * @param outputid	Unknown.
 * @return		True if successful otherwise false.
 * @error		Invalid entity index or no mod support.
 */
native bool:AcceptEntityInput(dest, const String:input[], activator=-1, caller=-1, outputid=0);

/**
 * Sets a bool value in the global variant object.
 *
 * @param val		Input value.
 * @noreturn
 */
native SetVariantBool(bool:val);

/**
 * Sets a string in the global variant object.
 *
 * @param str		Input string.
 * @noreturn
 */
native SetVariantString(const String:str[]);

/**
 * Sets an integer value in the global variant object.
 *
 * @param val		Input value.
 * @noreturn
 */
native SetVariantInt(val);

/**
 * Sets a floating point value in the global variant object.
 *
 * @param val		Input value.
 * @noreturn
 */
native SetVariantFloat(Float:val);

/**
 * Sets a 3D vector in the global variant object.
 *
 * @param vec		Input vector.
 * @noreturn
 */
native SetVariantVector3D(const Float:vec[3]);

/**
 * Sets a 3D position vector in the global variant object.
 *
 * @param vec		Input position vector.
 * @noreturn
 */
native SetVariantPosVector3D(const Float:vec[3]);

/**
 * Sets a color in the global variant object.
 *
 * @param color		Input color.
 * @noreturn
 */
native SetVariantColor(const color[4]);

/**
 * Sets an entity in the global variant object.
 *
 * @param		Entity index.
 * @noreturn
 * @error		Invalid entity index.
 */
native SetVariantEntity(entity);
