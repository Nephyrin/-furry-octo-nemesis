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

#include <stdio.h>
#include "PluginSys.h"
#include "ShareSys.h"
#include "LibrarySys.h"
#include "HandleSys.h"
#include "ForwardSys.h"
#include "sourcemm_api.h"
#include "sourcemod.h"
#include "TextParsers.h"
#include "Logger.h"
#include "ExtensionSys.h"
#include "sm_srvcmds.h"
#include "sm_stringutil.h"
#include "ConCmdManager.h"
#include "PlayerManager.h"
#include "CoreConfig.h"
#include "Translator.h"

CPluginManager g_PluginSys;
HandleType_t g_PluginType = 0;
IdentityType_t g_PluginIdent = 0;

CPlugin::CPlugin(const char *file)
{
	static int MySerial = 0;

	m_type = PluginType_Private;
	m_status = Plugin_Uncompiled;
	m_serial = ++MySerial;
	m_plugin = NULL;
	m_errormsg[256] = '\0';
	snprintf(m_filename, sizeof(m_filename), "%s", file);
	m_handle = 0;
	m_ident = NULL;
	m_pProps = sm_trie_create();
	m_FakeNativesMissing = false;
	m_LibraryMissing = false;
	m_bGotAllLoaded = false;
}

CPlugin::~CPlugin()
{
	if (m_handle)
	{
		HandleSecurity sec;
		sec.pOwner = g_PluginSys.GetIdentity();
		sec.pIdentity = sec.pOwner;

		g_HandleSys.FreeHandle(m_handle, &sec);
		g_ShareSys.DestroyIdentity(m_ident);
	}

	if (m_ctx.base)
	{
		delete m_ctx.base;
		m_ctx.base = NULL;
	}
	if (m_ctx.ctx)
	{
		m_ctx.vm->FreeContext(m_ctx.ctx);
		m_ctx.ctx = NULL;
	}
	if (m_ctx.co)
	{
		m_ctx.vm->AbortCompilation(m_ctx.co);
		m_ctx.co = NULL;
	}

	if (m_plugin)
	{
		g_pSourcePawn->FreeFromMemory(m_plugin);
		m_plugin = NULL;
	}
	if (m_pProps)
	{
		sm_trie_destroy(m_pProps);
	}
	for (size_t i=0; i<m_configs.size(); i++)
	{
		delete m_configs[i];
	}
	m_configs.clear();
}

void CPlugin::InitIdentity()
{
	if (!m_handle)
	{
		m_ident = g_ShareSys.CreateIdentity(g_PluginIdent, this);
		m_handle = g_HandleSys.CreateHandle(g_PluginType, this, g_PluginSys.GetIdentity(), g_PluginSys.GetIdentity(), NULL);
		m_ctx.base->SetIdentity(m_ident);
	}
}

unsigned int CPlugin::CalcMemUsage()
{
	unsigned int base_size = 
		sizeof(CPlugin) 
		+ sizeof(IdentityToken_t)
		+ (m_PhraseFiles.size() * sizeof(unsigned int))
		+ (m_dependents.size() * sizeof(CPlugin *))
		+ (m_dependsOn.size() * sizeof(CPlugin *))
		+ (m_fakeNatives.size() * (sizeof(FakeNative *) + sizeof(FakeNative)))
		+ (m_WeakNatives.size() * sizeof(WeakNative))
		+ (m_configs.size() * (sizeof(AutoConfig *) + sizeof(AutoConfig)))
		+ sm_trie_mem_usage(m_pProps);

	for (unsigned int i = 0; i < m_configs.size(); i++)
	{
		base_size += m_configs[i]->autocfg.size();
		base_size += m_configs[i]->folder.size();
	}

	for (List<String>::iterator i = m_Libraries.begin();
		 i != m_Libraries.end();
		 i++)
	{
		base_size += (*i).size();
	}

	for (List<String>::iterator i = m_RequiredLibs.begin();
		 i != m_RequiredLibs.end();
		 i++)
	{
		base_size += (*i).size();
	}

	for (List<FakeNative *>::iterator i = m_fakeNatives.begin();
		 i != m_fakeNatives.end();
		 i++)
	{
		base_size += (*i)->name.size();
	}

	if (m_plugin != NULL)
	{
		base_size += sizeof(sp_plugin_t);
		base_size += m_plugin->data_size;
		base_size += m_plugin->pcode_size;
		base_size += (m_plugin->info.natives_num * sizeof(sp_file_natives_t));
		base_size += (m_plugin->info.publics_num * sizeof(sp_file_publics_t));
		base_size += (m_plugin->info.pubvars_num * sizeof(sp_file_pubvars_t));
		base_size += (m_plugin->debug.files_num * sizeof(sp_fdbg_file_t));
		base_size += (m_plugin->debug.lines_num * sizeof(sp_fdbg_line_t));
		base_size += (m_plugin->debug.syms_num * sizeof(sp_fdbg_symbol_t));
		/* We can't get strtab size, oh well. */
	}

	if (m_ctx.base != NULL)
	{
		base_size += sizeof(BaseContext);
		base_size += m_ctx.base->GetPublicsNum() * sizeof(CFunction);
	}
	if (m_ctx.ctx != NULL)
	{
		base_size += m_ctx.ctx->mem_size;
		base_size += (m_plugin->debug.files_num * sizeof(sp_debug_file_t));
		base_size += (m_plugin->debug.lines_num * sizeof(sp_debug_line_t));
		base_size += (m_plugin->debug.syms_num * sizeof(sp_debug_symbol_t));
		base_size += (m_plugin->info.pubvars_num * sizeof(sp_pubvar_t));
		base_size += (m_plugin->info.publics_num * sizeof(sp_public_t));
		base_size += (m_plugin->info.natives_num * sizeof(sp_native_t));
		/* We also don't know the JIT code size, oh well. */
	}

	return base_size;
}

Handle_t CPlugin::GetMyHandle()
{
	return m_handle;
}

CPlugin *CPlugin::CreatePlugin(const char *file, char *error, size_t maxlength)
{
	char fullpath[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_SM, fullpath, sizeof(fullpath), "plugins/%s", file);
	FILE *fp = fopen(fullpath, "rb");

	CPlugin *pPlugin = new CPlugin(file);

	if (!fp)
	{
		if (error)
		{
			snprintf(error, maxlength, "Unable to open file");
		}
		pPlugin->m_status = Plugin_BadLoad;
		return pPlugin;
	}

	int err;
	sp_plugin_t *pl = g_pSourcePawn->LoadFromFilePointer(fp, &err);
	if (pl == NULL)
	{
		fclose(fp);
		if (error)
		{
			snprintf(error, maxlength, "Error %d while parsing plugin", err);
		}
		pPlugin->m_status = Plugin_BadLoad;
		return pPlugin;
	}

	fclose(fp);

	pPlugin->m_plugin = pl;

	return pPlugin;
}

bool CPlugin::GetProperty(const char *prop, void **ptr, bool remove/* =false */)
{
	bool exists = sm_trie_retrieve(m_pProps, prop, ptr);

	if (exists && remove)
	{
		sm_trie_delete(m_pProps, prop);
	}

	return exists;
}

bool CPlugin::SetProperty(const char *prop, void *ptr)
{
	return sm_trie_insert(m_pProps, prop, ptr);
}

ICompilation *CPlugin::StartMyCompile(IVirtualMachine *vm)
{
	if (!m_plugin)
	{
		return NULL;
	}

	/* :NOTICE: We will eventually need to change these natives
	 * for swapping in new contexts
	 */
	if (m_ctx.co || m_ctx.ctx)
	{
		return NULL;
	}

	m_status = Plugin_Uncompiled;

	m_ctx.vm = vm;
	m_ctx.co = vm->StartCompilation(m_plugin);

	return m_ctx.co;
}

void CPlugin::CancelMyCompile()
{
	if (!m_ctx.co)
	{
		return;
	}

	m_ctx.vm->AbortCompilation(m_ctx.co);
	m_ctx.co = NULL;
	m_ctx.vm = NULL;
}

bool CPlugin::FinishMyCompile(char *error, size_t maxlength)
{
	if (!m_ctx.co)
	{
		return false;
	}

	int err;
	m_ctx.ctx = m_ctx.vm->CompileToContext(m_ctx.co, &err);
	if (!m_ctx.ctx)
	{
		memset(&m_ctx, 0, sizeof(m_ctx));
		if (error)
		{
			snprintf(error, maxlength, "JIT failed to compile (error %d)", err);
		}
		return false;
	}

	m_ctx.base = new BaseContext(m_ctx.ctx);
	m_ctx.ctx->user[SM_CONTEXTVAR_MYSELF] = (void *)this;

	m_status = Plugin_Created;
	m_ctx.co = NULL;

	UpdateInfo();

	return true;
}

void CPlugin::SetErrorState(PluginStatus status, const char *error_fmt, ...)
{
	PluginStatus old_status = m_status;
	m_status = status;

	if (old_status == Plugin_Running)
	{
		/* Tell everyone we're now paused */
		g_PluginSys._SetPauseState(this, true);
	}

	va_list ap;
	va_start(ap, error_fmt);
	vsnprintf(m_errormsg, sizeof(m_errormsg), error_fmt, ap);
	va_end(ap);

	if (m_ctx.ctx)
	{
		m_ctx.ctx->flags |= SPFLAG_PLUGIN_PAUSED;
	}
}

