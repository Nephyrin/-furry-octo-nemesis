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

#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "sm_stringutil.h"
#include "Logger.h"
#include "PluginSys.h"
#include "Translator.h"
#include "PlayerManager.h"

#define LADJUST			0x00000004		/* left adjustment */
#define ZEROPAD			0x00000080		/* zero (as opposed to blank) pad */
#define UPPERDIGITS		0x00000200		/* make alpha digits uppercase */
#define to_digit(c)		((c) - '0')
#define is_digit(c)		((unsigned)to_digit(c) <= 9)

#define CHECK_ARGS(x) \
	if ((arg+x) > args) { \
		pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "String formatted incorrectly - parameter %d (total %d)", arg, args); \
		return 0; \
	}

size_t CorePlayerTranslate(int client, char *buffer, size_t maxlength, const char *phrase, void **params)
{
	Translation pTrans;
	TransError err;

	err = g_pCorePhrases->GetTranslation(phrase, g_Translator.GetClientLanguage(client), &pTrans);
	if (err != Trans_Okay)
	{
		err = g_pCorePhrases->GetTranslation(phrase, g_Translator.GetServerLanguage(), &pTrans);
		if (err != Trans_Okay && g_Translator.GetServerLanguage() != CORELANG_ENGLISH)
		{
			err = g_pCorePhrases->GetTranslation(phrase, CORELANG_ENGLISH, &pTrans);
		}
	}

	if (err != Trans_Okay)
	{
		return UTIL_Format(buffer, maxlength, "%s", phrase);
	}

	return g_Translator.Translate(buffer, maxlength, params, &pTrans);
}

inline bool TryTranslation(CPlugin *pl, const char *key, unsigned int langid, unsigned int langcount, Translation *pTrans)
{
	TransError err = Trans_BadLanguage;
	CPhraseFile *phrfl;

	for (size_t i=0; i<langcount && err!=Trans_Okay; i++)
	{
		phrfl = g_Translator.GetFileByIndex(pl->GetLangFileByIndex(i));
		err = phrfl->GetTranslation(key, langid, pTrans);
	}

	return (err == Trans_Okay) ? true : false;
}

inline void ReorderTranslationParams(const Translation *pTrans, cell_t *params)
{
	cell_t new_params[MAX_TRANSLATE_PARAMS];
	for (unsigned int i = 0; i < pTrans->fmt_count; i++)
	{
		new_params[i] = params[pTrans->fmt_order[i]];
	}
	memcpy(params, new_params, pTrans->fmt_count * sizeof(cell_t));
}

size_t Translate(char *buffer,
				 size_t maxlen,
				 IPluginContext *pCtx,
				 const char *key,
				 cell_t target,
				 const cell_t *params,
				 int *arg,
				 bool *error)
{
	unsigned int langid;
	*error = false;
	Translation pTrans;
	CPlugin *pl = (CPlugin *)g_PluginSys.FindPluginByContext(pCtx->GetContext());
	size_t langcount = pl->GetLangFileCount();
	unsigned int max_params = 0;

try_serverlang:
	if (target == LANG_SERVER)
	{
		langid = g_Translator.GetServerLanguage();
 	}
	else if ((target >= 1) && (target <= g_Players.GetMaxClients()))
	{
		langid = g_Translator.GetClientLanguage(target);
	}
	else
	{
		pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "Translation failed: invalid client index %d", target);
		goto error_out;
	}

	if (!TryTranslation(pl, key, langid, langcount, &pTrans))
	{
		if (target != LANG_SERVER && langid != g_Translator.GetServerLanguage())
		{
			target = LANG_SERVER;
			goto try_serverlang;
		}
		else if (langid != LANGUAGE_ENGLISH)
		{
			if (!TryTranslation(pl, key, LANGUAGE_ENGLISH, langcount, &pTrans))
			{
				pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "Language phrase \"%s\" not found", key);
				goto error_out;
			}
		}
		else
		{
			pCtx->ThrowNativeErrorEx(SP_ERROR_PARAM, "Language phrase \"%s\" not found", key);
			goto error_out;
		}
	}

	max_params = pTrans.fmt_count;

	if (max_params)
	{
		cell_t new_params[MAX_TRANSLATE_PARAMS];

		/* Check if we're going to over the limit */
		if ((*arg) + (max_params - 1) > (size_t)params[0])
		{
			pCtx->ThrowNativeErrorEx(SP_ERROR_PARAMS_MAX, 
				"Translation string formatted incorrectly - missing at least %d parameters", 
				((*arg + (max_params - 1)) - params[0])
				);
			goto error_out;
		}

		/* If we need to re-order the parameters, do so with a temporary array.
		 * Otherwise, we could run into trouble with continual formats, a la ShowActivity().
		 */
		memcpy(new_params, params, sizeof(cell_t) * (params[0] + 1));
		ReorderTranslationParams(&pTrans, &new_params[*arg]);

		return atcprintf(buffer, maxlen, pTrans.szPhrase, pCtx, new_params, arg);
	}
	else
	{
		return atcprintf(buffer, maxlen, pTrans.szPhrase, pCtx, params, arg);
	}
	
