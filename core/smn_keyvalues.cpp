/**
 * vim: set ts=4 :
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

#include "sourcemod.h"
#include "sourcemm_api.h"
#include "sm_stringutil.h"
#include "HandleSys.h"
#include <KeyValues.h>

HandleType_t g_KeyValueType;

struct KeyValueStack 
{
	KeyValues *pBase;
	CStack<KeyValues *> pCurRoot;
};

class KeyValuekNatives : 
	public SMGlobalClass,
	public IHandleTypeDispatch
{
public:
	void OnSourceModAllInitialized()
	{
		g_KeyValueType = g_HandleSys.CreateType("KeyValues", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	}
	void OnSourceModShutdown()
	{
		g_HandleSys.RemoveType(g_KeyValueType, g_pCoreIdent);
		g_KeyValueType = 0;
	}
	void OnHandleDestroy(HandleType_t type, void *object)
	{
		KeyValueStack *pStk = reinterpret_cast<KeyValueStack *>(object);
		pStk->pBase->deleteThis();
		delete pStk;
	}
};

static cell_t smn_KvSetString(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key, *value;
	pCtx->LocalToString(params[2], &key);
	pCtx->LocalToString(params[3], &value);

	pStk->pCurRoot.front()->SetString(key, value);

	return 1;
}

static cell_t smn_KvSetNum(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	pCtx->LocalToString(params[2], &key);

	pStk->pCurRoot.front()->SetInt(key, params[3]);

	return 1;
}

static cell_t smn_KvSetUint64(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	cell_t *addr;
	uint64 value;
	pCtx->LocalToString(params[2], &key);
	pCtx->LocalToPhysAddr(params[3], &addr);

	value = static_cast<uint64>(*addr);
	pStk->pCurRoot.front()->SetUint64(key, value);

	return 1;
}

static cell_t smn_KvSetFloat(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	pCtx->LocalToString(params[2], &key);

	pStk->pCurRoot.front()->SetFloat(key, sp_ctof(params[3]));

	return 1;
}

static cell_t smn_KvSetColor(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	pCtx->LocalToString(params[2], &key);

	Color color(params[3], params[4], params[5], params[6]);
	pStk->pCurRoot.front()->SetColor(key, color);

	return 1;
}

static cell_t smn_KvGetString(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	const char *value;
	char *key, *defvalue;
	pCtx->LocalToString(params[2], &key);
	pCtx->LocalToString(params[5], &defvalue);

	value = pStk->pCurRoot.front()->GetString(key, defvalue);
	pCtx->StringToLocalUTF8(params[3], params[4], value, NULL);

	return 1;
}

static cell_t smn_KvGetNum(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	int value;
	char *key;
	pCtx->LocalToString(params[2], &key);

	value = pStk->pCurRoot.front()->GetInt(key, params[3]);

	return value;
}

static cell_t smn_KvGetFloat(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	float value;
	char *key;
	pCtx->LocalToString(params[2], &key);

	value = pStk->pCurRoot.front()->GetFloat(key, sp_ctof(params[3]));

	return sp_ftoc(value);
}

static cell_t smn_KvGetColor(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	Color color;
	char *key;
	cell_t *r, *g, *b, *a;
	pCtx->LocalToString(params[2], &key);
	pCtx->LocalToPhysAddr(params[3], &r);
	pCtx->LocalToPhysAddr(params[4], &g);
	pCtx->LocalToPhysAddr(params[5], &b);
	pCtx->LocalToPhysAddr(params[6], &a);

	color = pStk->pCurRoot.front()->GetColor(key);
	*r = color.r();
	*g = color.g();
	*b = color.b();
	*a = color.a();

	return 1;
}

static cell_t smn_KvGetUint64(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	char *key;
	cell_t *addr, *defvalue;
	uint64 value;
	pCtx->LocalToString(params[2], &key);
	pCtx->LocalToPhysAddr(params[3], &addr);
	pCtx->LocalToPhysAddr(params[4], &defvalue);

	value = pStk->pCurRoot.front()->GetUint64(key, static_cast<uint64>(*defvalue));
	*reinterpret_cast<uint64 *>(addr) = value;

	return 1;
}

static cell_t smn_CreateKeyValues(IPluginContext *pCtx, const cell_t *params)
{
	KeyValueStack *pStk;
	char *name, *firstkey, *firstvalue;
	bool is_empty;

	pCtx->LocalToString(params[1], &name);
	pCtx->LocalToString(params[2], &firstkey);
	pCtx->LocalToString(params[3], &firstvalue);

	is_empty = (firstkey[0] == '\0');
	pStk = new KeyValueStack;
	pStk->pBase = new KeyValues(name, is_empty ? NULL : firstkey, (is_empty||(firstvalue[0]=='\0')) ? NULL : firstvalue);
	pStk->pCurRoot.push(pStk->pBase);

	return g_HandleSys.CreateHandle(g_KeyValueType, pStk, pCtx->GetIdentity(), g_pCoreIdent, NULL);
}

static cell_t smn_KvJumpToKey(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *name;	
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &name);

	KeyValues *pSubKey = pStk->pBase->FindKey(name, (params[3]) ? true : false);
	if (!pSubKey)
	{
		return 0;
	}
	pStk->pCurRoot.push(pSubKey);

	return 1;
}

static cell_t smn_KvJumpFirstSubKey(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	KeyValues *pSubKey = pStk->pCurRoot.front();
	KeyValues *pFirstSubKey = pSubKey->GetFirstSubKey();
	if (!pFirstSubKey)
	{
		return 0;
	}
	pStk->pCurRoot.push(pFirstSubKey);

	return 1;
}

static cell_t smn_KvJumpNextSubKey(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	KeyValues *pSubKey = pStk->pCurRoot.front();
	KeyValues *pNextKey = pSubKey->GetNextKey();
	if (!pNextKey)
	{
		return 0;
	}
	pStk->pCurRoot.push(pNextKey);

	return 1;	
}

static cell_t smn_KvGoBack(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	if (pStk->pCurRoot.size() == 1)
	{
		return 0;
	}
	pStk->pCurRoot.pop();

	return 1;
}

static cell_t smn_KvRewind(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	while (pStk->pCurRoot.size() > 1)
	{
		pStk->pCurRoot.pop();
	}

	return 1;
}

static cell_t smn_KvGetSectionName(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	KeyValues *pSection = pStk->pCurRoot.front();
	const char *name = pSection->GetName();
	if (!name)
	{
		return 0;
	}
	pCtx->StringToLocalUTF8(params[2], params[3], name, NULL);

	return 1;
}

static cell_t smn_KvSetSectionName(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *name;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &name);

	KeyValues *pSection = pStk->pCurRoot.front();
	pSection->SetName(name);

	return 1;
}

static cell_t smn_KvGetDataType(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *name;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &name);

	return pStk->pCurRoot.front()->GetDataType(name);
}

static cell_t smn_KeyValuesToFile(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *path;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &path);

	char realpath[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);

	return pStk->pCurRoot.front()->SaveToFile(basefilesystem, realpath);
}

static cell_t smn_FileToKeyValues(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	char *path;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pCtx->LocalToString(params[2], &path);

	char realpath[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_Game, realpath, sizeof(realpath), "%s", path);

	return pStk->pCurRoot.front()->LoadFromFile(basefilesystem, realpath);
}

static cell_t smn_KvSetEscapeSequences(IPluginContext *pCtx, const cell_t *params)
{
	Handle_t hndl = static_cast<Handle_t>(params[1]);
	HandleError herr;
	HandleSecurity sec;
	KeyValueStack *pStk;

	sec.pOwner = NULL;
	sec.pIdentity = g_pCoreIdent;

	if ((herr=g_HandleSys.ReadHandle(hndl, g_KeyValueType, &sec, (void **)&pStk))
		!= HandleError_None)
	{
		return pCtx->ThrowNativeError("Invalid key value handle %x (error %d)", hndl, herr);
	}

	pStk->pCurRoot.front()->UsesEscapeSequences(params[2] ? true : false);

	return 1;
}

static KeyValuekNatives s_KeyValuekNatives;

REGISTER_NATIVES(keyvaluenatives)
{
	{"KvSetString",				smn_KvSetString},
	{"KvSetNum",				smn_KvSetNum},
	{"KvSetUint64",				smn_KvSetUint64},
	{"KvSetFloat",				smn_KvSetFloat},
	{"KvSetColor",				smn_KvSetColor},
	{"KvGetString",				smn_KvGetString},
	{"KvGetNum",				smn_KvGetNum},
	{"KvGetFloat",				smn_KvGetFloat},
	{"KvGetColor",				smn_KvGetColor},
	{"KvGetUint64",				smn_KvGetUint64},
	{"CreateKeyValues",			smn_CreateKeyValues},
	{"KvJumpToKey",				smn_KvJumpToKey},
	{"KvJumpFirstSubKey",		smn_KvJumpFirstSubKey},
	{"KvJumpNextSubKey",		smn_KvJumpNextSubKey},
	{"KvGoBack",				smn_KvGoBack},
	{"KvRewind",				smn_KvRewind},
	{"KvGetSectionName",		smn_KvGetSectionName},
	{"KvSetSectionName",		smn_KvSetSectionName},
	{"KvGetDataType",			smn_KvGetDataType},
	{"KeyValuesToFile",			smn_KeyValuesToFile},
	{"FileToKeyValues",			smn_FileToKeyValues},
	{"KvSetEscapeSequences",	smn_KvSetEscapeSequences},
	{NULL,						NULL}
};