void CPlugin::UpdateInfo()
{
	/* Now grab the info */
	uint32_t idx;
	IPluginContext *base = GetBaseContext();
	int err = base->FindPubvarByName("myinfo", &idx);

	memset(&m_info, 0, sizeof(m_info));

	if (err == SP_ERROR_NONE)
	{
		struct sm_plugininfo_c_t
		{
			cell_t name;
			cell_t description;
			cell_t author;
			cell_t version;
			cell_t url;
		};
		sm_plugininfo_c_t *cinfo;
		cell_t local_addr;

		base->GetPubvarAddrs(idx, &local_addr, (cell_t **)&cinfo);
		base->LocalToString(cinfo->name, (char **)&m_info.name);
		base->LocalToString(cinfo->description, (char **)&m_info.description);
		base->LocalToString(cinfo->author, (char **)&m_info.author);
		base->LocalToString(cinfo->url, (char **)&m_info.url);
		base->LocalToString(cinfo->version, (char **)&m_info.version);
	}

	m_info.author = m_info.author ? m_info.author : "";
	m_info.description = m_info.description ? m_info.description : "";
	m_info.name = m_info.name ? m_info.name : "";
	m_info.url = m_info.url ? m_info.url : "";
	m_info.version = m_info.version ? m_info.version : "";
}

void CPlugin::Call_OnPluginStart()
{
	if (m_status != Plugin_Loaded)
	{
		return;
	}

	m_status = Plugin_Running;

	cell_t result;
	IPluginFunction *pFunction = m_ctx.base->GetFunctionByName("OnPluginStart");
	if (!pFunction)
	{
		return;
	}

	int err;
	if ((err=pFunction->Execute(&result)) != SP_ERROR_NONE)
	{
		SetErrorState(Plugin_Error, "Error detected in plugin startup (see error logs)");
	}
}

void CPlugin::Call_OnPluginEnd()
{
	if (m_status > Plugin_Paused)
	{
		return;
	}

	cell_t result;
	IPluginFunction *pFunction = m_ctx.base->GetFunctionByName("OnPluginEnd");
	if (!pFunction)
	{
		return;
	}

	pFunction->Execute(&result);
}

void CPlugin::Call_OnAllPluginsLoaded()
{
	if (m_status > Plugin_Paused)
	{
		return;
	}

	if (m_bGotAllLoaded)
	{
		return;
	}

	m_bGotAllLoaded = true;

	cell_t result;
	IPluginFunction *pFunction = m_ctx.base->GetFunctionByName("OnAllPluginsLoaded");
	if (pFunction != NULL)
	{
		pFunction->Execute(&result);
	}

	if (g_OnMapStarted)
	{
		if ((pFunction = m_ctx.base->GetFunctionByName("OnMapStart")) != NULL)
		{
			pFunction->Execute(NULL);
		}
	}

	if (SM_AreConfigsExecuted())
	{
		SM_ExecuteForPlugin(GetBaseContext());
	}
}

bool CPlugin::Call_AskPluginLoad(char *error, size_t maxlength)
{
	if (m_status != Plugin_Created)
	{
		return false;
	}

	m_status = Plugin_Loaded;

	int err;
	cell_t result;
	IPluginFunction *pFunction = m_ctx.base->GetFunctionByName("AskPluginLoad");

	if (!pFunction)
	{
		return true;
	}

	pFunction->PushCell(m_handle);
	pFunction->PushCell(g_PluginSys.IsLateLoadTime() ? 1 : 0);
	pFunction->PushStringEx(error, maxlength, 0, SM_PARAM_COPYBACK);
	pFunction->PushCell(maxlength);
	if ((err=pFunction->Execute(&result)) != SP_ERROR_NONE)
	{
		return false;
	}

	if (!result || m_status != Plugin_Loaded)
	{
		return false;
	}

	return true;
}

const sp_plugin_t *CPlugin::GetPluginStructure()
{
	return m_plugin;
}

IPluginContext *CPlugin::GetBaseContext()
{
	return m_ctx.base;
}

sp_context_t *CPlugin::GetContext()
{
	return m_ctx.ctx;
}

const char *CPlugin::GetFilename()
{
	return m_filename;
}

PluginType CPlugin::GetType()
{
	return m_type;
}

const sm_plugininfo_t *CPlugin::GetPublicInfo()
{
	return &m_info;
}

unsigned int CPlugin::GetSerial()
{
	return m_serial;
}

PluginStatus CPlugin::GetStatus()
{
	return m_status;
}

bool CPlugin::IsDebugging()
{
	if (!m_ctx.ctx)
	{
		return false;
	}

	return ((m_ctx.ctx->flags & SP_FLAG_DEBUG) == SP_FLAG_DEBUG);
}

void CPlugin::LibraryActions(bool dropping)
{
	List<String>::iterator iter;
	for (iter = m_Libraries.begin();
		iter != m_Libraries.end();
		iter++)
	{
		g_PluginSys.OnLibraryAction((*iter).c_str(), true, dropping);
	}
}

bool CPlugin::SetPauseState(bool paused)
{
	if (paused && GetStatus() != Plugin_Running)
	{
		return false;
	} else if (!paused && GetStatus() != Plugin_Paused) {
		return false;
	}

	if (paused)
	{
		LibraryActions(true);
	}

	IPluginFunction *pFunction = m_ctx.base->GetFunctionByName("OnPluginPauseChange");
	if (pFunction)
	{
		cell_t result;
		pFunction->PushCell(paused ? 1 : 0);
		pFunction->Execute(&result);
	}

	if (paused)
	{
		m_status = Plugin_Paused;
		m_ctx.ctx->flags |= SPFLAG_PLUGIN_PAUSED;
	} else {
		m_status = Plugin_Running;
		m_ctx.ctx->flags &= ~SPFLAG_PLUGIN_PAUSED;
	}

	g_PluginSys._SetPauseState(this, paused);

	if (!paused)
	{
		LibraryActions(false);
	}

	return true;
}

IdentityToken_t *CPlugin::GetIdentity()
{
	return m_ident;
}

bool CPlugin::ToggleDebugMode(bool debug, char *error, size_t maxlength)
{
	int err;

	if (!IsRunnable())
	{
		if (error)
		{
			snprintf(error, maxlength, "Plugin is not runnable.");
		}
		return false;
	}

	if (debug && IsDebugging())
	{
		if (error)
		{
			snprintf(error, maxlength, "Plugin is already in debug mode.");
		}
		return false;
	}
	else if (!debug && !IsDebugging())
	{
		if (error)
		{
			snprintf(error, maxlength, "Plugins is already in production mode.");
		}
		return false;
	}

	ICompilation *co = g_pVM->StartCompilation(m_ctx.ctx->plugin);
	if (!g_pVM->SetCompilationOption(co, "debug", (debug) ? "1" : "0"))
	{
		if (error)
		{
			snprintf(error, maxlength, "Failed to change plugin mode (JIT failure).");
		}
		return false;
	}

	sp_context_t *new_ctx = g_pVM->CompileToContext(co, &err);

	if (new_ctx)
	{
		memcpy(new_ctx->memory, m_ctx.ctx->memory, m_ctx.ctx->mem_size);
		new_ctx->hp = m_ctx.ctx->hp;
		new_ctx->sp = m_ctx.ctx->sp;
		new_ctx->frm = m_ctx.ctx->frm;
		new_ctx->dbreak = m_ctx.ctx->dbreak;
		new_ctx->context = m_ctx.ctx->context;
		memcpy(new_ctx->user, m_ctx.ctx->user, sizeof(m_ctx.ctx->user));

		uint32_t nativeCount = m_plugin->info.natives_num;
		for (uint32_t i=0; i<nativeCount; i++)
		{
			new_ctx->natives[i].pfn = m_ctx.ctx->natives[i].pfn;
			new_ctx->natives[i].status = m_ctx.ctx->natives[i].status;
		}

		g_pVM->FreeContext(m_ctx.ctx);
		m_ctx.ctx = new_ctx;
		m_ctx.base->SetContext(new_ctx);

		UpdateInfo();
	}
	else
	{
		if (error)
		{
			snprintf(error, maxlength, "Failed to recompile plugin (JIT error %d).", err);
		}
		return false;
	}

	return true;
}

bool CPlugin::IsRunnable()
{
	return (m_status <= Plugin_Paused) ? true : false;
}

time_t CPlugin::GetFileTimeStamp()
{
	char path[PLATFORM_MAX_PATH];
	g_SourceMod.BuildPath(Path_SM, path, sizeof(path), "plugins/%s", m_filename);
#ifdef PLATFORM_WINDOWS
	struct _stat s;
	if (_stat(path, &s) != 0)
#elif defined PLATFORM_POSIX
	struct stat s;
	if (stat(path, &s) != 0)
#endif
	{
		return 0;
	}
	else
	{
		return s.st_mtime;
	}
}

time_t CPlugin::GetTimeStamp()
{
	return m_LastAccess;
}

void CPlugin::SetTimeStamp(time_t t)
{
	m_LastAccess = t;
}

void CPlugin::AddLangFile(unsigned int index)
{
	m_PhraseFiles.push_back(index);
}

size_t CPlugin::GetLangFileCount()
{
	return m_PhraseFiles.size();
}

unsigned int CPlugin::GetLangFileByIndex(unsigned int index)
{
	return m_PhraseFiles.at(index);
}

void CPlugin::DependencyDropped(CPlugin *pOwner)
{
	if (!m_ctx.ctx)
	{
		return;
	}

	List<String>::iterator reqlib_iter;
	List<String>::iterator lib_iter;
	for (lib_iter=pOwner->m_Libraries.begin(); lib_iter!=pOwner->m_Libraries.end(); lib_iter++)
	{
		for (reqlib_iter=m_RequiredLibs.begin(); reqlib_iter!=m_RequiredLibs.end(); reqlib_iter++)
		{
			if ((*reqlib_iter) == (*lib_iter))
			{
				m_LibraryMissing = true;
			}
		}	
	}

	List<FakeNative *>::iterator iter;
	FakeNative *pNative;
	sp_native_t *native;
	uint32_t idx;
	unsigned int unbound = 0;

	for (iter = pOwner->m_fakeNatives.begin();
		 iter != pOwner->m_fakeNatives.end();
		 iter++)
	{
		pNative = (*iter);
		/* Find this native! */
		if (m_ctx.base->FindNativeByName(pNative->name.c_str(), &idx) != SP_ERROR_NONE)
		{
			continue;
		}
		/* Unbind it */
		m_ctx.base->GetNativeByIndex(idx, &native);
		native->pfn = NULL;
		native->status = SP_NATIVE_UNBOUND;
		unbound++;
	}

	if (unbound)
	{
		m_FakeNativesMissing = true;
	}

	/* :IDEA: in the future, add native trapping? */
	if (m_FakeNativesMissing || m_LibraryMissing)
	{
		SetErrorState(Plugin_Error, "Depends on plugin: %s", pOwner->GetFilename());
	}

	m_dependsOn.remove(pOwner);
}

