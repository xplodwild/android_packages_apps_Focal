// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _CartMetadata_h_
#define _CartMetadata_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/NativeMetadataSupport/IMetadata.h"

namespace IFF_RIFF {

// =================================================================================================

class CartMetadata : public IMetadata {

public:

	enum {

		// IDs for the mapped cart chunk fields.
		kVersion,				// Local text, max 4 bytes
		kTitle,					// Local text, max 64 bytes
		kArtist,				// Local text, max 64 bytes
		kCutID,					// Local text, max 64 bytes
		kClientID,				// Local text, max 64 bytes
		kCategory,				// Local text, max 64 bytes
		kClassification,		// Local text, max 64 bytes
		kOutCue,				// Local text, max 64 bytes
		kStartDate,				// Local text, max 10 bytes
		kStartTime,				// Local text, max 8 bytes
		kEndDate,				// Local text, max 10 bytes
		kEndTime,				// Local text, max 8 bytes
		kProducerAppID,			// Local text, max 64 bytes
		kProducerAppVersion,	// Local text, max 64 bytes
		kUserDef,				// Local text, max 64 bytes
		kURL,					// Local text, max 1024 bytes
		kTagText,				// Local text, no limit
		kLevelReference,		// Little endian unsigned 32
		kPostTimer,				// array[8] of usage(Uns32), value(Uns32)
		kReserved,				// 276 reserved bytes

		// Constants for the range of fixed length text fields.
		kFirstFixedTextField = kVersion,
		kLastFixedTextField  = kURL

	};

	struct StoredCartTimer {
		XMP_Uns32 usage;
		XMP_Uns32 value;
	};

	enum { kPostTimerLength = 8 };

	CartMetadata();
	virtual ~CartMetadata();

	void parse ( const XMP_Uns8* chunkData, XMP_Uns64 chunkSize );
	void parse ( XMP_IO* input ) { IMetadata::parse ( input ); }
	
	XMP_Uns64 serialize ( XMP_Uns8** buffer );

protected:

	virtual	bool isEmptyValue ( XMP_Uns32 id, ValueObject& valueObj );

private:

	// Operators hidden on purpose.
	CartMetadata ( const CartMetadata& ) {};
	CartMetadata& operator= ( const CartMetadata& ) { return *this; };

};	// CartMetadata

}	// namespace IFF_RIFF

#endif
