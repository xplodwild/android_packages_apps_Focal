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

#include "XMPFiles/source/FormatSupport/ASF_Support.hpp"
#include "source/UnicodeConversions.hpp"
#include "source/XIO.hpp"

#if XMP_WinBuild
	#define snprintf _snprintf
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
	#pragma warning ( disable : 4267 )	// *** conversion (from size_t), possible loss of date (many 64 bit related)
#endif

// =============================================================================================

// Platforms other than Win
#if ! XMP_WinBuild
int IsEqualGUID ( const GUID& guid1, const GUID& guid2 )
{
	return (memcmp ( &guid1, &guid2, sizeof(GUID) ) == 0);
}
#endif

ASF_Support::ASF_Support() : legacyManager(0),progressTracker(0), posFileSizeInfo(0) {}

ASF_Support::ASF_Support ( ASF_LegacyManager* _legacyManager,XMP_ProgressTracker* _progressTracker ) 
	:legacyManager(_legacyManager),progressTracker(_progressTracker), posFileSizeInfo(0)
{
}

ASF_Support::~ASF_Support()
{
	legacyManager = 0;
}

// =============================================================================================

long ASF_Support::OpenASF ( XMP_IO* fileRef, ObjectState & inOutObjectState )
{
	XMP_Uns64 pos = 0;
	XMP_Uns64 len;

	try {
		pos = fileRef->Rewind();
	} catch ( ... ) {}

	if ( pos != 0 ) return 0;

	// read first and following chunks
	while ( ReadObject ( fileRef, inOutObjectState, &len, pos) ) {}

	return inOutObjectState.objects.size();

}

// =============================================================================================

bool ASF_Support::ReadObject ( XMP_IO* fileRef, ObjectState & inOutObjectState, XMP_Uns64 * objectLength, XMP_Uns64 & inOutPosition )
{

	try {

		XMP_Uns64 startPosition = inOutPosition;
		XMP_Uns32 bytesRead;
		ASF_ObjectBase objectBase;

		bytesRead = fileRef->ReadAll ( &objectBase, kASF_ObjectBaseLen );
		if ( bytesRead != kASF_ObjectBaseLen ) return false;

		*objectLength = GetUns64LE ( &objectBase.size );
		inOutPosition += *objectLength;

		ObjectData	newObject;

		newObject.pos = startPosition;
		newObject.len = *objectLength;
		newObject.guid = objectBase.guid;

		// xmpIsLastObject indicates, that the XMP-object is the last top-level object
		// reset here, if any another object is read
		inOutObjectState.xmpIsLastObject = false;

		if ( IsEqualGUID ( ASF_Header_Object, newObject.guid ) ) {

			// header object ?
			this->ReadHeaderObject ( fileRef, inOutObjectState, newObject );

		} else if ( IsEqualGUID ( ASF_XMP_Metadata, newObject.guid ) ) {

			// check object for XMP GUID
			inOutObjectState.xmpPos = newObject.pos + kASF_ObjectBaseLen;
			inOutObjectState.xmpLen = newObject.len - kASF_ObjectBaseLen;
			inOutObjectState.xmpIsLastObject = true;
			inOutObjectState.xmpObject = newObject;
			newObject.xmp = true;

		}

		inOutObjectState.objects.push_back ( newObject );

		fileRef->Seek ( inOutPosition, kXMP_SeekFromStart );

	} catch ( ... ) {

		return false;

	}

	return true;

}

// =============================================================================================

