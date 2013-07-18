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

#include "XMPFiles/source/FormatSupport/WAVE/WAVEReconcile.h"
#include "XMPFiles/source/FormatSupport/WAVE/DISPMetadata.h"
#include "XMPFiles/source/FormatSupport/WAVE/INFOMetadata.h"
#include "XMPFiles/source/FormatSupport/WAVE/BEXTMetadata.h"
#include "XMPFiles/source/FormatSupport/WAVE/CartMetadata.h"

// cr8r is not yet required for WAVE
//#include "Cr8rMetadata.h"

#include "XMPFiles/source/NativeMetadataSupport/MetadataSet.h"

#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"

using namespace IFF_RIFF;

// ************** legacy Mappings ***************** //

static const MetadataPropertyInfo kBextProperties[] =
{
//	  XMP NS		XMP Property Name		Native Metadata Identifier			Native Datatype				XMP	Datatype		Delete	Priority	ExportPolicy
	{ kXMP_NS_BWF,	"description",			BEXTMetadata::kDescription,			kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },	// bext:description <-> BEXT:Description
	{ kXMP_NS_BWF,	"originator",			BEXTMetadata::kOriginator,			kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },	// bext:originator <-> BEXT:originator
	{ kXMP_NS_BWF,	"originatorReference",	BEXTMetadata::kOriginatorReference,	kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },	// bext:OriginatorReference <-> BEXT:OriginatorReference
	{ kXMP_NS_BWF,	"originationDate",		BEXTMetadata::kOriginationDate,		kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },	// bext:originationDate <-> BEXT:originationDate
	{ kXMP_NS_BWF,	"originationTime",		BEXTMetadata::kOriginationTime,		kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },	// bext:originationTime <-> BEXT:originationTime
	{ kXMP_NS_BWF,	"timeReference",		BEXTMetadata::kTimeReference,		kNativeType_Uns64,			kXMPType_Simple,	true,	false,		kExport_Always },	// bext:timeReference <-> BEXT:TimeReferenceLow + BEXT:TimeReferenceHigh
	// Special case: On export BEXT:version is always written as 1
	{ kXMP_NS_BWF,	"version",				BEXTMetadata::kVersion,				kNativeType_Uns16,			kXMPType_Simple,	true,	false,		kExport_Never },	// bext:version <-> BEXT:version
	// special case: bext:umid <-> BEXT:UMID
	{ kXMP_NS_BWF,	"codingHistory",		BEXTMetadata::kCodingHistory,		kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },	// bext:codingHistory <-> BEXT:codingHistory
	{ NULL }
};

