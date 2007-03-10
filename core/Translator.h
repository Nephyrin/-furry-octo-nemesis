/**
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

#ifndef _INCLUDE_SOURCEMOD_TRANSLATOR_H_
#define _INCLUDE_SOURCEMOD_TRANSLATOR_H_

#include "sm_trie.h"
#include <sh_string.h>
#include <sh_vector.h>
#include "sm_globals.h"
#include "sm_memtable.h"
#include "ITextParsers.h"

#define MAX_TRANSLATE_PARAMS		32

/* :TODO: write a templatized version of tries? */

using namespace SourceHook;
class CTranslator;

enum PhraseParseState
{
	PPS_None = 0,
	PPS_Phrases,
	PPS_InPhrase,
};

struct Language
{
	char m_code2[3];
	int m_FullName;
};

struct Translation
{
	const char *szPhrase;		/**< Translated phrase. */
	unsigned int fmt_count;		/**< Number of format parameters. */
	int *fmt_order;				/**< Format phrase order. */
};

enum TransError
{
	Trans_Okay = 0,
	Trans_BadLanguage = 1,
	Trans_BadPhrase = 2,
	Trans_BadPhraseLanguage = 3,
	Trans_BadPhraseFile = 4,
};

class CPhraseFile : public ITextListener_SMC
{
public:
	CPhraseFile(CTranslator *pTranslator, const char *file);
	~CPhraseFile();
public:
	void ReparseFile();
	const char *GetFilename();
	TransError GetTranslation(const char *szPhrase, unsigned int lang_id, Translation *pTrans);
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	void ReadSMC_ParseEnd(bool halted, bool failed);
	SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes);
	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes);
	SMCParseResult ReadSMC_LeavingSection();
	SMCParseResult ReadSMC_RawLine(const char *line, unsigned int curline);
private:
	void ParseError(const char *message, ...);
	void ParseWarning(const char *message, ...);
private:
	Trie *m_pPhraseLookup;
	String m_File;
	CTranslator *m_pTranslator;
	PhraseParseState m_ParseState;
	int m_CurPhrase;
	BaseMemTable *m_pMemory;
	BaseStringTable *m_pStringTab;
	unsigned int m_LangCount;
	String m_ParseError;
	String m_LastPhraseString;
	unsigned int m_CurLine;
	bool m_FileLogged;
};

class CTranslator : 
	public ITextListener_SMC,
	public SMGlobalClass
{
public:
	CTranslator();
	~CTranslator();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
public: //ITextListener_SMC
	void ReadSMC_ParseStart();
	SMCParseResult ReadSMC_NewSection(const char *name, bool opt_quotes);
	SMCParseResult ReadSMC_KeyValue(const char *key, const char *value, bool key_quotes, bool value_quotes);
	SMCParseResult ReadSMC_LeavingSection();
public:
	void RebuildLanguageDatabase(const char *lang_header_file);
	unsigned int FindOrAddPhraseFile(const char *phrase_file);
	BaseStringTable *GetStringTable();
	unsigned int GetLanguageCount();
	bool GetLanguageByCode(const char *code, unsigned int *index);
	size_t Translate(char *buffer, size_t maxlength, void **params, const Translation *pTrans);
	CPhraseFile *GetFileByIndex(unsigned int index);
	TransError CoreTrans(int client, 
						 char *buffer, 
						 size_t maxlength, 
						 const char *phrase, 
						 void **params, 
						 size_t *outlen=NULL);
private:
	bool AddLanguage(const char *langcode, const char *description);
private:
	CVector<Language *> m_Languages;
	CVector<CPhraseFile *> m_Files;
	BaseStringTable *m_pStringTab;
	Trie *m_pLCodeLookup;
	bool m_InLanguageSection;
	String m_CustomError;
};

extern CPhraseFile *g_pCorePhrases;
extern CTranslator g_Translator;

#endif //_INCLUDE_SOURCEMOD_TRANSLATOR_H_