unsigned int CPlugin::GetConfigCount()
{
	return (unsigned int)m_configs.size();
}

AutoConfig *CPlugin::GetConfig(unsigned int i)
{
	if (i >= GetConfigCount())
	{
		return NULL;
	}

	return m_configs[i];
}

void CPlugin::AddConfig(bool autoCreate, const char *cfg, const char *folder)
{
	/* Do a check for duplicates to prevent double-execution */
	for (size_t i = 0; i < m_configs.size(); i++)
	{
		if (m_configs[i]->autocfg.compare(cfg) == 0
			&& m_configs[i]->folder.compare(folder) == 0
			&& m_configs[i]->create == autoCreate)
		{
			return;
		}
	}

	AutoConfig *c = new AutoConfig;

	c->autocfg = cfg;
	c->folder = folder;
	c->create = autoCreate;

	m_configs.push_back(c);
}

/*******************
 * PLUGIN ITERATOR *
 *******************/

CPluginManager::CPluginIterator::CPluginIterator(List<CPlugin *> *_mylist)
{
	mylist = _mylist;
	Reset();
}

IPlugin *CPluginManager::CPluginIterator::GetPlugin()
{
	return (*current);
}

bool CPluginManager::CPluginIterator::MorePlugins()
{
	return (current != mylist->end());
}

void CPluginManager::CPluginIterator::NextPlugin()
{
	current++;
}

void CPluginManager::CPluginIterator::Release()
{
	g_PluginSys.ReleaseIterator(this);
}

CPluginManager::CPluginIterator::~CPluginIterator()
{
}

void CPluginManager::CPluginIterator::Reset()
{
	current = mylist->begin();
}

/******************
 * PLUGIN MANAGER *
 ******************/

CPluginManager::CPluginManager()
{
	m_LoadLookup = sm_trie_create();
	m_AllPluginsLoaded = false;
	m_MyIdent = NULL;
	m_pNativeLookup = sm_trie_create();
	m_pCoreNatives = sm_trie_create();
	m_LoadingLocked = false;
}

CPluginManager::~CPluginManager()
{
	/* :NOTICE: 
	 * Ignore the fact that there might be plugins in the cache.
	 * This usually means that Core is not being unloaded properly, and everything
	 * will crash anyway.  YAY
	 */
	sm_trie_destroy(m_LoadLookup);
	sm_trie_destroy(m_pNativeLookup);
	sm_trie_destroy(m_pCoreNatives);

	CStack<CPluginManager::CPluginIterator *>::iterator iter;
	for (iter=m_iters.begin(); iter!=m_iters.end(); iter++)
	{
		delete (*iter);
	}
	m_iters.popall();
}

void CPluginManager::Shutdown()
{
	List<CPlugin *>::iterator iter;

	while ((iter = m_plugins.begin()) != m_plugins.end())
	{
		UnloadPlugin(*iter);
	}
}

void CPluginManager::LoadAll_FirstPass(const char *config, const char *basedir)
{
	/* First read in the database of plugin settings */
	SMCError err;
	SMCStates states;
	m_AllPluginsLoaded = false;
	if ((err=textparsers->ParseFile_SMC(config, &m_PluginInfo, &states)) != SMCError_Okay)
	{
		g_Logger.LogError("[SM] Encountered fatal error parsing file \"%s\"", config);
		const char *err_msg = textparsers->GetSMCErrorString(err);
		if (err_msg)
		{
			g_Logger.LogError("[SM] Parse error encountered: \"%s\"", err_msg);
		}
	}

	LoadPluginsFromDir(basedir, NULL);
}

void CPluginManager::LoadPluginsFromDir(const char *basedir, const char *localpath)
{
	char base_path[PLATFORM_MAX_PATH];

	/* Form the current path to start reading from */
	if (localpath == NULL)
	{
		g_LibSys.PathFormat(base_path, sizeof(base_path), "%s", basedir);
	} else {
		g_LibSys.PathFormat(base_path, sizeof(base_path), "%s/%s", basedir, localpath);
	}

	IDirectory *dir = g_LibSys.OpenDirectory(base_path);

	if (!dir)
	{
		char error[256];
		g_LibSys.GetPlatformError(error, sizeof(error));
		g_Logger.LogError("[SM] Failure reading from plugins path: %s", localpath);
		g_Logger.LogError("[SM] Platform returned error: %s", error);
		return;
	}

	while (dir->MoreFiles())
	{
		if (dir->IsEntryDirectory() 
			&& (strcmp(dir->GetEntryName(), ".") != 0)
			&& (strcmp(dir->GetEntryName(), "..") != 0)
			&& (strcmp(dir->GetEntryName(), "disabled") != 0)
			&& (strcmp(dir->GetEntryName(), "optional") != 0))
		{
			char new_local[PLATFORM_MAX_PATH];
			if (localpath == NULL)
			{
				/* If no path yet, don't add a former slash */
				snprintf(new_local, sizeof(new_local), "%s", dir->GetEntryName());
			} else {
				g_LibSys.PathFormat(new_local, sizeof(new_local), "%s/%s", localpath, dir->GetEntryName());
			}
			LoadPluginsFromDir(basedir, new_local);
		} else if (dir->IsEntryFile()) {
			const char *name = dir->GetEntryName();
			size_t len = strlen(name);
			if (len >= 4
				&& strcmp(&name[len-4], ".smx") == 0)
			{
				/* If the filename matches, load the plugin */
				char plugin[PLATFORM_MAX_PATH];
				if (localpath == NULL)
				{
					snprintf(plugin, sizeof(plugin), "%s", name);
				} else {
					g_LibSys.PathFormat(plugin, sizeof(plugin), "%s/%s", localpath, name);
				}
				LoadAutoPlugin(plugin);
			}
		}
		dir->NextEntry();
	}
	g_LibSys.CloseDirectory(dir);
}

LoadRes CPluginManager::_LoadPlugin(CPlugin **_plugin, const char *path, bool debug, PluginType type, char error[], size_t maxlength)
{
	if (m_LoadingLocked)
	{
		return LoadRes_NeverLoad;
	}

	bool no_load = false;
	PluginSettings *pset;
	unsigned int setcount = m_PluginInfo.GetSettingsNum();
	for (unsigned int i=0; i<setcount; i++)
	{
		if ((pset = m_PluginInfo.GetSettingsIfMatch(i, path)) == NULL)
		{
			continue;
		}
		if (pset->blockload_val)
		{
			no_load = true;
			break;
		}
	}

	/**
	 * Does this plugin already exist?
	 */
	CPlugin *pPlugin;
	if (sm_trie_retrieve(m_LoadLookup, path, (void **)&pPlugin))
	{
		/* Check to see if we should try reloading it */
		if (pPlugin->GetStatus() == Plugin_BadLoad
			|| pPlugin->GetStatus() == Plugin_Error
			|| pPlugin->GetStatus() == Plugin_Failed
			|| no_load)
		{
			UnloadPlugin(pPlugin);
		}
		else
		{
			if (_plugin)
			{
				*_plugin = pPlugin;
			}
			return LoadRes_AlreadyLoaded;
		}
	}

	if (no_load)
	{
		return LoadRes_NeverLoad;
	}

	pPlugin = CPlugin::CreatePlugin(path, error, maxlength);

	assert(pPlugin != NULL);

	pPlugin->m_type = PluginType_MapUpdated;

	ICompilation *co = NULL;

	if (pPlugin->m_status == Plugin_Uncompiled)
	{
		co = pPlugin->StartMyCompile(g_pVM);
	}

	for (unsigned int i=0; i<setcount; i++)
	{
		if ((pset=m_PluginInfo.GetSettingsIfMatch(i, path)) == NULL)
		{
			continue;
		}
		pPlugin->m_type = pset->type_val;
		if (co)
		{
			for (unsigned int j=0; j<pset->opts_num; j++)
			{
				const char *key, *val;
				m_PluginInfo.GetOptionsForPlugin(pset, j, &key, &val);
				if (!key || !val)
				{
					continue;
				}
				if (!g_pVM->SetCompilationOption(co, key, val))
				{
					if (error)
					{
						snprintf(error, maxlength, "Unable to set JIT option (key \"%s\") (value \"%s\")", key, val);
					}
					pPlugin->CancelMyCompile();
					co = NULL;
					break;
				}
			}
		}
	}

	/* Do the actual compiling */
	if (co)
	{
		pPlugin->FinishMyCompile(error, maxlength);
		co = NULL;
	}

	/* Get the status */
	if (pPlugin->GetStatus() == Plugin_Created)
	{
		AddCoreNativesToPlugin(pPlugin);
		pPlugin->InitIdentity();
		if (pPlugin->Call_AskPluginLoad(error, maxlength))
		{
			/* Autoload any modules */
			LoadOrRequireExtensions(pPlugin, 1, error, maxlength);
		}
		else
		{
			pPlugin->SetErrorState(Plugin_Failed, "%s", error);
		}
	}

	/* Save the time stamp */
	time_t t = pPlugin->GetFileTimeStamp();
	pPlugin->SetTimeStamp(t);

	if (_plugin)
	{
		*_plugin = pPlugin;
	}

	return (pPlugin->GetStatus() == Plugin_Loaded) ? LoadRes_Successful : LoadRes_Failure;
}