static const MetadataPropertyInfo kINFOProperties[] =
{
//	  XMP NS		XMP Property Name		Native Metadata Identifier		Native Datatype				XMP	Datatype		Delete	Priority	ExportPolicy
	{ kXMP_NS_DM,	"artist",				INFOMetadata::kArtist,			kNativeType_StrUTF8,		kXMPType_Simple,	false,	true,		kExport_Always },		// xmpDM:artist <-> IART
	{ kXMP_NS_DM,	"logComment",			INFOMetadata::kComments,		kNativeType_StrUTF8,		kXMPType_Simple,	false,	true,		kExport_Always },		// xmpDM:logComment <-> ICMT
	{ kXMP_NS_DC,	"rights",				INFOMetadata::kCopyright,		kNativeType_StrUTF8,		kXMPType_Localized,	false,	true,		kExport_Always },		// dc:rights <-> ICOP
	{ kXMP_NS_XMP,	"CreateDate",			INFOMetadata::kCreationDate,	kNativeType_StrUTF8,		kXMPType_Simple,	false,	true,		kExport_Always },		// xmp:CreateDate <-> ICRD
	{ kXMP_NS_DM,	"engineer",				INFOMetadata::kEngineer,		kNativeType_StrUTF8,		kXMPType_Simple,	false,	true,		kExport_Always },		// xmpDM:engineer <-> IENG
	{ kXMP_NS_DM,	"genre",				INFOMetadata::kGenre,			kNativeType_StrUTF8,		kXMPType_Simple,	false,	true,		kExport_Always },		// xmpDM:genre <-> IGNR
	{ kXMP_NS_XMP,	"CreatorTool",			INFOMetadata::kSoftware,		kNativeType_StrUTF8,		kXMPType_Simple,	false,	true,		kExport_Always },		// xmp:CreatorTool <-> ISFT
	{ kXMP_NS_DC,	"source",				INFOMetadata::kMedium,			kNativeType_StrUTF8,		kXMPType_Simple,	false,	false,		kExport_Always },		// dc:source <-> IMED, not in old digest
	{ kXMP_NS_DC,	"type",					INFOMetadata::kSourceForm,		kNativeType_StrUTF8,		kXMPType_Array,		false,	false,		kExport_Always },		// dc:type <-> ISRF, not in old digest

	// new mappings
	{ kXMP_NS_RIFFINFO,	"name",				INFOMetadata::kName,			kNativeType_StrUTF8,		kXMPType_Localized,	true,	false,		kExport_Always },		// riffinfo:name <-> INAM
	{ kXMP_NS_RIFFINFO,	"archivalLocation",	INFOMetadata::kArchivalLocation,kNativeType_StrUTF8,		kXMPType_Simple,	true,	false,		kExport_Always },		// riffinfo:archivalLocation  <-> IARL
	{ kXMP_NS_RIFFINFO,	"commissioned",		INFOMetadata::kCommissioned,	kNativeType_StrUTF8,		kXMPType_Simple,	true,	false,		kExport_Always },		// riffinfo:commissioned  <-> ICMS
	// special case, the native value is a semicolon-separated array
	// { kXMP_NS_DC,		"subject",			INFOMetadata::kKeywords,		kNativeType_StrUTF8,		kXMPType_Array,		false,	false,		kExport_Always },		// dc:subject  <-> IKEY
	{ kXMP_NS_RIFFINFO,	"product",			INFOMetadata::kProduct,			kNativeType_StrUTF8,		kXMPType_Simple,	true,	false,		kExport_Always },		// riffinfo:product <-> IPRD
	{ kXMP_NS_DC,		"description",		INFOMetadata::kSubject,			kNativeType_StrUTF8,		kXMPType_Localized,	false,	false,		kExport_Always },		// dc:description <-> ISBJ
	{ kXMP_NS_RIFFINFO,	"source",			INFOMetadata::kSource,			kNativeType_StrUTF8,		kXMPType_Simple,	true,	false,		kExport_Always },		// riffinfo:source  <-> ISRC
	{ kXMP_NS_RIFFINFO,	"technician",		INFOMetadata::kTechnican,		kNativeType_StrUTF8,		kXMPType_Simple,	true,	false,		kExport_Always },		// riffinfo:technician <-> ITCH

	{ NULL }
};

static const MetadataPropertyInfo kDISPProperties[] =
{
//	  XMP NS		XMP Property Name		Native Metadata Identifier		Datatype					Datatype			Delete	Priority	ExportPolicy
	{ kXMP_NS_DC,	"title",				DISPMetadata::kTitle,			kNativeType_StrUTF8,		kXMPType_Localized,	false,	true,		kExport_Always },		// dc:title <-> DISP
	// Special case: DISP will overwrite LIST/INFO:INAM in dc:title if existing
	{ NULL }
};

static const MetadataPropertyInfo kCartProperties[] =
{
//	  XMP NS			XMP Property Name		Native Metadata Identifier			Datatype					Datatype			Delete	Priority	ExportPolicy
	{ kXMP_NS_AEScart,	"Version",				CartMetadata::kVersion,				kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"Title",				CartMetadata::kTitle,				kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"Artist",				CartMetadata::kArtist,				kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"CutID",				CartMetadata::kCutID,				kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"ClientID",				CartMetadata::kClientID,			kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"Category",				CartMetadata::kCategory,			kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"Classification",		CartMetadata::kClassification,		kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"OutCue",				CartMetadata::kOutCue,				kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"StartDate",			CartMetadata::kStartDate,			kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"StartTime",			CartMetadata::kStartTime,			kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"EndDate",				CartMetadata::kEndDate,				kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"EndTime",				CartMetadata::kEndTime,				kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"ProducerAppID",		CartMetadata::kProducerAppID,		kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"ProducerAppVersion",	CartMetadata::kProducerAppVersion,	kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"UserDef",				CartMetadata::kUserDef,				kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"URL",					CartMetadata::kURL,					kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"TagText",				CartMetadata::kTagText,				kNativeType_StrLocal,		kXMPType_Simple,	true,	false,		kExport_Always },
	{ kXMP_NS_AEScart,	"LevelReference",		CartMetadata::kLevelReference,		kNativeType_Int32,			kXMPType_Simple,	true,	false,		kExport_Always },
	// Special case Post Timer
	{ NULL }
};

