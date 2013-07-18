// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================


#ifndef __HOSTAPI_H__
#define __HOSTAPI_H__	1
#include "PluginHandler.h"

#define XMP_HOST_API_VERSION_1	1	// CS6
#define XMP_HOST_API_VERSION_4	4	// CS7 and beyond

#define XMP_HOST_API_VERSION	XMP_HOST_API_VERSION_4


namespace XMP_PLUGIN
{

#ifdef __cplusplus
extern "C" {
#endif

struct FileIO_API;
struct String_API;
struct Abort_API;
struct StandardHandler_API;
typedef char* StringPtr;

/** @brief Request additional API suite from the host.
 *
 *  RequestAPISuite should be called during plugin initialization time
 *  to request additional versioned APIs from the plugin host. If the name or version
 *  of the requested API suite is unknown to the host an error is returned.
 *
 *  @param apiName The name of the API suite.
 *  @param apiVersion The version of the API suite.
 *  @param apiSuite On successful exit points to the API suite.
 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
 *  @return kXMPErr_NoError on success otherwise error id of the failure.
 */	
typedef XMPErrorID (*RequestAPISuiteFn)( const char* apiName, XMP_Uns32 apiVersion, void** apiSuite, WXMP_Error* wError );
	

/** @struct HostAPI
 *  @brief This is a Host API structure.
 *
 *  Please don't change this struct.
 *  Additional host functionality should be added through RequestAPISuite().
 */
struct HostAPI
{
	/** 
	 *  Size of the structure.
	 */
	XMP_Uns32				mSize;
   
	/** 
	 *  Version number of the API.
	 */
	XMP_Uns32				mVersion;
	
	/**
	 *  Pointer to a structure which contains file system APIs to access XMP_IO.
	 */
	FileIO_API*				mFileIOAPI;

	/**
	 *  Pointer to a structure which contains string related api.
	 */
	String_API*				mStrAPI;

	/**
	 * Pointer to a structure which contains API for user abort functionality
	 */
	Abort_API*				mAbortAPI;

	/**
	 * Pointer to a structure which contains API for accessing the standard file handler
	 */
	StandardHandler_API*	mStandardHandlerAPI;
	
	//
	// VERSION 4
	//
	
	/**
	 * Function to request additional APIs from the host
	 */
	RequestAPISuiteFn		mRequestAPISuite;
	
};


/** @struct FileIO_API
 *  @brief APIs for file I/O inside XMPFiles. These APIs are provided by the host.
 *
 *  This structure contains function pointers to access XMP_IO.
 */
struct FileIO_API
{
	/** 
	 *  Size of the structure.
	 */
	XMP_Uns32		mSize;
   
	/** @brief Read into a buffer, returning the number of bytes read.
	 *  @param io pointer to the file.
	 *  @param buffer A pointer to the buffer which will be filled in.
	 *  @param count The length of the buffer in bytes.
	 *  @param readAll True if reading less than the requested amount is a failure.
	 *  @param byteRead contains the number of bytes read from io.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */	
	typedef XMPErrorID (*ReadProc)( XMP_IORef io, void* buffer, XMP_Uns32 count, XMP_Bool readAll, XMP_Uns32& byteRead, WXMP_Error * wError );
	ReadProc		mReadProc;

	/** @brief Write from a buffer.
	 *  @param io pointer to the file.
	 *  @param buffer A pointer to the buffer.
	 *  @param count The length of the buffer in bytes.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */	
	typedef XMPErrorID (*WriteProc)( XMP_IORef io, void* buffer, XMP_Uns32 count, WXMP_Error * wError );
	WriteProc		mWriteProc;

	/** @brief Set the I/O position, returning the new absolute offset in bytes.
	 *
	 *  Set the I/O position, returning the new absolute offset in bytes. The offset parameter may
	 *  be positive or negative. A seek beyond EOF is allowed when writing and extends the file, it
	 *  is equivalent to seeking to EOF then writing the needed amount of undefined data. A
	 *  read-only seek beyond EOF throws an exception. Throwing \c XMPError is recommended.
	 *
	 *  @param offset The offset relative to the mode.
	 *  @param mode The mode, or origin, of the seek.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*SeekProc)( XMP_IORef io, XMP_Int64& offset, SeekMode mode, WXMP_Error * wError );
	SeekProc		mSeekProc;

	/** @brief length pointer to an 64 bit integer which will be filled.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*LengthProc)( XMP_IORef io, XMP_Int64& length, WXMP_Error * wError );
	LengthProc		mLengthProc;

	/** @brief Truncate the file to the given length.
	 *
	 *  Truncate the file to the given length. The I/O position after truncation is unchanged if
	 *  still valid, otherwise it is set to the new EOF. Throws an exception if the new length is
	 *  longer than the file's current length. Throwing \c XMPError is recommended.
	 *
	 *  @param length The new length for the file, must be less than or equal to the current length.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*TruncateProc)( XMP_IORef io, XMP_Int64 length, WXMP_Error * wError );
	TruncateProc		mTruncateProc;

	/** @brief Create an associated temp file for use in a safe-save style operation.
	 *
	 *  Create an associated temp file, for example in the same directory and with a related name.
	 *  Returns an already existing temp with no other action. The temp must be opened for
	 *  read-write access. It will be used in a safe-save style operation, using some of the
	 *  original file plus new portions to write the temp, then replacing the original from the temp
	 *  when done. Throws an exception if the owning object is opened for read-only access, or if
	 *  the temp file cannot be created. Throwing \c XMPError is recommended.
	 *
	 *  The temp file is normally closed and deleted, and the temporary \c XMP_IO object deleted, by
	 *  a call to \c AbsorbTemp or \c DeleteTemp. It must be closed and deleted by the derived \c
	 *  XMP_IO object's destructor if necessary.
	 *
	 *  DeriveTemp may be called on a temporary \c XMP_IO object.
	 *
	 *  @tempIO A pointer to the XMP_IO which will be filled by the function.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*DeriveTempProc)( XMP_IORef io, XMP_IORef& tempIO, WXMP_Error * wError );
	DeriveTempProc		mDeriveTempProc;

	/** @brief Replace the owning file's content with that of the temp.
	 *
	 *  Used at the end of a safe-save style operation to replace the original content with that
	 *  from the associated temp file. The temp file must be closed and deleted after the content
	 *  swap. The temporary \c XMP_IO object is deleted. Throws an exception if the temp file cannot
	 *  be absorbed. Throwing \c XMPError is recommended.
	 *  @param io pointer to the file.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*AbsorbTempProc)( XMP_IORef io, WXMP_Error * wError );
	AbsorbTempProc		mAbsorbTempProc;

	/** @brief Delete a temp file, leaving the original alone.
	 *
	 *  Used for a failed safe-save style operation. The temp file is closed and deleted without
	 *  being absorbed, and the temporary \c XMP_IO object is deleted. Does nothing if no temp
	 *  exists.
	 *  @param io pointer to the file.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*DeleteTempProc)( XMP_IORef io, WXMP_Error * wError );
	DeleteTempProc		mDeleteTempProc;
};

/** @struct String_API
 *  @brief APIs to provide functionality to create and release string buffer.
 */
struct String_API
{
	/** @brief Allocate a buffer of size /param size. This buffer should be released by calling ReleaseBufferProc.
	 *
	 *  @param buffer Pointer to StringBuffer which will be assigned a memory block of size /param size.
	 *  @param size Size of the buffer.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*CreateBufferProc)( StringPtr* buffer, XMP_Uns32 size, WXMP_Error * wError );
	CreateBufferProc   mCreateBufferProc;

	/** @brief Release the buffer allocated by CreateBufferProc.
	 *
	 *  @param buffer The buffer pointer which needs to be released.
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*ReleaseBufferProc)( StringPtr buffer, WXMP_Error * wError );
	ReleaseBufferProc  mReleaseBufferProc;
};

/** @string Abort_API
 *  @brief API to handle user abort functionality.
 */
struct Abort_API
{
	/** @brief Ask XMPFiles if current operation should be aborted.
	 *
	 *  @param aborted Contains true if the current operation should be aborted
	 *  @param wError WXMP_Error structure which will be filled by the API if any error occurs.
	 *  @return kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*CheckAbort)( SessionRef session, XMP_Bool * aborted, WXMP_Error* wError );
	CheckAbort	mCheckAbort;
};

struct StandardHandler_API
{
	/** @brief Check format with standard file handler
	 *
	 * Call the replaced file handler (if available) to check the format of the data source.
	 *
	 * @param session		File handler session (referring to replacement handler)
	 * @param format		The file format identifier
	 * @param path			Path to the file that needs to be checked
	 * @param checkOK		On return true if the data can be handled by the file handler
	 * @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
	 * @return				kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*CheckFormatStandardHandler)( SessionRef session, XMP_FileFormat format, StringPtr path, XMP_Bool & checkOK, WXMP_Error* wError );
	CheckFormatStandardHandler mCheckFormatStandardHandler;

	/** @brief Get XMP from standard file handler
	 *
	 * Call the standard file handler in order to retrieve XMP from it.
	 *
	 * @param session		File handler session (referring to replacement handler)
	 * @param format		The file format identifier
	 * @param path			Path to the file that should be proceeded
	 * @param meta			Will on success contain reference to the XMP data as returned by the standard handler
	 * @param containsXMP	Returns true if the standard handler detected XMP
	 * @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
	 * @return				kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*GetXMPStandardHandler)( SessionRef session, XMP_FileFormat format, StringPtr path, XMPMetaRef meta, XMP_Bool * containsXMP, WXMP_Error* wError );
	GetXMPStandardHandler	mGetXMPStandardHandler;
};

struct StandardHandler_API_V2
{
	/** @brief Check format with standard file handler
	 *
	 * Call the replaced file handler (if available) to check the format of the data source.
	 *
	 * @param session		File handler session (referring to replacement handler)
	 * @param format		The file format identifier
	 * @param path			Path to the file that needs to be checked
	 * @param checkOK		On return true if the data can be handled by the file handler
	 * @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
	 * @return				kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*CheckFormatStandardHandler)( SessionRef session, XMP_FileFormat format, StringPtr path, XMP_Bool & checkOK, WXMP_Error* wError );
	CheckFormatStandardHandler mCheckFormatStandardHandler;

	/** @brief Get XMP from standard file handler
	 *
	 * Call the standard file handler in order to retrieve XMP from it.
	 *
	 * @param session		File handler session (referring to replacement handler)
	 * @param format		The file format identifier
	 * @param path			Path to the file that should be proceeded
	 * @param xmpStr		Will on success contain serialized XMP Packet from the standard Handler
	 * @param containsXMP	Returns true if the standard handler detected XMP
	 * @param wError		WXMP_Error structure which will be filled by the API if any error occurs.
	 * @return				kXMPErr_NoError on success otherwise error id of the failure.
	 */
	typedef XMPErrorID (*GetXMPStandardHandler)( SessionRef session, XMP_FileFormat format, StringPtr path, XMP_StringPtr* xmpStr, XMP_Bool * containsXMP, WXMP_Error* wError );
	GetXMPStandardHandler	mGetXMPStandardHandler;
};


#ifdef __cplusplus
}
#endif

} //namespace XMP_PLUGIN
#endif // __HOSTAPI_H__