bool ASF_Support::ReadHeaderObject ( XMP_IO* fileRef, ObjectState& inOutObjectState, const ObjectData& newObject )
{
	if ( ! IsEqualGUID ( ASF_Header_Object, newObject.guid) || (! legacyManager ) ) return false;

	std::string buffer;

	legacyManager->SetPadding(0);

	try {

		// read header-object structure
		XMP_Uns64 pos = newObject.pos;
		XMP_Uns32 bufferSize = kASF_ObjectBaseLen + 6;

		buffer.clear();
		buffer.reserve ( bufferSize );
		buffer.assign ( bufferSize, ' ' );
		fileRef->Seek ( pos, kXMP_SeekFromStart );
		fileRef->ReadAll ( const_cast<char*>(buffer.data()), bufferSize );

		XMP_Uns64 read = bufferSize;
		pos += bufferSize;

		// read contained header objects
		XMP_Uns32 numberOfHeaders = GetUns32LE ( &buffer[24] );
		ASF_ObjectBase objectBase;

		while ( read < newObject.len ) {

			fileRef->Seek ( pos, kXMP_SeekFromStart );
			if ( kASF_ObjectBaseLen != fileRef->Read ( &objectBase, kASF_ObjectBaseLen, true ) ) break;

			fileRef->Seek ( pos, kXMP_SeekFromStart );
			objectBase.size = GetUns64LE ( &objectBase.size );

			if ( IsEqualGUID ( ASF_File_Properties_Object, objectBase.guid) && (objectBase.size >= 104 ) ) {

				buffer.clear();
				buffer.reserve ( XMP_Uns32( objectBase.size ) );
				buffer.assign ( XMP_Uns32( objectBase.size ), ' ' );
				fileRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );

				// save position of filesize-information
				posFileSizeInfo = (pos + 40);

				// creation date
				std::string sub ( buffer.substr ( 48, 8 ) );
				legacyManager->SetField ( ASF_LegacyManager::fieldCreationDate, sub );

				// broadcast flag set ?
				XMP_Uns32 flags = GetUns32LE ( &buffer[88] );
				inOutObjectState.broadcast = (flags & 1);
				legacyManager->SetBroadcast ( inOutObjectState.broadcast );

				legacyManager->SetObjectExists ( ASF_LegacyManager::objectFileProperties );

			} else if ( IsEqualGUID ( ASF_Content_Description_Object, objectBase.guid) && (objectBase.size >= 34 ) ) {

				buffer.clear();
				buffer.reserve ( XMP_Uns32( objectBase.size ) );
				buffer.assign ( XMP_Uns32( objectBase.size ), ' ' );
				fileRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );

				XMP_Uns16 titleLen = GetUns16LE ( &buffer[24] );
				XMP_Uns16 authorLen = GetUns16LE ( &buffer[26] );
				XMP_Uns16 copyrightLen = GetUns16LE ( &buffer[28] );
				XMP_Uns16 descriptionLen = GetUns16LE ( &buffer[30] );
				XMP_Uns16 ratingLen = GetUns16LE ( &buffer[32] );

				XMP_Uns16 fieldPos = 34;

				std::string titleStr = buffer.substr ( fieldPos, titleLen );
				fieldPos += titleLen;
				legacyManager->SetField ( ASF_LegacyManager::fieldTitle, titleStr );

				std::string authorStr = buffer.substr ( fieldPos, authorLen );
				fieldPos += authorLen;
				legacyManager->SetField ( ASF_LegacyManager::fieldAuthor, authorStr );

				std::string copyrightStr = buffer.substr ( fieldPos, copyrightLen );
				fieldPos += copyrightLen;
				legacyManager->SetField ( ASF_LegacyManager::fieldCopyright, copyrightStr );

				std::string descriptionStr = buffer.substr ( fieldPos, descriptionLen );
				fieldPos += descriptionLen;
				legacyManager->SetField ( ASF_LegacyManager::fieldDescription, descriptionStr );

				/* rating is currently not part of reconciliation
				std::string ratingStr = buffer.substr ( fieldPos, ratingLen );
				fieldPos += ratingLen;
				legacyData.append ( titleStr );
				*/

				legacyManager->SetObjectExists ( ASF_LegacyManager::objectContentDescription );

			} else if ( IsEqualGUID ( ASF_Content_Branding_Object, objectBase.guid ) ) {

				buffer.clear();
				buffer.reserve ( XMP_Uns32( objectBase.size ) );
				buffer.assign ( XMP_Uns32( objectBase.size ), ' ' );
				fileRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );

				XMP_Uns32 fieldPos = 28;

				// copyright URL is 3. element with variable size
				for ( int i = 1; i <= 3 ; ++i ) {
					XMP_Uns32 len = GetUns32LE ( &buffer[fieldPos] );
					if ( i == 3 ) {
						std::string copyrightURLStr = buffer.substr ( fieldPos + 4, len );
						legacyManager->SetField ( ASF_LegacyManager::fieldCopyrightURL, copyrightURLStr );
					}
					fieldPos += (len + 4);
				}

				legacyManager->SetObjectExists ( ASF_LegacyManager::objectContentBranding );

#if ! Exclude_LicenseURL_Recon

			} else if ( IsEqualGUID ( ASF_Content_Encryption_Object, objectBase.guid ) ) {

				buffer.clear();
				buffer.reserve ( XMP_Uns32( objectBase.size ) );
				buffer.assign ( XMP_Uns32( objectBase.size ), ' ' );
				fileRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );

				XMP_Uns32 fieldPos = 24;

				// license URL is 4. element with variable size
				for ( int i = 1; i <= 4 ; ++i ) {
					XMP_Uns32 len = GetUns32LE ( &buffer[fieldPos] );
					if ( i == 4 ) {
						std::string licenseURLStr = buffer.substr ( fieldPos + 4, len );
						legacyManager->SetField ( ASF_LegacyManager::fieldLicenseURL, licenseURLStr );
					}
					fieldPos += (len + 4);
				}

				legacyManager->SetObjectExists ( objectContentEncryption );

#endif

			} else if ( IsEqualGUID ( ASF_Padding_Object, objectBase.guid ) ) {

				legacyManager->SetPadding ( legacyManager->GetPadding() + (objectBase.size - 24) );

			} else if ( IsEqualGUID ( ASF_Header_Extension_Object, objectBase.guid ) ) {

				this->ReadHeaderExtensionObject ( fileRef, inOutObjectState, pos, objectBase );

			}

			pos += objectBase.size;
			read += objectBase.size;
		}

	} catch ( ... ) {

		return false;

	}

	legacyManager->ComputeDigest();

	return true;
}

// =============================================================================================

