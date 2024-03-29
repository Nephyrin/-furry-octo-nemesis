/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#include "HandleSys.h"
#include "PluginSys.h"
#include "UserMessages.h"
#include "smn_usermsgs.h"

HandleType_t g_WrBitBufType;
HandleType_t g_RdBitBufType;
Handle_t g_CurMsgHandle;
Handle_t g_ReadBufHandle;
bf_read g_ReadBitBuf;

int g_MsgPlayers[256];
bool g_IsMsgInExec = false;

typedef List<MsgListenerWrapper *> MsgWrapperList;
typedef List<MsgListenerWrapper *>::iterator MsgWrapperIter;

class UsrMessageNatives :
	public SMGlobalClass,
	public IHandleTypeDispatch,
	public IPluginsListener
{
public:
	~UsrMessageNatives();
public: //SMGlobalClass, IHandleTypeDispatch, IPluginListener
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
	void OnHandleDestroy(HandleType_t type, void *object);
	bool GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize);
	void OnPluginUnloaded(IPlugin *plugin);
public:
	MsgListenerWrapper *CreateListener(IPluginContext *pCtx);
	bool FindListener(int msgid, IPluginContext *pCtx, IPluginFunction *pHook, bool intercept, MsgWrapperIter *iter);
	bool DeleteListener(IPluginContext *pCtx, MsgWrapperIter iter);
private:
	CStack<MsgListenerWrapper *> m_FreeListeners;
};

UsrMessageNatives::~UsrMessageNatives()
{
	CStack<MsgListenerWrapper *>::iterator iter;
	for (iter=m_FreeListeners.begin(); iter!=m_FreeListeners.end(); iter++)
	{
		delete (*iter);
	}
	m_FreeListeners.popall();
}

void UsrMessageNatives::OnSourceModAllInitialized()
{
	HandleAccess sec;

	g_HandleSys.InitAccessDefaults(NULL, &sec);
	sec.access[HandleAccess_Delete] = HANDLE_RESTRICT_IDENTITY;

	g_WrBitBufType = g_HandleSys.CreateType("BitBufWriter", this, 0, NULL, NULL, g_pCoreIdent, NULL);
	g_RdBitBufType = g_HandleSys.CreateType("BitBufReader", this, 0, NULL, &sec, g_pCoreIdent, NULL);

	g_ReadBufHandle = g_HandleSys.CreateHandle(g_RdBitBufType, &g_ReadBitBuf, NULL, g_pCoreIdent, NULL);

	g_PluginSys.AddPluginsListener(this);
}

void UsrMessageNatives::OnSourceModShutdown()
{
	HandleSecurity sec;
	sec.pIdentity = g_pCoreIdent;

	g_HandleSys.FreeHandle(g_ReadBufHandle, &sec);

	g_HandleSys.RemoveType(g_WrBitBufType, g_pCoreIdent);
	g_HandleSys.RemoveType(g_RdBitBufType, g_pCoreIdent);

	g_WrBitBufType = 0;
	g_RdBitBufType = 0;
}

void UsrMessageNatives::OnHandleDestroy(HandleType_t type, void *object)
{
}

bool UsrMessageNatives::GetHandleApproxSize(HandleType_t type, void *object, unsigned int *pSize)
{
	*pSize = sizeof(bf_read);

	return true;
}

void UsrMessageNatives::OnPluginUnloaded(IPlugin *plugin)
{
	MsgWrapperList *pList;

	if (plugin->GetProperty("MsgListeners", reinterpret_cast<void **>(&pList), true))
	{
		MsgWrapperIter iter;
		MsgListenerWrapper *pListener;

		for (iter=pList->begin(); iter!=pList->end(); iter++)
		{
			pListener = (*iter);
			if (g_UserMsgs.UnhookUserMessage(pListener->GetMessageId(), pListener, pListener->IsInterceptHook()))
			{
				m_FreeListeners.push(pListener);
			}
		}

		delete pList;
	}
}

MsgListenerWrapper *UsrMessageNatives::CreateListener(IPluginContext *pCtx)
{
	MsgWrapperList *pList;
	MsgListenerWrapper *pListener;
	IPlugin *pl = g_PluginSys.FindPluginByContext(pCtx->GetContext());

	if (m_FreeListeners.empty())
	{
		pListener = new MsgListenerWrapper;
	} else {
		pListener = m_FreeListeners.front();
		m_FreeListeners.pop();
	}

	if (!pl->GetProperty("MsgListeners", reinterpret_cast<void **>(&pList)))
	{
		pList = new List<MsgListenerWrapper *>;
		pl->SetProperty("MsgListeners", pList);
	}

	pList->push_back(pListener);

	return pListener;
}

