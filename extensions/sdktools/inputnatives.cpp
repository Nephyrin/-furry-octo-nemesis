/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod SDKTools Extension
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

#include "extension.h"
#include <datamap.h>

#define SIZEOF_VARIANT_T		20

ICallWrapper *g_pAcceptInput = NULL;
unsigned char g_Variant_t[SIZEOF_VARIANT_T] = {0};

#define ENTINDEX_TO_CBASEENTITY(index, buffer) \
	pEdict = engine->PEntityOfEntIndex(index); \
	if (!pEdict || pEdict->IsFree()) \
	{ \
		return pContext->ThrowNativeError("Entity %d is not valid or is freed", index); \
	} \
	pUnk = pEdict->GetUnknown(); \
	if (!pUnk) \
	{ \
		return pContext->ThrowNativeError("Entity %d is a not an IServerUnknown", index); \
	} \
	buffer = pUnk->GetBaseEntity(); \
	if (!buffer) \
	{ \
		return pContext->ThrowNativeError("Entity %d is not a CBaseEntity", index); \
	}

/* Hack to init the variant_t object for the first time */
class VariantFirstTimeInit
{
public:
	VariantFirstTimeInit()
	{
		*(unsigned int *)(&g_Variant_t[12]) = INVALID_EHANDLE_INDEX;
	}
} g_VariantFirstTimeInit;

inline void _init_variant_t()
{
	unsigned char *vptr = g_Variant_t;

	*(int *)vptr = 0;
	vptr += sizeof(int)*3;
	*(unsigned long *)vptr = INVALID_EHANDLE_INDEX;
	vptr += sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_VOID;
}

static cell_t AcceptEntityInput(IPluginContext *pContext, const cell_t *params)
{
	if (!g_pAcceptInput)
	{
		int offset;
		if (!g_pGameConf->GetOffset("AcceptInput", &offset))
		{
			return pContext->ThrowNativeError("\"AcceptEntityInput\" not supported by this mod");
		}

		PassInfo pass[6];
		pass[0].type = PassType_Basic;
		pass[0].flags = PASSFLAG_BYVAL;
		pass[0].size = sizeof(const char *);
		pass[1].type = pass[2].type = PassType_Basic;
		pass[1].flags = pass[2].flags = PASSFLAG_BYVAL;
		pass[1].size = pass[2].size = sizeof(CBaseEntity *);
		pass[3].type = PassType_Object;
		pass[3].flags = PASSFLAG_BYVAL|PASSFLAG_OCTOR|PASSFLAG_ODTOR|PASSFLAG_OASSIGNOP;
		pass[3].size = SIZEOF_VARIANT_T;
		pass[4].type = PassType_Basic;
		pass[4].flags = PASSFLAG_BYVAL;
		pass[4].size = sizeof(int);
		pass[5].type = PassType_Basic;
		pass[5].flags = PASSFLAG_BYVAL;
		pass[5].size = sizeof(bool);

		if (!(g_pAcceptInput=g_pBinTools->CreateVCall(offset, 0, 0, &pass[5], pass, 5)))
		{
			pContext->ThrowNativeError("\"AcceptEntityInput\" wrapper failed to initialized");
		}
	}

	CBaseEntity *pActivator, *pCaller, *pDest;
	edict_t *pEdict;
	IServerUnknown *pUnk;

	char *inputname;
	unsigned char vstk[sizeof(void *) + sizeof(const char *) + sizeof(CBaseEntity *)*2 + SIZEOF_VARIANT_T + sizeof(int)];
	unsigned char *vptr = vstk;

	ENTINDEX_TO_CBASEENTITY(params[1], pDest);
	pContext->LocalToString(params[2], &inputname);
	if (params[3] == -1)
	{
		pActivator = NULL;
	} else {
		ENTINDEX_TO_CBASEENTITY(params[3], pActivator);
	}
	if (params[4] == -1)
	{
		pCaller = NULL;
	} else {
		ENTINDEX_TO_CBASEENTITY(params[4], pCaller);
	}

	*(void **)vptr = pDest;
	vptr += sizeof(void *);
	*(const char **)vptr = inputname;
	vptr += sizeof(const char *);
	*(CBaseEntity **)vptr = pActivator;
	vptr += sizeof(CBaseEntity *);
	*(CBaseEntity **)vptr = pCaller;
	vptr += sizeof(CBaseEntity *);
	memcpy(vptr, g_Variant_t, SIZEOF_VARIANT_T);
	vptr += SIZEOF_VARIANT_T;
	*(int *)vptr = params[5];

	bool ret;
	g_pAcceptInput->Execute(vstk, &ret);

	_init_variant_t();

	return (ret) ? 1 : 0;
}

