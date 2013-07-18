// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "XMPFiles/source/FormatSupport/WAVE/CartMetadata.h"

#include "source/EndianUtils.hpp"

using namespace IFF_RIFF;

// =============================================================================================

// Types and globals for the stored form of the cart chunk.

#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( 1 )
#else
#pragma pack ( push, 1 )
#endif //#if SUNOS_SPARC || SUNOS_X86

struct StoredCartChunk {
	char Version[4];	// All of the fixed size text fields are null-filled local text,
	char Title[64];		//	but need not have a terminating null.
	char Artist[64];
	char CutID[64];
	char ClientID[64];
	char Category[64];
	char Classification[64];
	char OutCue[64];
	char StartDate[10];
	char StartTime[8];
	char EndDate[10];
	char EndTime[8];
	char ProducerAppID[64];
	char ProducerAppVersion[64];
	char UserDef[64];
	XMP_Int32 LevelReference;		// Little endian in the file.
	CartMetadata::StoredCartTimer PostTimer[CartMetadata::kPostTimerLength];
	char Reserved[276];
	char URL[1024];
	// char TagText[];	- fills the tail of the chunk
};

static const size_t kMinimumCartChunkSize = sizeof(StoredCartChunk);

#if SUNOS_SPARC || SUNOS_X86
#pragma pack ( )
#else
#pragma pack ( pop )
#endif //#if SUNOS_SPARC || SUNOS_X86

static const size_t kFixedTextCount = (CartMetadata::kLastFixedTextField - CartMetadata::kFirstFixedTextField + 1);

struct FixedTextFieldInfo {
	size_t length;
	size_t offset;
};

static const FixedTextFieldInfo kFixedTextFields [kFixedTextCount] = {
	// ! The order of the items must match the order of the mapped field enum in the header.
	{  4, offsetof ( StoredCartChunk, Version ) },
	{ 64, offsetof ( StoredCartChunk, Title ) },
	{ 64, offsetof ( StoredCartChunk, Artist ) },
	{ 64, offsetof ( StoredCartChunk, CutID ) },
	{ 64, offsetof ( StoredCartChunk, ClientID ) },
	{ 64, offsetof ( StoredCartChunk, Category ) },
	{ 64, offsetof ( StoredCartChunk, Classification ) },
	{ 64, offsetof ( StoredCartChunk, OutCue ) },
	{ 10, offsetof ( StoredCartChunk, StartDate ) },
	{  8, offsetof ( StoredCartChunk, StartTime ) },
	{ 10, offsetof ( StoredCartChunk, EndDate ) },
	{  8, offsetof ( StoredCartChunk, EndTime ) },
	{ 64, offsetof ( StoredCartChunk, ProducerAppID ) },
	{ 64, offsetof ( StoredCartChunk, ProducerAppVersion ) },
	{ 64, offsetof ( StoredCartChunk, UserDef ) },
	{ 1024, offsetof ( StoredCartChunk, URL ) }
};

// =================================================================================================

CartMetadata::CartMetadata() {
	// Nothing to do.
}

// =================================================================================================

CartMetadata::~CartMetadata() {
	// Nothing to do.
}

// =================================================================================================

static size_t FindZeroByte ( const char * textPtr, size_t maxLength ) {
	// Return the offset of the first zero byte, up to maxLength.
	size_t length = 0;
	while ( (length < maxLength) && (textPtr[length] != 0) ) ++length;
	return length;
}

// =================================================================================================

void CartMetadata::parse ( const XMP_Uns8* chunkData, XMP_Uns64 chunkSize ) 
{
	// Make sure the chunk has a reasonable size.
	if ( chunkSize > 1000*1000*1000 )
	{
		XMP_Throw ( "Not a valid Cart chunk", kXMPErr_BadFileFormat );
	}
	
	StoredCartChunk* fileChunk;

	// If the chunk is too small, copy and pad with zeros
	if ( chunkSize < kMinimumCartChunkSize )
	{
		fileChunk = new StoredCartChunk;
		memset( fileChunk, 0, kMinimumCartChunkSize );
		memcpy( fileChunk, chunkData, static_cast<size_t>(chunkSize) ); // AUDIT: safe, chunkSize is smaller than buffer
	}
	else
	{
		fileChunk = (StoredCartChunk*)chunkData;
	}

	try
	{
		std::string localStr;
	
		// Extract the binary LevelReference field.

		this->setValue<XMP_Int32> ( kLevelReference, (XMP_Int32) GetUns32LE ( &fileChunk->LevelReference ) );
	
		// Extract the PostTimer Array
		// first ensure the correct endianess
		StoredCartTimer timerArray[CartMetadata::kPostTimerLength];
		for ( XMP_Uns32 i = 0; i < CartMetadata::kPostTimerLength; i++ )
		{
			timerArray[i].usage = GetUns32BE( &fileChunk->PostTimer[i].usage ); 
			timerArray[i].value = GetUns32LE( &fileChunk->PostTimer[i].value );
		}
		this->setArray<StoredCartTimer> (kPostTimer, timerArray, CartMetadata::kPostTimerLength);

		// Extract the trailing TagText portion, if any. Keep the local encoding, the conversion to
		// Unicode is done later when importing to XMP.
	
		if ( chunkSize > kMinimumCartChunkSize ) {
	
			const char * tagTextPtr = (char*)fileChunk + sizeof(StoredCartChunk);
			const size_t trailerSize = (size_t)chunkSize - kMinimumCartChunkSize;
			const size_t tagTextSize = FindZeroByte ( tagTextPtr, trailerSize );
			localStr.assign ( tagTextPtr, tagTextSize );
			this->setValue<std::string> ( kTagText, localStr );
	
		}
	
		// Extract the fixed length text fields. Keep the local encoding, the conversion to Unicode is
		// done later when importing to XMP.
	
		for ( int i = CartMetadata::kFirstFixedTextField; i <= CartMetadata::kLastFixedTextField; ++i ) {
	
			const FixedTextFieldInfo& currField = kFixedTextFields[i];
			const char * textPtr = (char*)fileChunk + currField.offset;
			size_t textLen = FindZeroByte ( textPtr, currField.length );
			if ( textLen > 0 ) {
				localStr.assign ( textPtr, textLen );
				this->setValue<std::string> ( i, localStr );
			}
	
		}

		this->resetChanges();
	}
	catch ( ... ) // setValue/setArray might throw
	{
		// If the chunk had been too small, it has been copied to a new buffer padded with zeros
		if ( chunkSize < kMinimumCartChunkSize )
		{
			delete fileChunk;
		}
		throw;
	}
	
	if ( chunkSize < kMinimumCartChunkSize )
	{
		delete fileChunk;
	}

}	// CartMetadata::parse

