// =================================================================================================
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:	Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include!
#include "public/include/XMP_Const.h"

#include "public/include/client-glue/WXMPMeta.hpp"

#include "XMPCore/source/XMPCore_Impl.hpp"
#include "XMPCore/source/XMPMeta.hpp"

#if XMP_WinBuild
	#pragma warning ( disable : 4101 ) // unreferenced local variable
	#pragma warning ( disable : 4189 ) // local variable is initialized but not referenced
	#pragma warning ( disable : 4702 ) // unreachable code
	#pragma warning ( disable : 4800 ) // forcing value to bool 'true' or 'false' (performance warning)
	#if XMP_DebugBuild
		#pragma warning ( disable : 4297 ) // function assumed not to throw an exception but does
	#endif
#endif

#if __cplusplus
extern "C" {
#endif

// =================================================================================================
// Init/Term Wrappers
// ==================

/* class static */ void
WXMPMeta_GetVersionInfo_1 ( XMP_VersionInfo * info )
{
	WXMP_Result * wResult = &void_wResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_NoLock ( "WXMPMeta_GetVersionInfo_1" )

		XMPMeta::GetVersionInfo ( info );

	XMP_EXIT_NoThrow
}

// -------------------------------------------------------------------------------------------------

/* class static */ void
WXMPMeta_Initialize_1 ( WXMP_Result * wResult )
{
	XMP_ENTER_NoLock ( "WXMPMeta_Initialize_1" )

		wResult->int32Result = XMPMeta::Initialize();

	XMP_EXIT
}
// -------------------------------------------------------------------------------------------------

/* class static */ void
WXMPMeta_Terminate_1()
{
	WXMP_Result * wResult = &void_wResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_NoLock ( "WXMPMeta_Terminate_1" )

		XMPMeta::Terminate();

	XMP_EXIT_NoThrow
}

// =================================================================================================
// CTor/DTor Wrappers
// ==================

void
WXMPMeta_CTor_1 ( WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPMeta_CTor_1" )	// No lib object yet, use the static entry.

		XMPMeta * xmpObj = new XMPMeta();
		++xmpObj->clientRefs;
		XMP_Assert ( xmpObj->clientRefs == 1 );
		wResult->ptrResult = XMPMetaRef ( xmpObj );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_IncrementRefCount_1 ( XMPMetaRef xmpObjRef )
{
	WXMP_Result * wResult = &void_wResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_IncrementRefCount_1" )

		++thiz->clientRefs;
		XMP_Assert ( thiz->clientRefs > 0 );

	XMP_EXIT_NoThrow
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DecrementRefCount_1 ( XMPMetaRef xmpObjRef )
{
	WXMP_Result * wResult = &void_wResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_DecrementRefCount_1" )

		XMP_Assert ( thiz->clientRefs > 0 );
		--thiz->clientRefs;
		if ( thiz->clientRefs <= 0 ) {
			objLock.Release();
			delete ( thiz );
		}

	XMP_EXIT_NoThrow
}

// =================================================================================================
// Class Static Wrappers
// =====================

/* class static */ void
WXMPMeta_GetGlobalOptions_1 ( WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPMeta_GetGlobalOptions_1" )

		XMP_OptionBits options = XMPMeta::GetGlobalOptions();
		wResult->int32Result = options;

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

/* class static */ void
WXMPMeta_SetGlobalOptions_1 ( XMP_OptionBits options,
							  WXMP_Result *	 wResult )
{
	XMP_ENTER_Static ( "WXMPMeta_SetGlobalOptions_1" )

		XMPMeta::SetGlobalOptions ( options );

	XMP_EXIT
}
// -------------------------------------------------------------------------------------------------

/* class static */ void
WXMPMeta_DumpNamespaces_1 ( XMP_TextOutputProc outProc,
							void *			   refCon,
							WXMP_Result *	   wResult )
{
	XMP_ENTER_Static ( "WXMPMeta_DumpNamespaces_1" )

		if ( outProc == 0 ) XMP_Throw ( "Null client output routine", kXMPErr_BadParam );
		
		XMP_Status status = XMPMeta::DumpNamespaces ( outProc, refCon );
		wResult->int32Result = status;

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

/* class static */ void
WXMPMeta_RegisterNamespace_1 ( XMP_StringPtr namespaceURI,
							   XMP_StringPtr suggestedPrefix,
							   void *        actualPrefix,
							   SetClientStringProc SetClientString,
							   WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPMeta_RegisterNamespace_1" )

		if ( (namespaceURI == 0) || (*namespaceURI == 0) ) XMP_Throw ( "Empty namespace URI", kXMPErr_BadSchema );
		if ( (suggestedPrefix == 0) || (*suggestedPrefix == 0) ) XMP_Throw ( "Empty suggested prefix", kXMPErr_BadSchema );

		XMP_StringPtr prefixPtr = 0;
		XMP_StringLen prefixSize = 0;

		bool prefixMatch = XMPMeta::RegisterNamespace ( namespaceURI, suggestedPrefix, &prefixPtr, &prefixSize );
		wResult->int32Result = prefixMatch;
		
		if ( actualPrefix != 0 ) (*SetClientString) ( actualPrefix, prefixPtr, prefixSize );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

/* class static */ void
WXMPMeta_GetNamespacePrefix_1 ( XMP_StringPtr namespaceURI,
								void *        namespacePrefix,
								SetClientStringProc SetClientString,
								WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPMeta_GetNamespacePrefix_1" )

		if ( (namespaceURI == 0) || (*namespaceURI == 0) ) XMP_Throw ( "Empty namespace URI", kXMPErr_BadSchema );

		XMP_StringPtr prefixPtr = 0;
		XMP_StringLen prefixSize = 0;

		bool found = XMPMeta::GetNamespacePrefix ( namespaceURI, &prefixPtr, &prefixSize );
		wResult->int32Result = found;
		
		if ( found && (namespacePrefix != 0) ) (*SetClientString) ( namespacePrefix, prefixPtr, prefixSize );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

/* class static */ void
WXMPMeta_GetNamespaceURI_1 ( XMP_StringPtr namespacePrefix,
							 void *        namespaceURI,
							 SetClientStringProc SetClientString,
							 WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPMeta_GetNamespaceURI_1" )

		if ( (namespacePrefix == 0) || (*namespacePrefix == 0) ) XMP_Throw ( "Empty namespace prefix", kXMPErr_BadSchema );

		XMP_StringPtr uriPtr = 0;
		XMP_StringLen uriSize = 0;
	   
		bool found = XMPMeta::GetNamespaceURI ( namespacePrefix, &uriPtr, &uriSize );
		wResult->int32Result = found;
		
		if ( found && (namespaceURI != 0) ) (*SetClientString) ( namespaceURI, uriPtr, uriSize );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

/* class static */ void
WXMPMeta_DeleteNamespace_1 ( XMP_StringPtr namespaceURI,
							 WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPMeta_DeleteNamespace_1" )

		if ( (namespaceURI == 0) || (*namespaceURI == 0) ) XMP_Throw ( "Empty namespace URI", kXMPErr_BadSchema );

		XMPMeta::DeleteNamespace ( namespaceURI );

	XMP_EXIT
}

// =================================================================================================
// Class Method Wrappers
// =====================

void
WXMPMeta_GetProperty_1 ( XMPMetaRef		  xmpObjRef,
						 XMP_StringPtr	  schemaNS,
						 XMP_StringPtr	  propName,
						 void *           propValue,
						 XMP_OptionBits * options,
						 SetClientStringProc SetClientString,
						 WXMP_Result *	  wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetProperty_1" )
	
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );
		
		XMP_StringPtr valuePtr = 0;
		XMP_StringLen valueSize = 0;
		if ( options == 0 ) options = &voidOptionBits;

		bool found = thiz.GetProperty ( schemaNS, propName, &valuePtr, &valueSize, options );
		wResult->int32Result = found;
		
		if ( found && (propValue != 0) ) (*SetClientString) ( propValue, valuePtr, valueSize );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetArrayItem_1 ( XMPMetaRef	   xmpObjRef,
						  XMP_StringPtr	   schemaNS,
						  XMP_StringPtr	   arrayName,
						  XMP_Index		   itemIndex,
						  void *           itemValue,
						  XMP_OptionBits * options,
						  SetClientStringProc SetClientString,
						  WXMP_Result *	   wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetArrayItem_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (arrayName == 0) || (*arrayName == 0) ) XMP_Throw ( "Empty array name", kXMPErr_BadXPath );
		
		XMP_StringPtr valuePtr = 0;
		XMP_StringLen valueSize = 0;
		if ( options == 0 ) options = &voidOptionBits;

		bool found = thiz.GetArrayItem ( schemaNS, arrayName, itemIndex, &valuePtr, &valueSize, options );
		wResult->int32Result = found;
		
		if ( found && (itemValue != 0) ) (*SetClientString) ( itemValue, valuePtr, valueSize );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetStructField_1 ( XMPMetaRef		 xmpObjRef,
							XMP_StringPtr	 schemaNS,
							XMP_StringPtr	 structName,
							XMP_StringPtr	 fieldNS,
							XMP_StringPtr	 fieldName,
							void *	         fieldValue,
							XMP_OptionBits * options,
							SetClientStringProc SetClientString,
							WXMP_Result *	 wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetStructField_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (structName == 0) || (*structName == 0) ) XMP_Throw ( "Empty struct name", kXMPErr_BadXPath );
		if ( (fieldNS == 0) || (*fieldNS == 0) ) XMP_Throw ( "Empty field namespace URI", kXMPErr_BadSchema );
		if ( (fieldName == 0) || (*fieldName == 0) ) XMP_Throw ( "Empty field name", kXMPErr_BadXPath );
		
		XMP_StringPtr valuePtr = 0;
		XMP_StringLen valueSize = 0;
		if ( options == 0 ) options = &voidOptionBits;

		bool found = thiz.GetStructField ( schemaNS, structName, fieldNS, fieldName, &valuePtr, &valueSize, options );
		wResult->int32Result = found;
		
		if ( found && (fieldValue != 0) ) (*SetClientString) ( fieldValue, valuePtr, valueSize );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetQualifier_1 ( XMPMetaRef	   xmpObjRef,
						  XMP_StringPtr	   schemaNS,
						  XMP_StringPtr	   propName,
						  XMP_StringPtr	   qualNS,
						  XMP_StringPtr	   qualName,
						  void *           qualValue,
						  XMP_OptionBits * options,
						  SetClientStringProc SetClientString,
						  WXMP_Result *	   wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetQualifier_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );
		if ( (qualNS == 0) || (*qualNS == 0) ) XMP_Throw ( "Empty qualifier namespace URI", kXMPErr_BadSchema );
		if ( (qualName == 0) || (*qualName == 0) ) XMP_Throw ( "Empty qualifier name", kXMPErr_BadXPath );
		
		XMP_StringPtr valuePtr = 0;
		XMP_StringLen valueSize = 0;
		if ( options == 0 ) options = &voidOptionBits;

		bool found = thiz.GetQualifier ( schemaNS, propName, qualNS, qualName, &valuePtr, &valueSize, options );
		wResult->int32Result = found;
		
		if ( found && (qualValue != 0) ) (*SetClientString) ( qualValue, valuePtr, valueSize );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetProperty_1 ( XMPMetaRef		xmpObjRef,
						 XMP_StringPtr	schemaNS,
						 XMP_StringPtr	propName,
						 XMP_StringPtr	propValue,
						 XMP_OptionBits options,
						 WXMP_Result *	wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetProperty_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		thiz->SetProperty ( schemaNS, propName, propValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetArrayItem_1 ( XMPMetaRef	 xmpObjRef,
						  XMP_StringPtr	 schemaNS,
						  XMP_StringPtr	 arrayName,
						  XMP_Index		 itemIndex,
						  XMP_StringPtr	 itemValue,
						  XMP_OptionBits options,
						  WXMP_Result *	 wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetArrayItem_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (arrayName == 0) || (*arrayName == 0) ) XMP_Throw ( "Empty array name", kXMPErr_BadXPath );

		thiz->SetArrayItem ( schemaNS, arrayName, itemIndex, itemValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_AppendArrayItem_1 ( XMPMetaRef		xmpObjRef,
							 XMP_StringPtr	schemaNS,
							 XMP_StringPtr	arrayName,
							 XMP_OptionBits arrayOptions,
							 XMP_StringPtr	itemValue,
							 XMP_OptionBits options,
							 WXMP_Result *	wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_AppendArrayItem_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (arrayName == 0) || (*arrayName == 0) ) XMP_Throw ( "Empty array name", kXMPErr_BadXPath );

		thiz->AppendArrayItem ( schemaNS, arrayName, arrayOptions, itemValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetStructField_1 ( XMPMetaRef	   xmpObjRef,
							XMP_StringPtr  schemaNS,
							XMP_StringPtr  structName,
							XMP_StringPtr  fieldNS,
							XMP_StringPtr  fieldName,
							XMP_StringPtr  fieldValue,
							XMP_OptionBits options,
							WXMP_Result *  wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetStructField_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (structName == 0) || (*structName == 0) ) XMP_Throw ( "Empty struct name", kXMPErr_BadXPath );
		if ( (fieldNS == 0) || (*fieldNS == 0) ) XMP_Throw ( "Empty field namespace URI", kXMPErr_BadSchema );
		if ( (fieldName == 0) || (*fieldName == 0) ) XMP_Throw ( "Empty field name", kXMPErr_BadXPath );

		thiz->SetStructField ( schemaNS, structName, fieldNS, fieldName, fieldValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetQualifier_1 ( XMPMetaRef	 xmpObjRef,
						  XMP_StringPtr	 schemaNS,
						  XMP_StringPtr	 propName,
						  XMP_StringPtr	 qualNS,
						  XMP_StringPtr	 qualName,
						  XMP_StringPtr	 qualValue,
						  XMP_OptionBits options,
						  WXMP_Result *	 wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetQualifier_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );
		if ( (qualNS == 0) || (*qualNS == 0) ) XMP_Throw ( "Empty qualifier namespace URI", kXMPErr_BadSchema );
		if ( (qualName == 0) || (*qualName == 0) ) XMP_Throw ( "Empty qualifier name", kXMPErr_BadXPath );

		thiz->SetQualifier ( schemaNS, propName, qualNS, qualName, qualValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DeleteProperty_1 ( XMPMetaRef	  xmpObjRef,
							XMP_StringPtr schemaNS,
							XMP_StringPtr propName,
							WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_DeleteProperty_1" )
 
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		thiz->DeleteProperty ( schemaNS, propName );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DeleteArrayItem_1 ( XMPMetaRef	   xmpObjRef,
							 XMP_StringPtr schemaNS,
							 XMP_StringPtr arrayName,
							 XMP_Index	   itemIndex,
							 WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_DeleteArrayItem_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (arrayName == 0) || (*arrayName == 0) ) XMP_Throw ( "Empty array name", kXMPErr_BadXPath );

		thiz->DeleteArrayItem ( schemaNS, arrayName, itemIndex );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DeleteStructField_1 ( XMPMetaRef	 xmpObjRef,
							   XMP_StringPtr schemaNS,
							   XMP_StringPtr structName,
							   XMP_StringPtr fieldNS,
							   XMP_StringPtr fieldName,
							   WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_DeleteStructField_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (structName == 0) || (*structName == 0) ) XMP_Throw ( "Empty struct name", kXMPErr_BadXPath );
		if ( (fieldNS == 0) || (*fieldNS == 0) ) XMP_Throw ( "Empty field namespace URI", kXMPErr_BadSchema );
		if ( (fieldName == 0) || (*fieldName == 0) ) XMP_Throw ( "Empty field name", kXMPErr_BadXPath );

		thiz->DeleteStructField ( schemaNS, structName, fieldNS, fieldName );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DeleteQualifier_1 ( XMPMetaRef	   xmpObjRef,
							 XMP_StringPtr schemaNS,
							 XMP_StringPtr propName,
							 XMP_StringPtr qualNS,
							 XMP_StringPtr qualName,
							 WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_DeleteQualifier_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );
		if ( (qualNS == 0) || (*qualNS == 0) ) XMP_Throw ( "Empty qualifier namespace URI", kXMPErr_BadSchema );
		if ( (qualName == 0) || (*qualName == 0) ) XMP_Throw ( "Empty qualifier name", kXMPErr_BadXPath );

		thiz->DeleteQualifier ( schemaNS, propName, qualNS, qualName );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DoesPropertyExist_1 ( XMPMetaRef	 xmpObjRef,
							   XMP_StringPtr schemaNS,
							   XMP_StringPtr propName,
							   WXMP_Result * wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_DoesPropertyExist_1" )
	
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		bool found = thiz.DoesPropertyExist ( schemaNS, propName );
		wResult->int32Result = found;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DoesArrayItemExist_1 ( XMPMetaRef	  xmpObjRef,
								XMP_StringPtr schemaNS,
								XMP_StringPtr arrayName,
								XMP_Index	  itemIndex,
								WXMP_Result * wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_DoesArrayItemExist_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (arrayName == 0) || (*arrayName == 0) ) XMP_Throw ( "Empty array name", kXMPErr_BadXPath );

		bool found = thiz.DoesArrayItemExist ( schemaNS, arrayName, itemIndex );
		wResult->int32Result = found;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DoesStructFieldExist_1 ( XMPMetaRef	xmpObjRef,
								  XMP_StringPtr schemaNS,
								  XMP_StringPtr structName,
								  XMP_StringPtr fieldNS,
								  XMP_StringPtr fieldName,
								  WXMP_Result * wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_DoesStructFieldExist_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (structName == 0) || (*structName == 0) ) XMP_Throw ( "Empty struct name", kXMPErr_BadXPath );
		if ( (fieldNS == 0) || (*fieldNS == 0) ) XMP_Throw ( "Empty field namespace URI", kXMPErr_BadSchema );
		if ( (fieldName == 0) || (*fieldName == 0) ) XMP_Throw ( "Empty field name", kXMPErr_BadXPath );

		bool found = thiz.DoesStructFieldExist ( schemaNS, structName, fieldNS, fieldName );
		wResult->int32Result = found;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DoesQualifierExist_1 ( XMPMetaRef	  xmpObjRef,
								XMP_StringPtr schemaNS,
								XMP_StringPtr propName,
								XMP_StringPtr qualNS,
								XMP_StringPtr qualName,
								WXMP_Result * wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_DoesQualifierExist_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );
		if ( (qualNS == 0) || (*qualNS == 0) ) XMP_Throw ( "Empty qualifier namespace URI", kXMPErr_BadSchema );
		if ( (qualName == 0) || (*qualName == 0) ) XMP_Throw ( "Empty qualifier name", kXMPErr_BadXPath );

		bool found = thiz.DoesQualifierExist ( schemaNS, propName, qualNS, qualName );
		wResult->int32Result = found;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetLocalizedText_1 ( XMPMetaRef	   xmpObjRef,
							  XMP_StringPtr	   schemaNS,
							  XMP_StringPtr	   arrayName,
							  XMP_StringPtr	   genericLang,
							  XMP_StringPtr	   specificLang,
							  void *           actualLang,
							  void *           itemValue,
							  XMP_OptionBits * options,
							  SetClientStringProc SetClientString,
							  WXMP_Result *	   wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetLocalizedText_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (arrayName == 0) || (*arrayName == 0) ) XMP_Throw ( "Empty array name", kXMPErr_BadXPath );
		if ( genericLang == 0 ) genericLang = "";
		if ( (specificLang == 0) ||(*specificLang == 0) ) XMP_Throw ( "Empty specific language", kXMPErr_BadParam );
		
		XMP_StringPtr langPtr = 0;
		XMP_StringLen langSize = 0;
		XMP_StringPtr valuePtr = 0;
		XMP_StringLen valueSize = 0;
		if ( options == 0 ) options = &voidOptionBits;

		bool found = thiz.GetLocalizedText ( schemaNS, arrayName, genericLang, specificLang,
											 &langPtr, &langSize, &valuePtr, &valueSize, options );
		wResult->int32Result = found;
		
		if ( found ) {
			if ( actualLang != 0 ) (*SetClientString) ( actualLang, langPtr, langSize );
			if ( itemValue != 0 ) (*SetClientString) ( itemValue, valuePtr, valueSize );
		}

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetLocalizedText_1 ( XMPMetaRef	 xmpObjRef,
							  XMP_StringPtr	 schemaNS,
							  XMP_StringPtr	 arrayName,
							  XMP_StringPtr	 genericLang,
							  XMP_StringPtr	 specificLang,
							  XMP_StringPtr	 itemValue,
							  XMP_OptionBits options,
							  WXMP_Result *	 wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetLocalizedText_1" )

		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (arrayName == 0) || (*arrayName == 0) ) XMP_Throw ( "Empty array name", kXMPErr_BadXPath );
		if ( genericLang == 0 ) genericLang = "";
		if ( (specificLang == 0) ||(*specificLang == 0) ) XMP_Throw ( "Empty specific language", kXMPErr_BadParam );
		if ( itemValue == 0 ) itemValue = "";

		thiz->SetLocalizedText ( schemaNS, arrayName, genericLang, specificLang, itemValue, options );
		
	XMP_EXIT
}

void
WXMPMeta_DeleteLocalizedText_1 ( XMPMetaRef	   xmpObjRef,
                                 XMP_StringPtr schemaNS,
                                 XMP_StringPtr arrayName,
                                 XMP_StringPtr genericLang,
                                 XMP_StringPtr specificLang,
                                 WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_DeleteLocalizedText_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (arrayName == 0) || (*arrayName == 0) ) XMP_Throw ( "Empty array name", kXMPErr_BadXPath );
		if ( genericLang == 0 ) genericLang = "";
		if ( (specificLang == 0) ||(*specificLang == 0) ) XMP_Throw ( "Empty specific language", kXMPErr_BadParam );
		
		thiz->DeleteLocalizedText ( schemaNS, arrayName, genericLang, specificLang );

	XMP_EXIT
}


// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetProperty_Bool_1 ( XMPMetaRef	   xmpObjRef,
							  XMP_StringPtr	   schemaNS,
							  XMP_StringPtr	   propName,
							  XMP_Bool *	   propValue,
							  XMP_OptionBits * options,
							  WXMP_Result *	   wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetProperty_Bool_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		if ( propValue == 0 ) propValue = &voidByte;
		if ( options == 0 ) options = &voidOptionBits;

		bool value;
		bool found = thiz.GetProperty_Bool ( schemaNS, propName, &value, options );
		if ( propValue != 0 ) *propValue = value;
		wResult->int32Result = found;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetProperty_Int_1 ( XMPMetaRef		  xmpObjRef,
							 XMP_StringPtr	  schemaNS,
							 XMP_StringPtr	  propName,
							 XMP_Int32 *	  propValue,
							 XMP_OptionBits * options,
							 WXMP_Result *	  wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetProperty_Int_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		if ( propValue == 0 ) propValue = &voidInt32;
		if ( options == 0 ) options = &voidOptionBits;

		bool found = thiz.GetProperty_Int ( schemaNS, propName, propValue, options );
		wResult->int32Result = found;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetProperty_Int64_1 ( XMPMetaRef		xmpObjRef,
							   XMP_StringPtr	schemaNS,
							   XMP_StringPtr	propName,
							   XMP_Int64 *	    propValue,
							   XMP_OptionBits * options,
							   WXMP_Result *	wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetProperty_Int64_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		if ( propValue == 0 ) propValue = &voidInt64;
		if ( options == 0 ) options = &voidOptionBits;

		bool found = thiz.GetProperty_Int64 ( schemaNS, propName, propValue, options );
		wResult->int32Result = found;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetProperty_Float_1 ( XMPMetaRef		xmpObjRef,
							   XMP_StringPtr	schemaNS,
							   XMP_StringPtr	propName,
							   double *			propValue,
							   XMP_OptionBits * options,
							   WXMP_Result *	wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetProperty_Float_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		if ( propValue == 0 ) propValue = &voidDouble;
		if ( options == 0 ) options = &voidOptionBits;

		bool found = thiz.GetProperty_Float ( schemaNS, propName, propValue, options );
		wResult->int32Result = found;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetProperty_Date_1 ( XMPMetaRef	   xmpObjRef,
							  XMP_StringPtr	   schemaNS,
							  XMP_StringPtr	   propName,
							  XMP_DateTime *   propValue,
							  XMP_OptionBits * options,
							  WXMP_Result *	   wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetProperty_Date_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		if ( propValue == 0 ) propValue = &voidDateTime;
		if ( options == 0 ) options = &voidOptionBits;

		bool found = thiz.GetProperty_Date ( schemaNS, propName, propValue, options );
		wResult->int32Result = found;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetProperty_Bool_1 ( XMPMetaRef	 xmpObjRef,
							  XMP_StringPtr	 schemaNS,
							  XMP_StringPtr	 propName,
							  XMP_Bool		 propValue,
							  XMP_OptionBits options,
							  WXMP_Result *	 wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetProperty_Bool_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		thiz->SetProperty_Bool ( schemaNS, propName, propValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetProperty_Int_1 ( XMPMetaRef		xmpObjRef,
							 XMP_StringPtr	schemaNS,
							 XMP_StringPtr	propName,
							 XMP_Int32		propValue,
							 XMP_OptionBits options,
							 WXMP_Result *	wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetProperty_Int_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		thiz->SetProperty_Int ( schemaNS, propName, propValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetProperty_Int64_1 ( XMPMetaRef	  xmpObjRef,
							   XMP_StringPtr  schemaNS,
							   XMP_StringPtr  propName,
							   XMP_Int64	  propValue,
							   XMP_OptionBits options,
							   WXMP_Result *  wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetProperty_Int64_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		thiz->SetProperty_Int64 ( schemaNS, propName, propValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetProperty_Float_1 ( XMPMetaRef	  xmpObjRef,
							   XMP_StringPtr  schemaNS,
							   XMP_StringPtr  propName,
							   double		  propValue,
							   XMP_OptionBits options,
							   WXMP_Result *  wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetProperty_Float_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		thiz->SetProperty_Float ( schemaNS, propName, propValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetProperty_Date_1 ( XMPMetaRef		   xmpObjRef,
							  XMP_StringPtr		   schemaNS,
							  XMP_StringPtr		   propName,
							  const XMP_DateTime & propValue,
							  XMP_OptionBits	   options,
							  WXMP_Result *		   wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetProperty_Date_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (propName == 0) || (*propName == 0) ) XMP_Throw ( "Empty property name", kXMPErr_BadXPath );

		thiz->SetProperty_Date ( schemaNS, propName, propValue, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_DumpObject_1 ( XMPMetaRef		   xmpObjRef,
						XMP_TextOutputProc outProc,
						void *			   refCon,
						WXMP_Result *	   wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_DumpObject_1" )

		if ( outProc == 0 ) XMP_Throw ( "Null client output routine", kXMPErr_BadParam );
		
		thiz.DumpObject ( outProc, refCon );
		wResult->int32Result = 0;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_Sort_1 ( XMPMetaRef	xmpObjRef,
				  WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_Sort_1" )

		thiz->Sort();
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_Erase_1 ( XMPMetaRef	 xmpObjRef,
				   WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_Erase_1" )

		thiz->Erase();
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_Clone_1 ( XMPMetaRef	  xmpObjRef,
				   XMP_OptionBits options,
				   WXMP_Result *  wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_Clone_1" )

		XMPMeta * xClone = new XMPMeta;	// ! Don't need an output lock, final ref assignment in client glue.
		thiz.Clone ( xClone, options );
		XMP_Assert ( xClone->clientRefs == 0 );	// ! Gets incremented in TXMPMeta::Clone.
		wResult->ptrResult = xClone;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_CountArrayItems_1 ( XMPMetaRef	   xmpObjRef,
							 XMP_StringPtr schemaNS,
							 XMP_StringPtr arrayName,
							 WXMP_Result * wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_CountArrayItems_1" )
		
		if ( (schemaNS == 0) || (*schemaNS == 0) ) XMP_Throw ( "Empty schema namespace URI", kXMPErr_BadSchema );
		if ( (arrayName == 0) || (*arrayName == 0) ) XMP_Throw ( "Empty array name", kXMPErr_BadXPath );

		XMP_Index count = thiz.CountArrayItems ( schemaNS, arrayName );
		wResult->int32Result = count;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetObjectName_1 ( XMPMetaRef	 xmpObjRef,
						   void *        objName,
						   SetClientStringProc SetClientString,
						   WXMP_Result * wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetObjectName_1" )

		XMP_StringPtr namePtr = 0;
	    XMP_StringLen nameSize = 0;

		thiz.GetObjectName ( &namePtr, &nameSize );
		if ( objName != 0 ) (*SetClientString) ( objName, namePtr, nameSize );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetObjectName_1 ( XMPMetaRef	 xmpObjRef,
						   XMP_StringPtr name,
						   WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetObjectName_1" )

		if ( name == 0 ) name = "";

		thiz->SetObjectName ( name );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_GetObjectOptions_1 ( XMPMetaRef    xmpObjRef,
							  WXMP_Result * wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_GetObjectOptions_1" )

		XMP_OptionBits options = thiz.GetObjectOptions();
		wResult->int32Result = options;
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetObjectOptions_1 ( XMPMetaRef	 xmpObjRef,
							  XMP_OptionBits options,
							  WXMP_Result *	 wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetObjectOptions_1" )
	
		thiz->SetObjectOptions ( options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_ParseFromBuffer_1 ( XMPMetaRef		xmpObjRef,
							 XMP_StringPtr	buffer,
							 XMP_StringLen	bufferSize,
							 XMP_OptionBits options,
							 WXMP_Result *	wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_ParseFromBuffer_1" )

		thiz->ParseFromBuffer ( buffer, bufferSize, options );
		
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SerializeToBuffer_1 ( XMPMetaRef	  xmpObjRef,
							   void *         pktString,
							   XMP_OptionBits options,
							   XMP_StringLen  padding,
							   XMP_StringPtr  newline,
							   XMP_StringPtr  indent,
							   XMP_Index	  baseIndent,
							   SetClientStringProc SetClientString,
							   WXMP_Result *  wResult ) /* const */
{
	XMP_ENTER_ObjRead ( XMPMeta, "WXMPMeta_SerializeToBuffer_1" )

		XMP_VarString localStr;
		
		if ( newline == 0 ) newline = "";
		if ( indent == 0 ) indent = "";
		
		thiz.SerializeToBuffer ( &localStr, options, padding, newline, indent, baseIndent );
		if ( pktString != 0 ) (*SetClientString) ( pktString, localStr.c_str(), localStr.size() );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetDefaultErrorCallback_1 ( XMPMeta_ErrorCallbackWrapper wrapperProc,
									 XMPMeta_ErrorCallbackProc    clientProc,
									 void *        context,
									 XMP_Uns32     limit,
									 WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPMeta_SetDefaultErrorCallback_1" )

		XMPMeta::SetDefaultErrorCallback ( wrapperProc, clientProc, context, limit );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_SetErrorCallback_1 ( XMPMetaRef    xmpObjRef,
                              XMPMeta_ErrorCallbackWrapper wrapperProc,
							  XMPMeta_ErrorCallbackProc    clientProc,
							  void *        context,
							  XMP_Uns32     limit,
							  WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_SetErrorCallback_1" )

		thiz->SetErrorCallback ( wrapperProc, clientProc, context, limit );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPMeta_ResetErrorCallbackLimit_1 ( XMPMetaRef    xmpObjRef,
                          			 XMP_Uns32     limit,
									 WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPMeta, "WXMPMeta_ResetErrorCallbackLimit_1" )

		thiz->ResetErrorCallbackLimit ( limit );

	XMP_EXIT
}

// =================================================================================================

#if __cplusplus
} /* extern "C" */
#endif