// cr8r is not yet required for WAVE
//
//static const MetadataPropertyInfo kCr8rProperties[] =
//{
////	  XMP NS				XMP Property Name							Native Metadata Identifier	Native Datatype		XMP Datatype		Delete	Priority	ExportPolicy
//	{ kXMP_NS_CreatorAtom,	"macAtom/creatorAtom:applicationCode",		Cr8rMetadata::kCreatorCode,	kNativeType_Uns32,	kXMPType_Simple,	false,	false,		kExport_Always },	// creatorAtom:macAtom/creatorAtom:applicationCode <-> Cr8r.creatorCode 
//	{ kXMP_NS_CreatorAtom,	"macAtom/creatorAtom:invocationAppleEvent",	Cr8rMetadata::kAppleEvent,	kNativeType_Uns32,	kXMPType_Simple,	false,	false,		kExport_Always },	// creatorAtom:macAtom/creatorAtom:invocationAppleEvent <-> Cr8r.appleEvent
//	{ kXMP_NS_CreatorAtom,	"windowsAtom/creatorAtom:extension",		Cr8rMetadata::kFileExt,		kNativeType_Str,	kXMPType_Simple,	false,	false,		kExport_Always },	// creatorAtom:windowsAtom/creatorAtom:extension <-> Cr8r.fileExt
//	{ kXMP_NS_CreatorAtom,	"windowsAtom/creatorAtom:invocationFlags",	Cr8rMetadata::kAppOptions,	kNativeType_Str,	kXMPType_Simple,	false,	false,		kExport_Always },	// creatorAtom:windowsAtom/creatorAtom:invocationFlags <-> Cr8r.appOptions
//	{ kXMP_NS_XMP,			"CreatorTool",								Cr8rMetadata::kAppName,		kNativeType_Str,	kXMPType_Simple,	false,	false,		kExport_Always },	// xmp:CreatorTool <-> Cr8r.appName
//	{ NULL }
//};

// ! PrmL atom has all special mappings

// ************** legacy Mappings end ***************** //