error_out:
	*error = true;
	return 0;
}

void AddString(char **buf_p, size_t &maxlen, const char *string, int width, int prec)
{
	int size = 0;
	char *buf;
	static char nlstr[] = {'(','n','u','l','l',')','\0'};

	buf = *buf_p;

	if (string == NULL)
	{
		string = nlstr;
		prec = -1;
	}

	if (prec >= 0)
	{
		for (size = 0; size < prec; size++) 
		{
			if (string[size] == '\0')
			{
				break;
			}
		}
	}
	else
	{
		while (string[size++]);
		size--;
	}

	if (size > (int)maxlen)
	{
		size = maxlen;
	}

	maxlen -= size;
	width -= size;

	while (size--)
	{
		*buf++ = *string++;
	}

	while ((width-- > 0) && maxlen)
	{
		*buf++ = ' ';
		maxlen--;
	}

	*buf_p = buf;
}

void AddFloat(char **buf_p, size_t &maxlen, double fval, int width, int prec, int flags)
{
	int digits;					// non-fraction part digits
	double tmp;					// temporary
	char *buf = *buf_p;			// output buffer pointer
	int val;					// temporary
	int sign = 0;				// 0: positive, 1: negative
	int fieldlength;			// for padding
	int significant_digits = 0;	// number of significant digits written
	const int MAX_SIGNIFICANT_DIGITS = 16;

	// default precision
	if (prec < 0)
	{
		prec = 6;
	}

	// get the sign
	if (fval < 0)
	{
		fval = -fval;
		sign = 1;
	}

	// compute whole-part digits count
	digits = (int)log10(fval) + 1;

	// Only print 0.something if 0 < fval < 1
	if (digits < 1)
	{
		digits = 1;
	}

	// compute the field length
	fieldlength = digits + prec + ((prec > 0) ? 1 : 0) + sign;

	// minus sign BEFORE left padding if padding with zeros
	if (sign && maxlen && (flags & ZEROPAD))
	{
		*buf++ = '-';
		maxlen--;
	}

	// right justify if required
	if ((flags & LADJUST) == 0)
	{
		while ((fieldlength < width) && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			maxlen--;
		}
	}

	// minus sign AFTER left padding if padding with spaces
	if (sign && maxlen && !(flags & ZEROPAD))
	{
		*buf++ = '-';
		maxlen--;
	}

	// write the whole part
	tmp = pow(10.0, digits-1);
	while ((digits--) && maxlen)
	{
		if (++significant_digits > MAX_SIGNIFICANT_DIGITS)
		{
			*buf++ = '0';
		}
		else
		{
			val = (int)(fval / tmp);
			*buf++ = '0' + val;
			fval -= val * tmp;
			tmp *= 0.1;
		}
		maxlen--;
	}

	// write the fraction part
	if (maxlen)
	{
		*buf++ = '.';
		maxlen--;
	}

	tmp = pow(10.0, prec);

	fval *= tmp;
	while (prec-- && maxlen)
	{
		if (++significant_digits > MAX_SIGNIFICANT_DIGITS)
		{
			*buf++ = '0';
		}
		else
		{
			tmp *= 0.1;
			val = (int)(fval / tmp);
			*buf++ = '0' + val;
			fval -= val * tmp;
		}
		maxlen--;
	}

	// left justify if required
	if (flags & LADJUST)
	{
		while ((fieldlength < width) && maxlen)
		{
			// right-padding only with spaces, ZEROPAD is ignored
			*buf++ = ' ';
			width--;
			maxlen--;
		}
	}

	// update parent's buffer pointer
	*buf_p = buf;
}

