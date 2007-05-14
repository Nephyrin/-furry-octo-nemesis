/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SMEXT_BINTOOLS_H_
#define _INCLUDE_SMEXT_BINTOOLS_H_

#include <IShareSys.h>

#define SMINTERFACE_BINTOOLS_NAME		"IBinTools"
#define SMINTERFACE_BINTOOLS_VERSION	1

/**
 * @brief Function calling encoding utilities
 * @file IBinTools.h
 */

namespace SourceMod
{
	/**
	 * @brief Supported calling conventions
	 */
	enum CallConvention
	{
		CallConv_ThisCall,		/**< This call (object pointer required) */
		CallConv_Cdecl,			/**< Standard C call */
	};

	/**
	 * @brief Describes how a parameter should be passed
	 */
	enum PassType
	{
		PassType_Basic,			/**< Plain old register data (pointers, integers) */
		PassType_Float,			/**< Floating point data */
		PassType_Object,		/**< Object or structure */
	};

	#define PASSFLAG_BYVAL		(1<<0)		/**< Passing by value */
	#define PASSFLAG_BYREF		(1<<1)		/**< Passing by reference */
	#define PASSFLAG_ODTOR		(1<<2)		/**< Object has a destructor */
	#define PASSFLAG_OCTOR		(1<<3)		/**< Object has a constructor */
	#define PASSFLAG_OASSIGNOP	(1<<4)		/**< Object has an assignment operator */

	/**
	 * @brief Parameter passing information
	 */
	struct PassInfo
	{
		PassType type;			/**< PassType value */
		unsigned int flags;		/**< Pass/return flags */
		size_t size;			/**< Size of the data being passed */
	};

	/**
	 * @brief Parameter encoding information
	 */
	struct PassEncode
	{
		PassInfo info;			/**< Parameter information */
		size_t offset;			/**< Offset into the virtual stack */
	};

	/**
	 * @brief Wraps a C/C++ call.
	 */
	class ICallWrapper
	{
	public:
		/**
		 * @brief Returns the calling convention.
		 *
		 * @return				CallConvention value.
		 */
		virtual CallConvention GetCallConvention() =0;

		/**
		 * @brief Returns parameter info.
		 *
		 * @param num			Parameter number to get (starting from 0).
		 * @return				A PassInfo pointer.
		 */
		virtual const PassEncode *GetParamInfo(unsigned int num) =0;

		/**
		 * @brief Returns return type info.
		 *
		 * @return				A PassInfo pointer.
		 */
		virtual const PassInfo *GetReturnInfo() =0;

		/**
		 * @brief Returns the number of parameters.
		 */
		virtual unsigned int GetParamCount() =0;

		/**
		 * @brief Execute the contained function.
		 */
		virtual void Execute(void *vParamStack, void *retBuffer) =0;
	};

	/**
	 * @brief Binary tools interface.
	 */
	class IBinTools : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_BINTOOLS_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_BINTOOLS_VERSION;
		}
	public:
		/**
		 * @brief Creates a call decoder.
		 *
		 * Note: CallConv_ThisCall requires an implicit first parameter
		 * of PassType_Basic / PASSFLAG_BYVAL / sizeof(void *).  However,
		 * this should only be given to the Execute() function, and never
		 * listed in the paramInfo array.
		 *
		 * @param address			Address to use as a call.
		 * @param cv				Calling convention.
		 * @param retInfo			Return type information, or NULL for void.
		 * @param paramInfo			Array of parameters.
		 * @param numParams			Number of parameters in the array.
		 * @return					A new ICallWrapper function.
		 */
		virtual ICallWrapper *CreateCall(void *address,
											CallConvention cv,
											const PassInfo *retInfo,
											const PassInfo paramInfo[],
											unsigned int numParams) =0;

		/**
		 * @brief Creates a vtable call decoder.
		 *
		 * Note: CallConv_ThisCall requires an implicit first parameter
		 * of PassType_Basic / PASSFLAG_BYVAL / sizeof(void *).  However,
		 * this should only be given to the Execute() function, and never
		 * listed in the paramInfo array.
		 *
		 * @param vtblIdx			Index into the virtual table.
		 * @param vtblOffs			Offset of the virtual table.
		 * @param thisOffs			Offset of the this pointer of the virtual table.
		 * @param retInfo			Return type information, or NULL for void.
		 * @param paramInfo			Array of parameters.
		 * @param numParams			Number of parameters in the array.
		 * @return					A new ICallWrapper function.
		 */
		virtual ICallWrapper *CreateVCall(unsigned int vtblIdx,
											unsigned int vtblOffs,
											unsigned int thisOffs,
											const PassInfo *retInfo,
											const PassInfo paramInfo[],
											unsigned int numParams) =0;
	};
}

#endif //_INCLUDE_SMEXT_BINTOOLS_H_