IPlugin *CPluginManager::LoadPlugin(const char *path, bool debug, PluginType type, char error[], size_t maxlength, bool *wasloaded)
{
	CPlugin *pl;
	LoadRes res;

	*wasloaded = false;
	if ((res=_LoadPlugin(&pl, path, debug, type, error, maxlength)) == LoadRes_Failure)
	{
		delete pl;
		return NULL;
	}

	if (res == LoadRes_AlreadyLoaded)
	{
		*wasloaded = true;
		return pl;
	}

	if (res == LoadRes_NeverLoad)
	{
		if (m_LoadingLocked)
		{
			UTIL_Format(error, maxlength, "There is a global plugin loading lock in effect");
		}
		else
		{
			UTIL_Format(error, maxlength, "This plugin is blocked from loading (see plugin_settings.cfg)");
		}
		return NULL;
	}

	AddPlugin(pl);

	/* Run second pass if we need to */
	if (IsLateLoadTime() && pl->GetStatus() == Plugin_Loaded)
	{
		if (!RunSecondPass(pl, error, maxlength))
		{
			UnloadPlugin(pl);
			return NULL;
		}
		pl->Call_OnAllPluginsLoaded();
	}

	return pl;
}

void CPluginManager::LoadAutoPlugin(const char *plugin)
{
	CPlugin *pl = NULL;
	LoadRes res;
	char error[255] = "Unknown error";

	if ((res=_LoadPlugin(&pl, plugin, false, PluginType_MapUpdated, error, sizeof(error))) == LoadRes_Failure)
	{
		g_Logger.LogError("[SM] Failed to load plugin \"%s\": %s", plugin, error);
		pl->SetErrorState(Plugin_Failed, "%s", error);
	}

	if (res == LoadRes_Successful)
	{
		AddPlugin(pl);
	}
}

void CPluginManager::AddPlugin(CPlugin *pPlugin)
{
	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;

	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginCreated(pPlugin);
	}

	m_plugins.push_back(pPlugin);
	sm_trie_insert(m_LoadLookup, pPlugin->m_filename, pPlugin);
}

void CPluginManager::LoadAll_SecondPass()
{
	List<CPlugin *>::iterator iter;
	CPlugin *pPlugin;

	char error[256];
	for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++)
	{
		pPlugin = (*iter);
		if (pPlugin->GetStatus() == Plugin_Loaded)
		{
			error[0] = '\0';
			if (!RunSecondPass(pPlugin, error, sizeof(error)))
			{
				g_Logger.LogError("[SM] Unable to load plugin \"%s\": %s", pPlugin->GetFilename(), error);
				pPlugin->SetErrorState(Plugin_Failed, "%s", error);
			}
		}
	}

	m_AllPluginsLoaded = true;
}

bool CPluginManager::FindOrRequirePluginDeps(CPlugin *pPlugin, char *error, size_t maxlength)
{
	struct _pl
	{
		cell_t name;
		cell_t file;
		cell_t required;
	} *pl;

	IPluginContext *pBase = pPlugin->GetBaseContext();
	uint32_t num = pBase->GetPubVarsNum();
	sp_pubvar_t *pubvar;
	char *name, *file;
	char pathfile[PLATFORM_MAX_PATH];

	for (uint32_t i=0; i<num; i++)
	{
		if (pBase->GetPubvarByIndex(i, &pubvar) != SP_ERROR_NONE)
		{
			continue;
		}
		if (strncmp(pubvar->name, "__pl_", 5) == 0)
		{
			pl = (_pl *)pubvar->offs;
			if (pBase->LocalToString(pl->file, &file) != SP_ERROR_NONE)
			{
				continue;
			}
			if (pBase->LocalToString(pl->name, &name) != SP_ERROR_NONE)
			{
				continue;
			}
			g_LibSys.GetFileFromPath(pathfile, sizeof(pathfile), pPlugin->GetFilename());
			if (strcmp(pathfile, file) == 0)
			{
				continue;
			}
			if (pl->required == false)
			{
				IPluginFunction *pFunc;
				char buffer[64];
				UTIL_Format(buffer, sizeof(buffer), "__pl_%s_SetNTVOptional", &pubvar->name[5]);
				if ((pFunc=pBase->GetFunctionByName(buffer)))
				{
					cell_t res;
					pFunc->Execute(&res);
					if (pPlugin->GetContext()->n_err != SP_ERROR_NONE)
					{
						if (error)
						{
							snprintf(error, maxlength, "Fatal error during initializing plugin load");
						}
						return false;
					}
				}
			}
			else
			{
				/* Check that we aren't registering the same library twice */
				if (pPlugin->m_RequiredLibs.find(name) == pPlugin->m_RequiredLibs.end())
				{
					pPlugin->m_RequiredLibs.push_back(name);
				}
				else
				{
					continue;
				}
				List<CPlugin *>::iterator iter;
				CPlugin *pl;
				bool found = false;
				for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++)
				{
					pl = (*iter);
					if (pl->m_Libraries.find(name) != pl->m_Libraries.end())
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					if (error)
					{
						snprintf(error, maxlength, "Could not find required plugin \"%s\"", name);
					}
					return false;
				}
			}
		}
	}

	return true;
}

