// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2010 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles/source/NativeMetadataSupport/IReconcile.h"

#include "XMPFiles/source/NativeMetadataSupport/MetadataSet.h"
#include "XMPFiles/source/NativeMetadataSupport/IMetadata.h"

#include "XMPFiles/source/FormatSupport/Reconcile_Impl.hpp"

//-----------------------------------------------------------------------------
// 
// WAVEReconcile::importNativeToXMP(...)
// 
// Purpose: [static] Import native metadata container into XMP.
//					 This method is supposed to import all native metadata values 
//					 that are listed in the property table from the IMetadata 
//					 instance to the SXMPMeta instance.
// 
//-----------------------------------------------------------------------------

bool IReconcile::importNativeToXMP( SXMPMeta& outXMP, const IMetadata& nativeMeta, const MetadataPropertyInfo* propertyInfo, bool xmpPriority )
{
	bool changed = false;

	std::string xmpValue;
	XMP_Uns32 index = 0;

	do
	{
		if( propertyInfo[index].mXMPSchemaNS != NULL )
		{
			bool xmpPropertyExists = false;

			//
			// need to know if the XMP property exists either
			// if a priority needs to be considered or if
			// the value isn't set by a native value
			//
			switch( propertyInfo[index].mXMPType )
			{
				case kXMPType_Simple:
				{
					xmpPropertyExists = outXMP.DoesPropertyExist( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName );
				}
				break;

				case kXMPType_Localized:
				{
					std::string actualLang;
					bool result = outXMP.GetLocalizedText( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName, "" , "x-default" , &actualLang, NULL, NULL );
					// If existing and the appropriate one
					xmpPropertyExists = result && (actualLang == "x-default");
				}
				break;

				case kXMPType_Array:
				case kXMPType_OrderedArray:
				{
					xmpPropertyExists = outXMP.DoesArrayItemExist( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName, 1 );
				}
				break;

				default:
				{
					XMP_Throw( "Unknown XMP data type", kXMPErr_InternalFailure );
				}
				break;
			}

			//
			// import native property if there is no priority or
			// the XMP property doesn't exist yet
			//
			if( ! propertyInfo[index].mConsiderPriority || ! xmpPriority || ! xmpPropertyExists )
			{
				//
				// if the native property exists
				//
				if( nativeMeta.valueExists( propertyInfo[index].mMetadataID ) )
				{
					//
					// prepare native property value for XMP
					// depending on the native data type
					//
					xmpValue.erase();

					switch( propertyInfo[index].mNativeType )
					{
						case kNativeType_Str:
						{
							xmpValue = nativeMeta.getValue<std::string>( propertyInfo[index].mMetadataID );
						}
						break;

						case kNativeType_StrASCII:
						{
							convertToASCII( nativeMeta.getValue<std::string>( propertyInfo[index].mMetadataID ), xmpValue );
						}
						break;

						case kNativeType_StrLocal:
						{
							ReconcileUtils::NativeToUTF8( nativeMeta.getValue<std::string>( propertyInfo[index].mMetadataID ), xmpValue );
						}
						break;

						case kNativeType_StrUTF8:
						{
							ReconcileUtils::NativeToUTF8( nativeMeta.getValue<std::string>( propertyInfo[index].mMetadataID ), xmpValue );
						}
						break;

						case kNativeType_Uns64:
						{
							SXMPUtils::ConvertFromInt64( nativeMeta.getValue<XMP_Uns64>( propertyInfo[index].mMetadataID ), "%llu", &xmpValue );
						}
						break;

						case kNativeType_Uns32:
						{
							SXMPUtils::ConvertFromInt( nativeMeta.getValue<XMP_Uns32>( propertyInfo[index].mMetadataID ), "%lu", &xmpValue );
						}
						break;

						case kNativeType_Int32:
						{
							SXMPUtils::ConvertFromInt( nativeMeta.getValue<XMP_Int32>( propertyInfo[index].mMetadataID ), 0 /* default format */, &xmpValue );
						}
						break;

						case kNativeType_Uns16:
						{
							SXMPUtils::ConvertFromInt( nativeMeta.getValue<XMP_Uns16>( propertyInfo[index].mMetadataID ), "%lu", &xmpValue );
						}
						break;

						default:
						{
							XMP_Throw( "Unknown native data type", kXMPErr_InternalFailure );
						}
						break;
					};

					if( ! xmpValue.empty() )
					{
						//
						// set the new XMP value depending on the 
						// XMP data type
						//
						switch( propertyInfo[index].mXMPType )
						{
							case kXMPType_Localized:
							{
								outXMP.SetLocalizedText( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName, NULL, "x-default", xmpValue.c_str() );
							}
							break;

							case kXMPType_Array:
							{
								// Overwrite any existing array
								outXMP.DeleteProperty( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName );
								outXMP.AppendArrayItem( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName, kXMP_PropArrayIsUnordered, xmpValue.c_str(), kXMP_NoOptions );
							}
							break;

							case kXMPType_OrderedArray:
							{
								// Overwrite any existing array
								outXMP.DeleteProperty( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName );
								outXMP.AppendArrayItem( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName, kXMP_PropArrayIsOrdered, xmpValue.c_str(), kXMP_NoOptions );
							}
							break;

							default:
							{
								outXMP.SetProperty( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName, xmpValue.c_str() );	
							}
							break;
						}

						changed = true;
					}
				}
				else if( propertyInfo[index].mDeleteXMPIfNoNative && xmpPropertyExists )
				{
					//
					// native value doesn't exists, delete the XMP property
					//
					outXMP.DeleteProperty( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName );
					changed = xmpPropertyExists;
				}
			}
		}

		index++;

	} while( propertyInfo[index].mXMPSchemaNS != NULL );

	return changed;
}