// =================================================================================================

XMP_Uns64 CartMetadata::serialize ( XMP_Uns8** outBuffer )
{

	if ( outBuffer == NULL )
	{
		XMP_Throw ( "Invalid buffer", kXMPErr_InternalFailure );
	}

	*outBuffer = 0;	// Default in case of early exit.
	
	// Set up the output buffer.
	
	size_t trailerSize = 0;
	std::string tagTextStr;
	if ( this->valueExists ( kTagText ) ) {
		tagTextStr = this->getValue<std::string> ( kTagText );
		trailerSize = tagTextStr.size() + 1;	// Include the final nul.
	}
	
	size_t chunkSize = sizeof(StoredCartChunk) + trailerSize;
	XMP_Uns8* buffer = new XMP_Uns8 [ chunkSize ];
	if ( buffer == 0 ) XMP_Throw ( "Cannot allocate cart chunk buffer", kXMPErr_NoMemory );
	memset ( buffer, 0, chunkSize );	// Fill with zeros for missing or short fields.
	
	StoredCartChunk* newChunk = (StoredCartChunk*)buffer;

	// Insert the binary LevelReference field.
	
	if ( this->valueExists ( kLevelReference ) ) {
		newChunk->LevelReference = MakeUns32LE ( (XMP_Uns32) this->getValue<XMP_Int32> ( kLevelReference ) );
	}

	// Insert the PostTimer Array
	if ( this->valueExists ( kPostTimer ) ) {
		XMP_Uns32 size = 0;
		const StoredCartTimer* timerArray = this->getArray<StoredCartTimer> ( kPostTimer, size );
		XMP_Assert (size == CartMetadata::kPostTimerLength );

		for ( XMP_Uns32 i = 0; i<CartMetadata::kPostTimerLength; i++ )
		{
			newChunk->PostTimer[i].usage = MakeUns32BE(timerArray[i].usage); // ensure the FOURCC is written in Big Endian
			newChunk->PostTimer[i].value = MakeUns32LE(timerArray[i].value); // ensure the value is written as Little Endian
		}
	}

	// Insert the trailing TagText portion, if any. The conversion to local encoding should have
	// been done when exporting from XMP. Include the final nul.

	if ( ! tagTextStr.empty() ) {
		XMP_Uns8* tagTextPtr = buffer + sizeof(StoredCartChunk);
		strncpy ( (char*)tagTextPtr, tagTextStr.c_str(), trailerSize );	// AUDIT: Safe, see prior allocation.
	}
	
	// Insert the fixed length text fields. The conversion to local encoding should have been done
	// when exporting from XMP.

	std::string currStr;
	
	for ( int i = CartMetadata::kFirstFixedTextField; i <= CartMetadata::kLastFixedTextField; ++i ) {
	
		if ( ! this->valueExists ( i ) ) continue;
		const FixedTextFieldInfo& currField = kFixedTextFields[i];

		currStr = this->getValue<std::string> ( i );
		if ( currStr.empty() ) continue;
		if ( currStr.size() > currField.length ) currStr.erase ( currField.length );
		XMP_Assert ( currStr.size() <= currField.length );
		
		strncpy ( (char*)(buffer + currField.offset), currStr.c_str(), currStr.size() );	// 	// AUDIT: Safe, within fixed bounds.
	
	}

	// Done.
	
	*outBuffer = buffer;
	return chunkSize;

}	// CartMetadata::serialize

// =================================================================================================

bool CartMetadata::isEmptyValue ( XMP_Uns32 id, ValueObject& valueObj ) {

	bool isEmpty = true;
	
	switch( id )
	{
		case kLevelReference:
		{
			//XMP_Int32
			TValueObject<XMP_Int32>* binObj = dynamic_cast<TValueObject<XMP_Int32>*>(&valueObj);
			isEmpty = ( binObj == 0 );	// All values are valid.
		}
		break;
		
		case kPostTimer:
		{
			// Array[8]*2
			TArrayObject<StoredCartTimer>* obj = dynamic_cast<TArrayObject<StoredCartTimer>*>(&valueObj);

			if( obj != NULL )
			{
				XMP_Uns32 size	 = 0;
				const StoredCartTimer* const buffer = obj->getArray( size );

				isEmpty = ( size == 0 );
			}
		}
		break;

		default:
		{
			// String values
			TValueObject<std::string>* strObj = dynamic_cast<TValueObject<std::string>*>(&valueObj);
			isEmpty = ( (strObj == 0) || strObj->getValue().empty() ); 
		}
		break;
	}
	
	return isEmpty;

}	// CartMetadata::isEmptyValue

// =================================================================================================
