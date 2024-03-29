/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#include "DebugReporter.h"
#include "Logger.h"
#include "PluginSys.h"
#include "sm_stringutil.h"

DebugReport g_DbgReporter;

/* I'm really lazy.  This should probably be exported to ISourcePawnEngine someday,
 * but we need to make sure the JIT will deal with the version bump.
 */
extern const char *GetSourcePawnErrorMessage(int error);

void DebugReport::OnSourceModAllInitialized()
{
	g_pSourcePawn->SetDebugListener(this);
}

void DebugReport::GenerateError(IPluginContext *ctx, cell_t func_idx, int err, const char *message, ...)
{
	va_list ap;
	char buffer[512];

	va_start(ap, message);
	UTIL_FormatArgs(buffer, sizeof(buffer), message, ap);
	va_end(ap);

	const char *plname = g_PluginSys.FindPluginByContext(ctx->GetContext())->GetFilename();
	const char *error = GetSourcePawnErrorMessage(err);

	if (error)
	{
		g_Logger.LogError("[SM] Plugin \"%s\" encountered error %d: %s", plname, err, error);
	} else {
		g_Logger.LogError("[SM] Plugin \"%s\" encountered unknown error %d", plname, err);
	}

	g_Logger.LogError("[SM] %s", buffer);

	if (func_idx != -1)
	{
		if (func_idx & 1)
		{
			func_idx >>= 1;
			sp_public_t *function;
			if (ctx->GetPublicByIndex(func_idx, &function) == SP_ERROR_NONE)
			{
				g_Logger.LogError("[SM] Unable to call function \"%s\" due to above error(s).", function->name);
			}
		}
	}
}

void DebugReport::GenerateCodeError(IPluginContext *pContext, uint32_t code_addr, int err, const char *message, ...)
{
	va_list ap;
	char buffer[512];

	va_start(ap, message);
	UTIL_FormatArgs(buffer, sizeof(buffer), message, ap);
	va_end(ap);

	const char *plname = g_PluginSys.FindPluginByContext(pContext->GetContext())->GetFilename();
	const char *error = GetSourcePawnErrorMessage(err);

	if (error)
	{
		g_Logger.LogError("[SM] Plugin \"%s\" encountered error %d: %s", plname, err, error);
	} else {
		g_Logger.LogError("[SM] Plugin \"%s\" encountered unknown error %d", plname, err);
	}

	g_Logger.LogError("[SM] %s", buffer);

	IPluginDebugInfo *pDebug;
	if ((pDebug = pContext->GetDebugInfo()) == NULL)
	{
		g_Logger.LogError("[SM] Debug mode is not enabled for \"%s\"", plname);
		g_Logger.LogError("[SM] To enable debug mode, edit plugin_settings.cfg, or type: sm plugins debug %d on",
			_GetPluginIndex(pContext));
		return;
	}

	const char *name;
	if (pDebug->LookupFunction(code_addr, &name) == SP_ERROR_NONE)
	{
		g_Logger.LogError("[SM] Unable to call function \"%s\" due to above error(s).", name);
	} else {
		g_Logger.LogError("[SM] Unable to call function (name unknown, address \"%x\").", code_addr);
	}
}

void DebugReport::OnContextExecuteError(IPluginContext *ctx, IContextTrace *error)
{
	const char *lastname;
	const char *plname = g_PluginSys.FindPluginByContext(ctx->GetContext())->GetFilename();
	int n_err = error->GetErrorCode();

	if (n_err != SP_ERROR_NATIVE)
	{
		g_Logger.LogError("[SM] Plugin encountered error %d: %s",
			n_err,
			error->GetErrorString());
	}

	if ((lastname=error->GetLastNative(NULL)) != NULL)
	{
		const char *custerr;
		if ((custerr=error->GetCustomErrorString()) != NULL)
		{
			g_Logger.LogError("[SM] Native \"%s\" reported: %s", lastname, custerr);
		} else {
			g_Logger.LogError("[SM] Native \"%s\" encountered a generic error.", lastname);
		}
	}

	if (!error->DebugInfoAvailable())
	{
		g_Logger.LogError("[SM] Debug mode is not enabled for \"%s\"", plname);
		g_Logger.LogError("[SM] To enable debug mode, edit plugin_settings.cfg, or type: sm plugins debug %d on",
			_GetPluginIndex(ctx));
		return;
	}

	CallStackInfo stk_info;
	int i = 0;
	g_Logger.LogError("[SM] Displaying call stack trace for plugin \"%s\":", plname);
	while (error->GetTraceInfo(&stk_info))
	{
		g_Logger.LogError("[SM]   [%d]  Line %d, %s::%s()",
			i++,
			stk_info.line,
			stk_info.filename,
			stk_info.function);
	}
}

int DebugReport::_GetPluginIndex(IPluginContext *ctx)
{
	int id = 1;
	IPluginIterator *iter = g_PluginSys.GetPluginIterator();

	for (; iter->MorePlugins(); iter->NextPlugin(), id++)
	{
		IPlugin *pl = iter->GetPlugin();
		if (pl->GetBaseContext() == ctx)
		{
			iter->Release();
			return id;
		}
	}

	iter->Release();

	/* If we don't know which plugin this is, it's one being loaded.  Fake its index for now. */

	return g_PluginSys.GetPluginCount() + 1;
}