bool ASF_Support::WriteHeaderObject ( XMP_IO* sourceRef, XMP_IO* destRef, const ObjectData& object, ASF_LegacyManager& _legacyManager, bool usePadding )
{
	if ( ! IsEqualGUID ( ASF_Header_Object, object.guid ) ) return false;

	std::string buffer;
	XMP_Uns16 valueUns16LE;
	XMP_Uns32 valueUns32LE;
	XMP_Uns64 valueUns64LE;

	try {

		// read header-object structure
		XMP_Uns64 pos = object.pos;
		XMP_Uns32 bufferSize = kASF_ObjectBaseLen + 6;

		buffer.clear();
		buffer.reserve ( bufferSize );
		buffer.assign ( bufferSize, ' ' );
		sourceRef->Seek ( pos, kXMP_SeekFromStart );
		sourceRef->ReadAll ( const_cast<char*>(buffer.data()), bufferSize );

		XMP_Uns64 read = bufferSize;
		pos += bufferSize;

		// read contained header objects
		XMP_Uns32 numberOfHeaders = GetUns32LE ( &buffer[24] );
		ASF_ObjectBase objectBase;

		// prepare new header in memory
		std::string header;

		int changedObjects = _legacyManager.changedObjects();
		int exportedObjects = 0;
		int writtenObjects = 0;

		header.append ( buffer.c_str(), bufferSize );

		while ( read < object.len ) {

			sourceRef->Seek ( pos, kXMP_SeekFromStart );
			if ( kASF_ObjectBaseLen != sourceRef->Read ( &objectBase, kASF_ObjectBaseLen, true ) ) break;

			sourceRef->Seek ( pos, kXMP_SeekFromStart );
			objectBase.size = GetUns64LE ( &objectBase.size );

			int headerStartPos = header.size();

			// save position of filesize-information
			if ( IsEqualGUID ( ASF_File_Properties_Object, objectBase.guid ) ) {
				posFileSizeInfo = (headerStartPos + 40);
			}

			// write objects
			if ( IsEqualGUID ( ASF_File_Properties_Object, objectBase.guid ) &&
				(objectBase.size >= 104) && (changedObjects & ASF_LegacyManager::objectFileProperties) ) {

				// copy object and replace creation-date
				buffer.reserve ( XMP_Uns32 ( objectBase.size ) );
				buffer.assign ( XMP_Uns32 ( objectBase.size ), ' ' );
				sourceRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );
				header.append ( buffer, 0, XMP_Uns32( objectBase.size ) );

				if ( ! _legacyManager.GetBroadcast() ) {
					buffer = _legacyManager.GetField ( ASF_LegacyManager::fieldCreationDate );
					ReplaceString ( header, buffer, (headerStartPos + 48), 8 );
				}

				exportedObjects |= ASF_LegacyManager::objectFileProperties;

			} else if ( IsEqualGUID ( ASF_Content_Description_Object, objectBase.guid ) &&
						(objectBase.size >= 34) && (changedObjects & ASF_LegacyManager::objectContentDescription) ) {

				// re-create object with xmp-data
				buffer.reserve ( XMP_Uns32( objectBase.size ) );
				buffer.assign ( XMP_Uns32( objectBase.size ), ' ' );
				sourceRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );
				// write header only 
				header.append ( buffer, 0, XMP_Uns32( kASF_ObjectBaseLen ) );

				// write length fields

				XMP_Uns16 titleLen = _legacyManager.GetField ( ASF_LegacyManager::fieldTitle).size( );
				valueUns16LE = MakeUns16LE ( titleLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				XMP_Uns16 authorLen = _legacyManager.GetField ( ASF_LegacyManager::fieldAuthor).size( );
				valueUns16LE = MakeUns16LE ( authorLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				XMP_Uns16 copyrightLen = _legacyManager.GetField ( ASF_LegacyManager::fieldCopyright).size( );
				valueUns16LE = MakeUns16LE ( copyrightLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				XMP_Uns16 descriptionLen = _legacyManager.GetField ( ASF_LegacyManager::fieldDescription).size( );
				valueUns16LE = MakeUns16LE ( descriptionLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				// retrieve existing overall length of preceding fields
				XMP_Uns16 precedingLen = 0;
				precedingLen += GetUns16LE ( &buffer[24] ); // Title
				precedingLen += GetUns16LE ( &buffer[26] ); // Author
				precedingLen += GetUns16LE ( &buffer[28] ); // Copyright
				precedingLen += GetUns16LE ( &buffer[30] ); // Description
				// retrieve existing 'Rating' length
				XMP_Uns16 ratingLen = GetUns16LE ( &buffer[32] ); // Rating
				valueUns16LE = MakeUns16LE ( ratingLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				// write field contents

				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldTitle ) );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldAuthor ) );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldCopyright ) );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldDescription ) );
				header.append ( buffer, (34 + precedingLen), ratingLen );

				// update new object size
				valueUns64LE = MakeUns64LE ( header.size() - headerStartPos );
				std::string newSize ( (const char*)&valueUns64LE, 8 );
				ReplaceString ( header, newSize, (headerStartPos + 16), 8 );

				exportedObjects |= ASF_LegacyManager::objectContentDescription;

			} else if ( IsEqualGUID ( ASF_Content_Branding_Object, objectBase.guid ) &&
						(changedObjects & ASF_LegacyManager::objectContentBranding) ) {

				// re-create object with xmp-data
				buffer.reserve ( XMP_Uns32( objectBase.size ) );
				buffer.assign ( XMP_Uns32( objectBase.size ), ' ' );
				sourceRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );

				// calculate size of fields coming before 'Copyright URL'
				XMP_Uns32 length = 28;
				length += (GetUns32LE ( &buffer[length] ) + 4); // Banner Image Data
				length += (GetUns32LE ( &buffer[length] ) + 4); // Banner Image URL

				// write first part of header
				header.append ( buffer, 0, length );

				// copyright URL
				length = _legacyManager.GetField ( ASF_LegacyManager::fieldCopyrightURL).size( );
				valueUns32LE = MakeUns32LE ( length );
				header.append ( (const char*)&valueUns32LE, 4 );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldCopyrightURL ) );

				// update new object size
				valueUns64LE = MakeUns64LE ( header.size() - headerStartPos );
				std::string newSize ( (const char*)&valueUns64LE, 8 );
				ReplaceString ( header, newSize, (headerStartPos + 16), 8 );

				exportedObjects |= ASF_LegacyManager::objectContentBranding;

#if ! Exclude_LicenseURL_Recon

			} else if ( IsEqualGUID ( ASF_Content_Encryption_Object, objectBase.guid ) &&
						(changedObjects & ASF_LegacyManager::objectContentEncryption) ) {

				// re-create object with xmp-data
				buffer.reserve ( XMP_Uns32( objectBase.size ) );
				buffer.assign ( XMP_Uns32( objectBase.size ), ' ' );
				sourceRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );

				// calculate size of fields coming before 'License URL'
				XMP_Uns32 length = 24;
				length += (GetUns32LE ( &buffer[length] ) + 4); // Secret Data
				length += (GetUns32LE ( &buffer[length] ) + 4); // Protection Type
				length += (GetUns32LE ( &buffer[length] ) + 4); // Key ID

				// write first part of header
				header.append ( buffer, 0, length );

				// License URL
				length = _legacyManager.GetField ( ASF_LegacyManager::fieldLicenseURL).size( );
				valueUns32LE = MakeUns32LE ( length );
				header.append ( (const char*)&valueUns32LE, 4 );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldLicenseURL ) );

				// update new object size
				valueUns64LE = MakeUns64LE ( header.size() - headerStartPos );
				std::string newSize ( (const char*)&valueUns64LE, 8 );
				ReplaceString ( header, newSize, (headerStartPos + 16), 8 );

				exportedObjects |= ASF_LegacyManager::objectContentEncryption;

