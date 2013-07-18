// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "HostAPIAccess.h"
#include <cstring>
#include <string>
#define TXMP_STRING_TYPE std::string
#include "XMP.hpp"

namespace XMP_PLUGIN
{

///////////////////////////////////////////////////////////////////////////////
//
// API handling 
//
	
static HostAPIRef sHostAPI = NULL;
static XMP_Uns32 sHostAPIVersion = 0;

StandardHandler_API_V2* sStandardHandler_V2 = NULL;
	
// ============================================================================
	
static bool CheckAPICompatibility_V1 ( const HostAPIRef hostAPI )
{
	return ( hostAPI
			&& hostAPI->mFileIOAPI
			&& hostAPI->mStrAPI
			&& hostAPI->mAbortAPI
			&& hostAPI->mStandardHandlerAPI );
}

static bool CheckAPICompatibility_V4 ( const HostAPIRef hostAPI )
{
	return ( CheckAPICompatibility_V1( hostAPI )
			&& hostAPI->mRequestAPISuite != NULL );
}

// ============================================================================

bool SetHostAPI( HostAPIRef hostAPI )
{
	bool valid = false;
	if( hostAPI && hostAPI->mVersion > 0 )
	{
		if ( hostAPI->mVersion <= 3 )
		{
			// Old host API before plugin versioning changes
			valid = CheckAPICompatibility_V1( hostAPI );
		}
		else
		{
			// New host API including RequestAPISuite.
			// This version of the HostAPI struct should not be changed.
			valid = CheckAPICompatibility_V4( hostAPI );
		}
	}
	
	if( valid )
	{
		sHostAPI = hostAPI;
		sHostAPIVersion = hostAPI->mVersion;
	}
	
	return valid;
}

// ============================================================================

static HostAPIRef GetHostAPI()
{
	return sHostAPI;
}

// ============================================================================

static XMP_Uns32 GetHostAPIVersion()
{
	return sHostAPIVersion;
}

// ============================================================================

inline void CheckError( WXMP_Error & error )
{
	if( error.mErrorID != kXMPErr_NoError )
	{
		throw XMP_Error( error.mErrorID, error.mErrorMsg );
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// class IOAdapter
//

XMP_Uns32 IOAdapter::Read( void* buffer, XMP_Uns32 count, bool readAll ) const
{
	WXMP_Error error;
	XMP_Uns32 result;
	GetHostAPI()->mFileIOAPI->mReadProc( this->mFileRef, buffer, count, readAll, result, &error );
	CheckError( error );
	return result;
}

// ============================================================================

void IOAdapter::Write( void* buffer, XMP_Uns32 count ) const
{
	WXMP_Error error;
	GetHostAPI()->mFileIOAPI->mWriteProc( this->mFileRef, buffer, count, &error );
	CheckError( error );
}

// ============================================================================

void IOAdapter::Seek( XMP_Int64& offset, SeekMode mode ) const
{
	WXMP_Error error;
	GetHostAPI()->mFileIOAPI->mSeekProc( this->mFileRef, offset, mode, &error );
	CheckError( error );
}

// ============================================================================

XMP_Int64 IOAdapter::Length() const
{
	WXMP_Error error;
	XMP_Int64 length = 0;
	GetHostAPI()->mFileIOAPI->mLengthProc( this->mFileRef, length, &error );
	CheckError( error );
	return length;
}

// ============================================================================

void IOAdapter::Truncate( XMP_Int64 length ) const
{
	WXMP_Error error;
	GetHostAPI()->mFileIOAPI->mTruncateProc( this->mFileRef, length, &error );
	CheckError( error );
}

// ============================================================================

XMP_IORef IOAdapter::DeriveTemp() const
{
	WXMP_Error error;
	XMP_IORef tempIO;
	GetHostAPI()->mFileIOAPI->mDeriveTempProc( this->mFileRef, tempIO, &error );
	CheckError( error );
	return tempIO;
}

// ============================================================================

void IOAdapter::AbsorbTemp() const
{
	WXMP_Error error;
	GetHostAPI()->mFileIOAPI->mAbsorbTempProc( this->mFileRef, &error );
	CheckError( error );
}

// ============================================================================

void IOAdapter::DeleteTemp() const
{
	WXMP_Error error;
	GetHostAPI()->mFileIOAPI->mDeleteTempProc( this->mFileRef, &error );
	CheckError( error );
}

///////////////////////////////////////////////////////////////////////////////
//
// Host Strings
//

StringPtr HostStringCreateBuffer( XMP_Uns32 size )
{
	WXMP_Error error;
	StringPtr buffer = NULL;
	GetHostAPI()->mStrAPI->mCreateBufferProc( &buffer, size, &error );
	CheckError( error );
	return buffer;
}

// ============================================================================

void HostStringReleaseBuffer( StringPtr buffer )
{
	WXMP_Error error;
	GetHostAPI()->mStrAPI->mReleaseBufferProc( buffer, &error );
	CheckError( error );
}

///////////////////////////////////////////////////////////////////////////////
//
// Abort functionality
//

bool CheckAbort( SessionRef session )
{
	WXMP_Error error;
	XMP_Bool abort = false;
	GetHostAPI()->mAbortAPI->mCheckAbort( session, &abort, &error );

	if( error.mErrorID == kXMPErr_Unavailable )
	{
		abort = false;
	}
	else if( error.mErrorID != kXMPErr_NoError )
	{
		throw XMP_Error( error.mErrorID, error.mErrorMsg );
	}

	return ConvertXMP_BoolToBool( abort );
}

///////////////////////////////////////////////////////////////////////////////
//
// Standard file handler access
//


bool CheckFormatStandard( SessionRef session, XMP_FileFormat format, const StringPtr path )
{
	WXMP_Error error;
	XMP_Bool ret = true;
	if ( sStandardHandler_V2 == NULL )
	{
		throw XMP_Error( kXMPErr_Unavailable, "StandardHandler suite unavailable" );
	}
	sStandardHandler_V2->mCheckFormatStandardHandler( session, format, path, ret, &error );

	if( error.mErrorID != kXMPErr_NoError )
	{
		throw XMP_Error( error.mErrorID, error.mErrorMsg );
	}

	return ConvertXMP_BoolToBool( ret );
}

// ============================================================================

bool GetXMPStandard( SessionRef session, XMP_FileFormat format, const StringPtr path, std::string& xmpStr, bool* containsXMP )
{
	WXMP_Error error;
	bool ret = true;
	XMP_StringPtr outXmp= NULL;
	XMP_Bool cXMP = kXMP_Bool_False;
	if ( sStandardHandler_V2 == NULL )
	{
		throw XMP_Error( kXMPErr_Unavailable, "StandardHandler suite unavailable" );
	}
	sStandardHandler_V2->mGetXMPStandardHandler( session, format, path, &outXmp, &cXMP, &error );
	*containsXMP = ConvertXMP_BoolToBool( cXMP );

	if( error.mErrorID == kXMPErr_NoFileHandler || error.mErrorID == kXMPErr_BadFileFormat)
	{
		ret = false;
	}
	else if( error.mErrorID != kXMPErr_NoError )
	{
		throw XMP_Error( error.mErrorID, error.mErrorMsg );
	}
	xmpStr=outXmp;
	HostStringReleaseBuffer( (StringPtr)outXmp ) ;
	return ret;
}

// ============================================================================

void* RequestAPISuite( const char* apiName, XMP_Uns32 apiVersion )
{
	void* suite = NULL;
	
	WXMP_Error error;
	
	if (GetHostAPIVersion() >= 4)
	{
		GetHostAPI()->mRequestAPISuite( apiName, apiVersion, &suite, &error );
		CheckError(error);
	}
	else
	{
		throw XMP_Error( kXMPErr_Unavailable, "RequestAPISuite unavailable (host too old)" );
	}
	

	return suite;
}


} //namespace XMP_PLUGIN