bool CPluginManager::LoadOrRequireExtensions(CPlugin *pPlugin, unsigned int pass, char *error, size_t maxlength)
{
	/* Find any extensions this plugin needs */
	struct _ext
	{
		cell_t name;
		cell_t file;
		cell_t autoload;
		cell_t required;
	} *ext;

	IPluginContext *pBase = pPlugin->GetBaseContext();
	uint32_t num = pBase->GetPubVarsNum();
	sp_pubvar_t *pubvar;
	IExtension *pExt;
	char path[PLATFORM_MAX_PATH];
	char *file, *name;
	for (uint32_t i=0; i<num; i++)
	{
		if (pBase->GetPubvarByIndex(i, &pubvar) != SP_ERROR_NONE)
		{
			continue;
		}
		if (strncmp(pubvar->name, "__ext_", 6) == 0)
		{
			ext = (_ext *)pubvar->offs;
			if (pBase->LocalToString(ext->file, &file) != SP_ERROR_NONE)
			{
				continue;
			}
			if (pBase->LocalToString(ext->name, &name) != SP_ERROR_NONE)
			{
				continue;
			}
			if (pass == 1)
			{
				/* Attempt to auto-load if necessary */
				if (ext->autoload)
				{
					g_LibSys.PathFormat(path, PLATFORM_MAX_PATH, "%s", file);
					g_Extensions.LoadAutoExtension(path);
				}
			}
			else if (pass == 2)
			{
				/* Is this required? */
				if (ext->required)
				{
					g_LibSys.PathFormat(path, PLATFORM_MAX_PATH, "%s", file);
					if ((pExt = g_Extensions.FindExtensionByFile(path)) == NULL)
					{
						pExt = g_Extensions.FindExtensionByName(name);
					}
					/* :TODO: should we bind to unloaded extensions?
					 * Currently the extension manager will ignore this.
					 */
					if (!pExt || !pExt->IsRunning(NULL, 0))
					{
						if (error)
						{
							snprintf(error, maxlength, "Required extension \"%s\" file(\"%s\") not running", name, file);
						}
						return false;
					}
					else
					{
						g_Extensions.BindChildPlugin(pExt, pPlugin);
					}
				}
				else
				{
					IPluginFunction *pFunc;
					char buffer[64];
					UTIL_Format(buffer, sizeof(buffer), "__ext_%s_SetNTVOptional", &pubvar->name[6]);

					if ((pFunc = pBase->GetFunctionByName(buffer)) != NULL)
					{
						cell_t res;
						pFunc->Execute(&res);
						if (pPlugin->GetContext()->n_err != SP_ERROR_NONE)
						{
							if (error)
							{
								UTIL_Format(error, maxlength, "Fatal error during plugin initialization (ext req)");
							}
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}

bool CPluginManager::RunSecondPass(CPlugin *pPlugin, char *error, size_t maxlength)
{
	/* Second pass for extension requirements */
	if (!LoadOrRequireExtensions(pPlugin, 2, error, maxlength))
	{
		return false;
	}

	/* Bind all extra natives */
	g_Extensions.BindAllNativesToPlugin(pPlugin);

	if (!FindOrRequirePluginDeps(pPlugin, error, maxlength))
	{
		return false;
	}

	AddFakeNativesToPlugin(pPlugin);

	/* Find any unbound natives
	 * Right now, these are not allowed
	 */
	IPluginContext *pContext = pPlugin->GetBaseContext();
	uint32_t num = pContext->GetNativesNum();
	sp_native_t *native;
	for (unsigned int i=0; i<num; i++)
	{
		if (pContext->GetNativeByIndex(i, &native) != SP_ERROR_NONE)
		{
			break;
		}
		if (native->status == SP_NATIVE_UNBOUND
			&& native->name[0] != '@'
			&& !(native->flags & SP_NTVFLAG_OPTIONAL))
		{
			if (error)
			{
				snprintf(error, maxlength, "Native \"%s\" was not found.", native->name);
			}
			return false;
		}
	}

	/* Finish by telling all listeners */
	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;
	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginLoaded(pPlugin);
	}
	
	/* Tell this plugin to finish initializing itself */
	pPlugin->Call_OnPluginStart();

	/* Now, if we have fake natives, go through all plugins that might need rebinding */
	if (pPlugin->GetStatus() <= Plugin_Paused && pPlugin->m_fakeNatives.size())
	{
		List<CPlugin *>::iterator pl_iter;
		CPlugin *pOther;
		for (pl_iter = m_plugins.begin();
			 pl_iter != m_plugins.end();
			 pl_iter++)
		{
			pOther = (*pl_iter);
			if ((pOther->GetStatus() == Plugin_Error
				&& (pOther->m_FakeNativesMissing || pOther->m_LibraryMissing))
				|| pOther->m_FakeNativesMissing)
			{
				TryRefreshDependencies(pOther);
			}
		}
	}

	/* Go through our libraries and tell other plugins they're added */
	List<String>::iterator s_iter;
	for (s_iter = pPlugin->m_Libraries.begin();
		s_iter != pPlugin->m_Libraries.end();
		s_iter++)
	{
		OnLibraryAction((*s_iter).c_str(), true, false);
	}

	/* Finally, add the core language file */
	pPlugin->AddLangFile(g_pCorePhraseID);

	return true;
}

void CPluginManager::AddCoreNativesToPlugin(CPlugin *pPlugin)
{
	IPluginContext *pContext = pPlugin->GetBaseContext();
	

	uint32_t natives = pContext->GetNativesNum();
	sp_native_t *native;
	SPVM_NATIVE_FUNC pfn;
	for (uint32_t i=0; i<natives; i++)
	{
		if (pContext->GetNativeByIndex(i, &native) != SP_ERROR_NONE)
		{
			continue;
		}
		if (native->status == SP_NATIVE_BOUND)
		{
			continue;
		}
		if (!sm_trie_retrieve(m_pCoreNatives, native->name, (void **)&pfn))
		{
			continue;
		}
		pContext->BindNativeToIndex(i, pfn);
	}
}

SPVM_NATIVE_FUNC CPluginManager::FindCoreNative(const char *name)
{
	SPVM_NATIVE_FUNC pfn;

	if (!sm_trie_retrieve(m_pCoreNatives, name, (void **)&pfn))
	{
		return NULL;
	}

	return pfn;
}

void CPluginManager::TryRefreshDependencies(CPlugin *pPlugin)
{
	assert(pPlugin->GetBaseContext() != NULL);

	AddFakeNativesToPlugin(pPlugin);

	List<String>::iterator lib_iter;
	List<String>::iterator req_iter;
	List<CPlugin *>::iterator pl_iter;
	CPlugin *pl;
	for (req_iter=pPlugin->m_RequiredLibs.begin(); req_iter!=pPlugin->m_RequiredLibs.end(); req_iter++)
	{
		bool found = false;
		for (pl_iter=m_plugins.begin(); pl_iter!=m_plugins.end(); pl_iter++)
		{
			pl = (*pl_iter);
			for (lib_iter=pl->m_Libraries.begin(); lib_iter!=pl->m_Libraries.end(); lib_iter++)
			{
				if ((*req_iter) == (*lib_iter))
				{
					found = true;
				}
			}
		}
		if (!found)
		{
			pPlugin->SetErrorState(Plugin_Error, "Library not found: %s", (*req_iter).c_str());
			return;
		}
	}

	/* Find any unbound natives
	 * Right now, these are not allowed
	 */
	IPluginContext *pContext = pPlugin->GetBaseContext();
	uint32_t num = pContext->GetNativesNum();
	sp_native_t *native;
	for (unsigned int i=0; i<num; i++)
	{
		if (pContext->GetNativeByIndex(i, &native) != SP_ERROR_NONE)
		{
			break;
		}
		if (native->status == SP_NATIVE_UNBOUND && !(native->flags & SP_NTVFLAG_OPTIONAL))
		{
			pPlugin->SetErrorState(Plugin_Error, "Native not found: %s", native->name);
			return;
		}
	}

	if (pPlugin->GetStatus() == Plugin_Error)
	{
		/* If we got here, all natives are okay again! */
		pPlugin->m_status = Plugin_Running;
		if ((pPlugin->m_ctx.ctx->flags & SPFLAG_PLUGIN_PAUSED) == SPFLAG_PLUGIN_PAUSED)
		{
			pPlugin->m_ctx.ctx->flags &= ~SPFLAG_PLUGIN_PAUSED;
			_SetPauseState(pPlugin, false);
		}
	}
}

void CPluginManager::AddFakeNativesToPlugin(CPlugin *pPlugin)
{
	IPluginContext *pContext = pPlugin->GetBaseContext();
	sp_nativeinfo_t native;

	List<FakeNative *>::iterator iter;
	FakeNative *pNative;
	sp_context_t *ctx;
	for (iter = m_Natives.begin(); iter != m_Natives.end(); iter++)
	{
		pNative = (*iter);
		ctx = pNative->ctx->GetContext();
		if ((ctx->flags & SPFLAG_PLUGIN_PAUSED) == SPFLAG_PLUGIN_PAUSED)
		{
			/* Ignore natives in paused plugins */
			continue;
		}
		native.name = pNative->name.c_str();
		native.func = pNative->func;
		if (pContext->BindNative(&native) == SP_ERROR_NONE)
		{
			uint32_t idx;
			pContext->FindNativeByName(native.name, &idx);
			if (pPlugin->GetContext()->natives[idx].flags & SP_NTVFLAG_OPTIONAL)
			{
				WeakNative wkn(pPlugin, idx);
				GetPluginByCtx(ctx)->m_WeakNatives.push_back(wkn);
				continue;
			}
			/* Add us as a dependency, but we're careful not to do this circularly! */
			if (pNative->ctx != pContext)
			{
				CPlugin *pOther = GetPluginByCtx(ctx);
				if (pOther->m_dependents.find(pPlugin) == pOther->m_dependents.end())
				{
					pOther->m_dependents.push_back(pPlugin);
				}
				if (pPlugin->m_dependsOn.find(pOther) == pPlugin->m_dependsOn.end())
				{
					pPlugin->m_dependsOn.push_back(pOther);
				}
			}
		}
	}
}

bool CPluginManager::UnloadPlugin(IPlugin *plugin)
{
	CPlugin *pPlugin = (CPlugin *)plugin;

	/* This prevents removal during insertion or anything else weird */
	if (m_plugins.find(pPlugin) == m_plugins.end())
	{
		return false;
	}

	IPluginContext *pContext = plugin->GetBaseContext();
	if (pContext->IsInExec())
	{
		char buffer[255];
		UTIL_Format(buffer, sizeof(buffer), "sm plugins unload %s\n", plugin->GetFilename());
		engine->ServerCommand(buffer);
		return false;
	}

	/* Remove us from the lookup table and linked list */
	m_plugins.remove(pPlugin);
	sm_trie_delete(m_LoadLookup, pPlugin->m_filename);

	/* Go through our libraries and tell other plugins they're gone */
	List<String>::iterator s_iter;
	for (s_iter = pPlugin->m_Libraries.begin();
		 s_iter != pPlugin->m_Libraries.end();
		 s_iter++)
	{
		OnLibraryAction((*s_iter).c_str(), true, true);
	}

	/* Go through all dependent plugins and tell them this plugin is now gone */
	List<CPlugin *>::iterator pl_iter;
	CPlugin *pOther;
	for (pl_iter = pPlugin->m_dependents.begin();
		 pl_iter != pPlugin->m_dependents.end();
		 pl_iter++)
	{
		pOther = (*pl_iter);
		pOther->DependencyDropped(pPlugin);
	}

	/* Tell everyone we depend on that we no longer exist */
	for (pl_iter = pPlugin->m_dependsOn.begin();
		 pl_iter != pPlugin->m_dependsOn.end();
		 pl_iter++)
	{
		pOther = (*pl_iter);
		pOther->m_dependents.remove(pPlugin);
	}

	/* Remove weak references to us */
	for (pl_iter = m_plugins.begin();
		 pl_iter != m_plugins.end();
		 pl_iter++)
	{
		pOther = (*pl_iter);
		List<WeakNative>::iterator wk_iter = pOther->m_WeakNatives.begin();
		while (wk_iter != pOther->m_WeakNatives.end())
		{
			if ((*wk_iter).pl == pPlugin)
			{
				wk_iter = pOther->m_WeakNatives.erase(wk_iter);
			} else {
				wk_iter++;
			}
		}
	}

	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;

	if (pPlugin->GetStatus() <= Plugin_Error)
	{
		/* Notify listeners of unloading */
		for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
		{
			pListener = (*iter);
			pListener->OnPluginUnloaded(pPlugin);
		}
		/* Notify plugin */
		pPlugin->Call_OnPluginEnd();
	}

	/* Unbound weak natives */
	List<WeakNative>::iterator wk_iter;
	for (wk_iter=pPlugin->m_WeakNatives.begin(); wk_iter!=pPlugin->m_WeakNatives.end(); wk_iter++)
	{
		WeakNative & wkn = (*wk_iter);
		sp_context_t *ctx = wkn.pl->GetContext();
		ctx->natives[wkn.idx].status = SP_NATIVE_UNBOUND;
		wkn.pl->m_FakeNativesMissing = true;
	}

	/* Remove all of our native functions */
	List<FakeNative *>::iterator fn_iter;
	FakeNative *pNative;
	for (fn_iter = pPlugin->m_fakeNatives.begin();
		fn_iter != pPlugin->m_fakeNatives.end();
		fn_iter++)
	{
		pNative = (*fn_iter);
		m_Natives.remove(pNative);
		sm_trie_delete(m_pNativeLookup, pNative->name.c_str());
		g_pVM->DestroyFakeNative(pNative->func);
		delete pNative;
	}

	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		/* Notify listeners of destruction */
		pListener = (*iter);
		pListener->OnPluginDestroyed(pPlugin);
	}
	
	/* Tell the plugin to delete itself */
	delete pPlugin;

	return true;
}

IPlugin *CPluginManager::FindPluginByContext(const sp_context_t *ctx)
{
	IPlugin *pl = (IPlugin *)ctx->user[SM_CONTEXTVAR_MYSELF];
	return pl;
}

unsigned int CPluginManager::GetPluginCount()
{
	return m_plugins.size();
}

void CPluginManager::AddPluginsListener(IPluginsListener *listener)
{
	m_listeners.push_back(listener);
}

void CPluginManager::RemovePluginsListener(IPluginsListener *listener)
{
	m_listeners.remove(listener);
}

IPluginIterator *CPluginManager::GetPluginIterator()
{
	if (m_iters.empty())
	{
		return new CPluginIterator(&m_plugins);
	} else {
		CPluginIterator *iter = m_iters.front();
		m_iters.pop();
		iter->Reset();
		return iter;
	}
}

void CPluginManager::ReleaseIterator(CPluginIterator *iter)
{
	m_iters.push(iter);
}

bool CPluginManager::TestAliasMatch(const char *alias, const char *localpath)
{
	/* As an optimization, we do not call strlen, but compute the length in the first pass */
	size_t alias_len = 0;
	size_t local_len = 0;

	const char *ptr = alias;
	unsigned int alias_explicit_paths = 0;
	unsigned int alias_path_end = 0;
	while (*ptr != '\0')
	{
		if (*ptr == '\\' || *ptr == '/')
		{
			alias_explicit_paths++;
			alias_path_end = alias_len;
		}
		alias_len++;
		ptr++;
	}

	if (alias_explicit_paths && alias_path_end == alias_len - 1)
	{
		/* Trailing slash is totally invalid here */
		return false;
	}

	ptr = localpath;
	unsigned int local_explicit_paths = 0;
	unsigned int local_path_end = 0;
	while (*ptr != '\0')
	{
		if (*ptr == '\\' || *ptr == '/')
		{
			local_explicit_paths++;
			local_path_end = local_len;
		}
		local_len++;
		ptr++;
	}

	/* If the alias has more explicit paths than the real path,
	 * no match will be possible.
	 */
	if (alias_explicit_paths > local_explicit_paths)
	{
		return false;
	}

	if (alias_explicit_paths)
	{
		/* We need to find if the paths match now.  For example, these should all match:
		 * csdm     csdm
		 * csdm     optional/csdm
		 * csdm/ban optional/crab/csdm/ban
		*/
		const char *aliasptr = alias;
		const char *localptr = localpath;
		bool match = true;
		do
		{
			if (*aliasptr != *localptr)
			{
				/* We have to knock one path off */
				local_explicit_paths--;
				if (alias_explicit_paths > local_explicit_paths)
				{
					/* Skip out if we're gonna have an impossible match */
					return false;
				}
				/* Eat up localptr tokens until we get a result */
				while (((localptr - localpath) < (int)local_path_end)
					&& *localptr != '/'
					&& *localptr != '\\')
				{
					localptr++;
				}
				/* Check if we hit the end of our searchable area.
				 * This probably isn't possible because of the path 
				 * count check, but it's a good idea anyway.
				 */
				if ((localptr - localpath) >= (int)local_path_end)
				{
					return false;
				} else {
					/* Consume the slash token */
					localptr++;
				}
				/* Reset the alias pointer so we can continue consuming */
				aliasptr = alias;
				match = false;
				continue;
			}
			/* Note:
			 * This is safe because if localptr terminates early, aliasptr will too
			 */
			do
			{
				/* We should never reach the end of the string because of this check. */
				bool aliasend = (aliasptr - alias) > (int)alias_path_end;
				bool localend = (localptr - localpath) > (int)local_path_end;
				if (aliasend || localend)
				{
					if (aliasend && localend)
					{
						/* we matched, and we can break out now */
						match = true;
						break;
					}
					/* Otherwise, we've hit the end somehow and rest won't match up.  Break out. */
					match = false;
					break;
				}

				/* If we got here, it's safe to compare the next two tokens */
				if (*localptr != *aliasptr)
				{
					match = false;
					break;
				}
				localptr++;
				aliasptr++;
			} while (true);
		} while (!match);
	}

	/* If we got here, it's time to compare filenames */
	const char *aliasptr = alias;
	const char *localptr = localpath;

	if (alias_explicit_paths)
	{
		aliasptr = &alias[alias_path_end + 1];
	}

	if (local_explicit_paths)
	{
		localptr = &localpath[local_path_end + 1];
	}

	while (true)
	{
		if (*aliasptr == '*')
		{
			/* First, see if this is the last character */
			if ((unsigned)(aliasptr - alias) == alias_len - 1)
			{
				/* If so, there's no need to match anything else */
				return true;
			}
			/* Otherwise, we need to search for an appropriate matching sequence in local.
			 * Note that we only need to search up to the next asterisk.
			 */
			aliasptr++;
			bool match = true;
			const char *local_orig = localptr;
			do
			{
				match = true;
				while (*aliasptr != '\0' && *aliasptr != '*')
				{
					/* Since aliasptr is never '\0', localptr hitting the end will fail */
					if (*aliasptr != *localptr)
					{
						match = false;
						break;
					}
					aliasptr++;
					localptr++;
				}
				if (!match)
				{
					/* If we didn't get a match, we need to advance the search stream.
					 * This will let us skip tokens while still searching for another match.
					 */
					localptr = ++local_orig;
					/* Make sure we don't go out of bounds */
					if (*localptr == '\0')
					{
						break;
					}
				}
			} while (!match);

			if (!match)
			{
				return false;
			} else {
				/* If we got a match, move on to the next token */
				continue;
			}
		} else if (*aliasptr == '\0') {
			if (*localptr == '\0'
				||
				strcmp(localptr, ".smx") == 0)
			{
				return true;
			} else {
				return false;
			}
		} else if (*aliasptr != *localptr) {
			return false;
		}
		aliasptr++;
		localptr++;
	}

	return true;
}

bool CPluginManager::IsLateLoadTime() const
{
	return (m_AllPluginsLoaded || !g_SourceMod.IsMapLoading());
}

void CPluginManager::OnSourceModAllInitialized()
{
	m_MyIdent = g_ShareSys.CreateCoreIdentity();

	HandleAccess sec;
	g_HandleSys.InitAccessDefaults(NULL, &sec);

	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY;
	sec.access[HandleAccess_Clone] = HANDLE_RESTRICT_IDENTITY;

	g_PluginType = g_HandleSys.CreateType("Plugin", this, 0, NULL, &sec, m_MyIdent, NULL);
	g_PluginIdent = g_ShareSys.CreateIdentType("PLUGIN");

	g_RootMenu.AddRootConsoleCommand("plugins", "Manage Plugins", this);

	g_ShareSys.AddInterface(NULL, this);
}

void CPluginManager::OnSourceModShutdown()
{
	g_RootMenu.RemoveRootConsoleCommand("plugins", this);

	UnloadAll();

	g_HandleSys.RemoveType(g_PluginType, m_MyIdent);
	g_ShareSys.DestroyIdentType(g_PluginIdent);
	g_ShareSys.DestroyIdentity(m_MyIdent);
}

void CPluginManager::OnHandleDestroy(HandleType_t type, void *object)
{
	/* We don't care about the internal object, actually */
}

bool CPluginManager::GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
{
	*pSize = ((CPlugin *)object)->CalcMemUsage();
	return true;
}

void CPluginManager::RegisterNativesFromCore(sp_nativeinfo_t *natives)
{
	for (unsigned int i = 0; natives[i].func != NULL; i++)
	{
		sm_trie_insert(m_pCoreNatives, natives[i].name, (void *)natives[i].func);
	}
}

IPlugin *CPluginManager::PluginFromHandle(Handle_t handle, HandleError *err)
{
	IPlugin *pPlugin;
	HandleError _err;

	HandleSecurity sec;

	sec.pOwner = NULL;
	sec.pIdentity = m_MyIdent;

	if ((_err=g_HandleSys.ReadHandle(handle, g_PluginType, &sec, (void **)&pPlugin)) != HandleError_None)
	{
		pPlugin = NULL;
	}

	if (err)
	{
		*err = _err;
	}

	return pPlugin;
}

CPlugin *CPluginManager::GetPluginByOrder(int num)
{
	if (num < 1 || num > (int)GetPluginCount())
	{
		return NULL;
	}

	CPlugin *pl;
	int id = 1;

	IPluginIterator *iter = GetPluginIterator();
	for (; iter->MorePlugins() && id<num; iter->NextPlugin(), id++) {}

	pl = (CPlugin *)(iter->GetPlugin());
	iter->Release();

	return pl;
}

const char *CPluginManager::GetStatusText(PluginStatus st)
{
	switch (st)
	{
	case Plugin_Running:
		return "Running";
	case Plugin_Paused:
		return "Paused";
	case Plugin_Error:
		return "Error";
	case Plugin_Uncompiled:
		return "Uncompiled";
	case Plugin_BadLoad:
		return "Bad Load";
	case Plugin_Failed:
		return "Failed";
	default:
		assert(false);
		return "-";
	}
}

void CPluginManager::OnRootConsoleCommand(const char *cmdname, const CCommand &command)
{
	int argcount = command.ArgC();
	if (argcount >= 3)
	{
		const char *cmd = command.Arg(2);
		if (strcmp(cmd, "list") == 0)
		{
			char buffer[256];
			unsigned int id = 1;
			int plnum = GetPluginCount();

			if (!plnum)
			{
				g_RootMenu.ConsolePrint("[SM] No plugins loaded");
				return;
			}
			else
			{
				g_RootMenu.ConsolePrint("[SM] Listing %d plugin%s:", GetPluginCount(), (plnum > 1) ? "s" : "");
			}

			SourceHook::List<IPlugin *> m_FailList;

			IPluginIterator *iter = GetPluginIterator();
			for (; iter->MorePlugins(); iter->NextPlugin(), id++)
			{
				IPlugin *pl = iter->GetPlugin();
				assert(pl->GetStatus() != Plugin_Created);
				int len = 0;
				const sm_plugininfo_t *info = pl->GetPublicInfo();
				if (pl->GetStatus() != Plugin_Running)
				{
					len += UTIL_Format(buffer, sizeof(buffer), "  %02d <%s>", id, GetStatusText(pl->GetStatus()));

					if (pl->GetStatus() <= Plugin_Error)
					{
						/* Plugin has failed to load. */
						m_FailList.push_back(pl);
					}
				}
				else
				{
					len += UTIL_Format(buffer, sizeof(buffer), "  %02d", id);
				}
				len += UTIL_Format(&buffer[len], sizeof(buffer)-len, " \"%s\"", (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());
				if (IS_STR_FILLED(info->version))
				{
					len += UTIL_Format(&buffer[len], sizeof(buffer)-len, " (%s)", info->version);
				}
				if (IS_STR_FILLED(info->author))
				{
					UTIL_Format(&buffer[len], sizeof(buffer)-len, " by %s", info->author);
				}
				g_RootMenu.ConsolePrint("%s", buffer);
			}

			iter->Release();

			if (!m_FailList.empty())
			{
				g_RootMenu.ConsolePrint("Load Errors:");

				SourceHook::List<IPlugin *>::iterator _iter;

				CPlugin *pl;

				for (_iter=m_FailList.begin(); _iter!=m_FailList.end(); _iter++)
				{
					pl = (CPlugin *)*_iter;
					g_RootMenu.ConsolePrint("%s: %s",(IS_STR_FILLED(pl->GetPublicInfo()->name)) ? pl->GetPublicInfo()->name : pl->GetFilename(), pl->m_errormsg);
				}
			}

			return;
		}
		else if (strcmp(cmd, "load") == 0)
		{
			if (argcount < 4)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm plugins load <file>");
				return;
			}

			char error[128];
			bool wasloaded;
			const char *filename = command.Arg(3);

			char pluginfile[256];
			const char *ext = g_LibSys.GetFileExtension(filename) ? "" : ".smx";
			UTIL_Format(pluginfile, sizeof(pluginfile), "%s%s", filename, ext);

			IPlugin *pl = LoadPlugin(pluginfile, false, PluginType_MapUpdated, error, sizeof(error), &wasloaded);

			if (wasloaded)
			{
				g_RootMenu.ConsolePrint("[SM] Plugin %s is already loaded.", pluginfile);
				return;
			}

			if (pl)
			{
				g_RootMenu.ConsolePrint("[SM] Loaded plugin %s successfully.", pluginfile);
			}
			else
			{
				g_RootMenu.ConsolePrint("[SM] Plugin %s failed to load: %s.", pluginfile, error);
			}

			return;
		}
		else if (strcmp(cmd, "unload") == 0)
		{
			if (argcount < 4)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm plugins unload <#|file>");
				return;
			}

			CPlugin *pl;
			char *end;
			const char *arg = command.Arg(3);
			int id = strtol(arg, &end, 10);

			if (*end == '\0')
			{
				pl = GetPluginByOrder(id);
				if (!pl)
				{
					g_RootMenu.ConsolePrint("[SM] Plugin index %d not found.", id);
					return;
				}
			}
			else
			{
				char pluginfile[256];
				const char *ext = g_LibSys.GetFileExtension(arg) ? "" : ".smx";
				UTIL_Format(pluginfile, sizeof(pluginfile), "%s%s", arg, ext);

				if (!sm_trie_retrieve(m_LoadLookup, pluginfile, (void **)&pl))
				{
					g_RootMenu.ConsolePrint("[SM] Plugin %s is not loaded.", pluginfile);
					return;
				}
			}

			char name[PLATFORM_MAX_PATH];
			const sm_plugininfo_t *info = pl->GetPublicInfo();
			strcpy(name, (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());

			if (UnloadPlugin(pl))
			{
				g_RootMenu.ConsolePrint("[SM] Plugin %s unloaded successfully.", name);
			}
			else
			{
				g_RootMenu.ConsolePrint("[SM] Failed to unload plugin %s.", name);
			}

			return;
		}
		else if (strcmp(cmd, "unload_all") == 0)
		{
			g_PluginSys.UnloadAll();
			g_RootMenu.ConsolePrint("[SM] All plugins have been unloaded.");
			return;
		}
		else if (strcmp(cmd, "load_lock") == 0)
		{
			if (m_LoadingLocked)
			{
				g_RootMenu.ConsolePrint("[SM] There is already a loading lock in effect.");
			}
			else
			{
				m_LoadingLocked = true;
				g_RootMenu.ConsolePrint("[SM] Loading is now locked; no plugins will be loaded or re-loaded.");
			}
			return;
		}
		else if (strcmp(cmd, "load_unlock") == 0)
		{
			if (m_LoadingLocked)
			{
				m_LoadingLocked = false;
				g_RootMenu.ConsolePrint("[SM] The loading lock is no longer in effect.");
			}
			else
			{
				g_RootMenu.ConsolePrint("[SM] There was no loading lock in effect.");
			}
			return;
		}
		else if (strcmp(cmd, "info") == 0)
		{
			if (argcount < 4)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm plugins info <#>");
				return;
			}

			CPlugin *pl;
			char *end;
			const char *arg = command.Arg(3);
			int id = strtol(arg, &end, 10);

			if (*end == '\0')
			{
				pl = GetPluginByOrder(id);
				if (!pl)
				{
					g_RootMenu.ConsolePrint("[SM] Plugin index %d not found.", id);
					return;
				}
			}
			else
			{
				char pluginfile[256];
				const char *ext = g_LibSys.GetFileExtension(arg) ? "" : ".smx";
				UTIL_Format(pluginfile, sizeof(pluginfile), "%s%s", arg, ext);

				if (!sm_trie_retrieve(m_LoadLookup, pluginfile, (void **)&pl))
				{
					g_RootMenu.ConsolePrint("[SM] Plugin %s is not loaded.", pluginfile);
					return;
				}
			}

			const sm_plugininfo_t *info = pl->GetPublicInfo();

			g_RootMenu.ConsolePrint("  Filename: %s", pl->GetFilename());
			if (pl->GetStatus() <= Plugin_Error)
			{
				if (IS_STR_FILLED(info->name))
				{
					if (IS_STR_FILLED(info->description))
					{
						g_RootMenu.ConsolePrint("  Title: %s (%s)", info->name, info->description);
					} else {
						g_RootMenu.ConsolePrint("  Title: %s", info->name);
					}
				}
				if (IS_STR_FILLED(info->author))
				{
					g_RootMenu.ConsolePrint("  Author: %s", info->author);
				}
				if (IS_STR_FILLED(info->version))
				{
					g_RootMenu.ConsolePrint("  Version: %s", info->version);
				}
				if (IS_STR_FILLED(info->url))
				{
					g_RootMenu.ConsolePrint("  URL: %s", info->url);
				}
				if (pl->GetStatus() == Plugin_Error)
				{
					g_RootMenu.ConsolePrint("  Error: %s", pl->m_errormsg);
				}
				else
				{
					if (pl->GetStatus() == Plugin_Running)
					{
						if (pl->IsDebugging())
						{
							g_RootMenu.ConsolePrint("  Status: running, debugging");
						}
						else
						{
							g_RootMenu.ConsolePrint("  Status: running");
						}
					}
					else
					{
						g_RootMenu.ConsolePrint("  Status: not running");
					}

					const char *typestr = "";
					switch (pl->GetType())
					{
					case PluginType_MapUpdated:
						typestr = "Map Change if Updated";
						break;
					case PluginType_MapOnly:
						typestr = "Map Change";
						break;
					case PluginType_Private:
					case PluginType_Global:
						typestr = "Never";
						break;
					}

					g_RootMenu.ConsolePrint("  Reloads: %s", typestr);
				}
				if (pl->m_FileVersion >= 3)
				{
					g_RootMenu.ConsolePrint("  Timestamp: %s", pl->m_DateTime);
				}
			}
			else
			{
				g_RootMenu.ConsolePrint("  Load error: %s", pl->m_errormsg);
				g_RootMenu.ConsolePrint("  File info: (title \"%s\") (version \"%s\")", 
										info->name ? info->name : "<none>",
										info->version ? info->version : "<none>");
				if (IS_STR_FILLED(info->url))
				{
					g_RootMenu.ConsolePrint("  File URL: %s", info->url);
				}
			}

			return;
		}
		else if (strcmp(cmd, "debug") == 0)
		{
			if (argcount < 5)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm plugins debug <#> [on|off]");
				return;
			}

			CPlugin *pl;
			char *end;
			const char *arg = command.Arg(3);
			int id = strtol(arg, &end, 10);

			if (*end == '\0')
			{
				pl = GetPluginByOrder(id);
				if (!pl)
				{
					g_RootMenu.ConsolePrint("[SM] Plugin index %d not found.", id);
					return;
				}
			}
			else
			{
				char pluginfile[256];
				const char *ext = g_LibSys.GetFileExtension(arg) ? "" : ".smx";
				UTIL_Format(pluginfile, sizeof(pluginfile), "%s%s", arg, ext);

				if (!sm_trie_retrieve(m_LoadLookup, pluginfile, (void **)&pl))
				{
					g_RootMenu.ConsolePrint("[SM] Plugin %s is not loaded.", pluginfile);
					return;
				}
			}

			int res;
			const char *mode = command.Arg(4);
			if ((res=strcmp("on", mode)) && strcmp("off", mode))
			{
				g_RootMenu.ConsolePrint("[SM] The only possible options are \"on\" and \"off.\"");
				return;
			}

			bool debug;
			if (!res)
			{
				debug = true;
			}
			else
			{
				debug = false;
			}

			if (debug && pl->IsDebugging())
			{
				g_RootMenu.ConsolePrint("[SM] This plugin is already in debug mode.");
				return;
			}
			else if (!debug && !pl->IsDebugging())
			{
				g_RootMenu.ConsolePrint("[SM] Debug mode is already disabled in this plugin.");
				return;
			}

			char error[256];
			if (pl->ToggleDebugMode(debug, error, sizeof(error)))
			{
				g_RootMenu.ConsolePrint("[SM] Successfully toggled debug mode on plugin %s.", pl->GetFilename());
				return;
			}
			else
			{
				g_RootMenu.ConsolePrint("[SM] Could not toggle debug mode on plugin %s.", pl->GetFilename());
				g_RootMenu.ConsolePrint("[SM] Plugin returned error: %s", error);
				return;
			}
		}
		else if (strcmp(cmd, "refresh") == 0)
		{
			g_SourceMod.DoGlobalPluginLoads();
			g_RootMenu.ConsolePrint("[SM] The plugin list has been refreshed and reloaded.");
			return;
		}
		else if (strcmp(cmd, "reload") == 0)
		{
			if (argcount < 4)
			{
				g_RootMenu.ConsolePrint("[SM] Usage: sm plugins reload <#|file>");
				return;
			}

			CPlugin *pl;
			char *end;
			const char *arg = command.Arg(3);
			int id = strtol(arg, &end, 10);

			if (*end == '\0')
			{
				pl = GetPluginByOrder(id);
				if (!pl)
				{
					g_RootMenu.ConsolePrint("[SM] Plugin index %d not found.", id);
					return;
				}
			}
			else
			{
				char pluginfile[256];
				const char *ext = g_LibSys.GetFileExtension(arg) ? "" : ".smx";
				UTIL_Format(pluginfile, sizeof(pluginfile), "%s%s", arg, ext);

				if (!sm_trie_retrieve(m_LoadLookup, pluginfile, (void **)&pl))
				{
					g_RootMenu.ConsolePrint("[SM] Plugin %s is not loaded.", pluginfile);
					return;
				}
			}

			char name[PLATFORM_MAX_PATH];
			const sm_plugininfo_t *info = pl->GetPublicInfo();
			strcpy(name, (IS_STR_FILLED(info->name)) ? info->name : pl->GetFilename());

			if (ReloadPlugin(pl))
			{
				g_RootMenu.ConsolePrint("[SM] Plugin %s reloaded successfully.", name);
			}
			else
			{
				g_RootMenu.ConsolePrint("[SM] Failed to reload plugin %s.", name);
			}

			return;
		}
	}

	/* Draw the main menu */
	g_RootMenu.ConsolePrint("SourceMod Plugins Menu:");
	g_RootMenu.DrawGenericOption("debug", "Toggle debug mode on a plugin");
	g_RootMenu.DrawGenericOption("info", "Information about a plugin");
	g_RootMenu.DrawGenericOption("list", "Show loaded plugins");
	g_RootMenu.DrawGenericOption("load", "Load a plugin");
	g_RootMenu.DrawGenericOption("load_lock", "Prevents any more plugins from being loaded");
	g_RootMenu.DrawGenericOption("load_unlock", "Re-enables plugin loading");
	g_RootMenu.DrawGenericOption("refresh", "Reloads/refreshes all plugins in the plugins folder");
	g_RootMenu.DrawGenericOption("reload", "Reloads a plugin");
	g_RootMenu.DrawGenericOption("unload", "Unload a plugin");
	g_RootMenu.DrawGenericOption("unload_all", "Unloads all plugins");
}

bool CPluginManager::ReloadPlugin(CPlugin *pl)
{
	List<CPlugin *>::iterator iter;
	char filename[PLATFORM_MAX_PATH];
	bool debug, wasloaded;
	PluginType ptype;
	IPlugin *newpl;
	int id = 1;

	strcpy(filename, pl->m_filename);
	debug = pl->IsDebugging();
	ptype = pl->GetType();

	for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++, id++)
	{
		if ((*iter) == pl)
		{
			break;
		}
	}

	if (!UnloadPlugin(pl))
	{
		return false;
	}
	if (!(newpl=LoadPlugin(filename, debug, ptype, NULL, 0, &wasloaded)))
	{
		return false;
	}

	for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++)
	{
		if ((*iter) == (CPlugin *)newpl)
		{
			m_plugins.erase(iter);
			break;
		}
	}

	int i;
	for (i=1, iter=m_plugins.begin(); iter!=m_plugins.end() && i<id; iter++, i++)
	{
	}
	m_plugins.insert(iter, (CPlugin *)newpl);

	return true;
}