//-----------------------------------------------------------------------------
// 
// WAVEReconcile::exportXMPToNative(...)
// 
// Purpose: [static] Export XMP values to native metadata container.
//					 This method is supposed to export all native metadata values 
//					 that are listed in the property table from the XMP container 
//					 to the IMetadata instance.
// 
//-----------------------------------------------------------------------------

bool IReconcile::exportXMPToNative( IMetadata& outNativeMeta, SXMPMeta& inXMP, const MetadataPropertyInfo* propertyInfo )
{
	std::string xmpValue;
	XMP_Uns32 index = 0;

	do
	{
		if( propertyInfo[index].mXMPSchemaNS != NULL && propertyInfo[index].mExportPolicy != kExport_Never )
		{
			bool xmpPropertyExists = false;

			//
			// get value from XMP depending on
			// the XMP data type
			//
			switch( propertyInfo[index].mXMPType )
			{
				case kXMPType_Localized:
				{
					std::string lang;
					xmpPropertyExists = inXMP.GetLocalizedText( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName, NULL, "x-default", &lang, &xmpValue, 0 );
				}
				break;

				case kXMPType_Array:
				case kXMPType_OrderedArray:
				{
					// TOTRACK currently only the first array item is used
					if( inXMP.CountArrayItems( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName ) > 0 )
					{
						xmpPropertyExists = inXMP.GetArrayItem( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName, 1, &xmpValue, 0 );
					}
				}
				break;

				default:
				{
					xmpPropertyExists = inXMP.GetProperty( propertyInfo[index].mXMPSchemaNS, propertyInfo[index].mXMPPropName, &xmpValue, 0 );
				}
				break;
			}

			//
			// convert xmp value and set native property
			// depending on the native data type and the export policy
			//
			if( xmpPropertyExists &&
				( propertyInfo[index].mExportPolicy != kExport_InjectOnly || 
				! outNativeMeta.valueExists( propertyInfo[index].mMetadataID ) ) )
			{

				switch( propertyInfo[index].mNativeType )
				{
					case kNativeType_StrASCII:
					{
						std::string ascii;
						convertToASCII( xmpValue, ascii );
						outNativeMeta.setValue<std::string>( propertyInfo[index].mMetadataID, ascii );
					}
					break;

					case kNativeType_Str:
					case kNativeType_StrUTF8:
					{
						outNativeMeta.setValue<std::string>( propertyInfo[index].mMetadataID, xmpValue );
					}
					break;

					case kNativeType_StrLocal:
					{
						std::string value;

						try
						{
							ReconcileUtils::UTF8ToLocal( xmpValue.c_str(), xmpValue.size(), &value );
							outNativeMeta.setValue<std::string>( propertyInfo[index].mMetadataID, value );
						}
						catch( XMP_Error& e )
						{
							if( e.GetID() != kXMPErr_Unavailable )
							{
								// rethrow exception if it wasn't caused
								// by missing encoding functionality (UNIX)
								throw e;
							}
						}
					}
					break;

					case kNativeType_Uns64:
					{
						XMP_Int64 value = 0;
						bool error = false;

						try
						{
							value = SXMPUtils::ConvertToInt64( xmpValue );
						}
						catch( XMP_Error& e )
						{
							if ( e.GetID() != kXMPErr_BadParam )
							{
								// rethrow exception if it wasn't caused by an
								// invalid parameter for the conversion
								throw e;
							}
							error = true;
						}

						// Only write the value if it could be converted to a number and has a positive value
						if( ! error && value >= 0 )
						{
							outNativeMeta.setValue<XMP_Uns64>( propertyInfo[index].mMetadataID, static_cast<XMP_Uns64>(value) );
						}
					}
					break;

					case kNativeType_Uns32:
					{
						XMP_Int32 value = 0;
						bool error = false;

						try
						{
							value = SXMPUtils::ConvertToInt( xmpValue );
						}
						catch( XMP_Error& e )
						{
							if ( e.GetID() != kXMPErr_BadParam )
							{
								// rethrow exception if it wasn't caused by an
								// invalid parameter for the conversion
								throw e;
							}
							error = true;
						}

						// Only write the value if it could be converted to a number and has a positive value
						if( ! error && value >= 0 )
						{
							outNativeMeta.setValue<XMP_Uns32>( propertyInfo[index].mMetadataID, static_cast<XMP_Uns32>(value) );
						}
					}
					break;

					case kNativeType_Int32:
					{
						XMP_Int32 value = 0;
						bool error = false;

						try
						{
							value = SXMPUtils::ConvertToInt( xmpValue );
						}
						catch( XMP_Error& e )
						{
							if ( e.GetID() != kXMPErr_BadParam )
							{
								// rethrow exception if it wasn't caused by an
								// invalid parameter for the conversion
								throw e;
							}
							error = true;
						}

						// Only write the value if it could be converted to a number
						if( ! error )
						{
							outNativeMeta.setValue<XMP_Int32>( propertyInfo[index].mMetadataID, static_cast<XMP_Int32>(value) );
						}
					}
					break;

					case kNativeType_Uns16:
					{
						XMP_Int32 value = 0;
						bool error = false;

						try
						{
							value = SXMPUtils::ConvertToInt( xmpValue );
						}
						catch( XMP_Error& e )
						{
							if ( e.GetID() != kXMPErr_BadParam )
							{
								// rethrow exception if it wasn't caused by an
								// invalid parameter for the conversion
								throw e;
							}
							error = true;
						}
						
						// Only write the value if it could be converted to a number and has a positive value
						if( ! error && value >= 0 )
						{
							outNativeMeta.setValue<XMP_Uns16>( propertyInfo[index].mMetadataID, static_cast<XMP_Uns16>(value) );
						}
					}
					break;

					default:
					{
						XMP_Throw( "Unknown native data type", kXMPErr_InternalFailure );
					}
					break;
				}
			}
			else
			{
				if( propertyInfo[index].mExportPolicy == kExport_Always )
				{
					//
					// delete native value if the corresponding XMP value doesn't exists
					//
					outNativeMeta.deleteValue( propertyInfo[index].mMetadataID );
				}
			}
		}

		index++;

	} while( propertyInfo[index].mXMPSchemaNS != NULL );

	return outNativeMeta.hasChanged();
}