XMP_Bool WAVEReconcile::importToXMP( SXMPMeta& outXMP, const MetadataSet& inMetaData )
{
	bool changed = false;

	// the reconciliation is based on the existing outXMP packet

	//
	// ! The existence of a digest leads to prefering pre-existing XMP over legacy properties.
	//
	bool hasDigest = outXMP.GetProperty( kXMP_NS_WAV, "NativeDigest", NULL , NULL );

	if ( hasDigest )
	{
		// remove, as digests are no longer used.
		outXMP.DeleteProperty( kXMP_NS_WAV, "NativeDigest" );
	}


	if ( ! ignoreLocalText ) 
	{

		//
		// Import BEXT
		//
		BEXTMetadata *bextMeta = inMetaData.get<BEXTMetadata>();

		if (bextMeta != NULL)
		{
			changed |= IReconcile::importNativeToXMP( outXMP, *bextMeta, kBextProperties, false );

			// bext:umid <-> BEXT:UMID
			if ( bextMeta->valueExists( BEXTMetadata::kUMID ) )
			{
				XMP_Uns32 umidSize = 0;
				const XMP_Uns8* const umid = bextMeta->getArray<XMP_Uns8>( BEXTMetadata::kUMID, umidSize );

				std::string xmpValue;
				bool allZero = encodeToHexString( umid, xmpValue );
			
				if( ! allZero )
				{	
					outXMP.SetProperty(kXMP_NS_BWF, "umid", xmpValue.c_str());	
					changed = true;
				}
			}

		}


		//
		// Import cart
		//
		CartMetadata* cartData = inMetaData.get<CartMetadata>();
		if ( cartData != NULL ) 
		{

			if (cartData->valueExists( CartMetadata::kPostTimer ) )
			{
				// first get Array
				XMP_Uns32 size = 0;
				const CartMetadata::StoredCartTimer* timerArray = cartData->getArray<CartMetadata::StoredCartTimer> ( CartMetadata::kPostTimer, size );
				XMP_Assert (size == CartMetadata::kPostTimerLength );

				char usage [5];
				XMP_Uns32 usageBE = 0;
				char value [25];  // Unsigned has 10 dezimal digets (buffer has 25 just to be save)
				std::string  path = "";
				memset ( usage, 0, 5 );	// Fill with zeros
				memset ( value, 0, 25 );	// Fill with zeros

				outXMP.DeleteProperty( kXMP_NS_AEScart, "PostTimer");
				outXMP.AppendArrayItem( kXMP_NS_AEScart, "PostTimer", kXMP_PropArrayIsOrdered, NULL, kXMP_PropValueIsStruct );

				for ( XMP_Uns32 i = 0; i<CartMetadata::kPostTimerLength; i++ )
				{	
					// Ensure to write as Big Endian
					usageBE = MakeUns32BE(timerArray[i].usage);
					memcpy( usage, &usageBE, 4 );

					snprintf ( value, 24,	"%u", timerArray[i].value);

					SXMPUtils::ComposeArrayItemPath( kXMP_NS_AEScart, "PostTimer", i+1, &path);
					outXMP.SetStructField( kXMP_NS_AEScart,  path.c_str(), kXMP_NS_AEScart, "Usage", usage );
					outXMP.SetStructField( kXMP_NS_AEScart, path.c_str(), kXMP_NS_AEScart, "Value", value );
				}

				changed = true;
			}

			// import rest if cart properties
			changed |= IReconcile::importNativeToXMP ( outXMP, *cartData, kCartProperties, false );
		}
	
	}

	// cr8r is not yet required for WAVE

	////
	//// import cr8r
	//// Special case: If both Cr8r.AppName and LIST/INFO:ISFT are present, ISFT shall win.
	//// therefor import Cr8r first.
	////
	//Cr8rMetadata* cr8rMeta = inMetaData.get<Cr8rMetadata>();

	//if( cr8rMeta != NULL )
	//{
	//	changed |= IReconcile::importNativeToXMP( outXMP, *cr8rMeta, kCr8rProperties, false );
	//}

	//
	// Import LIST/INFO
	//
	INFOMetadata *infoMeta = inMetaData.get<INFOMetadata>();
	bool hasINAM = false;
	std::string actualLang;
	bool hasDCTitle = outXMP.GetLocalizedText( kXMP_NS_DC, "title", "" , "x-default" , &actualLang, NULL, NULL );

	if (infoMeta != NULL)
	{
		//
		// Remember if List/INFO:INAM has been imported
		//
		hasINAM = infoMeta->valueExists( INFOMetadata::kName );

		// Keywords are a ;-separated list and is therefore handled manually,
		// leveraging the XMPUtils functions
		if (infoMeta->valueExists( INFOMetadata::kKeywords ) )
		{
			std::string keywordsUTF8;
			outXMP.DeleteProperty( kXMP_NS_DC, "subject" );
			ReconcileUtils::NativeToUTF8( infoMeta->getValue<std::string>( INFOMetadata::kKeywords ), keywordsUTF8 );
			SXMPUtils::SeparateArrayItems( &outXMP, kXMP_NS_DC, "subject", kXMP_PropArrayIsUnordered, keywordsUTF8 );
			changed = true;
		}

		//
		// import properties
		//
		changed |= IReconcile::importNativeToXMP( outXMP, *infoMeta, kINFOProperties, hasDigest );
	}

	//
	// Import DISP
	// ! DISP will overwrite dc:title
	//
	bool hasDISP = false;

	DISPMetadata *dispMeta = inMetaData.get<DISPMetadata>();

	if( dispMeta != NULL && dispMeta->valueExists( DISPMetadata::kTitle) )
	{
		changed |= IReconcile::importNativeToXMP( outXMP, *dispMeta, kDISPProperties, hasDigest );
		hasDISP = true;
	}

	if( !hasDISP )
	{
		//
		// map INAM to dc:title ONLY in the case if:
		// * DISP does NOT exists
		// * dc:title does NOT exists
		// * INAM exists
		//
		if( !hasDCTitle && hasINAM )
		{
			std::string xmpValue;
			ReconcileUtils::NativeToUTF8( infoMeta->getValue<std::string>( INFOMetadata::kName ), xmpValue );
			outXMP.SetLocalizedText( kXMP_NS_DC, "title", NULL, "x-default", xmpValue.c_str() );
		}
	}

	return changed;
}	// importToXMP


