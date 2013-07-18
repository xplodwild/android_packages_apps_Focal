#ifndef __XMPUtils_hpp__
#define __XMPUtils_hpp__

// =================================================================================================
// Copyright 2003 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:	Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"
#include "public/include/XMP_Const.h"

#include "XMPCore/source/XMPMeta.hpp"
#include "XMPCore/source/XMPCore_Impl.hpp"
#include "public/include/client-glue/WXMPUtils.hpp"

// -------------------------------------------------------------------------------------------------

class	XMPUtils {
public:
	
	static bool
	Initialize();	// ! For internal use only!
	
	static void
	Terminate() RELEASE_NO_THROW;	// ! For internal use only!

	// ---------------------------------------------------------------------------------------------

	static void
	ComposeArrayItemPath ( XMP_StringPtr   schemaNS,
						   XMP_StringPtr   arrayName,
						   XMP_Index	   itemIndex,
						   XMP_VarString * fullPath );

	static void
	ComposeStructFieldPath ( XMP_StringPtr	 schemaNS,
							 XMP_StringPtr	 structName,
							 XMP_StringPtr	 fieldNS,
							 XMP_StringPtr	 fieldName,
							 XMP_VarString * fullPath );

	static void
	ComposeQualifierPath ( XMP_StringPtr   schemaNS,
						   XMP_StringPtr   propName,
						   XMP_StringPtr   qualNS,
						   XMP_StringPtr   qualName,
						   XMP_VarString * fullPath );

	static void
	ComposeLangSelector ( XMP_StringPtr	  schemaNS,
						  XMP_StringPtr	  arrayName,
						  XMP_StringPtr	  langName,
						  XMP_VarString * fullPath );

	static void
	ComposeFieldSelector ( XMP_StringPtr   schemaNS,
						   XMP_StringPtr   arrayName,
						   XMP_StringPtr   fieldNS,
						   XMP_StringPtr   fieldName,
						   XMP_StringPtr   fieldValue,
						   XMP_VarString * fullPath );

	// ---------------------------------------------------------------------------------------------

	static void
	ConvertFromBool ( bool			  binValue,
					  XMP_VarString * strValue );

	static void
	ConvertFromInt ( XMP_Int32		 binValue,
					 XMP_StringPtr	 format,
					 XMP_VarString * strValue );

	static void
	ConvertFromInt64 ( XMP_Int64	   binValue,
					   XMP_StringPtr   format,
					   XMP_VarString * strValue );

	static void
	ConvertFromFloat ( double		   binValue,
					   XMP_StringPtr   format,
					   XMP_VarString * strValue );

	static void
	ConvertFromDate ( const XMP_DateTime & binValue,
					  XMP_VarString *	   strValue );

	// ---------------------------------------------------------------------------------------------

	static bool
	ConvertToBool ( XMP_StringPtr strValue );

	static XMP_Int32
	ConvertToInt ( XMP_StringPtr strValue );

	static XMP_Int64
	ConvertToInt64 ( XMP_StringPtr strValue );

	static double
	ConvertToFloat ( XMP_StringPtr strValue );

	static void
	ConvertToDate  ( XMP_StringPtr	strValue,
					 XMP_DateTime * binValue );

	// ---------------------------------------------------------------------------------------------

	static void
	CurrentDateTime ( XMP_DateTime * time );

	static void
	SetTimeZone ( XMP_DateTime * time );

	static void
	ConvertToUTCTime ( XMP_DateTime * time );

	static void
	ConvertToLocalTime ( XMP_DateTime * time );

	static int
	CompareDateTime ( const XMP_DateTime & left,
					  const XMP_DateTime & right );
	// ---------------------------------------------------------------------------------------------

	static void
	EncodeToBase64 ( XMP_StringPtr	 rawStr,
					 XMP_StringLen	 rawLen,
					 XMP_VarString * encodedStr );

	static void
	DecodeFromBase64 ( XMP_StringPtr   encodedStr,
					   XMP_StringLen   encodedLen,
					   XMP_VarString * rawStr );

	// ---------------------------------------------------------------------------------------------

	static void
	PackageForJPEG ( const XMPMeta & xmpObj,
					 XMP_VarString * stdStr,
					 XMP_VarString * extStr,
					 XMP_VarString * digestStr );

	static void
	MergeFromJPEG ( XMPMeta *       fullXMP,
                    const XMPMeta & extendedXMP );

	// ---------------------------------------------------------------------------------------------

	static void
	CatenateArrayItems ( const XMPMeta & xmpObj,
						 XMP_StringPtr	 schemaNS,
						 XMP_StringPtr	 arrayName,
						 XMP_StringPtr	 separator,
						 XMP_StringPtr	 quotes,
						 XMP_OptionBits	 options,
						 XMP_VarString * catedStr );

	static void
	SeparateArrayItems ( XMPMeta *		xmpObj,
						 XMP_StringPtr	schemaNS,
						 XMP_StringPtr	arrayName,
						 XMP_OptionBits options,
						 XMP_StringPtr	catedStr );

	static void
	ApplyTemplate ( XMPMeta *	    workingXMP,
					const XMPMeta & templateXMP,
					XMP_OptionBits  actions );

	static void
	RemoveProperties ( XMPMeta *	  xmpObj,
					   XMP_StringPtr  schemaNS,
					   XMP_StringPtr  propName,
					   XMP_OptionBits options );

	static void
	DuplicateSubtree ( const XMPMeta & source,
					   XMPMeta *	   dest,
					   XMP_StringPtr   sourceNS,
					   XMP_StringPtr   sourceRoot,
					   XMP_StringPtr   destNS,
					   XMP_StringPtr   destRoot,
					   XMP_OptionBits  options );

};	// XMPUtils

// =================================================================================================

#endif	// __XMPUtils_hpp__