void AddBinary(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags)
{
	char text[32];
	int digits;
	char *buf;

	digits = 0;
	do
	{
		if (val & 1)
		{
			text[digits++] = '1';
		}
		else
		{
			text[digits++] = '0';
		}
		val >>= 1;
	} while (val);

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while (digits < width && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			maxlen--;
		}
	}

	while (digits-- && maxlen)
	{
		*buf++ = text[digits];
		width--;
		maxlen--;
	}

	if (flags & LADJUST)
	{
		while (width-- && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			maxlen--;
		}
	}

	*buf_p = buf;
}

void AddUInt(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags)
{
	char text[32];
	int digits;
	char *buf;

	digits = 0;
	do
	{
		text[digits++] = '0' + val % 10;
		val /= 10;
	} while (val);

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while (digits < width && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			maxlen--;
		}
	}

	while (digits-- && maxlen)
	{
		*buf++ = text[digits];
		width--;
		maxlen--;
	}

	if (flags & LADJUST)
	{
		while (width-- && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			maxlen--;
		}
	}

	*buf_p = buf;
}

void AddInt(char **buf_p, size_t &maxlen, int val, int width, int flags)
{
	char text[32];
	int digits;
	int signedVal;
	char *buf;
	unsigned int unsignedVal;

	digits = 0;
	signedVal = val;
	if (val < 0)
	{
		/* we want the unsigned version */
		unsignedVal = abs(val);
	}
	else
	{
		unsignedVal = val;
	}

	do
	{
		text[digits++] = '0' + unsignedVal % 10;
		unsignedVal /= 10;
	} while (unsignedVal);

	if (signedVal < 0)
	{
		text[digits++] = '-';
	}

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while ((digits < width) && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			maxlen--;
		}
	}

	while (digits-- && maxlen)
	{
		*buf++ = text[digits];
		width--;
		maxlen--;
	}

	if (flags & LADJUST)
	{
		while (width-- && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			maxlen--;
		}
	}

	*buf_p = buf;
}

void AddHex(char **buf_p, size_t &maxlen, unsigned int val, int width, int flags)
{
	char text[32];
	int digits;
	char *buf;
	char digit;
	int hexadjust;

	if (flags & UPPERDIGITS)
	{
		hexadjust = 'A' - '9' - 1;
	}
	else
	{
		hexadjust = 'a' - '9' - 1;
	}

	digits = 0;
	do 
	{
		digit = ('0' + val % 16);
		if (digit > '9')
		{
			digit += hexadjust;
		}

		text[digits++] = digit;
		val /= 16;
	} while(val);

	buf = *buf_p;

	if (!(flags & LADJUST))
	{
		while (digits < width && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			width--;
			maxlen--;
		}
	}

	while (digits-- && maxlen)
	{
		*buf++ = text[digits];
		width--;
		maxlen--;
	}

	if (flags & LADJUST)
	{
		while (width-- && maxlen)
		{
			*buf++ = (flags & ZEROPAD) ? '0' : ' ';
			maxlen--;
		}
	}

	*buf_p = buf;
}