bool UsrMessageNatives::FindListener(int msgid, IPluginContext *pCtx, IPluginFunction *pHook, bool intercept, MsgWrapperIter *iter)
{
	MsgWrapperList *pList;
	MsgWrapperIter _iter;
	MsgListenerWrapper *pListener;
	IPlugin *pl = g_PluginSys.FindPluginByContext(pCtx->GetContext());

	if (!pl->GetProperty("MsgListeners", reinterpret_cast<void **>(&pList)))
	{
		return false;
	}

	for (_iter=pList->begin(); _iter!=pList->end(); _iter++)
	{
		pListener = (*_iter);
		if ((msgid == pListener->GetMessageId()) 
			&& (intercept == pListener->IsInterceptHook()) 
			&& (pHook == pListener->GetHookedFunction()))
		{
			*iter = _iter;
			return true;
		}
	}

	return false;
}

bool UsrMessageNatives::DeleteListener(IPluginContext *pCtx, MsgWrapperIter iter)
{
	MsgWrapperList *pList;
	MsgListenerWrapper *pListener;
	IPlugin *pl = g_PluginSys.FindPluginByContext(pCtx->GetContext());

	if (!pl->GetProperty("MsgListeners", reinterpret_cast<void **>(&pList)))
	{
		return false;
	}

	pListener = (*iter);
	pList->erase(iter);
	m_FreeListeners.push(pListener);

	return true;
}

/***************************************
 *                                     *
 * USER MESSAGE WRAPPER IMPLEMENTATION *
 *                                     *
 ***************************************/

void MsgListenerWrapper::Initialize(int msgid, IPluginFunction *hook, IPluginFunction *notify, bool intercept)
{
	if (intercept)
	{
		m_Intercept = hook;
		m_Hook = NULL;
	} else {
		m_Hook = hook;
		m_Intercept = NULL;
	}

	if (notify)
	{
		m_Notify = notify;
	} else {
		m_Notify = NULL;
	}

	m_MsgId = msgid;
	m_IsInterceptHook = intercept;
}

size_t MsgListenerWrapper::_FillInPlayers(int *pl_array, IRecipientFilter *pFilter)
{
	size_t size = static_cast<size_t>(pFilter->GetRecipientCount());

	for (size_t i=0; i<size; i++)
	{
		pl_array[i] = pFilter->GetRecipientIndex(i);
	}

	return size;
}

bool MsgListenerWrapper::IsInterceptHook() const
{
	return m_IsInterceptHook;
}

int MsgListenerWrapper::GetMessageId() const
{
	return m_MsgId;
}

IPluginFunction *MsgListenerWrapper::GetHookedFunction() const
{
	if (m_Hook)
	{
		return m_Hook;
	} else {
		return m_Intercept;
	}
}

IPluginFunction *MsgListenerWrapper::GetNotifyFunction() const
{
	return m_Notify;
}

void MsgListenerWrapper::OnUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
{
	cell_t res;
	size_t size = _FillInPlayers(g_MsgPlayers, pFilter);

	g_ReadBitBuf.StartReading(bf->GetBasePointer(), bf->GetNumBytesWritten());

	m_Hook->PushCell(msg_id);
	m_Hook->PushCell(g_ReadBufHandle);
	m_Hook->PushArray(g_MsgPlayers, size);
	m_Hook->PushCell(size);
	m_Hook->PushCell(pFilter->IsReliable());
	m_Hook->PushCell(pFilter->IsInitMessage());
	m_Hook->Execute(&res);
}

ResultType MsgListenerWrapper::InterceptUserMessage(int msg_id, bf_write *bf, IRecipientFilter *pFilter)
{
	cell_t res = static_cast<cell_t>(Pl_Continue);
	size_t size = _FillInPlayers(g_MsgPlayers, pFilter);

	g_ReadBitBuf.StartReading(bf->GetBasePointer(), bf->GetNumBytesWritten());

	m_Intercept->PushCell(msg_id);
	m_Intercept->PushCell(g_ReadBufHandle);
	m_Intercept->PushArray(g_MsgPlayers, size);
	m_Intercept->PushCell(size);
	m_Intercept->PushCell(pFilter->IsReliable());
	m_Intercept->PushCell(pFilter->IsInitMessage());
	m_Intercept->Execute(&res);

	return static_cast<ResultType>(res);
}

void MsgListenerWrapper::OnUserMessageSent(int msg_id)
{
	if (!m_Notify)
	{
		return;
	}

	cell_t res;
	m_Notify->PushCell(msg_id);
	m_Notify->Execute(&res);
}

/***************************************
 *                                     *
 * USER MESSAGE NATIVE IMPLEMENTATIONS *
 *                                     *
 ***************************************/

static UsrMessageNatives s_UsrMessageNatives;

static cell_t smn_GetUserMessageId(IPluginContext *pCtx, const cell_t *params)
{
	char *msgname;
	pCtx->LocalToString(params[1], &msgname);

	return g_UserMsgs.GetMessageIndex(msgname);
}

static cell_t smn_GetUserMessageName(IPluginContext *pCtx, const cell_t *params)
{
	char *msgname;

	pCtx->LocalToPhysAddr(params[2], (cell_t **)&msgname);

	return g_UserMsgs.GetMessageName(params[1], msgname, params[3]);
}