#endif

			} else if ( IsEqualGUID ( ASF_Header_Extension_Object, objectBase.guid ) && usePadding ) {

				// re-create object if padding needs to be used
				buffer.reserve ( XMP_Uns32( objectBase.size ) );
				buffer.assign ( XMP_Uns32( objectBase.size ), ' ' );
				sourceRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );

				ASF_Support::WriteHeaderExtensionObject ( buffer, &header, objectBase, 0 );

			} else if ( IsEqualGUID ( ASF_Padding_Object, objectBase.guid ) && usePadding ) {

				// eliminate padding (will be created as last object)

			} else {

				// simply copy all other objects
				buffer.reserve ( XMP_Uns32( objectBase.size ) );
				buffer.assign ( XMP_Uns32( objectBase.size ), ' ' );
				sourceRef->ReadAll ( const_cast<char*>(buffer.data()), XMP_Int32(objectBase.size) );

				header.append ( buffer, 0, XMP_Uns32( objectBase.size ) );

			}

			pos += objectBase.size;
			read += objectBase.size;

			writtenObjects ++;

		}

		// any objects to create ?
		int newObjects = (changedObjects ^ exportedObjects);

		if ( newObjects ) {

			// create new objects with xmp-data
			int headerStartPos;
			ASF_ObjectBase newObjectBase;
			XMP_Uns32 length;

			if ( newObjects & ASF_LegacyManager::objectContentDescription ) {

				headerStartPos = header.size();
				newObjectBase.guid = ASF_Content_Description_Object;
				newObjectBase.size = 0;

				// write object header
				header.append ( (const char*)&newObjectBase, kASF_ObjectBaseLen );

				XMP_Uns16 titleLen = _legacyManager.GetField ( ASF_LegacyManager::fieldTitle).size( );
				valueUns16LE = MakeUns16LE ( titleLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				XMP_Uns16 authorLen = _legacyManager.GetField ( ASF_LegacyManager::fieldAuthor).size( );
				valueUns16LE = MakeUns16LE ( authorLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				XMP_Uns16 copyrightLen = _legacyManager.GetField ( ASF_LegacyManager::fieldCopyright).size( );
				valueUns16LE = MakeUns16LE ( copyrightLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				XMP_Uns16 descriptionLen = _legacyManager.GetField ( ASF_LegacyManager::fieldDescription).size( );
				valueUns16LE = MakeUns16LE ( descriptionLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				XMP_Uns16 ratingLen = 0;
				valueUns16LE = MakeUns16LE ( ratingLen );
				header.append ( (const char*)&valueUns16LE, 2 );

				// write field contents

				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldTitle ) );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldAuthor ) );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldCopyright ) );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldDescription ) );

				// update new object size
				valueUns64LE = MakeUns64LE ( header.size() - headerStartPos );
				std::string newSize ( (const char*)&valueUns64LE, 8 );
				ReplaceString ( header, newSize, (headerStartPos + 16), 8 );

				newObjects &= ~ASF_LegacyManager::objectContentDescription;

				writtenObjects ++;

			}
			
			if ( newObjects & ASF_LegacyManager::objectContentBranding ) {

				headerStartPos = header.size();
				newObjectBase.guid = ASF_Content_Branding_Object;
				newObjectBase.size = 0;

				// write object header
				header.append ( (const char*)&newObjectBase, kASF_ObjectBaseLen );

				// write 'empty' fields
				header.append ( 12, '\0' );

				// copyright URL
				length = _legacyManager.GetField ( ASF_LegacyManager::fieldCopyrightURL).size( );
				valueUns32LE = MakeUns32LE ( length );
				header.append ( (const char*)&valueUns32LE, 4 );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldCopyrightURL ) );

				// update new object size
				valueUns64LE = MakeUns64LE ( header.size() - headerStartPos );
				std::string newSize ( (const char*)&valueUns64LE, 8 );
				ReplaceString ( header, newSize, (headerStartPos + 16), 8 );

				newObjects &= ~ASF_LegacyManager::objectContentBranding;

				writtenObjects ++;

			}

#if ! Exclude_LicenseURL_Recon

			if ( newObjects & ASF_LegacyManager::objectContentEncryption ) {

				headerStartPos = header.size();
				newObjectBase.guid = ASF_Content_Encryption_Object;
				newObjectBase.size = 0;

				// write object header
				header.append ( (const char*)&newObjectBase, kASF_ObjectBaseLen );

				// write 'empty' fields
				header.append ( 12, '\0' );

				// License URL
				length = _legacyManager.GetField ( ASF_LegacyManager::fieldLicenseURL).size( );
				valueUns32LE = MakeUns32LE ( length );
				header.append ( (const char*)&valueUns32LE, 4 );
				header.append ( _legacyManager.GetField ( ASF_LegacyManager::fieldLicenseURL ) );

				// update new object size
				valueUns64LE = MakeUns64LE ( header.size() - headerStartPos );
				std::string newSize ( (const char*)&valueUns64LE, 8 );
				ReplaceString ( header, newSize, (headerStartPos + 16), 8 );

				newObjects &= ~ASF_LegacyManager::objectContentEncryption;

				writtenObjects ++;

			}

#endif

		}

		// create padding object ?
		if ( usePadding && (header.size ( ) < object.len ) ) {
			ASF_Support::CreatePaddingObject ( &header, (object.len - header.size()) );
			writtenObjects ++;
		}

		// update new header-object size
		valueUns64LE = MakeUns64LE ( header.size() );
		std::string newValue ( (const char*)&valueUns64LE, 8 );
		ReplaceString ( header, newValue, 16, 8 );

		// update new number of Header objects
		valueUns32LE = MakeUns32LE ( writtenObjects );
		newValue = std::string ( (const char*)&valueUns32LE, 4 );
		ReplaceString ( header, newValue, 24, 4 );

		// if we are operating on the same file (in-place update), place pointer before writing
		if ( sourceRef == destRef ) destRef->Seek ( object.pos, kXMP_SeekFromStart );
		if ( this->progressTracker != 0 ) 
		{
			XMP_Assert ( this->progressTracker->WorkInProgress() );
			this->progressTracker->AddTotalWork ( (float)header.size() );
		}
		// write header
		destRef->Write ( header.c_str(), header.size() );

	} catch ( ... ) {

		return false;

	}

	return true;

}

// =============================================================================================

bool ASF_Support::UpdateHeaderObject ( XMP_IO* fileRef, const ObjectData& object, ASF_LegacyManager& _legacyManager )
{
	return ASF_Support::WriteHeaderObject ( fileRef, fileRef, object, _legacyManager, true );
}

// =============================================================================================