size_t gnprintf(char *buffer, size_t maxlen, const char *format, void **args)
{
	if (!buffer || !maxlen)
	{
		return 0;
	}

	int arg = 0;
	char *buf_p;
	char ch;
	int flags;
	int width;
	int prec;
	int n;
	char sign;
	const char *fmt;
	size_t llen = maxlen - 1;

	buf_p = buffer;
	fmt = format;

	while (true)
	{
		// run through the format string until we hit a '%' or '\0'
		for (ch = *fmt; llen && ((ch = *fmt) != '\0') && (ch != '%'); fmt++)
		{
			*buf_p++ = ch;
			llen--;
		}
		if ((ch == '\0') || (llen <= 0))
		{
			goto done;
		}

		// skip over the '%'
		fmt++;

		// reset formatting state
		flags = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:
		ch = *fmt++;
reswitch:
		switch(ch)
		{
		case '-':
			{
				flags |= LADJUST;
				goto rflag;
			}
		case '.':
			{
				n = 0;
				while(is_digit((ch = *fmt++)))
				{
					n = 10 * n + (ch - '0');
				}
				prec = (n < 0) ? -1 : n;
				goto reswitch;
			}
		case '0':
			{
				flags |= ZEROPAD;
				goto rflag;
			}
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				n = 0;
				do
				{
					n = 10 * n + (ch - '0');
					ch = *fmt++;
				} while(is_digit(ch));
				width = n;
				goto reswitch;
			}
		case 'c':
			{
				if (!llen)
				{
					goto done;
				}
				char *c = (char *)args[arg];
				*buf_p++ = *c;
				llen--;
				arg++;
				break;
			}
		case 'b':
			{
				int *value = (int *)args[arg];
				AddBinary(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'd':
		case 'i':
			{
				int *value = (int *)args[arg];
				AddInt(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'u':
			{
				unsigned int *value = (unsigned int *)args[arg];
				AddUInt(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'f':
			{
				float *value = (float *)args[arg];
				AddFloat(&buf_p, llen, *value, width, prec, flags);
				arg++;
				break;
			}
		case 's':
			{
				const char *str = (const char *)args[arg];
				AddString(&buf_p, llen, str, width, prec);
				arg++;
				break;
			}
		case 'X':
			{
				unsigned int *value = (unsigned int *)args[arg];
				flags |= UPPERDIGITS;
				AddHex(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'x':
			{
				unsigned int *value = (unsigned int *)args[arg];
				AddHex(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case '%':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
		case '\0':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = '%';
				llen--;
				goto done;
			}
		default:
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
		}
	}

done:
	*buf_p = '\0';

	return (maxlen - llen - 1);
}

size_t atcprintf(char *buffer, size_t maxlen, const char *format, IPluginContext *pCtx, const cell_t *params, int *param)
{
	if (!buffer || !maxlen)
	{
		return 0;
	}

	int arg;
	int args = params[0];
	char *buf_p;
	char ch;
	int flags;
	int width;
	int prec;
	int n;
	char sign;
	const char *fmt;
	size_t llen = maxlen - 1;

	buf_p = buffer;
	arg = *param;
	fmt = format;

	while (true)
	{
		// run through the format string until we hit a '%' or '\0'
		for (ch = *fmt; llen && ((ch = *fmt) != '\0') && (ch != '%'); fmt++)
		{
			*buf_p++ = ch;
			llen--;
		}
		if ((ch == '\0') || (llen <= 0))
		{
			goto done;
		}

		// skip over the '%'
		fmt++;

		// reset formatting state
		flags = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag:
		ch = *fmt++;
reswitch:
		switch(ch)
		{
		case '-':
			{
				flags |= LADJUST;
				goto rflag;
			}
		case '.':
			{
				n = 0;
				while(is_digit((ch = *fmt++)))
				{
					n = 10 * n + (ch - '0');
				}
				prec = (n < 0) ? -1 : n;
				goto reswitch;
			}
		case '0':
			{
				flags |= ZEROPAD;
				goto rflag;
			}
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				n = 0;
				do
				{
					n = 10 * n + (ch - '0');
					ch = *fmt++;
				} while(is_digit(ch));
				width = n;
				goto reswitch;
			}
		case 'c':
			{
				CHECK_ARGS(0);
				if (!llen)
				{
					goto done;
				}
				char *c;
				pCtx->LocalToString(params[arg], &c);
				*buf_p++ = *c;
				llen--;
				arg++;
				break;
			}
		case 'b':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddBinary(&buf_p, llen, *value, width, flags);
				arg++;
				break;
			}
		case 'd':
		case 'i':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddInt(&buf_p, llen, static_cast<int>(*value), width, flags);
				arg++;
				break;
			}
		case 'u':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddUInt(&buf_p, llen, static_cast<unsigned int>(*value), width, flags);
				arg++;
				break;
			}
		case 'f':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddFloat(&buf_p, llen, sp_ctof(*value), width, prec, flags);
				arg++;
				break;
			}
		case 'L':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				char buffer[255];
				if (*value)
				{
					CPlayer *player = g_Players.GetPlayerByIndex(*value);
					if (!player || !player->IsConnected())
					{
						return pCtx->ThrowNativeError("Client index %d is invalid", *value);
					}
					const char *auth = player->GetAuthString();
					if (!auth || auth[0] == '\0')
					{
						auth = "STEAM_ID_PENDING";
					}
					int userid = engine->GetPlayerUserId(player->GetEdict());
					UTIL_Format(buffer, 
						sizeof(buffer), 
						"%s<%d><%s><>", 
						player->GetName(),
						userid,
						auth);
				}
				else
				{
					UTIL_Format(buffer,
						sizeof(buffer),
						"Console<0><Console><Console>");
				}
				AddString(&buf_p, llen, buffer, width, prec);
				arg++;
				break;
			}
		case 'N':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);

				const char *name = "Console";
				if (*value)
				{
					CPlayer *player = g_Players.GetPlayerByIndex(*value);
					if (!player || !player->IsConnected())
					{
						return pCtx->ThrowNativeError("Client index %d is invalid", *value);
					}
					name = player->GetName();
				}
				AddString(&buf_p, llen, name, width, prec);
				arg++;
				break;
			}
		case 's':
			{
				CHECK_ARGS(0);
				char *str;
				int err;
				if ((err=pCtx->LocalToString(params[arg], &str)) != SP_ERROR_NONE)
				{
					pCtx->ThrowNativeErrorEx(err, "Could not deference string");
					return 0;
				}
				AddString(&buf_p, llen, str, width, prec);
				arg++;
				break;
			}
		case 'T':
			{
				CHECK_ARGS(1);
				char *key;
				bool error;
				size_t res;
				cell_t *target;
				pCtx->LocalToString(params[arg++], &key);
				pCtx->LocalToPhysAddr(params[arg++], &target);
				res = Translate(buf_p, llen, pCtx, key, *target, params, &arg, &error);
				if (error)
				{
					return 0;
				}
				buf_p += res;
				llen -= res;
				break;
			}
		case 't':
			{
				CHECK_ARGS(0);
				char *key;
				bool error;
				size_t res;
				cell_t target = g_SourceMod.GetGlobalTarget();
				pCtx->LocalToString(params[arg++], &key);
				res = Translate(buf_p, llen, pCtx, key, target, params, &arg, &error);
				if (error)
				{
					return 0;
				}
				buf_p += res;
				llen -= res;
				break;
			}
		case 'X':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				flags |= UPPERDIGITS;
				AddHex(&buf_p, llen, static_cast<unsigned int>(*value), width, flags);
				arg++;
				break;
			}
		case 'x':
			{
				CHECK_ARGS(0);
				cell_t *value;
				pCtx->LocalToPhysAddr(params[arg], &value);
				AddHex(&buf_p, llen, static_cast<unsigned int>(*value), width, flags);
				arg++;
				break;
			}
		case '%':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
		case '\0':
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = '%';
				llen--;
				goto done;
			}
		default:
			{
				if (!llen)
				{
					goto done;
				}
				*buf_p++ = ch;
				llen--;
				break;
			}
		}
	}

