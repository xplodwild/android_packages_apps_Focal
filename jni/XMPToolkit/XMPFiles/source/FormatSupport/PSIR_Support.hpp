#ifndef __PSIR_Support_hpp__
#define __PSIR_Support_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"

#include "source/EndianUtils.hpp"
#include "source/XMP_ProgressTracker.hpp"

#include <map>

// =================================================================================================
/// \file PSIR_Support.hpp
/// \brief XMPFiles support for Photoshop image resources.
///
/// This header provides Photoshop image resource (PSIR) support specific to the needs of XMPFiles.
/// This is not intended for general purpose PSIR processing. PSIR_Manager is an abstract base
/// class with 2 concrete derived classes, PSIR_MemoryReader and PSIR_FileWriter.
///
/// PSIR_MemoryReader provides read-only support for PSIR streams that are small enough to be kept
/// entirely in memory. This allows optimizations to reduce heap usage and processing code. It is
/// sufficient for browsing access to the image resources (mainly the IPTC) in JPEG files. Think of
/// PSIR_MemoryReader as "memory-based AND read-only".
///
/// PSIR_FileWriter is for cases where updates are needed or the PSIR stream is too large to be kept
/// entirely in memory. Think of PSIR_FileWriter as "file-based OR read-write".
///
/// The needs of XMPFiles are well defined metadata access. Only a few image resources are handled.
/// This is the case for all of the derived classes, even though the memory based ones happen to
/// have all of the image resources in memory. Being "handled" means being in the image resource
/// map used by GetImgRsrc. The handled image resources are:
/// \li 1028 - IPTC
/// \li 1034 - Copyrighted flag
/// \li 1035 - Copyright information URL
/// \li 1058 - Exif metadata
/// \li 1060 - XMP
/// \li 1061 - IPTC digest
///
/// \note These classes are for use only when directly compiled and linked. They should not be
/// packaged in a DLL by themselves. They do not provide any form of C++ ABI protection.
// =================================================================================================


// These aren't inside PSIR_Manager because the static array can't be initialized there.

enum {
	k8BIM = 0x3842494DUL,		// The 4 ASCII characters '8BIM'.
	kMinImgRsrcSize = 4+2+2+4	// The minimum size for an image resource.
};

enum {
	kPSIR_IPTC           = 1028,
	kPSIR_CopyrightFlag  = 1034,
	kPSIR_CopyrightURL   = 1035,
	kPSIR_Exif           = 1058,
	kPSIR_XMP            = 1060,
	kPSIR_IPTCDigest     = 1061
};

enum { kPSIR_MetadataCount = 6 };
static const XMP_Uns16 kPSIR_MetadataIDs[] =	// ! Must be in descending order with 0 sentinel.
	{ kPSIR_IPTCDigest, kPSIR_XMP, kPSIR_Exif, kPSIR_CopyrightURL, kPSIR_CopyrightFlag, kPSIR_IPTC, 0 };

// =================================================================================================
// =================================================================================================

// NOTE: Although Photoshop image resources have a type and ID, for metadatya we only care about
// those of type "8BIM". Resources of other types are preserved in files, but can't be individually
// accessed through the PSIR_Manager API.

// =================================================================================================
// PSIR_Manager
// ============

class PSIR_Manager {	// The abstract base class.
public:

	// ---------------------------------------------------------------------------------------------
	// Types and constants

	struct ImgRsrcInfo {
		XMP_Uns16   id;
		XMP_Uns32   dataLen;
		const void* dataPtr;	// ! The data is read-only!
		XMP_Uns32   origOffset;	// The offset (at parse time) of the resource data.
		ImgRsrcInfo() : id(0), dataLen(0), dataPtr(0), origOffset(0) {};
		ImgRsrcInfo ( XMP_Uns16 _id, XMP_Uns32 _dataLen, void* _dataPtr, XMP_Uns32 _origOffset )
			: id(_id), dataLen(_dataLen), dataPtr(_dataPtr), origOffset(_origOffset) {};
	};

	// The origOffset is the absolute file offset for file parses, the memory block offset for
	// memory parses. It is the offset of the resource data portion, not the overall resource.

	// ---------------------------------------------------------------------------------------------
	// Get the information about a "handled" image resource. Returns false if the image resource is
	// not handled, even if it was present in the parsed input.

	virtual bool GetImgRsrc ( XMP_Uns16 id, ImgRsrcInfo* info ) const = 0;

	// ---------------------------------------------------------------------------------------------
	// Set the value for an image resource. It can be any resource, even one not originally handled.