XMP_Bool WAVEReconcile::exportFromXMP( MetadataSet& outMetaData, SXMPMeta& inXMP )
{
	// tracks if anything has been exported from the XMP
	bool changed = false;

	//
	// Export DISP
	//
	DISPMetadata *dispMeta = outMetaData.get<DISPMetadata>();
	if (dispMeta != NULL)
	{
		// dc:title <-> DISP
		changed |= IReconcile::exportXMPToNative( *dispMeta, inXMP, kDISPProperties );
	}

	
	if ( ! ignoreLocalText ) 
	{
		//
		// Export BEXT
		//

		BEXTMetadata *bextMeta = outMetaData.get<BEXTMetadata>();

		if (bextMeta != NULL)
		{
			IReconcile::exportXMPToNative( *bextMeta, inXMP, kBextProperties );

			std::string xmpValue;

			// bext:umid  <-> RIFF:WAVE/bext.UMID
			if (inXMP.GetProperty(kXMP_NS_BWF, "umid", &xmpValue, 0))
			{
				std::string umid;
			
				if( this->decodeFromHexString( xmpValue, umid ) )
				{
					//
					// if the XMP property doesn't contain a valid hex string then
					// keep the existing value in the umid BEXT field
					//
					bextMeta->setArray<XMP_Uns8>(BEXTMetadata::kUMID, reinterpret_cast<const XMP_Uns8*>(umid.data()), umid.length());
				}
			}
			else
			{
				bextMeta->deleteValue(BEXTMetadata::kUMID);
			}

			// bext:version  <-> RIFF:WAVE/bext.version
			// Special case: bext.version is always written as 1
			if (inXMP.GetProperty(kXMP_NS_BWF, "version", NULL, 0))
			{
				bextMeta->setValue<XMP_Uns16>(BEXTMetadata::kVersion, 1);
			}
			else
			{
				bextMeta->deleteValue(BEXTMetadata::kVersion);
			}

			// Remove BWF properties from the XMP
			SXMPUtils::RemoveProperties(&inXMP, kXMP_NS_BWF, NULL, kXMPUtil_DoAllProperties );

			changed |= bextMeta->hasChanged();
		}


		//
		// Export cart
		//
		CartMetadata* cartData = outMetaData.get<CartMetadata>();

		if ( cartData != NULL ) 
		{
			IReconcile::exportXMPToNative ( *cartData, inXMP, kCartProperties );
			
			// Export PostTimer
			if ( inXMP.DoesPropertyExist( kXMP_NS_AEScart, "PostTimer" ) )
			{
				if( inXMP.CountArrayItems( kXMP_NS_AEScart, "PostTimer" ) == CartMetadata::kPostTimerLength )
				{
					
					CartMetadata::StoredCartTimer timer[CartMetadata::kPostTimerLength];
					memset ( timer, 0, sizeof(CartMetadata::StoredCartTimer)*CartMetadata::kPostTimerLength );	// Fill with zeros
					std::string path = "";
					std::string usageSt = "";
					std::string valueSt = "";
					XMP_Bool invalidArray = false;
					XMP_Uns32 usage = 0;
					XMP_Uns32 value = 0;
					XMP_Int64 tmp = 0;
					XMP_OptionBits opts;
					

					for ( XMP_Uns32 i = 0; i<CartMetadata::kPostTimerLength && !invalidArray; i++ )
					{	
						// get options of array item
						inXMP.GetArrayItem( kXMP_NS_AEScart, "PostTimer", i+1, NULL, &opts );
						// copose path to array item
						SXMPUtils::ComposeArrayItemPath( kXMP_NS_AEScart, "PostTimer", i+1, &path);						

						if ( opts == kXMP_PropValueIsStruct &&
							 inXMP.DoesStructFieldExist ( kXMP_NS_AEScart, path.c_str(), kXMP_NS_AEScart, "Usage" ) &&
							 inXMP.DoesStructFieldExist ( kXMP_NS_AEScart, path.c_str(), kXMP_NS_AEScart, "Value" ) )
						{
							inXMP.GetStructField( kXMP_NS_AEScart, path.c_str(), kXMP_NS_AEScart, "Usage", &usageSt, NULL);
							inXMP.GetStructField( kXMP_NS_AEScart, path.c_str(), kXMP_NS_AEScart, "Value", &valueSt, NULL);

							if ( stringToFOURCC( usageSt,usage ) )
							{
								// don't add if the String is not set or not exactly 4 characters
								timer[i].usage = usage;
						
								// now the value
								if ( valueSt.length() > 0 )
								{
									tmp = SXMPUtils::ConvertToInt64(valueSt);
									if ( tmp > 0  && tmp <= 0xffffffff)
									{								
										value = static_cast<XMP_Uns32>(tmp); // save because I checked that the number is positiv.
										timer[i].value = value;
									}
									// else value stays 0								
								}
								// else value stays 0
							}
						} 
						else
						{
							invalidArray = true;
						}
					}

					if (!invalidArray) // if the structure of the array is wrong don't add anything
					{
						cartData->setArray<CartMetadata::StoredCartTimer> (CartMetadata::kPostTimer, timer, CartMetadata::kPostTimerLength);
					}
				} // Array length is wrong: don't add anything
			}
			else
			{
				cartData->deleteValue( CartMetadata::kPostTimer );
			}
			SXMPUtils::RemoveProperties ( &inXMP, kXMP_NS_AEScart, 0, kXMPUtil_DoAllProperties );
			changed |= cartData->hasChanged();
		}
	}

	//
	// export LIST:INFO
	//
	INFOMetadata *infoMeta = outMetaData.get<INFOMetadata>();

	if (infoMeta != NULL)
	{
		IReconcile::exportXMPToNative( *infoMeta, inXMP, kINFOProperties );

		if ( inXMP.DoesPropertyExist(	 kXMP_NS_DC, "subject" ) )
		{
			std::string catedStr;
			SXMPUtils::CatenateArrayItems( inXMP, kXMP_NS_DC, "subject", NULL, NULL, kXMP_NoOptions, &catedStr );
			infoMeta->setValue<std::string>(INFOMetadata::kKeywords, catedStr);
		}
		else
		{
			infoMeta->deleteValue( INFOMetadata::kKeywords );
		}

		// Remove RIFFINFO properties from the XMP
		SXMPUtils::RemoveProperties(&inXMP, kXMP_NS_RIFFINFO, NULL, kXMPUtil_DoAllProperties );

		changed |= infoMeta->hasChanged();
	}

	// cr8r is not yet required for WAVE

	////
	//// export cr8r
	////
	//Cr8rMetadata* cr8rMeta = outMetaData.get<Cr8rMetadata>();

	//if( cr8rMeta != NULL )
	//{
	//	changed |= IReconcile::exportXMPToNative( *cr8rMeta, inXMP, kCr8rProperties );
	//}

	//
	// remove WAV digest
	//
	inXMP.DeleteProperty( kXMP_NS_WAV, "NativeDigest" );

	return changed;
}	// exportFromXMP