bool ASF_Support::UpdateFileSize ( XMP_IO* fileRef )
{
	if ( fileRef == 0 ) return false;

	XMP_Uns64 posCurrent = fileRef->Seek ( 0, kXMP_SeekFromCurrent );
	XMP_Uns64 newSizeLE  = MakeUns64LE ( fileRef->Length() );

	if ( this->posFileSizeInfo != 0 ) {

		fileRef->Seek ( this->posFileSizeInfo, kXMP_SeekFromStart );
	
	} else {

		// The position of the file size field is not known, find it.

		ASF_ObjectBase objHeader;
		
		// Read the Header object at the start of the file.

		fileRef->Rewind();
		fileRef->ReadAll ( &objHeader, kASF_ObjectBaseLen );
		if ( ! IsEqualGUID ( ASF_Header_Object, objHeader.guid ) ) return false;
		
		XMP_Uns32 childCount;
		fileRef->ReadAll ( &childCount, 4 );
		childCount = GetUns32LE ( &childCount );
		
		fileRef->Seek ( 2, kXMP_SeekFromCurrent );	// Skip the 2 reserved bytes.
		
		// Look for the File Properties object in the Header's children.

		for ( ; childCount > 0; --childCount ) {
			fileRef->ReadAll ( &objHeader, kASF_ObjectBaseLen );
			if ( IsEqualGUID ( ASF_File_Properties_Object, objHeader.guid ) ) break;
			XMP_Uns64 dataLen = GetUns64LE ( &objHeader.size ) - 24;
			fileRef->Seek ( dataLen, kXMP_SeekFromCurrent );	// Skip this object's data.
		}
		if ( childCount == 0 ) return false;
		
		// Seek to the file size field.

		XMP_Uns64 fpoSize = GetUns64LE ( &objHeader.size );
		if ( fpoSize < (16+8+16+8) ) return false;
		fileRef->Seek ( 16, kXMP_SeekFromCurrent );	// Skip to the file size field.

	}

	fileRef->Write ( &newSizeLE, 8 );	// Write the new file size.

	fileRef->Seek ( posCurrent, kXMP_SeekFromStart );
	return true;

}

// =============================================================================================

bool ASF_Support::ReadHeaderExtensionObject ( XMP_IO* fileRef, ObjectState& inOutObjectState, const XMP_Uns64& _pos, const ASF_ObjectBase& _objectBase )
{
	if ( ! IsEqualGUID ( ASF_Header_Extension_Object, _objectBase.guid) || (! legacyManager ) ) return false;

	try {

		// read extended header-object structure beginning at the data part (offset = 46)
		const XMP_Uns64 offset = 46;
		XMP_Uns64 read = 0;
		XMP_Uns64 data = (_objectBase.size - offset);
		XMP_Uns64 pos = (_pos + offset);

		ASF_ObjectBase objectBase;

		while ( read < data ) {

			fileRef->Seek ( pos, kXMP_SeekFromStart );
			if ( kASF_ObjectBaseLen != fileRef->Read ( &objectBase, kASF_ObjectBaseLen, true ) ) break;

			objectBase.size = GetUns64LE ( &objectBase.size );

			if ( IsEqualGUID ( ASF_Padding_Object, objectBase.guid ) ) {
				legacyManager->SetPadding ( legacyManager->GetPadding() + (objectBase.size - 24) );
			}

			pos += objectBase.size;
			read += objectBase.size;

		}

	} catch ( ... ) {

		return false;

	}

	return true;

}

// =============================================================================================

bool ASF_Support::WriteHeaderExtensionObject ( const std::string& buffer, std::string* header, const ASF_ObjectBase& _objectBase, const int /*reservePadding*/ )
{
	if ( ! IsEqualGUID ( ASF_Header_Extension_Object, _objectBase.guid ) || (! header) || (buffer.size() < 46) ) return false;

	const XMP_Uns64 offset = 46;
	int startPos = header->size();

	// copy header base
	header->append ( buffer, 0, offset );

	// read extended header-object structure beginning at the data part (offset = 46)
	XMP_Uns64 read = 0;
	XMP_Uns64 data = (_objectBase.size - offset);
	XMP_Uns64 pos = offset;

	ASF_ObjectBase objectBase;

	while ( read < data ) {

		memcpy ( &objectBase, &buffer[int(pos)], kASF_ObjectBaseLen );
		objectBase.size = GetUns64LE ( &objectBase.size );

		if ( IsEqualGUID ( ASF_Padding_Object, objectBase.guid ) ) {
			// eliminate
		} else {
			// copy other objects
			header->append ( buffer, XMP_Uns32(pos), XMP_Uns32(objectBase.size) );
		}

		pos += objectBase.size;
		read += objectBase.size;

	}

	// update header extension data size
	XMP_Uns32 valueUns32LE = MakeUns32LE ( header->size() - startPos - offset );
	std::string newDataSize ( (const char*)&valueUns32LE, 4 );
	ReplaceString ( *header, newDataSize, (startPos + 42), 4 );

	// update new object size
	XMP_Uns64 valueUns64LE = MakeUns64LE ( header->size() - startPos );
	std::string newObjectSize ( (const char*)&valueUns64LE, 8 );
	ReplaceString ( *header, newObjectSize, (startPos + 16), 8 );

	return true;

}

// =============================================================================================

bool ASF_Support::CreatePaddingObject ( std::string* header, const XMP_Uns64 size )
{
	if ( ( ! header) || (size < 24) ) return false;

	ASF_ObjectBase newObjectBase;

	newObjectBase.guid = ASF_Padding_Object;
	newObjectBase.size = MakeUns64LE ( size );

	// write object header
	header->append ( (const char*)&newObjectBase, kASF_ObjectBaseLen );

	// write 'empty' padding
	header->append ( XMP_Uns32 ( size - 24 ), '\0' );

	return true;

}

// =============================================================================================

bool ASF_Support::WriteXMPObject ( XMP_IO* fileRef, XMP_Uns32 len, const char* inBuffer )
{
	bool ret = false;

	ASF_ObjectBase objectBase = { ASF_XMP_Metadata, 0 };
	objectBase.size = MakeUns64LE ( len + kASF_ObjectBaseLen );

	try {
		fileRef->Write ( &objectBase, kASF_ObjectBaseLen );
		fileRef->Write ( inBuffer, len );
		ret = true;
	} catch ( ... ) {}

	return ret;

}

// =============================================================================================

