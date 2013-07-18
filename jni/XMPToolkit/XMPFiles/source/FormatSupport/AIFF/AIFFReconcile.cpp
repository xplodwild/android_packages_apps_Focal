// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.
#include "public/include/XMP_Const.h"

#include "XMPFiles/source/FormatSupport/AIFF/AIFFReconcile.h"
#include "XMPFiles/source/FormatSupport/AIFF/AIFFMetadata.h"
#include "XMPFiles/source/NativeMetadataSupport/MetadataSet.h"
#include "source/XMP_LibUtils.hpp"
#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"
#include "source/XIO.hpp"


using namespace IFF_RIFF;

static const MetadataPropertyInfo kAIFFProperties[] =
{
//	  XMP NS		XMP Property Name	Native Metadata Identifier	Native Datatype			XMP Datatype		Delete	Priority	ExportPolicy
	{ kXMP_NS_DC,	"title",			AIFFMetadata::kName,		kNativeType_StrUTF8,	kXMPType_Localized,	true,	false,		kExport_Always },	// dc:title <-> FORM:AIFF/NAME
	{ kXMP_NS_DC,	"creator",			AIFFMetadata::kAuthor,		kNativeType_StrUTF8,	kXMPType_Array,		true,	false,		kExport_Always },	// dc:creator <-> FORM:AIFF/AUTH
	{ kXMP_NS_DC,	"rights",			AIFFMetadata::kCopyright,	kNativeType_StrUTF8,	kXMPType_Localized,	true,	false,		kExport_Always },	// dc:rights <-> FORM:AIFF/(c)
	{ kXMP_NS_DM,	"logComment",		AIFFMetadata::kAnnotation,	kNativeType_StrUTF8,	kXMPType_Simple,	true,	false,		kExport_Always },	// xmpDM:logComment <-> FORM:AIFF/ANNO
	{ NULL }
};

XMP_Bool AIFFReconcile::importToXMP( SXMPMeta& outXMP, const MetadataSet& inMetaData )
{
	XMP_Bool changed = false;

	// the reconciliation is based on the existing outXMP packet
	AIFFMetadata *aiffMeta = inMetaData.get<AIFFMetadata>();
	
	if (aiffMeta != NULL)
	{
		changed = IReconcile::importNativeToXMP( outXMP, *aiffMeta, kAIFFProperties, false );
	}

	return changed;
}//reconcile


XMP_Bool AIFFReconcile::exportFromXMP( MetadataSet& outMetaData, SXMPMeta& inXMP )
{
	XMP_Bool changed = false;

	// Get the appropriate metadata container
	AIFFMetadata *aiffMeta = outMetaData.get<AIFFMetadata>();

	// If the metadata container is not available, skip that part of the process
	if( aiffMeta != NULL )
	{
		changed = IReconcile::exportXMPToNative( *aiffMeta, inXMP, kAIFFProperties );
	}//if AIFF is set

	return changed;
}//dissolve