// ************** Helper Functions ***************** //

bool WAVEReconcile::encodeToHexString ( const XMP_Uns8* input, std::string& output )
{
	bool allZero = true; // assume for now
	XMP_Uns32 kFixedSize = 64; // Only used for UMID Bext field, which is fixed
	output.erase();

	if ( input != 0 )
	{
		output.reserve ( kFixedSize * 2 );
				
		for( XMP_Uns32 i = 0; i < kFixedSize; i++ )
		{
			// first, second nibble
			XMP_Uns8 first = input[i] >> 4;
			XMP_Uns8 second = input[i] & 0xF;

			if ( allZero && (( first != 0 ) || (second != 0)))
				allZero = false;

			output.append( 1, ReconcileUtils::kHexDigits[first] );
			output.append( 1, ReconcileUtils::kHexDigits[second] );	
		}
	}
	return allZero;
}	// encodeToHexString


bool WAVEReconcile::decodeFromHexString ( std::string input, std::string &output)
{
	if ( (input.length() % 2) != 0 )
		return false;
	output.erase();
	output.reserve ( input.length() / 2 );

	for( XMP_Uns32 i = 0; i < input.length(); )
	{	
		XMP_Uns8 upperNibble = input[i];
		if ( (upperNibble < 48) || ( (upperNibble > 57 ) && ( upperNibble < 65 ) ) || (upperNibble > 70) )
		{
			return false;
		}
		if ( upperNibble >= 65 )
		{		
			upperNibble -= 7; // shift A-F area adjacent to 0-9
		}
		upperNibble -= 48; // 'shift' to a value [0..15]
		upperNibble = ( upperNibble << 4 );
		i++;

		XMP_Uns8 lowerNibble = input[i];
		if ( (lowerNibble < 48) || ( (lowerNibble > 57 ) && ( lowerNibble < 65 ) ) || (lowerNibble > 70) )
		{
			return false;
		}
		if ( lowerNibble >= 65 )
		{		
			lowerNibble -= 7; // shift A-F area adjacent to 0-9
		}
		lowerNibble -= 48; // 'shift' to a value [0..15]
		i++;

		output.append ( 1, (upperNibble + lowerNibble) );
	}
	return true;
}	// decodeFromHexString

bool WAVEReconcile::stringToFOURCC ( std::string input, XMP_Uns32 &output )
{
	bool result = false;
	std::string asciiST = "";
	
	// convert to ACSII
	convertToASCII(input, asciiST);
	if ( asciiST.length() == 4 )
	{

		output = GetUns32BE( asciiST.c_str() );
		result = true;
	}
	
	return result;
}