bool ASF_Support::UpdateXMPObject ( XMP_IO* fileRef, const ObjectData& object, XMP_Uns32 len, const char * inBuffer )
{
	bool ret = false;

	ASF_ObjectBase objectBase = { ASF_XMP_Metadata, 0 };
	objectBase.size = MakeUns64LE ( len + kASF_ObjectBaseLen );

	try {
		fileRef->Seek ( object.pos, kXMP_SeekFromStart );
		fileRef->Write ( &objectBase, kASF_ObjectBaseLen );
		fileRef->Write ( inBuffer, len );
		ret = true;
	} catch ( ... ) {}

	return ret;

}

// =============================================================================================

bool ASF_Support::CopyObject ( XMP_IO* sourceRef, XMP_IO* destRef, const ObjectData& object )
{
	try {
		sourceRef->Seek ( object.pos, kXMP_SeekFromStart );
		XIO::Copy ( sourceRef, destRef, object.len );
	} catch ( ... ) {
		return false;
	}

	return true;

}

// =============================================================================================

bool ASF_Support::ReadBuffer ( XMP_IO* fileRef, XMP_Uns64 & pos, XMP_Uns64 len, char * outBuffer )
{
	try {

		if ( (fileRef == 0) || (outBuffer == 0) ) return false;
	
		fileRef->Seek ( pos, kXMP_SeekFromStart  );
		long bytesRead = fileRef->ReadAll ( outBuffer, XMP_Int32(len) );
		if ( XMP_Uns32 ( bytesRead ) != len ) return false;

		return true;

	} catch ( ... ) {}

	return false;

}

// =============================================================================================

bool ASF_Support::WriteBuffer ( XMP_IO* fileRef, XMP_Uns64 & pos, XMP_Uns32 len, const char * inBuffer )
{
	try {

		if ( (fileRef == 0) || (inBuffer == 0) ) return false;
	
		fileRef->Seek ( pos, kXMP_SeekFromStart  );
		fileRef->Write ( inBuffer, len  );
	
		return true;

	} catch ( ... ) {}

	return false;

}

// =================================================================================================

std::string ASF_Support::ReplaceString ( std::string& operand, std::string& str, int offset, int count )
{
	std::basic_string<char>::iterator iterF1, iterL1, iterF2, iterL2;

	iterF1 = operand.begin() + offset;
	iterL1 = operand.begin() + offset + count;
	iterF2 = str.begin();
	iterL2 = str.begin() + count;

	return operand.replace ( iterF1, iterL1, iterF2, iterL2 );

}

// =================================================================================================

ASF_LegacyManager::ASF_LegacyManager() : fields(fieldLast), broadcastSet(false), digestComputed(false),
										 imported(false), objectsExisting(0), objectsToExport(0), legacyDiff(0), padding(0)
{
	// Nothing more to do.
}

// =================================================================================================

ASF_LegacyManager::~ASF_LegacyManager()
{
	// Nothing to do.
}

// =================================================================================================

bool ASF_LegacyManager::SetField ( fieldType field, const std::string& value )
{
	if ( field >= fieldLast ) return false;

	unsigned int maxSize = this->GetFieldMaxSize ( field );

	if (value.size ( ) <= maxSize ) {
		fields[field] = value;
	} else {
		fields[field] = value.substr ( 0, maxSize );
	}

	if ( field == fieldCopyrightURL ) NormalizeStringDisplayASCII ( fields[field] );

	#if ! Exclude_LicenseURL_Recon
		if ( field == fieldLicenseURL ) NormalizeStringDisplayASCII ( fields[field] );
	#endif

	return true;

}

// =================================================================================================

std::string ASF_LegacyManager::GetField ( fieldType field )
{
	if ( field >= fieldLast ) return std::string();
	return fields[field];
}

// =================================================================================================

unsigned int ASF_LegacyManager::GetFieldMaxSize ( fieldType field )
{
	unsigned int maxSize = 0;

	switch ( field ) {

		case fieldCreationDate :
			maxSize = 8;
			break;

		case fieldTitle :
		case fieldAuthor :
		case fieldCopyright :
		case fieldDescription :
			maxSize = 0xFFFF;
			break;

		case fieldCopyrightURL :
#if ! Exclude_LicenseURL_Recon
		case fieldLicenseURL :
#endif
			maxSize = 0xFFFFFFFF;
			break;

		default:
			break;

	}

	return maxSize;

}

// =================================================================================================

void ASF_LegacyManager::SetObjectExists ( objectType object )
{
	objectsExisting |= object;
}

// =================================================================================================

void ASF_LegacyManager::SetBroadcast ( const bool broadcast )
{
	broadcastSet = broadcast;
}

// =================================================================================================

bool ASF_LegacyManager::GetBroadcast()
{
	return broadcastSet;
}

// =================================================================================================

void ASF_LegacyManager::ComputeDigest()
{
	MD5_CTX context;
	MD5_Digest digest;
	char buffer[40];

	MD5Init ( &context );
	digestStr.clear();
	digestStr.reserve ( 160 );

	for ( int type=0; type < fieldLast; ++type ) {

		if (fields[type].size ( ) > 0 ) {
			snprintf ( buffer, sizeof(buffer), "%d,", type );
			digestStr.append ( buffer );
			MD5Update ( &context, (XMP_Uns8*)fields[type].data(), fields[type].size() );
		}

	}

	if( digestStr.size() > 0 ) digestStr[digestStr.size()-1] = ';';

	MD5Final ( digest, &context );

	size_t in, out;
	for ( in = 0, out = 0; in < 16; in += 1, out += 2 ) {
		XMP_Uns8 byte = digest[in];
		buffer[out]   = ReconcileUtils::kHexDigits [ byte >> 4 ];
		buffer[out+1] = ReconcileUtils::kHexDigits [ byte & 0xF ];
	}
	buffer[32] = 0;

	digestStr.append ( buffer );

	digestComputed = true;

}

// =================================================================================================

bool ASF_LegacyManager::CheckDigest ( const SXMPMeta& xmp )
{
	bool ret = false;

	if ( ! digestComputed ) this->ComputeDigest();

	std::string oldDigest;

	if ( xmp.GetProperty ( kXMP_NS_ASF, "NativeDigest", &oldDigest, 0 ) ) {
		ret = (digestStr == oldDigest);
	}

	return ret;

}

// =================================================================================================

