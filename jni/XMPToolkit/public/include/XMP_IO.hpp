#ifndef __XMP_IO_hpp__
#define __XMP_IO_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "XMP_Const.h"

// =================================================================================================
/// \class XMP_IO XMP_IO.hpp
/// \brief Abstract base class for client-managed I/O with \c TXMPFiles.
///
/// \c XMP_IO is an abstract base class for client-managed I/O with \c TXMPFiles. This allows a
/// client to use the embedded metadata processing logic of \c TXMPFiles in cases where a string
/// file path cannot be provided, or where it is impractical to allow \c TXMPFiles to separately
/// open the file and do its own I/O. Although described in terms of files, any form of storage may
/// be used as long as the functions operate as defined.
///
/// This is not a general purpose I/O class. It contains only the necessary functions needed by the
/// internals of \c TXMPFiles. It is intended to be used as an adaptor for an existing I/O mechanism
/// that the client wants \c TXMPFiles to use.
///
/// To use \c XMP_IO, a client creates a derived class then uses the form of \c TCMPFiles::OpenFile
/// that takes an \c XMP_IO parameter instead of a string file path. The derived \c XMP_IO object
/// must be ready for use when \c TCMPFiles::OpenFile is called.
///
/// There are no Open or Close functions in \c XMP_IO, they are specific to each implementation. The
/// derived \c XMP_IO object must be open and ready for use before being passed to \c
/// TXMP_Files::OpenFile, and remain open and ready for use until \c TXMP_Files::CloseFile returns,
/// or some other fatal error occurs. The client has final responsibility for closing and
/// terminating the derived \c XMP_IO object.
// =================================================================================================

class XMP_IO {
public:

	// ---------------------------------------------------------------------------------------------
	/// @brief Read into a buffer, returning the number of bytes read.
	///
	/// Read into a buffer, returning the number of bytes read. Returns the actual number of bytes
	/// read. Throws an exception if requireSuccess is true and not enough data is available.
	/// Throwing \c XMPError is recommended. The buffer content and I/O position after a throw are
	/// undefined.
	///
	/// @param buffer A pointer to the buffer.
	/// @param count The length of the buffer in bytes.
	/// @param readAll True if reading less than the requested amount is a failure.
	///
	/// @return Returns the number of bytes read.

	enum { kReadAll = true };

	virtual XMP_Uns32 Read ( void* buffer, XMP_Uns32 count, bool readAll = false ) = 0;

	inline XMP_Uns32 ReadAll ( void* buffer, XMP_Uns32 bytes )
		{ return this->Read ( buffer, bytes, kReadAll ); };

	// ---------------------------------------------------------------------------------------------
	/// @brief Write from a buffer.
	///
	/// Write from a buffer, overwriting existing data and extending the file as necesary. All data
	/// must be written or an exception thrown. Throwing \c XMPError is recommended.
	///
	/// @param buffer A pointer to the buffer.
	/// @param count The length of the buffer in bytes.

	virtual void Write ( const void* buffer, XMP_Uns32 count ) = 0;

	// ---------------------------------------------------------------------------------------------
	/// @brief Set the I/O position, returning the new absolute offset in bytes.
	///
	/// Set the I/O position, returning the new absolute offset in bytes. The offset parameter may
	/// be positive or negative. A seek beyond EOF is allowed when writing and extends the file, it
	/// is equivalent to seeking to EOF then writing the needed amount of undefined data. A
	/// read-only seek beyond EOF throws an exception. Throwing \c XMPError is recommended.
	///
	/// @param offset The offset relative to the mode.
	/// @param mode The mode, or origin, of the seek.
	///
	/// @return The new absolute offset in bytes.

	virtual XMP_Int64 Seek ( XMP_Int64 offset, SeekMode mode ) = 0;

	inline XMP_Int64 Offset() { return this->Seek ( 0, kXMP_SeekFromCurrent ); };
	inline XMP_Int64 Rewind() { return this->Seek ( 0, kXMP_SeekFromStart ); };	// Always returns 0.
	inline XMP_Int64 ToEOF()  { return this->Seek ( 0, kXMP_SeekFromEnd ); };

	// ---------------------------------------------------------------------------------------------
	/// @brief Return the length of the file in bytes.
	///
	/// Return the length of the file in bytes. The I/O position is unchanged.
	///
	/// @return The length of the file in bytes.

	virtual XMP_Int64 Length() = 0;

	// ---------------------------------------------------------------------------------------------
	/// @brief Truncate the file to the given length.
	///
	/// Truncate the file to the given length. The I/O position after truncation is unchanged if
	/// still valid, otherwise it is set to the new EOF. Throws an exception if the new length is
	/// longer than the file's current length. Throwing \c XMPError is recommended.
	///
	/// @param length The new length for the file, must be less than or equal to the current length.

	virtual void Truncate ( XMP_Int64 length ) = 0;

	// ---------------------------------------------------------------------------------------------
	/// @brief Create an associated temp file for use in a safe-save style operation.
	///
	/// Create an associated temp file, for example in the same directory and with a related name.
	/// Returns an already existing temp with no other action. The temp must be opened for
	/// read-write access. It will be used in a safe-save style operation, using some of the
	/// original file plus new portions to write the temp, then replacing the original from the temp
	/// when done. Throws an exception if the owning object is opened for read-only access, or if
	/// the temp file cannot be created. Throwing \c XMPError is recommended.
	///
	/// The temp file is normally closed and deleted, and the temporary \c XMP_IO object deleted, by
	/// a call to \c AbsorbTemp or \c DeleteTemp. It must be closed and deleted by the derived \c
	/// XMP_IO object's destructor if necessary.
	///
	/// \c DeriveTemp may be called on a temporary \c XMP_IO object.
	///
	/// @return A pointer to the associated temporary \c XMP_IO object.

	virtual XMP_IO* DeriveTemp() = 0;

	// ---------------------------------------------------------------------------------------------
	/// @brief Replace the owning file's content with that of the temp.
	///
	/// Used at the end of a safe-save style operation to replace the original content with that
	/// from the associated temp file. The temp file must be closed and deleted after the content
	/// swap. The temporary \c XMP_IO object is deleted. Throws an exception if the temp file cannot
	/// be absorbed. Throwing \c XMPError is recommended.

	virtual void AbsorbTemp() = 0;

	// ---------------------------------------------------------------------------------------------
	/// @brief Delete a temp file, leaving the original alone.
	///
	/// Used for a failed safe-save style operation. The temp file is closed and deleted without
	/// being absorbed, and the temporary \c XMP_IO object is deleted. Does nothing if no temp
	/// exists.

	virtual void DeleteTemp() = 0;

	// ---------------------------------------------------------------------------------------------

	XMP_IO() {};
	virtual ~XMP_IO() {};

private:

	// ---------------------------------------------------------------------------------------------
	/// Copy construction and assignment are not public. That would require the implementation to
	/// share state across multiple XMP_IO objects.

	XMP_IO ( const XMP_IO & original );
	void operator= ( const XMP_IO& in ) { *this = in; /* Avoid Win compile warnings. */ };

};

#endif	// __XMP_IO_hpp__