void CPluginManager::ReloadOrUnloadPlugins()
{
	/* If we're in a load lock, just skip this whole bit. */
	if (m_LoadingLocked)
	{
		return;
	}

	List<CPlugin *>::iterator iter;
	List<CPlugin *> tmp_list = m_plugins;
	CPlugin *pl;
	time_t t;

	for (iter=tmp_list.begin(); iter!=tmp_list.end(); iter++)
	{
		pl = (*iter);
		if (pl->GetType() ==  PluginType_MapOnly)
		{
			UnloadPlugin((IPlugin *)pl);
		}
		else if (pl->GetType() == PluginType_MapUpdated)
		{
			t = pl->GetFileTimeStamp();
			if (!t || t > pl->GetTimeStamp())
			{
				pl->SetTimeStamp(t);
				UnloadPlugin((IPlugin *)pl);
			}
		}
	}
}

void CPluginManager::_SetPauseState(CPlugin *pl, bool paused)
{
	List<IPluginsListener *>::iterator iter;
	IPluginsListener *pListener;
	for (iter=m_listeners.begin(); iter!=m_listeners.end(); iter++)
	{
		pListener = (*iter);
		pListener->OnPluginPauseChange(pl, paused);
	}	
}

bool CPluginManager::AddFakeNative(IPluginFunction *pFunction, const char *name, SPVM_FAKENATIVE_FUNC func)
{
	if (sm_trie_retrieve(m_pNativeLookup, name, NULL))
	{
		return false;
	}

	FakeNative *pNative = new FakeNative;

	pNative->func = g_pVM->CreateFakeNative(func, pNative);
	if (!pNative->func)
	{
		delete pNative;
		return false;
	}

	pNative->call = pFunction;
	pNative->name.assign(name);
	pNative->ctx = pFunction->GetParentContext();

	m_Natives.push_back(pNative);
	sm_trie_insert(m_pNativeLookup, name,pNative);

	CPlugin *pPlugin = GetPluginByCtx(pNative->ctx->GetContext());
	pPlugin->m_fakeNatives.push_back(pNative);

	return true;
}

