/**
 * Copyright (C) 2013 The CyanogenMod Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>

#include <cstdio>
#include <vector>
#include <string>
#include <cstring>

// Must be defined to instantiate template classes
#define TXMP_STRING_TYPE std::string 

// Must be defined to give access to XMPFiles
#define XMP_INCLUDE_XMPFILES 1 

// Ensure XMP templates are instantiated
#include "public/include/XMP.incl_cpp"

// Provide access to the API
#include "public/include/XMP.hpp"

#include <iostream>
#include <fstream>

#include "mosaic/Log.h"
#define LOG_TAG "XMPHelper"

/**
 * JNI Declarations
 */
extern "C"
{
    JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved);
    JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved);
    JNIEXPORT jint JNICALL Java_org_cyanogenmod_nemesis_XMPHelper_writeXmpToFile(
            JNIEnv * env, jobject obj, jstring fileName, jstring xmpData);
};

/**
 * Helper functions
 */
void GetJStringContent(JNIEnv *AEnv, jstring AStr, std::string &ARes) {
	if (!AStr) {
		ARes.clear();
		return;
	}

	const char *s = AEnv->GetStringUTFChars(AStr,NULL);
	ARes=s;
	AEnv->ReleaseStringUTFChars(AStr,s);
}



/**
 * JNI Implementation
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    return JNI_VERSION_1_4;
}


JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved)
{

}

JNIEXPORT jint JNICALL Java_org_cyanogenmod_nemesis_XMPHelper_writeXmpToFile(
        JNIEnv * env, jobject obj, jstring fileName, jstring xmpData)
{
	// Convert JString to std::string
	std::string strFileName;
	GetJStringContent(env, fileName, strFileName);

	if(!SXMPMeta::Initialize())
	{
		LOGE("Could not initialize toolkit!");
		return -1;
	}
	
	XMP_OptionBits options = kXMPFiles_ServerMode | kXMPFiles_IgnoreLocalText;
	try
	{
		// Must initialize SXMPFiles before we use it
		if(SXMPFiles::Initialize(options))
		{

			// Options to open the file with - open for editing and use a smart handler
			XMP_OptionBits opts = kXMPFiles_OpenForUpdate | kXMPFiles_OpenUseSmartHandler;

			bool ok;
			SXMPFiles myFile;
			std::string status = "";

			// First we try and open the file
			ok = myFile.OpenFile(strFileName, kXMP_JPEGFile, opts);
			if( ! ok )
			{
				LOGI("No smart handler available for the file");
				LOGI("Trying packet scanning.");

				// Now try using packet scanning
				opts = kXMPFiles_OpenForUpdate | kXMPFiles_OpenUsePacketScanning;
				ok = myFile.OpenFile(strFileName, kXMP_JPEGFile, opts);
			}

			// File open, write to it!
			if(ok)
			{
				// Create XMP from RDF
				std::string strRdf;
				GetJStringContent(env, xmpData, strRdf);

				const char* rdf = strRdf.c_str(); // TODO: Make this less ugly

				SXMPMeta meta;
				// Loop over the rdf string and create the XMP object
				// 10 characters at a time 
				int i;
				for (i = 0; i < (long)strlen(rdf) - 10; i += 10 )
				{
					meta.ParseFromBuffer ( &rdf[i], 10, kXMP_ParseMoreBuffers );
				}
	
				// The last call has no kXMP_ParseMoreBuffers options, signifying 
				// this is the last input buffer
				meta.ParseFromBuffer ( &rdf[i], (XMP_StringLen) strlen(rdf) - i );

				// We now have the XMP Metadata object
				// Serialize the packet and write the buffer to a file
				// Let the padding be computed and use the default linefeed and indents without limits
				std::string metaBuffer;
				meta.SerializeToBuffer(&metaBuffer, 0, 0, "", "", 0);

				// Check we can put the XMP packet back into the file
				if(myFile.CanPutXMP(meta))
				{
					// If so then update the file with the modified XMP
					myFile.PutXMP(meta);
				}
			
				// Close the SXMPFile.  This *must* be called.  The XMP is not
				// actually written and the disk file is not closed until this call is made.
				myFile.CloseFile();

				LOGI("Wrote XMP metadata");
			}
			else
			{
				LOGE("Unable to open the file !!!");
			}
		}

		// Terminate the toolkit
		SXMPFiles::Terminate();
		SXMPMeta::Terminate();
	}
	catch(XMP_Error &e)
	{
		LOGE("ERROR: %s", e.GetErrMsg());
	}
}