	virtual void SetImgRsrc ( XMP_Uns16 id, const void* dataPtr, XMP_Uns32 length ) = 0;

	// ---------------------------------------------------------------------------------------------
	// Delete an image resource. Does nothing if the image resource does not exist.

	virtual void DeleteImgRsrc ( XMP_Uns16 id ) = 0;

	// ---------------------------------------------------------------------------------------------
	// Determine if the image resources are changed.

	virtual bool IsChanged() = 0;
	virtual bool IsLegacyChanged() = 0;

	// ---------------------------------------------------------------------------------------------

	virtual void ParseMemoryResources ( const void* data, XMP_Uns32 length, bool copyData = true ) = 0;
	virtual void ParseFileResources   ( XMP_IO* fileRef, XMP_Uns32 length ) = 0;

	// ---------------------------------------------------------------------------------------------
	// Update the image resources to reflect the changed values. Both \c UpdateMemoryResources and
	// \c UpdateFileResources return the new size of the image resource block. The dataPtr returned
	// by \c UpdateMemoryResources must be treated as read only. It exists until the PSIR_Manager
	// destructor is called. UpdateMemoryResources can be used on a read-only instance to get the
	// raw data block info.

	virtual XMP_Uns32 UpdateMemoryResources ( void** dataPtr ) = 0;
	virtual XMP_Uns32 UpdateFileResources   ( XMP_IO* sourceRef, XMP_IO* destRef,
											  XMP_AbortProc abortProc, void * abortArg,
											  XMP_ProgressTracker* progressTracker ) = 0;

	// ---------------------------------------------------------------------------------------------

	virtual ~PSIR_Manager() {};

protected:

	PSIR_Manager() {};

};	// PSIR_Manager


// =================================================================================================
// =================================================================================================


// =================================================================================================
// PSIR_MemoryReader
// =================

class PSIR_MemoryReader : public PSIR_Manager {	// The leaf class for memory-based read-only access.
public:

	bool GetImgRsrc ( XMP_Uns16 id, ImgRsrcInfo* info ) const;

	void SetImgRsrc ( XMP_Uns16 id, const void* dataPtr, XMP_Uns32 length ) { NotAppropriate(); };

	void DeleteImgRsrc ( XMP_Uns16 id ) { NotAppropriate(); };

	bool IsChanged() { return false; };
	bool IsLegacyChanged() { return false; };

	void ParseMemoryResources ( const void* data, XMP_Uns32 length, bool copyData = true );
	void ParseFileResources   ( XMP_IO* file, XMP_Uns32 length ) { NotAppropriate(); };

	XMP_Uns32 UpdateMemoryResources ( void** dataPtr ) { if ( dataPtr != 0 ) *dataPtr = psirContent; return psirLength; };
	XMP_Uns32 UpdateFileResources ( XMP_IO* sourceRef, XMP_IO* destRef,
									XMP_AbortProc abortProc, void * abortArg,
									XMP_ProgressTracker* progressTracker ) { NotAppropriate(); return 0; };

	PSIR_MemoryReader() : ownedContent(false), psirLength(0), psirContent(0) {};

	virtual ~PSIR_MemoryReader() { if ( this->ownedContent ) free ( this->psirContent ); };

private:

	// Memory usage notes: PSIR_MemoryReader is for memory-based read-only usage (both apply). There
	// is no need to ever allocate separate blocks of memory, everything is used directly from the
	// PSIR stream.

	bool ownedContent;

	XMP_Uns32 psirLength;
	XMP_Uns8* psirContent;

	typedef std::map<XMP_Uns16,ImgRsrcInfo>  ImgRsrcMap;

	ImgRsrcMap imgRsrcs;

	static inline void NotAppropriate() { XMP_Throw ( "Not appropriate for PSIR_Reader", kXMPErr_InternalFailure ); };

};	// PSIR_MemoryReader


// =================================================================================================
// =================================================================================================


// =================================================================================================
// PSIR_FileWriter
// ===============

class PSIR_FileWriter : public PSIR_Manager {	// The leaf class for file-based read-write access.
public:

	bool GetImgRsrc ( XMP_Uns16 id, ImgRsrcInfo* info ) const;

	void SetImgRsrc ( XMP_Uns16 id, const void* dataPtr, XMP_Uns32 length );

	void DeleteImgRsrc ( XMP_Uns16 id );

	bool IsChanged() { return this->changed; };

	bool IsLegacyChanged();