void ASF_LegacyManager::SetDigest ( SXMPMeta* xmp )
{
	if ( ! digestComputed ) this->ComputeDigest();

	xmp->SetProperty ( kXMP_NS_ASF, "NativeDigest", digestStr.c_str() );

}

// =================================================================================================

void ASF_LegacyManager::ImportLegacy ( SXMPMeta* xmp )
{
	std::string utf8;

	if ( ! broadcastSet ) {
		ConvertMSDateToISODate ( fields[fieldCreationDate], &utf8 );
		if ( ! utf8.empty() ) xmp->SetProperty ( kXMP_NS_XMP, "CreateDate", utf8.c_str(), kXMP_DeleteExisting );
	}

	FromUTF16 ( (UTF16Unit*)fields[fieldTitle].c_str(), (fields[fieldTitle].size() / 2), &utf8, false );
	if ( ! utf8.empty() ) xmp->SetLocalizedText ( kXMP_NS_DC, "title", "", "x-default", utf8.c_str(), kXMP_DeleteExisting );

	xmp->DeleteProperty ( kXMP_NS_DC, "creator" );
	FromUTF16 ( (UTF16Unit*)fields[fieldAuthor].c_str(), (fields[fieldAuthor].size() / 2), &utf8, false );
	if ( ! utf8.empty() ) SXMPUtils::SeparateArrayItems ( xmp, kXMP_NS_DC, "creator",
														  (kXMP_PropArrayIsOrdered | kXMPUtil_AllowCommas), utf8.c_str() );

	FromUTF16 ( (UTF16Unit*)fields[fieldCopyright].c_str(), (fields[fieldCopyright].size() / 2), &utf8, false );
	if ( ! utf8.empty() ) xmp->SetLocalizedText ( kXMP_NS_DC, "rights", "", "x-default", utf8.c_str(), kXMP_DeleteExisting );

	FromUTF16 ( (UTF16Unit*)fields[fieldDescription].c_str(), (fields[fieldDescription].size() / 2), &utf8, false );
	if ( ! utf8.empty() ) xmp->SetLocalizedText ( kXMP_NS_DC, "description", "", "x-default", utf8.c_str(), kXMP_DeleteExisting );

	if ( ! fields[fieldCopyrightURL].empty() ) xmp->SetProperty ( kXMP_NS_XMP_Rights, "WebStatement", fields[fieldCopyrightURL].c_str(), kXMP_DeleteExisting );

#if ! Exclude_LicenseURL_Recon
	if ( ! fields[fieldLicenseURL].empty() ) xmp->SetProperty ( kXMP_NS_XMP_Rights, "Certificate", fields[fieldLicenseURL].c_str(), kXMP_DeleteExisting );
#endif

	imported = true;

}

// =================================================================================================

int ASF_LegacyManager::ExportLegacy ( const SXMPMeta& xmp )
{
	int changed = 0;
	objectsToExport = 0;
	legacyDiff = 0;

	std::string utf8;
	std::string utf16;
	XMP_OptionBits flags;

	if ( ! broadcastSet ) {
		if ( xmp.GetProperty ( kXMP_NS_XMP, "CreateDate", &utf8, &flags ) ) {
			std::string date;
			ConvertISODateToMSDate ( utf8, &date );
			if ( fields[fieldCreationDate] != date ) {
				legacyDiff += date.size();
				legacyDiff -= fields[fieldCreationDate].size();
				this->SetField ( fieldCreationDate, date );
				objectsToExport |= objectFileProperties;
				changed ++;
			}
		}
	}

	if ( xmp.GetLocalizedText ( kXMP_NS_DC, "title", "", "x-default", 0, &utf8, &flags ) ) {
		NormalizeStringTrailingNull ( utf8 );
		ToUTF16 ( (const UTF8Unit*)utf8.data(), utf8.size(), &utf16, false );
		if ( fields[fieldTitle] != utf16 ) {
			legacyDiff += utf16.size();
			legacyDiff -= fields[fieldTitle].size();
			this->SetField ( fieldTitle, utf16 );
			objectsToExport |= objectContentDescription;
			changed ++;
		}
	}

	utf8.clear();
	SXMPUtils::CatenateArrayItems ( xmp, kXMP_NS_DC, "creator", 0, 0, kXMPUtil_AllowCommas, &utf8 );
	if ( ! utf8.empty() ) {
		NormalizeStringTrailingNull ( utf8 );
		ToUTF16 ( (const UTF8Unit*)utf8.data(), utf8.size(), &utf16, false );
		if ( fields[fieldAuthor] != utf16 ) {
			legacyDiff += utf16.size();
			legacyDiff -= fields[fieldAuthor].size();
			this->SetField ( fieldAuthor, utf16 );
			objectsToExport |= objectContentDescription;
			changed ++;
		}
	}

	if ( xmp.GetLocalizedText ( kXMP_NS_DC, "rights", "", "x-default", 0, &utf8, &flags ) ) {
		NormalizeStringTrailingNull ( utf8 );
		ToUTF16 ( (const UTF8Unit*)utf8.data(), utf8.size(), &utf16, false );
		if ( fields[fieldCopyright] != utf16 ) {
			legacyDiff += utf16.size();
			legacyDiff -= fields[fieldCopyright].size();
			this->SetField ( fieldCopyright, utf16 );
			objectsToExport |= objectContentDescription;
			changed ++;
		}
	}

	if ( xmp.GetLocalizedText ( kXMP_NS_DC, "description", "", "x-default", 0, &utf8, &flags ) ) {
		NormalizeStringTrailingNull ( utf8 );
		ToUTF16 ( (const UTF8Unit*)utf8.data(), utf8.size(), &utf16, false );
		if ( fields[fieldDescription] != utf16 ) {
			legacyDiff += utf16.size();
			legacyDiff -= fields[fieldDescription].size();
			this->SetField ( fieldDescription, utf16 );
			objectsToExport |= objectContentDescription;
			changed ++;
		}
	}

	if ( xmp.GetProperty ( kXMP_NS_XMP_Rights, "WebStatement", &utf8, &flags ) ) {
		NormalizeStringTrailingNull ( utf8 );
		if ( fields[fieldCopyrightURL] != utf8 ) {
			legacyDiff += utf8.size();
			legacyDiff -= fields[fieldCopyrightURL].size();
			this->SetField ( fieldCopyrightURL, utf8 );
			objectsToExport |= objectContentBranding;
			changed ++;
		}
	}

#if ! Exclude_LicenseURL_Recon
	if ( xmp.GetProperty ( kXMP_NS_XMP_Rights, "Certificate", &utf8, &flags ) ) {
		NormalizeStringTrailingNull ( utf8 );
		if ( fields[fieldLicenseURL] != utf8 ) {
			legacyDiff += utf8.size();
			legacyDiff -= fields[fieldLicenseURL].size();
			this->SetField ( fieldLicenseURL, utf8 );
			objectsToExport |= objectContentEncryption;
			changed ++;
		}
	}
#endif

	// find objects, that would need to be created on legacy export
	int newObjects = (objectsToExport & !objectsExisting);

	// calculate minimum storage for new objects, that might be created on export
	if ( newObjects & objectContentDescription )
		legacyDiff += sizeContentDescription;
	if ( newObjects & objectContentBranding )
		legacyDiff += sizeContentBranding;
	if ( newObjects & objectContentEncryption )
		legacyDiff += sizeContentEncryption;

	ComputeDigest();

	return changed;

}