static cell_t smn_StartMessage(IPluginContext *pCtx, const cell_t *params)
{
	char *msgname;
	cell_t *cl_array;
	int msgid;
	bf_write *pBitBuf;

	if (g_IsMsgInExec)
	{
		return pCtx->ThrowNativeError("Unable to execute a new message, there is already one in progress");
	}

	pCtx->LocalToString(params[1], &msgname);

	if ((msgid=g_UserMsgs.GetMessageIndex(msgname)) == INVALID_MESSAGE_ID)
	{
		return pCtx->ThrowNativeError("Invalid message name: \"%s\"", msgname);
	}

	pCtx->LocalToPhysAddr(params[2], &cl_array);

	pBitBuf = g_UserMsgs.StartMessage(msgid, cl_array, params[3], params[4]);
	if (!pBitBuf)
	{
		return pCtx->ThrowNativeError("Unable to execute a new message while in hook");
	}

	g_CurMsgHandle = g_HandleSys.CreateHandle(g_WrBitBufType, pBitBuf, pCtx->GetIdentity(), g_pCoreIdent, NULL);
	g_IsMsgInExec = true;

	return g_CurMsgHandle;
}

static cell_t smn_StartMessageEx(IPluginContext *pCtx, const cell_t *params)
{
	cell_t *cl_array;
	bf_write *pBitBuf;
	int msgid = params[1];

	if (g_IsMsgInExec)
	{
		return pCtx->ThrowNativeError("Unable to execute a new message, there is already one in progress");
	}

	if (msgid < 0 || msgid >= 255)
	{
		return pCtx->ThrowNativeError("Invalid message id supplied (%d)", msgid);
	}

	pCtx->LocalToPhysAddr(params[2], &cl_array);

	pBitBuf = g_UserMsgs.StartMessage(msgid, cl_array, params[3], params[4]);
	if (!pBitBuf)
	{
		return pCtx->ThrowNativeError("Unable to execute a new message while in hook");
	}

	g_CurMsgHandle = g_HandleSys.CreateHandle(g_WrBitBufType, pBitBuf, pCtx->GetIdentity(), g_pCoreIdent, NULL);
	g_IsMsgInExec = true;

	return g_CurMsgHandle;
}

static cell_t smn_EndMessage(IPluginContext *pCtx, const cell_t *params)
{
	HandleSecurity sec;

	if (!g_IsMsgInExec)
	{
		return pCtx->ThrowNativeError("Unable to end message, no message is in progress");
	}

	g_UserMsgs.EndMessage();

	sec.pOwner = pCtx->GetIdentity();
	sec.pIdentity = g_pCoreIdent;
	g_HandleSys.FreeHandle(g_CurMsgHandle, &sec);

	g_IsMsgInExec = false;

	return 1;
}

static cell_t smn_HookUserMessage(IPluginContext *pCtx, const cell_t *params)
{
	IPluginFunction *pHook, *pNotify;
	MsgListenerWrapper *pListener;
	bool intercept;
	int msgid = params[1];

	if (msgid < 0 || msgid >= 255)
	{
		return pCtx->ThrowNativeError("Invalid message id supplied (%d)", msgid);
	}

	pHook = pCtx->GetFunctionById(params[2]);
	if (!pHook)
	{
		return pCtx->ThrowNativeError("Invalid function id (%X)", params[2]);
	}
	pNotify = pCtx->GetFunctionById(params[4]);
	intercept = (params[3]) ? true : false;

	pListener = s_UsrMessageNatives.CreateListener(pCtx);
	pListener->Initialize(msgid, pHook, pNotify, intercept);

	g_UserMsgs.HookUserMessage(msgid, pListener, intercept);

	return 1;
}

static cell_t smn_UnhookUserMessage(IPluginContext *pCtx, const cell_t *params)
{
	IPluginFunction *pFunc;
	MsgListenerWrapper *pListener;
	MsgWrapperIter iter;
	bool intercept;
	int msgid = params[1];

	if (msgid < 0 || msgid >= 255)
	{
		return pCtx->ThrowNativeError("Invalid message id supplied (%d)", msgid);
	}

	pFunc = pCtx->GetFunctionById(params[2]);
	if (!pFunc)
	{
		return pCtx->ThrowNativeError("Invalid function id (%X)", params[2]);
	}
	intercept = (params[3]) ? true : false;

	if (!s_UsrMessageNatives.FindListener(msgid, pCtx, pFunc, intercept, &iter))
	{
		return pCtx->ThrowNativeError("Unable to unhook the current user message");
	}

	pListener = (*iter);
	if (!g_UserMsgs.UnhookUserMessage(msgid, pListener, intercept))
	{
		return pCtx->ThrowNativeError("Unable to unhook the current user message");
	}

	s_UsrMessageNatives.DeleteListener(pCtx, iter);

	return 1;
}

REGISTER_NATIVES(usrmsgnatives)
{
	{"GetUserMessageId",			smn_GetUserMessageId},
	{"GetUserMessageName",			smn_GetUserMessageName},
	{"StartMessage",				smn_StartMessage},
	{"StartMessageEx",				smn_StartMessageEx},
	{"EndMessage",					smn_EndMessage},
	{"HookUserMessage",				smn_HookUserMessage},
	{"UnhookUserMessage",			smn_UnhookUserMessage},
	{NULL,							NULL}
};
