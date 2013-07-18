// =================================================================================================
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include!
#include "public/include/XMP_Const.h"

#include "public/include/client-glue/WXMPIterator.hpp"

#include "XMPCore/source/XMPCore_Impl.hpp"
#include "XMPCore/source/XMPIterator.hpp"

#if XMP_WinBuild
	#pragma warning ( disable : 4101 ) // unreferenced local variable
	#pragma warning ( disable : 4189 ) // local variable is initialized but not referenced
	#pragma warning ( disable : 4800 ) // forcing value to bool 'true' or 'false' (performance warning)
	#if XMP_DebugBuild
		#pragma warning ( disable : 4297 ) // function assumed not to throw an exception but does
	#endif
#endif

#if __cplusplus
extern "C" {
#endif

// =================================================================================================
// CTor/DTor Wrappers
// ==================

void
WXMPIterator_PropCTor_1 ( XMPMetaRef     xmpRef,
                          XMP_StringPtr  schemaNS,
                          XMP_StringPtr  propName,
                          XMP_OptionBits options,
                          WXMP_Result *  wResult )
{
    XMP_ENTER_Static ( "WXMPIterator_PropCTor_1" )	// No lib object yet, use the static entry.

		if ( schemaNS == 0 ) schemaNS = "";
		if ( propName == 0 ) propName = "";

		const XMPMeta & xmpObj = WtoXMPMeta_Ref ( xmpRef );
		XMP_AutoLock metaLock ( &xmpObj.lock, kXMP_ReadLock );
		
		XMPIterator * iter = new XMPIterator ( xmpObj, schemaNS, propName, options );
		++iter->clientRefs;
		XMP_Assert ( iter->clientRefs == 1 );
		wResult->ptrResult = XMPIteratorRef ( iter );

    XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPIterator_TableCTor_1 ( XMP_StringPtr  schemaNS,
                           XMP_StringPtr  propName,
                           XMP_OptionBits options,
                           WXMP_Result *  wResult )
{
    XMP_ENTER_Static ( "WXMPIterator_TableCTor_1" )	// No lib object yet, use the static entry.

		if ( schemaNS == 0 ) schemaNS = "";
		if ( propName == 0 ) propName = "";

		XMPIterator * iter = new XMPIterator ( schemaNS, propName, options );
		++iter->clientRefs;
		XMP_Assert ( iter->clientRefs == 1 );
		wResult->ptrResult = XMPIteratorRef ( iter );

    XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPIterator_IncrementRefCount_1 ( XMPIteratorRef xmpObjRef )
{
	WXMP_Result * wResult = &void_wResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_ObjWrite ( XMPIterator, "WXMPIterator_IncrementRefCount_1" )

		++thiz->clientRefs;
		XMP_Assert ( thiz->clientRefs > 1 );

	XMP_EXIT_NoThrow
}

// -------------------------------------------------------------------------------------------------

void
WXMPIterator_DecrementRefCount_1 ( XMPIteratorRef xmpObjRef )
{
	WXMP_Result * wResult = &void_wResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_ObjWrite ( XMPIterator, "WXMPIterator_DecrementRefCount_1" )

		XMP_Assert ( thiz->clientRefs > 0 );
		--thiz->clientRefs;
		if ( thiz->clientRefs <= 0 ) {
			objLock.Release();
			delete ( thiz );
		}

	XMP_EXIT_NoThrow
}

// =================================================================================================
// Class Method Wrappers
// =====================

void
WXMPIterator_Next_1 ( XMPIteratorRef   xmpObjRef,
                      void *           schemaNS,
                      void *           propPath,
                      void *           propValue,
                      XMP_OptionBits * propOptions,
                      SetClientStringProc SetClientString,
                      WXMP_Result *    wResult )
{
    XMP_ENTER_ObjWrite ( XMPIterator, "WXMPIterator_Next_1" )

		XMP_StringPtr schemaPtr = 0;
		XMP_StringLen schemaLen = 0;
		XMP_StringPtr pathPtr = 0;
		XMP_StringLen pathLen = 0;
		XMP_StringPtr valuePtr = 0;
		XMP_StringLen valueLen = 0;
		
		if ( propOptions == 0 ) propOptions = &voidOptionBits;

		XMP_Assert( thiz->info.xmpObj != NULL );
		XMP_AutoLock metaLock ( &thiz->info.xmpObj->lock, kXMP_ReadLock, (thiz->info.xmpObj != 0) );

		XMP_Bool found = thiz->Next ( &schemaPtr, &schemaLen, &pathPtr, &pathLen, &valuePtr, &valueLen, propOptions );
		wResult->int32Result = found;
		
		if ( found ) {
			if ( schemaNS != 0 ) (*SetClientString) ( schemaNS, schemaPtr, schemaLen );
			if ( propPath != 0 ) (*SetClientString) ( propPath, pathPtr, pathLen );
			if ( propValue != 0 ) (*SetClientString) ( propValue, valuePtr, valueLen );
		}

    XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void
WXMPIterator_Skip_1 ( XMPIteratorRef xmpObjRef,
                      XMP_OptionBits options,
                      WXMP_Result *  wResult )
{
    XMP_ENTER_ObjWrite ( XMPIterator, "WXMPIterator_Skip_1" )

		XMP_Assert( thiz->info.xmpObj != NULL );
		XMP_AutoLock metaLock ( &thiz->info.xmpObj->lock, kXMP_ReadLock, (thiz->info.xmpObj != 0) );

		thiz->Skip ( options );

    XMP_EXIT
}

// =================================================================================================

#if __cplusplus
} /* extern "C" */
#endif
