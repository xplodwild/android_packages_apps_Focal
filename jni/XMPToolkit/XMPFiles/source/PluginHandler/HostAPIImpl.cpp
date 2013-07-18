// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "HostAPI.h"
#include "PluginManager.h"
#include "FileHandlerInstance.h"
#include "source/XIO.hpp"
#include "XMPFiles/source/HandlerRegistry.h"

using namespace Common;

namespace XMP_PLUGIN
{

///////////////////////////////////////////////////////////////////////////////
//
//		Exception handler
//

static void HandleException( WXMP_Error* wError ) 
{
	try
	{
		throw;
	}
	catch(  XMP_Error& xmpErr )
	{
		wError->mErrorMsg = xmpErr.GetErrMsg();
		wError->mErrorID = xmpErr.GetID();
	}
	catch( std::exception& stdErr )
	{
		wError->mErrorMsg = stdErr.what();
	}
	catch( ... )
	{
		wError->mErrorMsg = "Caught unknown exception";
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//		FileIO_API
//

static XMPErrorID FileSysRead( XMP_IORef io, void* buffer, XMP_Uns32 count, XMP_Bool readAll, XMP_Uns32& byteRead, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try
	{
		if( io != NULL )
		{
			::XMP_IO * thiz = (::XMP_IO*)io;
			byteRead = thiz->Read( buffer, count, ConvertXMP_BoolToBool( readAll ) );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID FileSysWrite( XMP_IORef io, void* buffer, XMP_Uns32 count, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try
	{
		if( io != NULL )
		{
			::XMP_IO * thiz = (::XMP_IO*)io;
			thiz->Write( buffer, count );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID FileSysSeek( XMP_IORef io, XMP_Int64& offset, SeekMode mode, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try
	{
		if( io != NULL )
		{
			::XMP_IO * thiz = (::XMP_IO*)io;
			thiz->Seek( offset, mode );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID FileSysLength( XMP_IORef io, XMP_Int64& length, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try
	{
		if( io != NULL )
		{
			::XMP_IO * thiz = (::XMP_IO*)io;
			length = thiz->Length();
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID FileSysTruncate( XMP_IORef io, XMP_Int64 length, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try
	{
		if( io != NULL )
		{
			::XMP_IO * thiz = (::XMP_IO*)io;
			thiz->Truncate( length );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID FileSysDeriveTemp( XMP_IORef io, XMP_IORef& tempIO, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try
	{
		if( io != NULL )
		{
			::XMP_IO * thiz = (::XMP_IO*)io;
			tempIO = thiz->DeriveTemp();
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID FileSysAbsorbTemp( XMP_IORef io, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try
	{
		if( io != NULL )
		{
			::XMP_IO * thiz = (::XMP_IO*)io;
			thiz->AbsorbTemp();
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID FileSysDeleteTemp( XMP_IORef io, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try
	{
		if( io != NULL )
		{
			::XMP_IO * thiz = (::XMP_IO*)io;
			thiz->DeleteTemp();
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static void GetFileSysAPI( FileIO_API* fileSys )
{
	if( fileSys )
	{
		fileSys->mReadProc			=	FileSysRead;
		fileSys->mWriteProc			=	FileSysWrite;
		fileSys->mSeekProc			=	FileSysSeek;
		fileSys->mLengthProc		=	FileSysLength;
		fileSys->mTruncateProc		=	FileSysTruncate;
		fileSys->mDeriveTempProc	=	FileSysDeriveTemp;
		fileSys->mAbsorbTempProc	=	FileSysAbsorbTemp;
		fileSys->mDeleteTempProc	=	FileSysDeleteTemp;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//		String_API
//

static XMPErrorID CreateBuffer( StringPtr* buffer, XMP_Uns32 size, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try 
	{
		if( buffer != NULL )
		{
			*buffer = (StringPtr) malloc( size );
			if( *buffer != NULL )
			{
				wError->mErrorID = kXMPErr_NoError;
			}
			else
			{
				wError->mErrorMsg = "Allocation failed";
			}
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static XMPErrorID ReleaseBuffer( StringPtr buffer, WXMP_Error * wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	try 
	{
		if( buffer )
		{
			free( buffer );
			wError->mErrorID = kXMPErr_NoError;
		}
	}
	catch( ... )
	{
		HandleException( wError );
	}

	return wError->mErrorID;
}

static void GetStringAPI( String_API* strAPI )
{
	if( strAPI )
	{
		strAPI->mCreateBufferProc			=	CreateBuffer;
		strAPI->mReleaseBufferProc			=	ReleaseBuffer;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//		Abort_API
//

static XMPErrorID CheckAbort( SessionRef session, XMP_Bool * aborted, WXMP_Error* wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID = kXMPErr_InternalFailure;

	if( aborted )
	{
		*aborted = kXMP_Bool_False;

		//
		// find FileHandlerInstance associated to session reference
		//
		FileHandlerInstancePtr instance = PluginManager::getHandlerInstance( session );

		if( instance != NULL )
		{
			//
			// execute abort procedure if available
			//
			wError->mErrorID	= kXMPErr_NoError;
			XMP_AbortProc proc	= instance->parent->abortProc;
			void* arg			= instance->parent->abortArg;

			if( proc )
			{
				try
				{
					*aborted = ConvertBoolToXMP_Bool( proc( arg ) );
				}
				catch( ... )
				{
					HandleException( wError );
				}
			}
		}
	}
	else
	{
		wError->mErrorMsg = "Invalid parameter";
	}

	return wError->mErrorID;
}

static void GetAbortAPI( Abort_API* abortAPI )
{
	if( abortAPI )
	{
		abortAPI->mCheckAbort = CheckAbort;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
//		StandardHandler_API
//

static XMPErrorID CheckFormatStandardHandler( SessionRef session, XMP_FileFormat format, StringPtr path, XMP_Bool & checkOK, WXMP_Error* wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID	= kXMPErr_InternalFailure;
	wError->mErrorMsg	= NULL;
	checkOK				= false;

	//
	// find FileHandlerInstance associated to session reference
	//
	FileHandlerInstancePtr instance = PluginManager::getHandlerInstance( session );

	if( instance != NULL && PluginManager::getHandlerPriority( instance ) == PluginManager::kReplacementHandler )
	{
		//
		// find previous file handler for file format identifier
		//
		XMPFileHandlerInfo* hdlInfo = HandlerRegistry::getInstance().getStandardHandlerInfo( format );

		if( hdlInfo != NULL && HandlerRegistry::getInstance().isReplaced( format ) )
		{
			if( hdlInfo->checkProc != NULL )
			{
				try
				{
					//
					// setup temporary XMPFiles instance
					//
					XMPFiles standardClient;
					standardClient.format = format;
					standardClient.SetFilePath( path );

					if( hdlInfo->flags & kXMPFiles_FolderBasedFormat )
					{
						//
						// process folder based handler
						//
						if( path != NULL )
						{
							// The following code corresponds to the one found in HandlerRegistry::selectSmartHandler,
							// in the folder based handler selection section of the function
							// In this case the format is already known, therefor no folder checks are needed here,
							// but the path must be split into the meaningful parts to call checkFolderFormat correctly

							std::string rootPath = path;
							std::string leafName;
							std::string fileExt;
							std::string gpName;
							std::string parentName;

							XIO::SplitLeafName ( &rootPath, &leafName );

							if( !leafName.empty() )
							{
								size_t extPos = leafName.size();

								for ( --extPos; extPos > 0; --extPos ) if ( leafName[extPos] == '.' ) break;

								if( leafName[extPos] == '.' ) 
								{
									fileExt.assign( &leafName[extPos+1] );
									MakeLowerCase( &fileExt );
									leafName.erase( extPos );
								}

								CheckFolderFormatProc CheckProc = (CheckFolderFormatProc) (hdlInfo->checkProc);

								// The case that a logical path to a clip has been passed, which does not point to a real file 
								if( Host_IO::GetFileMode( path ) == Host_IO::kFMode_DoesNotExist )
								{
									checkOK = CheckProc( hdlInfo->format, rootPath, gpName, parentName, leafName, &standardClient );
								}
								else
								{
									XIO::SplitLeafName( &rootPath, &parentName );
									XIO::SplitLeafName( &rootPath, &gpName );
									std::string origGPName ( gpName );	// ! Save the original case for XDCAM-FAM.
									MakeUpperCase( &parentName );
									MakeUpperCase( &gpName );

									if( format == kXMP_XDCAM_FAMFile && ( (parentName == "CLIP") || (parentName == "EDIT") || (parentName == "SUB") ) ) 
									{
										// ! The standard says Clip/Edit/Sub, but we just shifted to upper case.
										gpName = origGPName;	// ! XDCAM-FAM has just 1 level of inner folder, preserve the "MyMovie" case.
									}

									checkOK = CheckProc( hdlInfo->format, rootPath, gpName, parentName, leafName, &standardClient );
								}
							}
							else
							{
								wError->mErrorID = kXMPErr_BadParam;
							}
						}
						else
						{
							wError->mErrorID = kXMPErr_BadParam;
						}
					}
					else
					{
						//
						// file based handler (requires XMP_IO object)
						//
						CheckFileFormatProc CheckProc = (CheckFileFormatProc) (hdlInfo->checkProc);
						XMPFiles_IO* io = XMPFiles_IO::New_XMPFiles_IO ( path, true );
						checkOK = CheckProc( hdlInfo->format, path, io, &standardClient );
						delete io;
					}

					wError->mErrorID = kXMPErr_NoError;
				}
				catch( ... )
				{
					HandleException( wError );
				}
			}
		}
		else
		{
			wError->mErrorMsg = "No standard handler available";
		}
	}
	else
	{
		wError->mErrorMsg = "Standard file handler can't call prior handler";
	}

	return wError->mErrorID;
}

static XMPErrorID GetXMPStandardHandler( SessionRef session, XMP_FileFormat format, StringPtr path, XMPMetaRef xmpRef, XMP_Bool * xmpExists, WXMP_Error* wError )
{
	if( wError == NULL )	return kXMPErr_BadParam;

	wError->mErrorID	= kXMPErr_InternalFailure;
	wError->mErrorMsg	= NULL;

	//
	// find FileHandlerInstance associated to session reference
	//
	FileHandlerInstancePtr instance = PluginManager::getHandlerInstance( session );
	
	if( instance != NULL && PluginManager::getHandlerPriority( instance ) == PluginManager::kReplacementHandler )
	{
		//
		// find previous file handler for file format identifier
		//
		XMPFileHandlerInfo* hdlInfo = HandlerRegistry::getInstance().getStandardHandlerInfo( format );

		if( hdlInfo != NULL && HandlerRegistry::getInstance().isReplaced( format ) )
		{
			//
			// check format first
			//
			XMP_Bool suc = kXMP_Bool_False;

			XMPErrorID errorID = CheckFormatStandardHandler( session, format, path, suc, wError );

			if( errorID == kXMPErr_NoError && ConvertXMP_BoolToBool( suc ) )
			{
				//
				// setup temporary XMPFiles instance
				//
				XMPFiles standardClient;
				standardClient.format = format;
				standardClient.SetFilePath( path );

				SXMPMeta meta( xmpRef );
			
				try
				{
					//
					// open with passed handler info
					//
					suc = standardClient.OpenFile( *hdlInfo, path, kXMPFiles_OpenForRead );

					if( suc )
					{
						//
						// read meta data
						//
						suc = standardClient.GetXMP( &meta );

						if( xmpExists != NULL )		*xmpExists = suc;
					}
				}
				catch( ... )
				{
					HandleException( wError );
				}

				//
				// close and cleanup
				//
				try
				{
					standardClient.CloseFile();
				}
				catch( ... )
				{
					HandleException( wError );
				}
			}
			else if( errorID == kXMPErr_NoError )
			{
				wError->mErrorID = kXMPErr_BadFileFormat;
				wError->mErrorMsg = "Standard handler can't process file format";
			}
		}
		else
		{
			wError->mErrorID = kXMPErr_NoFileHandler;
			wError->mErrorMsg = "No standard handler available";
		}
	}
	else
	{
		wError->mErrorMsg = "Standard file handler can't call prior handler";
	}

	return wError->mErrorID;
}

static XMPErrorID GetXMPStandardHandler_V2( SessionRef session, XMP_FileFormat format, StringPtr path, XMP_StringPtr* xmpStr, XMP_Bool * xmpExists, WXMP_Error* wError )
{
	SXMPMeta meta;
    std::string xmp;
	GetXMPStandardHandler( session, format, path, meta.GetInternalRef(),  xmpExists, wError ) ;
	if( wError->mErrorID != kXMPErr_NoError ) 
		return wError->mErrorID;

	meta.SerializeToBuffer(&xmp, kXMP_NoOptions, 0);
	XMP_Uns32 length = (XMP_Uns32)xmp.size() + 1 ;
	StringPtr buffer = NULL;
	CreateBuffer( &buffer,length ,wError);	
	if( wError->mErrorID != kXMPErr_NoError ) 
		return wError->mErrorID;

	memcpy( buffer, xmp.c_str(), length );
	*xmpStr = buffer; // callee function should free the memory.
	return wError->mErrorID ;
}


static void GetStandardHandlerAPI( StandardHandler_API* standardHandlerAPI )
{
	if( standardHandlerAPI )
	{
		standardHandlerAPI->mCheckFormatStandardHandler	= CheckFormatStandardHandler;
		standardHandlerAPI->mGetXMPStandardHandler		= GetXMPStandardHandler;
	}

}


static XMPErrorID RequestAPISuite( const char* apiName, XMP_Uns32 apiVersion, void** apiSuite, WXMP_Error* wError )
{
	if( wError == NULL )
	{
		return kXMPErr_BadParam;
	}
	
	wError->mErrorID = kXMPErr_NoError;
	
	if( apiName == NULL
	   || apiVersion == 0
	   || apiSuite == NULL )
	{
		wError->mErrorID = kXMPErr_BadParam;
		return kXMPErr_BadParam;
	}
	
	// dummy suite used by unit test
	if( strcmp( apiName, "testDummy" ) == 0 && apiVersion == 1 )
	{
		*apiSuite = (void*) &RequestAPISuite;
	}
	else if ( ! strcmp( apiName, "StandardHandler" ) && apiVersion == 2 ) 
	{
		static const StandardHandler_API_V2 standardHandlerAPI =
		{
			&CheckFormatStandardHandler,
			&GetXMPStandardHandler_V2
		};
		*apiSuite=(void*)&standardHandlerAPI;
	}
	else
	{
		wError->mErrorID = kXMPErr_Unimplemented;
	}
	
	return wError->mErrorID;
}
	

///////////////////////////////////////////////////////////////////////////////
//
//		Init/Term Host APIs
//

// Because of changes to the plugin versioning strategy,
// the host API version is no longer tied to the plugin version
// and the host API struct is supposed to be frozen.
// New host APIs can be requested through the new function mRequestAPISuite.

void PluginManager::SetupHostAPI_V1( HostAPIRef hostAPI )
{
	// Get XMP_IO APIs
	hostAPI->mFileIOAPI = new FileIO_API();
	GetFileSysAPI( hostAPI->mFileIOAPI );

	// Get String APIs
	hostAPI->mStrAPI = new String_API();
	GetStringAPI( hostAPI->mStrAPI );

	// Get Abort API
	hostAPI->mAbortAPI = new Abort_API();
	GetAbortAPI( hostAPI->mAbortAPI );

	// Get standard handler APIs
	hostAPI->mStandardHandlerAPI = new StandardHandler_API();
	GetStandardHandlerAPI( hostAPI->mStandardHandlerAPI );

	hostAPI->mRequestAPISuite = NULL;
}

void PluginManager::SetupHostAPI_V2( HostAPIRef hostAPI )
{
	SetupHostAPI_V1 ( hostAPI );
}

void PluginManager::SetupHostAPI_V3( HostAPIRef hostAPI )
{
	SetupHostAPI_V2 ( hostAPI );
}

void PluginManager::SetupHostAPI_V4( HostAPIRef hostAPI )
{
	SetupHostAPI_V3 ( hostAPI );
	hostAPI->mRequestAPISuite = RequestAPISuite;
}

} //namespace XMP_PLUGIN
