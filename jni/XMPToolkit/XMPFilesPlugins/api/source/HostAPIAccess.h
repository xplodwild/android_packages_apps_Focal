// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef __HostAPIAccess_h__
#define __HostAPIAccess_h__	1
#include "HostAPI.h"
#include <string>

namespace XMP_PLUGIN
{

/** @brief Sets the host API struct for the plugin
 *
 *  The HostAPI struct will be passed in from the host during plugin initialization.
 *  The struct will contain a mVersion field which contains the actual version of the host API.
 *  As the plugin might be newer than the plugin host the plugin must always check if
 *  a host function is available before calling into the host.
 *
 *  @param hostAPI The HostAPI struct. The struct can be smaller than expected.
 *  @return True if the hostAPI was accepted by the plugin.
 */
bool SetHostAPI( HostAPIRef hostAPI );


/** @class IOAdapter
 *  @brief This class deals with file I/O. It's a wrapper class over host API's functions hostAPI->mFileIOAPI.
 *
 *  This class is a wrapper class over hostAPI File I/O apis. This class gives easy interface 
 *  for plug-in developer so that they can use it very easily.
 */
class IOAdapter 
{
public:

	/** @brief Read into a buffer, returning the number of bytes read.
	 *
	 *  Read into a buffer, returning the number of bytes read. Returns the actual number of bytes
	 *  read. Throws an exception if requireSuccess is true and not enough data is available.
	 *  Throwing \c XMPError is recommended. The buffer content and I/O position after a throw are
	 *  undefined.
	 *
	 *  @param buffer A pointer to the buffer.
	 *  @param count The length of the buffer in bytes.
	 *  @param readAll True if reading less than the requested amount is a failure.
	 *  @return Returns the number of bytes read.
	 */
	XMP_Uns32 Read( void* buffer, XMP_Uns32 count, bool readAll ) const;
	
	/** @brief Write from a buffer.
	 *
	 *  Write from a buffer, overwriting existing data and extending the file as necessary. All data
	 *  must be written or an exception thrown. Throwing \c XMPError is recommended.
	 *
	 *  @param buffer A pointer to the buffer.
	 *  @param count The length of the buffer in bytes.
	 */
	void Write( void* buffer, XMP_Uns32 count ) const;
	
	/** @brief Set the I/O position, returning the new absolute offset in bytes.
	 *
	 *  Set the I/O position, returning the new absolute offset in bytes. The offset parameter may
	 *  be positive or negative. A seek beyond EOF is allowed when writing and extends the file, it
	 *  is equivalent to seeking to EOF then writing the needed amount of undefined data. A
	 *  read-only seek beyond EOF throws an exception. Throwing \c XMPError is recommended.
	 *
	 *  @param offset The offset relative to the mode.
	 *  @param mode The mode, or origin, of the seek.
	 *  @return The new absolute offset in bytes.
	 */
	void Seek( XMP_Int64& offset, SeekMode mode ) const;
	
	/** @brief Return the length of the file in bytes.
	 *
	 *  Return the length of the file in bytes. The I/O position is unchanged.
	 *  @return The length of the file in bytes.
	 */
	XMP_Int64 Length() const;

	/** @brief Truncate the file to the given length.
	 *
	 *  Truncate the file to the given length. The I/O position after truncation is unchanged if
	 *  still valid, otherwise it is set to the new EOF. Throws an exception if the new length is
	 *  longer than the file's current length. Throwing \c XMPError is recommended.
	 *  @param length The new length for the file, must be less than or equal to the current length.
	 */	
	void Truncate( XMP_Int64 length ) const;

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
	 *  \c DeriveTemp may be called on a temporary \c XMP_IO object.
	 *
	 *  @return A pointer to the associated temporary \c XMP_IO object.
	 */
	XMP_IORef DeriveTemp() const;
	
	/** @brief Replace the owning file's content with that of the temp.
	 *
	 *  Used at the end of a safe-save style operation to replace the original content with that
	 *  from the associated temp file. The temp file must be closed and deleted after the content
	 *  swap. The temporary \c XMP_IO object is deleted. Throws an exception if the temp file cannot
	 *  be absorbed. Throwing \c XMPError is recommended.
	 */
	void AbsorbTemp() const;
	
	/** @brief Delete a temp file, leaving the original alone.
	 *
	 *  Used for a failed safe-save style operation. The temp file is closed and deleted without
	 *  being absorbed, and the temporary \c XMP_IO object is deleted. Does nothing if no temp exists.
	 */
	void DeleteTemp() const;
	
	IOAdapter( XMP_IORef io ) : mFileRef( io ) { }
	~IOAdapter() { }

private:
	XMP_IORef		mFileRef;	
};

typedef IOAdapter HostFileSys;

/** @brief Allocate a buffer of size /param size.
 *
 *  It is a wrapper function of host API function 'hostAPI->mStrAPI->mCreateBufferProc'. 
 *  Buffer allocated by this function should be released by calling HostStringReleaseBuffer.
 *
 *  @param size Size of the buffer.
 *  @return Pointer to a memory block of /param size. It throws exception if memory couldn't be allocated.
 */
StringPtr HostStringCreateBuffer( XMP_Uns32 size );

/** @brief Release the buffer allocated by HostStringCreateBuffer.
 *
 *  It is a wrapper function of host API function 'hostAPI->mStrAPI->mReleaseBufferProc'. 
 *
 *  @param buffer The buffer pointer which needs to be released.
 */
void HostStringReleaseBuffer( StringPtr buffer );

/** @brief Ask XMPFiles if current operation should be aborted.
 *
 *  @param session Related session
 *  @return true if the current operation should be aborted
 */
bool CheckAbort( SessionRef session );

/** @brief Check format with standard file handler
 *
 * Call the standard file handler to check the format of the data source.
 * This call expects that session refers to a replacement file handler. Otherwise
 * the call fails with an exception.
 *
 * @param session		File handler session (should refer to replacement handler)
 * @param format		The file format identifier
 * @param path			Path to the file that needs to be checked
 * @return				true on success
 */
bool CheckFormatStandard( SessionRef session, XMP_FileFormat format, const StringPtr path );

/** @brief Get XMP from standard file handler
 *
 * Call the standard file handler in order to retrieve XMP from it.
 * This call expects that session refers to a replacement file handler. Otherwise
 * this call fails with an exception.
 *
 * @param session		File handler session  (should refer to replacement handler)
 * @param format		The file format identifier
 * @param path			Path to the file that should be proceeded
 * @param xmpStr		Reference to serialized XMP packet. Will be populated with the XMP Packet as read by the standard file handler
 * @param containsXMP	Returns true if the standard handler detected XMP
 * @return				true on success
 */
bool GetXMPStandard( SessionRef session, XMP_FileFormat format, const StringPtr path, std::string& xmpStr, bool* containsXMP );

/** @brief Request additional API suite from the host.
 *
 *  RequestAPISuite should be called during plugin initialization time
 *  to request additional versioned APIs from the plugin host. If the name or version
 *  of the requested API suite is unknown NULL is returned.
 *
 *  @param apiName The name of the API suite.
 *  @param apiVersion The version of the API suite.
 *  @return pointer to the suite struct or NULL if name/version is unknown.
 */	
void* RequestAPISuite( const char* apiName, XMP_Uns32 apiVersion );


} //namespace XMP_PLUGIN

#endif  // __HostAPIAccess_h__
