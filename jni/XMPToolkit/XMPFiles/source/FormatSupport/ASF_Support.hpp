#ifndef __ASF_Support_hpp__
#define __ASF_Support_hpp__ 1

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

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "source/XIO.hpp"
#include "source/XMP_ProgressTracker.hpp"

// currently exclude LicenseURL from reconciliation
#define Exclude_LicenseURL_Recon 1
#define EXCLUDE_LICENSEURL_RECON 1

// Defines for platforms other than Win
#if ! XMP_WinBuild

	typedef struct _GUID
	{
		XMP_Uns32	Data1;
		XMP_Uns16	Data2;
		XMP_Uns16	Data3;
		XMP_Uns8	Data4[8];
	} GUID;

	int IsEqualGUID ( const GUID& guid1, const GUID& guid2 );

	static const GUID GUID_NULL = { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

#endif

// header object
static const GUID ASF_Header_Object = { MakeUns32LE(0x75b22630), MakeUns16LE(0x668e), MakeUns16LE(0x11cf), { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c } };
// contains ...
static const GUID ASF_File_Properties_Object = { MakeUns32LE(0x8cabdca1), MakeUns16LE(0xa947), MakeUns16LE(0x11cf), { 0x8e, 0xe4, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 } };
static const GUID ASF_Content_Description_Object = { MakeUns32LE(0x75b22633), MakeUns16LE(0x668e), MakeUns16LE(0x11cf), { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c } };
static const GUID ASF_Content_Branding_Object = { MakeUns32LE(0x2211b3fa), MakeUns16LE(0xbd23), MakeUns16LE(0x11d2), { 0xb4, 0xb7, 0x00, 0xa0, 0xc9, 0x55, 0xfc, 0x6e } };
static const GUID ASF_Content_Encryption_Object = { MakeUns32LE(0x2211b3fb), MakeUns16LE(0xbd23), MakeUns16LE(0x11d2), { 0xb4, 0xb7, 0x00, 0xa0, 0xc9, 0x55, 0xfc, 0x6e } };
// padding
// Remark: regarding to Microsofts spec only the ASF_Header_Object contains a ASF_Padding_Object
// Real world files show, that the ASF_Header_Extension_Object contains a ASF_Padding_Object
static const GUID ASF_Header_Extension_Object = { MakeUns32LE(0x5fbf03b5), MakeUns16LE(0xa92e), MakeUns16LE(0x11cf), { 0x8e, 0xe3, 0x00, 0xc0, 0x0c, 0x20, 0x53, 0x65 } };
static const GUID ASF_Padding_Object = { MakeUns32LE(0x1806d474), MakeUns16LE(0xcadf), MakeUns16LE(0x4509), { 0xa4, 0xba, 0x9a, 0xab, 0xcb, 0x96, 0xaa, 0xe8 } };

// data object
static const GUID ASF_Data_Object = { MakeUns32LE(0x75b22636), MakeUns16LE(0x668e), MakeUns16LE(0x11cf), { 0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c } };

// XMP object
static const GUID ASF_XMP_Metadata = { MakeUns32LE(0xbe7acfcb), MakeUns16LE(0x97a9), MakeUns16LE(0x42e8), { 0x9c, 0x71, 0x99, 0x94, 0x91, 0xe3, 0xaf, 0xac } };

static const int guidLen = sizeof(GUID);

typedef struct _ASF_ObjectBase
{
	GUID		guid;
	XMP_Uns64	size;

} ASF_ObjectBase;

static const XMP_Uns32 kASF_ObjectBaseLen = (XMP_Uns32) sizeof(ASF_ObjectBase);

// =================================================================================================

class ASF_LegacyManager {
public:

	enum objectType {
		objectFileProperties		= 1 << 0,
		objectContentDescription	= 1 << 1,
		objectContentBranding		= 1 << 2,
		objectContentEncryption		= 1 << 3
	};

	enum minObjectSize {
		sizeContentDescription	= 34,
		sizeContentBranding		= 40,
		sizeContentEncryption	= 40
	};

	enum fieldType {
		// File_Properties_Object
		fieldCreationDate = 0,
		// Content_Description_Object
		fieldTitle,
		fieldAuthor,
		fieldCopyright,
		fieldDescription,
		// Content_Branding_Object
		fieldCopyrightURL,
		#if ! Exclude_LicenseURL_Recon
			// Content_Encryption_Object
			fieldLicenseURL,
		#endif
		// last
		fieldLast
	};

	ASF_LegacyManager();
	virtual ~ASF_LegacyManager();

	bool SetField ( fieldType field, const std::string& value );
	std::string GetField ( fieldType field );
	unsigned int GetFieldMaxSize ( fieldType field );

	void SetObjectExists ( objectType object );

	void SetBroadcast ( const bool broadcast );
	bool GetBroadcast();

	void ComputeDigest();
	bool CheckDigest ( const SXMPMeta& xmp );
	void SetDigest ( SXMPMeta* xmp );

	void ImportLegacy ( SXMPMeta* xmp );
	int ExportLegacy ( const SXMPMeta& xmp );
	bool hasLegacyChanged();
	XMP_Int64 getLegacyDiff();
	int changedObjects();

	void SetPadding ( XMP_Int64 padding );
	XMP_Int64 GetPadding();

private:

	typedef std::vector<std::string> TFields;
	TFields fields;
	bool broadcastSet;

	std::string digestStr;
	bool digestComputed;

	bool imported;
	int objectsExisting;
	int objectsToExport;
	XMP_Int64 legacyDiff;
	XMP_Int64 padding;

	static std::string NormalizeStringDisplayASCII ( std::string& operand );
	static std::string NormalizeStringTrailingNull ( std::string& operand );

	static void ConvertMSDateToISODate ( std::string& source, std::string* dest );
	static void ConvertISODateToMSDate ( std::string& source, std::string* dest );

	static int DaysInMonth ( XMP_Int32 year, XMP_Int32 month );
	static bool IsLeapYear ( long year );

}; // class ASF_LegacyManager

// =================================================================================================

class ASF_Support {
public:

	class ObjectData {
		public:
			ObjectData() : pos(0), len(0), guid(GUID_NULL), xmp(false) {}
			virtual ~ObjectData() {}
			XMP_Uns64	pos;		// file offset of object
			XMP_Uns64	len;		// length of object data
			GUID		guid;		// object GUID
			bool		xmp;		// object with XMP ?
	};

	typedef std::vector<ObjectData> ObjectVector;
	typedef ObjectVector::iterator ObjectIterator;

	class ObjectState {

		public:
			ObjectState() : xmpPos(0), xmpLen(0), xmpIsLastObject(false), broadcast(false) {}
			virtual ~ObjectState() {}
			XMP_Uns64		xmpPos;
			XMP_Uns64		xmpLen;
			bool			xmpIsLastObject;
			bool			broadcast;
			ObjectData		xmpObject;
			ObjectVector	objects;
	};

	ASF_Support();
	ASF_Support ( ASF_LegacyManager* legacyManager, XMP_ProgressTracker* _progressTracker);
	virtual ~ASF_Support();

	long OpenASF ( XMP_IO* fileRef, ObjectState & inOutObjectState );

	bool ReadObject ( XMP_IO* fileRef, ObjectState & inOutObjectState, XMP_Uns64 * objectLength, XMP_Uns64 & inOutPosition );

	bool ReadHeaderObject ( XMP_IO* fileRef, ObjectState& inOutObjectState, const ObjectData& newObject );
	bool WriteHeaderObject ( XMP_IO* sourceRef, XMP_IO* destRef, const ObjectData& object, ASF_LegacyManager& legacyManager, bool usePadding );
	bool UpdateHeaderObject ( XMP_IO* fileRef, const ObjectData& object, ASF_LegacyManager& legacyManager );

	bool UpdateFileSize ( XMP_IO* fileRef );

	bool ReadHeaderExtensionObject ( XMP_IO* fileRef, ObjectState& inOutObjectState, const XMP_Uns64& pos, const ASF_ObjectBase& objectBase );
	static bool WriteHeaderExtensionObject ( const std::string& buffer, std::string* header, const ASF_ObjectBase& objectBase, const int reservePadding );

	static bool CreatePaddingObject ( std::string* header, const XMP_Uns64 size );

	static bool WriteXMPObject ( XMP_IO* fileRef, XMP_Uns32 len, const char* inBuffer );
	static bool UpdateXMPObject ( XMP_IO* fileRef, const ObjectData& object, XMP_Uns32 len, const char * inBuffer );
	static bool CopyObject ( XMP_IO* sourceRef, XMP_IO* destRef, const ObjectData& object );

	static bool ReadBuffer ( XMP_IO* fileRef, XMP_Uns64 & pos, XMP_Uns64 len, char * outBuffer );
	static bool WriteBuffer ( XMP_IO* fileRef, XMP_Uns64 & pos, XMP_Uns32 len, const char * inBuffer );

private:

	ASF_LegacyManager* legacyManager;
	XMP_ProgressTracker* progressTracker;//not owned by ASF_Support
	XMP_Uns64 posFileSizeInfo;

	static std::string ReplaceString ( std::string& operand, std::string& str, int offset, int count );

}; // class ASF_Support

// =================================================================================================

#endif	// __ASF_Support_hpp__
