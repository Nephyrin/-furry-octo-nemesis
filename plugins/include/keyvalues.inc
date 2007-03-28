/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */
 
#if defined _keyvalues_included
 #endinput
#endif
#define _keyvalues_included

/**
 * KeyValue data value types
 */
enum KvDataTypes
{
	KvData_None = 0,	/**< Type could not be identified, or no type */
	KvData_String,		/**< String value */
	KvData_Int,			/**< Integer value */
	KvData_Float,		/**< Floating point value */
	KvData_Ptr,			/**< Pointer value (sometimes called "long") */
	KvData_WString,		/**< Wide string value */
	KvData_Color,		/**< Color value */
	KvData_UInt64,		/**< Large integer value */
	/* --- */
	KvData_NUMTYPES, 
};

/**
 * Creates a new KeyValues structure.  The Handle must always be closed.
 *
 * @param name			Name of the root section.
 * @param firstKey		If non-empty, specifies the first key value.
 * @param firstValue	If firstKey is non-empty, specifies the first key's value.
 * @return				A Handle to a new KeyValues structure.
 */
native Handle:CreateKeyValues(const String:name[], const String:firstkey[], const String:firstValue[]);

/** 
 * Sets a string value of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param value			String value.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetString(Handle:kv, const String:key[], const String:value[]);

/** 
 * Sets an integer value of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param value			Value number.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetNum(Handle:kv, const String:key[], value);

/** 
 * Sets a large integer value of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param value			Large integer value (0=High bits, 1=Low bits)
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetUint64(Handle:kv, const String:key[], const value[2]);

/** 
 * Sets a floating point value of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param value			Floating point value.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetFloat(Handle:kv, const String:key[], Float:value);

/** 
 * Sets a set of color values of a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param r				Red value.
 * @param g				Green value.
 * @param b				Blue value.
 * @param a				Alpha value.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetColor(Handle:kv, const String:key[], r, g, b, a=0);

/** 
 * Retrieves a string value from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param value			Buffer to store key value in.
 * @param maxlength		Maximum length of the value buffer.
 * @param defvalue		Optional default value to use if the key is not found.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvGetString(Handle:kv, const String:key[], String:value[], maxlength, const String:defvalue[]="");

/** 
 * Retrieves an integer value from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param defvalue		Optional default value to use if the key is not found.
 * @return				Integer value of the key.
 * @error				Invalid Handle.
 */
native KvGetNum(Handle:kv, const String:key[], defvalue=0);

/** 
 * Retrieves a floating point value from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param defvalue		Optional default value to use if the key is not found.
 * @return				Floating point value of the key.
 * @error				Invalid Handle.
 */
native Float:KvGetFloat(Handle:kv, const String:key[], Float:defvalue=0.0);

/** 
 * Retrieves a set of color values from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param r				Red value, set by reference.
 * @param g				Green value, set by reference. 
 * @param b				Blue value, set by reference. 
 * @param a				Alpha value, set by reference. 
 * @noreturn
 * @error				Invalid Handle.
 */
native KvGetColor(Handle:kv, const String:key[], &r, &g, &b, &a);

/** 
 * Retrieves a large integer value from a KeyValues key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param value			Array to represent the large integer.
 * @param defvalue		Optional default value to use if the key is not found.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvGetUint64(Handle:kv, const String:key[], value[2], defvalue[2]={0,0});

/**
 * Sets the current position in the KeyValues tree to the given key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Name of the key.
 * @param create		If true, and the key does not exist, it will be created.
 * @return				True if the key exists, false if it does not and was not created.
 */
native bool:KvJumpToKey(Handle:kv, const String:key[], bool:create=false);

/**
 * Sets the current position in the KeyValues tree to the first sub key.
 *
 * @param kv			KeyValues Handle.
 * @return				True on success, false if there was no first sub key.
 * @error				Invalid Handle.
 */
native bool:KvJumpFirstSubKey(Handle:kv);

/**
 * Sets the current position in the KeyValues tree to the next sub key.
 *
 * @param kv			KeyValues Handle.
 * @return				True on success, false if there was no next sub key.
 * @error				Invalid Handle.
 */
native bool:KvJumpNextSubKey(Handle:kv);

/**
 * Jumps back to the previous position.  Returns false if there are no
 * previous positions (i.e., at the root node).  This should be called
 * once for each successful Jump call, in order to return to the top node.
 *
 * @param kv			KeyValues Handle.
 * @return				True on success, false if there is no higher node.
 * @error				Invalid Handle.
 */
native bool:KvGoBack(Handle:kv);

/**
 * Sets the position back to the top node, emptying the entire node 
 * traversal history.  This can be used instead of looping KvGoBack()
 * if recursive iteration is not important.
 *
 * @param kv			KeyValues Handle.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvRewind(Handle:kv);

/**
 * Retrieves the current section name.
 * 
 * @param kv			KeyValues Handle.
 * @param section		Buffer to store the section name.
 * @param maxlength		Maximum length of the name buffer.
 * @return				True on success, false on failure.
 * @error				Invalid Handle.
 */
native bool:KvGetSectionName(Handle:kv, String:section[], maxlength);

/**
 * Sets the current section name.
 * 
 * @param kv			KeyValues Handle.
 * @param section		Section name.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetSectionName(Handle:kv, const String:section[]);

/**
 * Returns the data type at a key.
 *
 * @param kv			KeyValues Handle.
 * @param key			Key name.
 * @return				KvDataType value of the key.
 * @error				Invalid Handle.
 */
native KvDataTypes:KvGetDataType(Handle:kv, const String:key[]);

/**
 * Converts a KeyValues tree to a file.  The tree is dumped
 * from the current position.
 *
 * @param kv			KeyValues Handle.
 * @param file			File to dump write to.
 * @return				True on success, false otherwise.
 * @error				Invalid Handle.
 */
native bool:KeyValuesToFile(Handle:kv, const String:file[]);

/**
 * Converts a file to a KeyValues tree.  The file is read into
 * the current position of the tree.
 *
 * @param kv			KeyValues Handle.
 * @param file			File to read from.
 * @return				True on success, false otherwise.
 * @error				Invalid Handle.
 */
native bool:FileToKeyValues(Handle:kv, const String:file[]);

/**
 * Sets whether or not the KeyValues parser will read escape sequences.
 * For example, \n would be read as a literal newline.  This defaults
 * to false for new KeyValues structures.
 *
 * @param kv			KeyValues Handle.
 * @param useEscapes	Whether or not to read escape sequences.
 * @noreturn
 * @error				Invalid Handle.
 */
native KvSetEscapeSequences(Handle:kv, bool:useEscapes);