static cell_t SetVariantBool(IPluginContext *pContext, const cell_t *params)
{
	unsigned char *vptr = g_Variant_t;

	*(bool *)vptr = (params[1]) ? true : false;
	vptr += sizeof(int)*3 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_BOOLEAN;

	return 1;
}

static cell_t SetVariantString(IPluginContext *pContext, const cell_t *params)
{
	char *str;
	unsigned char *vptr = g_Variant_t;

	pContext->LocalToString(params[1], &str);

	*(string_t *)vptr = MAKE_STRING(str);
	vptr += sizeof(int)*3 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_STRING;

	return 1;
}

static cell_t SetVariantInt(IPluginContext *pContext, const cell_t *params)
{
	unsigned char *vptr = g_Variant_t;

	*(int *)vptr = params[1];
	vptr += sizeof(int)*3 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_INTEGER;

	return 1;
}

static cell_t SetVariantFloat(IPluginContext *pContext, const cell_t *params)
{
	unsigned char *vptr = g_Variant_t;

	*(float *)vptr = sp_ctof(params[1]);
	vptr += sizeof(int)*3 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_FLOAT;

	return 1;
}

static cell_t SetVariantVector3D(IPluginContext *pContext, const cell_t *params)
{
	cell_t *val;
	unsigned char *vptr = g_Variant_t;

	pContext->LocalToPhysAddr(params[1], &val);

	*(float *)vptr = sp_ctof(val[0]);
	vptr += sizeof(float);
	*(float *)vptr = sp_ctof(val[1]);
	vptr += sizeof(float);
	*(float *)vptr = sp_ctof(val[2]);
	vptr += sizeof(float) + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_VECTOR;

	return 1;
}

static cell_t SetVariantPosVector3D(IPluginContext *pContext, const cell_t *params)
{
	cell_t *val;
	unsigned char *vptr = g_Variant_t;

	pContext->LocalToPhysAddr(params[1], &val);

	*(float *)vptr = sp_ctof(val[0]);
	vptr += sizeof(float);
	*(float *)vptr = sp_ctof(val[1]);
	vptr += sizeof(float);
	*(float *)vptr = sp_ctof(val[2]);
	vptr += sizeof(float) + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_POSITION_VECTOR;

	return 1;
}

static cell_t SetVariantColor(IPluginContext *pContext, const cell_t *params)
{
	cell_t *val;
	unsigned char *vptr = g_Variant_t;

	pContext->LocalToPhysAddr(params[1], &val);

	*(unsigned char *)vptr = val[0];
	vptr += sizeof(unsigned char);
	*(unsigned char *)vptr = val[1];
	vptr += sizeof(unsigned char);
	*(unsigned char *)vptr = val[2];
	vptr += sizeof(unsigned char);
	*(unsigned char *)vptr = val[3];
	vptr += sizeof(unsigned char) + sizeof(int)*2 + sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_COLOR32;

	return 1;
}

static cell_t SetVariantEntity(IPluginContext *pContext, const cell_t *params)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;
	IServerUnknown *pUnk;
	unsigned char *vptr = g_Variant_t;
	CBaseHandle bHandle;

	ENTINDEX_TO_CBASEENTITY(params[1], pEntity);
	bHandle = reinterpret_cast<IHandleEntity *>(pEntity)->GetRefEHandle();

	vptr += sizeof(int)*3;
	*(unsigned long *)vptr = (unsigned long)(bHandle.ToInt());
	vptr += sizeof(unsigned long);
	*(fieldtype_t *)vptr = FIELD_EHANDLE;

	return 1;
}

sp_nativeinfo_t g_EntInputNatives[] =
{
	{"AcceptEntityInput",			AcceptEntityInput},
	{"SetVariantBool",				SetVariantBool},
	{"SetVariantString",			SetVariantString},
	{"SetVariantInt",				SetVariantInt},
	{"SetVariantFloat",				SetVariantFloat},
	{"SetVariantVector3D",			SetVariantVector3D},
	{"SetVariantPosVector3D",		SetVariantPosVector3D},
	{"SetVariantColor",				SetVariantColor},
	{"SetVariantEntity",			SetVariantEntity},
	{NULL,							NULL},
};