void CPluginManager::AddFunctionsToForward(const char *name, IChangeableForward *pForward)
{
	List<CPlugin *>::iterator iter;
	CPlugin *pPlugin;
	IPluginFunction *pFunction;

	for (iter = m_plugins.begin(); iter != m_plugins.end(); iter++)
	{
		pPlugin = (*iter);

		if (pPlugin->GetStatus() <= Plugin_Paused)
		{
			pFunction = pPlugin->GetBaseContext()->GetFunctionByName(name);

			if (pFunction)
			{
				pForward->AddFunction(pFunction);
			}
		}
	}
}

CPlugin *CPluginManager::GetPluginFromIdentity(IdentityToken_t *pToken)
{
	if (pToken->type != g_PluginIdent)
	{
		return NULL;
	}

	return (CPlugin *)(pToken->ptr);
}

void CPluginManager::OnLibraryAction(const char *lib, bool is_a_plugin, bool drop)
{
	List<CPlugin *>::iterator iter;

	struct _pl
	{
		cell_t name;
		cell_t file;
		cell_t required;
	} *plc;

	struct _ext
	{
		cell_t name;
		cell_t file;
		cell_t autoload;
		cell_t required;
	} *ext;

	const char *name = drop ? "OnLibraryRemoved" : "OnLibraryAdded";

	for (iter=m_plugins.begin();
		 iter!=m_plugins.end();
		 iter++)
	{
		CPlugin *pl = (*iter);
		if (pl->GetStatus() != Plugin_Running)
		{
			continue;
		}
		IPluginContext *pContext = pl->GetBaseContext();
		IPluginFunction *pf = pContext->GetFunctionByName(name);
		if (!pf)
		{
			continue;
		}
		uint32_t num_vars = pContext->GetPubVarsNum();
		for (uint32_t i=0; i<num_vars; i++)
		{
			sp_pubvar_t *pubvar;
			if (pContext->GetPubvarByIndex(i, &pubvar) != SP_ERROR_NONE)
			{
				continue;
			}
			if (is_a_plugin && strncmp(pubvar->name, "__pl_", 5) == 0)
			{
				plc = (_pl *)pubvar->offs;
				if (plc->required)
				{
					continue;
				}
				char *str;
				pContext->LocalToString(plc->name, &str);
				if (strcmp(str, lib) != 0)
				{
					continue;
				}
				pf->PushString(lib);
				pf->Execute(NULL);
			}
			else if (!is_a_plugin && strncmp(pubvar->name, "__ext_", 6) == 0)
			{
				ext = (_ext *)pubvar->offs;
				if (ext->required)
				{
					continue;
				}
				char *str;
				pContext->LocalToString(ext->name, &str);
				if (strcmp(str, lib) != 0)
				{
					continue;
				}
				pf->PushString(lib);
				pf->Execute(NULL);
			}
		}
	}
}