	void ParseMemoryResources ( const void* data, XMP_Uns32 length, bool copyData = true );
	void ParseFileResources   ( XMP_IO* file, XMP_Uns32 length );

	XMP_Uns32 UpdateMemoryResources ( void** dataPtr );
	XMP_Uns32 UpdateFileResources   ( XMP_IO* sourceRef, XMP_IO* destRef,
									  XMP_AbortProc abortProc, void * abortArg,
									  XMP_ProgressTracker* progressTracker );

	PSIR_FileWriter() : changed(false), legacyDeleted(false), memParsed(false), fileParsed(false),
						ownedContent(false), memLength(0), memContent(0) {};

	virtual ~PSIR_FileWriter();

	// Memory usage notes: PSIR_FileWriter is for file-based OR read/write usage. For memory-based
	// streams the dataPtr and rsrcName are initially into the stream. The dataPtr becomes a
	// separate allocation when SetImgRsrc is called, the rsrcName stays into the original stream.
	// For file-based streams the dataPtr and rsrcName are always a separate allocation. Again,
	// the dataPtr changes when SetImgRsrc is called, the rsrcName stays unchanged.

	// ! The working data values are always big endian, no matter where stored. It is the client's
	// ! responsibility to flip them as necessary.

	static const bool kIsFileBased   = true;	// For use in the InternalRsrcInfo constructor.
	static const bool kIsMemoryBased = false;

	struct InternalRsrcInfo {
	public:

		bool      changed;
		bool      fileBased;
		XMP_Uns16 id;
		XMP_Uns32 dataLen;
		void*     dataPtr;		// ! Null if the value is not captured!
		XMP_Uns32 origOffset;	// The offset (at parse time) of the resource data.
		XMP_Uns8* rsrcName;		// ! A Pascal string, leading length byte, no nul terminator!

		inline void FreeData() {
			if ( this->fileBased || this->changed ) {
				if ( this->dataPtr != 0 ) { free ( this->dataPtr ); this->dataPtr = 0; }
			}
		}

		inline void FreeName() {
			if ( this->fileBased && (this->rsrcName != 0) ) {
				free ( this->rsrcName );
				this->rsrcName = 0;
			}
		}

		InternalRsrcInfo ( XMP_Uns16 _id, XMP_Uns32 _dataLen, bool _fileBased )
			: changed(false), fileBased(_fileBased), id(_id), dataLen(_dataLen), dataPtr(0),
			  origOffset(0), rsrcName(0) {};
		~InternalRsrcInfo() { this->FreeData(); this->FreeName(); };

		void operator= ( const InternalRsrcInfo & in )
		{
			// ! Gag! Transfer ownership of the dataPtr and rsrcName!
			this->FreeData();
			memcpy ( this, &in, sizeof(*this) );	// AUDIT: Use of sizeof(InternalRsrcInfo) is safe.
			*((void**)&in.dataPtr) = 0;		// The pointer is now owned by "this".
			*((void**)&in.rsrcName) = 0;	// The pointer is now owned by "this".
		};

	private:

		InternalRsrcInfo()	// Hidden on purpose, fileBased must be properly set.
			: changed(false), fileBased(false), id(0), dataLen(0), dataPtr(0), origOffset(0), rsrcName(0) {};

	};

	// The origOffset is the absolute file offset for file parses, the memory block offset for
	// memory parses. It is the offset of the resource data portion, not the overall resource.

private:

	bool changed, legacyDeleted;
	bool memParsed, fileParsed;
	bool ownedContent;

	XMP_Uns32 memLength;
	XMP_Uns8* memContent;

	typedef std::map<XMP_Uns16,InternalRsrcInfo>  InternalRsrcMap;
	InternalRsrcMap imgRsrcs;

	struct OtherRsrcInfo {		// For the resources of types other than "8BIM".
		XMP_Uns32 rsrcOffset;	// The offset of the resource origin, the type field.
		XMP_Uns32 rsrcLength;	// The full length of the resource, offset to the next resource.
		OtherRsrcInfo() : rsrcOffset(0), rsrcLength(0) {};
		OtherRsrcInfo ( XMP_Uns32 _rsrcOffset, XMP_Uns32 _rsrcLength )
			: rsrcOffset(_rsrcOffset), rsrcLength(_rsrcLength) {};
	};
	std::vector<OtherRsrcInfo> otherRsrcs;

	void DeleteExistingInfo();

};	// PSIR_FileWriter

#endif	// __PSIR_Support_hpp__
