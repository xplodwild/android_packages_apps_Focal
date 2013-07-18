// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"
#include "public/include/XMP_Const.h"

#include "public/include/client-glue/WXMPFiles.hpp"

#include "source/XMP_ProgressTracker.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "XMPFiles/source/XMPFiles.hpp"

#if XMP_WinBuild
	#if XMP_DebugBuild
		#pragma warning ( disable : 4297 ) // function assumed not to throw an exception but does
	#endif
#endif

#if __cplusplus
extern "C" {
#endif

// =================================================================================================

static WXMP_Result voidResult;	// Used for functions that  don't use the normal result mechanism.

// =================================================================================================

void WXMPFiles_GetVersionInfo_1 ( XMP_VersionInfo * versionInfo )
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_NoLock ( "WXMPFiles_GetVersionInfo_1" )

		XMPFiles::GetVersionInfo ( versionInfo );

	XMP_EXIT_NoThrow
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_Initialize_1 ( XMP_OptionBits options,
							  WXMP_Result *  wResult )
{
	XMP_ENTER_NoLock ( "WXMPFiles_Initialize_1" )

		wResult->int32Result = XMPFiles::Initialize ( options, NULL, NULL );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------

void WXMPFiles_Initialize_2 ( XMP_OptionBits options,
							  const char *   pluginFolder,
							  const char *   plugins,
							  WXMP_Result *  wResult )
{
	XMP_ENTER_NoLock ( "WXMPFiles_Initialize_1" )

		wResult->int32Result = XMPFiles::Initialize ( options, pluginFolder, plugins );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_Terminate_1()
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_NoLock ( "WXMPFiles_Terminate_1" )

		XMPFiles::Terminate();

	XMP_EXIT_NoThrow
}

// =================================================================================================

void WXMPFiles_CTor_1 ( WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPFiles_CTor_1" )	// No lib object yet, use the static entry.

		XMPFiles * newObj = new XMPFiles();
		++newObj->clientRefs;
		XMP_Assert ( newObj->clientRefs == 1 );
		wResult->ptrResult = newObj;

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_IncrementRefCount_1 ( XMPFilesRef xmpObjRef )
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_IncrementRefCount_1" )

		++thiz->clientRefs;
		XMP_Assert ( thiz->clientRefs > 0 );

	XMP_EXIT_NoThrow
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_DecrementRefCount_1 ( XMPFilesRef xmpObjRef )
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_DecrementRefCount_1" )

		XMP_Assert ( thiz->clientRefs > 0 );
		--thiz->clientRefs;
		if ( thiz->clientRefs <= 0 ) {
			objLock.Release();
			delete ( thiz );
		}

	XMP_EXIT_NoThrow
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_GetFormatInfo_1 ( XMP_FileFormat   format,
                                 XMP_OptionBits * flags,
                                 WXMP_Result *    wResult )
{
	XMP_ENTER_Static ( "WXMPFiles_GetFormatInfo_1" )

		wResult->int32Result = XMPFiles::GetFormatInfo ( format, flags );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_CheckFileFormat_1 ( XMP_StringPtr filePath,
								   WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPFiles_CheckFileFormat_1" )

		wResult->int32Result = XMPFiles::CheckFileFormat ( filePath );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_CheckPackageFormat_1 ( XMP_StringPtr folderPath,
                       				  WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPFiles_CheckPackageFormat_1" )

		wResult->int32Result = XMPFiles::CheckPackageFormat ( folderPath );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_GetFileModDate_1 ( XMP_StringPtr    filePath,
								  XMP_DateTime *   modDate,
								  XMP_FileFormat * format,
								  XMP_OptionBits   options,
								  WXMP_Result *    wResult )
{
	XMP_ENTER_Static ( "WXMPFiles_GetFileModDate_1" )

		wResult->int32Result = XMPFiles::GetFileModDate ( filePath, modDate, format, options );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_GetAssociatedResources_1 ( XMP_StringPtr             filePath,
										  void *                    resourceList,
										  XMP_FileFormat            format, 
										  XMP_OptionBits            options, 
					                      SetClientStringVectorProc SetClientStringVector,
										  WXMP_Result *             wResult )
{
	XMP_ENTER_Static ( "WXMPFiles_GetAssociatedResources_1" )

		if ( resourceList == 0 ) XMP_Throw ( "An result resource list vector must be provided", kXMPErr_BadParam );

		std::vector<std::string> resList;	// Pass a local vector, not the client's.
		(*SetClientStringVector) ( resourceList, 0, 0 );	// Clear the client's result vector.
		wResult->int32Result = XMPFiles::GetAssociatedResources ( filePath, &resList, format, options );

		if ( wResult->int32Result && (! resList.empty()) ) {
			const size_t fileCount = resList.size();
			std::vector<XMP_StringPtr> ptrArray;
			ptrArray.reserve ( fileCount );
			for ( size_t i = 0; i < fileCount; ++i ) ptrArray.push_back ( resList[i].c_str() );
			(*SetClientStringVector) ( resourceList, ptrArray.data(), fileCount );
		}
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_IsMetadataWritable_1 ( XMP_StringPtr    filePath,
							          XMP_Bool *       writable, 
							          XMP_FileFormat   format,
							          XMP_OptionBits   options, 
							          WXMP_Result *    wResult )
{
	XMP_ENTER_Static ( "WXMPFiles_IsMetadataWritable_1" )
		wResult->int32Result = XMPFiles::IsMetadataWritable ( filePath, writable, format, options );
	XMP_EXIT
}


// =================================================================================================

void WXMPFiles_OpenFile_1 ( XMPFilesRef    xmpObjRef,
                            XMP_StringPtr  filePath,
			                XMP_FileFormat format,
			                XMP_OptionBits openFlags,
                            WXMP_Result *  wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_OpenFile_1" )
		StartPerfCheck ( kAPIPerf_OpenFile, filePath );

		bool ok = thiz->OpenFile ( filePath, format, openFlags );
		wResult->int32Result = ok;

		EndPerfCheck ( kAPIPerf_OpenFile );
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

#if XMP_StaticBuild	// ! Client XMP_IO objects can only be used in static builds.
void WXMPFiles_OpenFile_2 ( XMPFilesRef    xmpObjRef,
                            XMP_IO*        clientIO,
			                XMP_FileFormat format,
			                XMP_OptionBits openFlags,
                            WXMP_Result *  wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_OpenFile_2" )
		StartPerfCheck ( kAPIPerf_OpenFile, "<client-managed>" );

		bool ok = thiz->OpenFile ( clientIO, format, openFlags );
		wResult->int32Result = ok;

		EndPerfCheck ( kAPIPerf_OpenFile );
	XMP_EXIT
}
#endif

// -------------------------------------------------------------------------------------------------

void WXMPFiles_CloseFile_1 ( XMPFilesRef    xmpObjRef,
                             XMP_OptionBits closeFlags,
                             WXMP_Result *  wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_CloseFile_1" )
		StartPerfCheck ( kAPIPerf_CloseFile, "" );

		thiz->CloseFile ( closeFlags );

		EndPerfCheck ( kAPIPerf_CloseFile );
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_GetFileInfo_1 ( XMPFilesRef      xmpObjRef,
                               void *           clientPath,
			                   XMP_OptionBits * openFlags,
			                   XMP_FileFormat * format,
			                   XMP_OptionBits * handlerFlags,
			                   SetClientStringProc SetClientString,
                               WXMP_Result *    wResult )
{
	XMP_ENTER_ObjRead ( XMPFiles, "WXMPFiles_GetFileInfo_1" )

		XMP_StringPtr pathStr;
		XMP_StringLen pathLen;

		bool isOpen = thiz.GetFileInfo ( &pathStr, &pathLen, openFlags, format, handlerFlags );
		if ( isOpen && (clientPath != 0) ) (*SetClientString) ( clientPath, pathStr, pathLen );
		wResult->int32Result = isOpen;

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_SetAbortProc_1 ( XMPFilesRef   xmpObjRef,
                         	    XMP_AbortProc abortProc,
							    void *        abortArg,
							    WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_SetAbortProc_1" )

		thiz->SetAbortProc ( abortProc, abortArg );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_GetXMP_1 ( XMPFilesRef      xmpObjRef,
                          XMPMetaRef       xmpRef,
		                  void *           clientPacket,
		                  XMP_PacketInfo * packetInfo,
		                  SetClientStringProc SetClientString,
                          WXMP_Result *    wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_GetXMP_1" )
		StartPerfCheck ( kAPIPerf_GetXMP, "" );

		bool hasXMP = false;
		XMP_StringPtr packetStr;
		XMP_StringLen packetLen;

		if ( xmpRef == 0 ) {
			hasXMP = thiz->GetXMP ( 0, &packetStr, &packetLen, packetInfo );
		} else {
			SXMPMeta xmpObj ( xmpRef );
			hasXMP = thiz->GetXMP ( &xmpObj, &packetStr, &packetLen, packetInfo );
		}

		if ( hasXMP && (clientPacket != 0) ) (*SetClientString) ( clientPacket, packetStr, packetLen );
		wResult->int32Result = hasXMP;

		EndPerfCheck ( kAPIPerf_GetXMP );
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_PutXMP_1 ( XMPFilesRef   xmpObjRef,
                          XMPMetaRef    xmpRef,	// ! Only one of the XMP object or packet are passed.
                          XMP_StringPtr xmpPacket,
                          XMP_StringLen xmpPacketLen,
                          WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_PutXMP_1" )
		StartPerfCheck ( kAPIPerf_PutXMP, "" );

		if ( xmpRef != 0 ) {
			thiz->PutXMP ( xmpRef );
		} else {
			thiz->PutXMP ( xmpPacket, xmpPacketLen );
		}

		EndPerfCheck ( kAPIPerf_PutXMP );
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_CanPutXMP_1 ( XMPFilesRef   xmpObjRef,
                             XMPMetaRef    xmpRef,	// ! Only one of the XMP object or packet are passed.
                             XMP_StringPtr xmpPacket,
                             XMP_StringLen xmpPacketLen,
                             WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_CanPutXMP_1" )
		StartPerfCheck ( kAPIPerf_CanPutXMP, "" );

		if ( xmpRef != 0 ) {
			wResult->int32Result = thiz->CanPutXMP ( xmpRef );
		} else {
			wResult->int32Result = thiz->CanPutXMP ( xmpPacket, xmpPacketLen );
		}

		EndPerfCheck ( kAPIPerf_CanPutXMP );
	XMP_EXIT
}

// =================================================================================================

void WXMPFiles_SetDefaultProgressCallback_1 ( XMP_ProgressReportWrapper wrapperProc,
											  XMP_ProgressReportProc    clientProc,
											  void *        context,
											  float         interval,
											  XMP_Bool      sendStartStop,
											  WXMP_Result * wResult )
{
	XMP_ENTER_Static ( "WXMPFiles_SetDefaultProgressCallback_1" )
	
		XMP_ProgressTracker::CallbackInfo cbInfo ( wrapperProc, clientProc, context, interval, ConvertXMP_BoolToBool( sendStartStop ) );
		XMPFiles::SetDefaultProgressCallback ( cbInfo );
	
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_SetProgressCallback_1 ( XMPFilesRef   xmpObjRef,
									   XMP_ProgressReportWrapper wrapperProc,
									   XMP_ProgressReportProc    clientProc,
									   void *        context,
									   float         interval,
									   XMP_Bool      sendStartStop,
									   WXMP_Result * wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_SetProgressCallback_1" )
	
		XMP_ProgressTracker::CallbackInfo cbInfo ( wrapperProc, clientProc, context, interval, ConvertXMP_BoolToBool( sendStartStop) );
		thiz->SetProgressCallback ( cbInfo );
	
	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_SetDefaultErrorCallback_1 ( XMPFiles_ErrorCallbackWrapper	wrapperProc,
										   XMPFiles_ErrorCallbackProc		clientProc,
										   void *							context,
										   XMP_Uns32						limit,
										   WXMP_Result *					wResult )
{
	XMP_ENTER_Static ( "WXMPFiles_SetDefaultErrorCallback_1" )

		XMPFiles::SetDefaultErrorCallback ( wrapperProc, clientProc, context, limit );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_SetErrorCallback_1 ( XMPFilesRef						xmpObjRef,
									XMPFiles_ErrorCallbackWrapper	wrapperProc,
									XMPFiles_ErrorCallbackProc		clientProc,
									void *							context,
									XMP_Uns32						limit,
									WXMP_Result *					wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_SetErrorCallback_1" )

		thiz->SetErrorCallback ( wrapperProc, clientProc, context, limit );

	XMP_EXIT
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_ResetErrorCallbackLimit_1 ( XMPFilesRef		xmpObjRef,
										   XMP_Uns32		limit,
										   WXMP_Result *	wResult )
{
	XMP_ENTER_ObjWrite ( XMPFiles, "WXMPFiles_ResetErrorCallbackLimit_1" )

		thiz->ResetErrorCallbackLimit ( limit );

	XMP_EXIT
}

// =================================================================================================

#if __cplusplus
}
#endif