bool CPluginManager::LibraryExists(const char *lib)
{
	List<CPlugin *>::iterator iter;

	for (iter=m_plugins.begin();
		 iter!=m_plugins.end();
		 iter++)
	{
		CPlugin *pl = (*iter);
		if (pl->GetStatus() != Plugin_Running)
		{
			continue;
		}
		List<String>::iterator s_iter;
		for (s_iter = pl->m_Libraries.begin();
			 s_iter != pl->m_Libraries.end();
			 s_iter++)
		{
			if ((*s_iter).compare(lib) == 0)
			{
				return true;
			}
		}
	}

	return false;
}

void CPluginManager::AllPluginsLoaded()
{
	List<CPlugin *>::iterator iter;
	CPlugin *pl;

	for (iter=m_plugins.begin(); iter!=m_plugins.end(); iter++)
	{
		pl = (*iter);
		pl->Call_OnAllPluginsLoaded();
	}
}

void CPluginManager::UnloadAll()
{
	List<CPlugin *>::iterator iter;
	while ( (iter = m_plugins.begin()) != m_plugins.end() )
	{
		UnloadPlugin((*iter));
	}
}

int CPluginManager::GetOrderOfPlugin(IPlugin *pl)
{
	int id = 1;
	List<CPlugin *>::iterator iter;

	for (iter = m_plugins.begin(); iter != m_plugins.end(); iter++, id++)
	{
		if ((*iter) == pl)
		{
			return id;
		}
	}

	return -1;
}

CPlugin *CPluginManager::FindPluginByConsoleArg(const char *arg)
{
	int id;
	char *end;
	CPlugin *pl;
	
	id = strtol(arg, &end, 10);

	if (*end == '\0')
	{
		pl = GetPluginByOrder(id);
		if (!pl)
		{
			return NULL;
		}
	}
	else
	{
		char pluginfile[256];
		const char *ext = g_LibSys.GetFileExtension(arg) ? "" : ".smx";
		UTIL_Format(pluginfile, sizeof(pluginfile), "%s%s", arg, ext);

		if (!sm_trie_retrieve(m_LoadLookup, pluginfile, (void **)&pl))
		{
			return NULL;
		}
	}

	return pl;
}