// =================================================================================================

bool ASF_LegacyManager::hasLegacyChanged()
{
	return (objectsToExport != 0);
}

// =================================================================================================

XMP_Int64 ASF_LegacyManager::getLegacyDiff()
{
	return (this->hasLegacyChanged() ? legacyDiff : 0);
}

// =================================================================================================

int ASF_LegacyManager::changedObjects()
{
	return objectsToExport;
}

// =================================================================================================

void ASF_LegacyManager::SetPadding ( XMP_Int64 _padding )
{
	padding = _padding;
}

// =================================================================================================

XMP_Int64 ASF_LegacyManager::GetPadding()
{
	return padding;
}

// =================================================================================================

std::string ASF_LegacyManager::NormalizeStringDisplayASCII ( std::string& operand )
{
	std::basic_string<char>::iterator current = operand.begin();
	std::basic_string<char>::iterator end = operand.end();;

	for ( ; (current != end); ++current ) {
		char element = *current;
		if ( ( (element < 0x21) && (element != 0x00)) || (element > 0x7e) ) {
			*current = '?';
		}
	}

	return operand;

}

// =================================================================================================

std::string ASF_LegacyManager::NormalizeStringTrailingNull ( std::string& operand )
{
	if ( ( operand.size() > 0) && (operand[operand.size() - 1] != '\0') ) {
		operand.append ( 1, '\0' );
	}

	return operand;

}

// =================================================================================================

int ASF_LegacyManager::DaysInMonth ( XMP_Int32 year, XMP_Int32 month )
{

	static short	daysInMonth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
									   // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec

	int days = daysInMonth [ month ];
	if ( (month == 2) && IsLeapYear ( year ) ) days += 1;
	
	return days;

}

// =================================================================================================

bool ASF_LegacyManager::IsLeapYear ( long year )
{

	if ( year < 0 ) year = -year + 1;		// Fold the negative years, assuming there is a year 0.

	if ( (year % 4) != 0 ) return false;	// Not a multiple of 4.
	if ( (year % 100) != 0 ) return true;	// A multiple of 4 but not a multiple of 100. 
	if ( (year % 400) == 0 ) return true;	// A multiple of 400.

	return false;							// A multiple of 100 but not a multiple of 400. 

}

// =================================================================================================

void ASF_LegacyManager::ConvertMSDateToISODate ( std::string& source, std::string* dest )
{

	XMP_Int64 creationDate = GetUns64LE ( source.c_str() );
	XMP_Int64 totalSecs = creationDate / (10*1000*1000);
	XMP_Int32 nanoSec = ( ( XMP_Int32) (creationDate - (totalSecs * 10*1000*1000)) ) * 100;

	XMP_Int32 days = (XMP_Int32) (totalSecs / 86400);
	totalSecs -= ( ( XMP_Int64)days * 86400 );

	XMP_Int32 hour = (XMP_Int32) (totalSecs / 3600);
	totalSecs -= ( ( XMP_Int64)hour * 3600 );

	XMP_Int32 minute = (XMP_Int32) (totalSecs / 60);
	totalSecs -= ( ( XMP_Int64)minute * 60 );

	XMP_Int32 second = (XMP_Int32)totalSecs;

	// A little more simple code converts this to an XMP date string:

	XMP_DateTime date;
	memset ( &date, 0, sizeof ( date ) );

	date.year = 1601;       // The MS date origin.
	date.month = 1;
	date.day = 1;

	date.day += days;   // Add in the delta.
	date.hour = hour;
	date.minute = minute;
	date.second = second;
	date.nanoSecond = nanoSec;

	date.hasTimeZone = true;	// ! Needed for ConvertToUTCTime to do anything.
	SXMPUtils::ConvertToUTCTime ( &date );   // Normalize the date/time.
	SXMPUtils::ConvertFromDate ( date, dest );   // Convert to an ISO 8601 string.

}

// =================================================================================================

void ASF_LegacyManager::ConvertISODateToMSDate ( std::string& source, std::string* dest )
{
	XMP_DateTime date;
	SXMPUtils::ConvertToDate ( source, &date );
	SXMPUtils::ConvertToUTCTime ( &date );

	XMP_Int64 creationDate;
	creationDate = date.nanoSecond / 100;
	creationDate += (XMP_Int64 ( date.second) * (10*1000*1000) );
	creationDate += (XMP_Int64 ( date.minute) * 60 * (10*1000*1000) );
	creationDate += (XMP_Int64 ( date.hour) * 3600 * (10*1000*1000) );

	XMP_Int32 days = (date.day - 1);

	--date.month;
	while ( date.month >= 1 ) {
		days += DaysInMonth ( date.year, date.month );
		--date.month;
	}

	--date.year;
	while ( date.year >= 1601 ) {
		days += (IsLeapYear ( date.year) ? 366 : 365 );
		--date.year;
	}

	creationDate += (XMP_Int64 ( days) * 86400 * (10*1000*1000) );

	creationDate = GetUns64LE ( &creationDate );
	dest->assign ( (const char*)&creationDate, 8 );

}