done:
	*buf_p = '\0';
	*param = arg;
	return (maxlen - llen - 1);
}

const char *stristr(const char *str, const char *substr)
{
	if (!*substr)
	{
		return ((char *)str);
	}

	char *needle = (char *)substr;
	char *prevloc = (char *)str;
	char *haystack = (char *)str;

	while (*haystack)
	{
		if (tolower(*haystack) == tolower(*needle))
		{
			haystack++;
			if (!*++needle)
			{
				return prevloc;
			}
		}
		else
		{
			haystack = ++prevloc;
			needle = (char *)substr;
		}
	}

	return NULL;
}

unsigned int strncopy(char *dest, const char *src, size_t count)
{
	if (!count)
	{
		return 0;
	}

	char *start = dest;
	while ((*src) && (--count))
	{
		*dest++ = *src++;
	}
	*dest = '\0';

	return (dest - start);
}

size_t UTIL_Format(char *buffer, size_t maxlength, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);
	va_end(ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}

size_t UTIL_FormatArgs(char *buffer, size_t maxlength, const char *fmt, va_list ap)
{
	size_t len = vsnprintf(buffer, maxlength, fmt, ap);

	if (len >= maxlength)
	{
		buffer[maxlength - 1] = '\0';
		return (maxlength - 1);
	}
	else
	{
		return len;
	}
}

