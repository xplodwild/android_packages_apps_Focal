// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _IReconcile_h_
#define _IReconcile_h_

#include <string>

#ifndef TXMP_STRING_TYPE
	#define TXMP_STRING_TYPE std::string
#endif

#include "XMP.hpp"

class MetadataSet;
class IMetadata;

enum XMPPropertyType
{
	kXMPType_Simple,
	// Structs can be treated as Simple if the whole path is given to the API
	kXMPType_Localized,
	// Unordered array (bag)
	kXMPType_Array,
	// Ordered array (seq)
	kXMPType_OrderedArray
};

/** Types that describe how the native property is interpreted */
enum MetadataPropertyType
{
	kNativeType_Str,		// Take the value as is
	kNativeType_StrASCII,	// Treat it as ASCII, convert if necessary
	kNativeType_StrUTF8,	// Treat it as UTF-8, convert if necessary
	kNativeType_StrLocal,	// Use local encoding
	kNativeType_Uns64,
	kNativeType_Uns32,
	kNativeType_Int32,
	kNativeType_Uns16
};

/** Types that describe how an XMP property is exported to native Metadata */
enum ExportPolicy
{
	kExport_Never = 0,		// Never export.
	kExport_Always = 1,		// Add, modify, or delete.
	kExport_NoDelete = 2,	// Add or modify, do not delete if no XMP.
	kExport_InjectOnly = 3	// Add tag if new, never modify or delete existing values.
};

struct MetadataPropertyInfo
{
	XMP_StringPtr			mXMPSchemaNS;
	XMP_StringPtr			mXMPPropName;
	XMP_Uns32				mMetadataID;
	MetadataPropertyType	mNativeType;
	XMPPropertyType			mXMPType;
	// If true, delete the XMP property if the native one does not exist on import
	bool					mDeleteXMPIfNoNative;
	// If true, any existing XMP has higher priority on import
	bool					mConsiderPriority;
	ExportPolicy			mExportPolicy;
};

class IReconcile
{
public:
	virtual ~IReconcile() {};
	/**
	 * Reconciles metadata from legacy formats into XMP.
	 * 
	 * @param outXMP the reconciliated XMP packet contains all XMP and legacy metadata,
	 *		  it is created and owned by the the caller. 
	 * @param inMetaData contains all legacy containers that are relevant for the processed file format AND 
	 *		  which actually contain data.
	 *		  If a container is not included in the set, it is omitted by the reconciliation method.
	 *		  Note: inMetaData#getXMP() and outXMP can be the same object
	 * @return XMP has been changed (true) or not (false)
	 *
	 * Throws exception if reconciliation is not possible.
	 */
	virtual XMP_Bool importToXMP( SXMPMeta& outXMP, const MetadataSet& inMetaData ) = 0;

	/**
	 * Dissolves metadata from the XMP object to legacy formats.
	 *
	 * @param outMetaData contains all legacy containers that are relevant for the processed file format
	 *		  (and the file handler is interested in).
	 *		  If a container is not included in the set, it is omitted by the dissolve method.
	 *		  Note: outMetaData#getXMP() and inXMP can be the same object
	 * @param inXMP the XMP packet that contains all XMP and legacy metadata,
	 *		  it is created and owned by the the caller. The legacy data is distributed into the legacy containers.
	 * @return legacy has been changed (true) or not (false)
	 *
	 * Throws exception if dissolving is not possible.
	 */
	virtual XMP_Bool exportFromXMP( MetadataSet& outMetaData, SXMPMeta& inXMP ) = 0;

protected:
	/**
		Import native metadata container into XMP.
		This method is supposed to import all native metadata values that are listed in the property table
		from the IMetadata instance to the SXMPMeta instance.

		@param outXMP			Target XMP container
		@param nativeMeta		Native metadata container
		@param propertyInfo		Property table that lists all values that are supposed to be imported
		@param xmpPriority		Pass true, if an existing XMP value has higher priority than the native metadata

		@return		true if any XMP properties were changed
	*/
	static bool importNativeToXMP( SXMPMeta& outXMP, const IMetadata& nativeMeta, const MetadataPropertyInfo* propertyInfo, bool xmpPriority );

	/**
		Export XMP values to native metadata container.
		This method is supposed to export all native metadata values that are listed in the property table
		from the XMP container to the IMetadata instance.

		@param outNativeMeta	Target native metadata container
		@param inXMP			XMP container
		@param propertyInfo		Property table that lists all values that are supposed to be exported

		@return		true if any native metadata value were changed
	*/
	static bool exportXMPToNative( IMetadata& outNativeMeta, SXMPMeta& inXMP, const MetadataPropertyInfo* propertyInfo );

	// Converts input string to an ascii output string
	// - terminates at first 0
	// - replaces all non ascii with 0x3F ('?')
	static void convertToASCII( const std::string& input, std::string& output );
};

#endif