//-----------------------------------------------------------------------------
// 
// IReconcile::convertToASCII(...)
// 
// Purpose: Converts input string to an ascii output string
//			- terminates at first 0
//			- replaces all non ascii with 0x3F ('?')
// 
//-----------------------------------------------------------------------------

void IReconcile::convertToASCII( const std::string& input, std::string& output )
{
	output.erase();
	output.reserve( input.length() );

	bool isUTF8 = ReconcileUtils::IsUTF8( input.c_str(), input.length() );

	XMP_StringPtr iter = input.c_str();

	for ( XMP_Uns32 i=0; i < input.length(); i++ )
	{
		XMP_Uns8 c = (XMP_Uns8) iter[i];
		if ( c == 0 )  // early 0 termination, leave.
			break;
		if ( c > 127 ) // uft-8 multi-byte sequence.
		{
			if ( isUTF8 ) // skip all high bytes
			{
				// how many bytes in this ?
				if ( c >= 0xC2 && c <= 0xDF )
					i+=1; // 2-byte sequence
				else if ( c >= 0xE0 && c <= 0xEF )
					i+=2; // 3-byte sequence
				else if ( c >= 0xF0 && c <= 0xF4 )
					i+=3; // 4-byte sequence
				else
					continue; //invalid sequence, look for next 'low' byte ..
			} // thereafter and 'else': just append a question mark:
			output.append( 1, '?' );
		}
		else // regular valid ascii. 1 byte.
		{
			output.append( 1, iter[i] );
		}
	}
}	// convertToASCII