char *sm_strdup(const char *str)
{
	char *ptr = new char[strlen(str)+1];
	strcpy(ptr, str);
	return ptr;
}

unsigned int UTIL_ReplaceAll(char *subject, size_t maxlength, const char *search, const char *replace)
{
	size_t searchLen = strlen(search);
	size_t replaceLen = strlen(replace);

	char *ptr = subject;
	unsigned int total = 0;
	while ((ptr = UTIL_ReplaceEx(ptr, maxlength, search, searchLen, replace, replaceLen)) != NULL)
	{
		total++;
		if (*ptr == '\0')
		{
			break;
		}
	}

	return total;
}

/**
 * NOTE: Do not edit this for the love of god unless you have 
 * read the test cases and understand the code behind each one.
 * While I don't guarantee there aren't mistakes, I do guarantee
 * that plugins will end up relying on tiny idiosyncracies of this
 * function, just like they did with AMX Mod X.
 *
 * There are explicitly more cases than the AMX Mod X version because
 * we're not doing a blind copy.  Each case is specifically optimized
 * for what needs to be done.  Even better, we don't have to error on
 * bad buffer sizes.  Instead, this function will smartly cut off the 
 * string in a way that pushes old data out.
 */
char *UTIL_ReplaceEx(char *subject, size_t maxLen, const char *search, size_t searchLen, const char *replace, size_t replaceLen)
{
	char *ptr = subject;
	size_t browsed = 0;
	size_t textLen = strlen(subject);

	/* It's not possible to search or replace */
	if (searchLen > textLen)
	{
		return NULL;
	}

	/* Handle the case of one byte replacement.
	 * It's only valid in one case.
	 */
	if (maxLen == 1)
	{
		/* If the search matches and the replace length is 0, 
		 * we can just terminate the string and be done.
		 */
		if (strcmp(subject, search) == 0 && replaceLen == 0)
		{
			*subject = '\0';
			return subject;
		}
		else
		{
			return NULL;
		}
	}

	/* Subtract one off the maxlength so we can include the null terminator */
	maxLen--;

	while (*ptr != '\0' && (browsed <= textLen - searchLen))
	{
		/* See if we get a comparison */
		if (strncmp(ptr, search, searchLen) == 0)
		{
			if (replaceLen > searchLen)
			{
				/* First, see if we have enough space to do this operation */
				if (maxLen - textLen < replaceLen - searchLen)
				{
					/* First, see if the replacement length goes out of bounds. */
					if (browsed + replaceLen >= maxLen)
					{
						/* EXAMPLE CASE:
						 * Subject: AABBBCCC
						 * Buffer : 12 bytes
						 * Search : BBB
						 * Replace: DDDDDDDDDD
						 * OUTPUT : AADDDDDDDDD
						 * POSITION:           ^
						 */
						/* If it does, we'll just bound the length and do a strcpy. */
						replaceLen = maxLen - browsed;
						/* Note, we add one to the final result for the null terminator */
						strncopy(ptr, replace, replaceLen+1);
					}
					else
					{
						/* EXAMPLE CASE:
						 * Subject: AABBBCCC
						 * Buffer : 12 bytes
						 * Search : BBB
						 * Replace: DDDDDDD
						 * OUTPUT : AADDDDDDDCC
						 * POSITION:         ^
						 */
						/* We're going to have some bytes left over... */
						size_t origBytesToCopy = (textLen - (browsed + searchLen)) + 1;
						size_t realBytesToCopy = (maxLen - (browsed + replaceLen)) + 1;
						char *moveFrom = ptr + searchLen + (origBytesToCopy - realBytesToCopy);
						char *moveTo = ptr + replaceLen;

						/* First, move our old data out of the way. */
						memmove(moveTo, moveFrom, realBytesToCopy);

						/* Now, do our replacement. */
						memcpy(ptr, replace, replaceLen);
					}
				}
				else
				{
					/* EXAMPLE CASE:
					 * Subject: AABBBCCC
					 * Buffer : 12 bytes
					 * Search : BBB
					 * Replace: DDDD
					 * OUTPUT : AADDDDCCC
					 * POSITION:      ^
					 */
					/* Yes, we have enough space.  Do a normal move operation. */
					char *moveFrom = ptr + searchLen;
					char *moveTo = ptr + replaceLen;

					/* First move our old data out of the way. */
					size_t bytesToCopy = (textLen - (browsed + searchLen)) + 1;
					memmove(moveTo, moveFrom, bytesToCopy);

					/* Now do our replacement. */
					memcpy(ptr, replace, replaceLen);
				}
			}
			else if (replaceLen < searchLen)
			{
				/* EXAMPLE CASE:
				 * Subject: AABBBCCC
				 * Buffer : 12 bytes
				 * Search : BBB
				 * Replace: D
				 * OUTPUT : AADCCC
				 * POSOTION:   ^
				 */
				/* If the replacement does not grow the string length, we do not
				 * need to do any fancy checking at all.  Yay!
				 */
				char *moveFrom = ptr + searchLen;		/* Start after the search pointer */
				char *moveTo = ptr + replaceLen;		/* Copy to where the replacement ends */

				/* Copy our replacement in, if any */
				if (replaceLen)
				{
					memcpy(ptr, replace, replaceLen);
				}

				/* Figure out how many bytes to move down, including null terminator */
				size_t bytesToCopy = (textLen - (browsed + searchLen)) + 1;

				/* Move the rest of the string down */
				memmove(moveTo, moveFrom, bytesToCopy);
			}
			else
			{
				/* EXAMPLE CASE:
				 * Subject: AABBBCCC
				 * Buffer : 12 bytes
				 * Search : BBB
				 * Replace: DDD
				 * OUTPUT : AADDDCCC
				 * POSITION:     ^
				 */
				/* We don't have to move anything around, just do a straight copy */
				memcpy(ptr, replace, replaceLen);
			}

			return ptr + replaceLen;
		}
		ptr++;
		browsed++;
	}

	return NULL;
}

char *UTIL_TrimWhitespace(char *str, size_t &len)
{
	char *end = str + len - 1;

	if (!len)
	{
		return str;
	}

	/* Iterate backwards through string until we reach first non-whitespace char */
	while (end >= str && textparsers->IsWhitespace(end))
	{
		end--;
		len--;
	}

	/* Replace first whitespace char (at the end) with null terminator.
	 * If there is none, we're just replacing the null terminator. 
	 */
	*(end + 1) = '\0';

	while (*str != '\0' && textparsers->IsWhitespace(str))
	{
		str++;
		len--;
	}

	return str;
}

