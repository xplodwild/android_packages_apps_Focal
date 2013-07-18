// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2012 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles/source/FormatSupport/PostScript_Support.hpp"
#include "XMP.hpp"
#include <algorithm>
#include <climits>

// =================================================================================================
// PostScript_Support::HasCodesGT127
// =================================
//
// function to detect character codes greater than 127 in a string
bool PostScript_Support::HasCodesGT127(const std::string & value)
{
	size_t vallen=value.length();
	for (size_t index=0;index<vallen;index++)
	{
		if ((unsigned char)value[index]>127)
		{
			return true;
		}
	}
	return false;
}

// =================================================================================================
// PostScript_Support::SkipTabsAndSpaces
// =====================================
//
// function moves the file pointer ahead such that it skips all tabs and spaces 
bool  PostScript_Support::SkipTabsAndSpaces(XMP_IO* file,IOBuffer& ioBuf)
{
	while ( true ) 
	{	
		if ( ! CheckFileSpace ( file, &ioBuf, 1 ) ) return false;
		if ( ! IsSpaceOrTab ( *ioBuf.ptr ) ) break;
		++ioBuf.ptr;
	}
	return true;
}

// =================================================================================================
// PostScript_Support::SkipUntilNewline
// ====================================
//
// function moves the file pointer ahead such that it skips all characters until a newline 
bool PostScript_Support::SkipUntilNewline(XMP_IO* file,IOBuffer& ioBuf)
{
	char ch;
	do 
	{
		if ( ! CheckFileSpace ( file, &ioBuf, 1 ) ) return false;
		ch = *ioBuf.ptr;
		++ioBuf.ptr;
	} while ( ! IsNewline ( ch ) );
	if (ch==kCR &&*ioBuf.ptr==kLF)
	{
		if ( ! CheckFileSpace ( file, &ioBuf, 1 ) ) return false;
		++ioBuf.ptr;
	}
	return true;
}


// =================================================================================================
// RevRefillBuffer and RevCheckFileSpace
// ======================================
//
// These helpers are similar to RefillBuffer and CheckFileSpace with the difference that the it traverses
// the file stream in reverse order
void PostScript_Support::RevRefillBuffer ( XMP_IO* fileRef, IOBuffer* ioBuf )
{
	// Refill including part of the current data, seek back to the new buffer origin and read.
	size_t reverseSeek = ioBuf->limit - ioBuf->ptr;
	if (ioBuf->filePos>kIOBufferSize)
	{
		ioBuf->filePos = fileRef->Seek ( -((XMP_Int64)(kIOBufferSize+reverseSeek)), kXMP_SeekFromCurrent );
		ioBuf->len = fileRef->Read ( &ioBuf->data[0], kIOBufferSize );
		ioBuf->ptr = &ioBuf->data[0]+ioBuf->len;
		ioBuf->limit = ioBuf->ptr ;
	}
	else
	{
		XMP_Int64 rev = (ioBuf->ptr-&ioBuf->data[0]) + ioBuf->filePos;
		ioBuf->filePos = fileRef->Seek ( 0, kXMP_SeekFromStart );		
		ioBuf->len = fileRef->Read ( &ioBuf->data[0], kIOBufferSize );
		if ( rev > (XMP_Int64)ioBuf->len )throw XMP_Error ( kXMPErr_ExternalFailure, "Seek failure in FillBuffer" );
		ioBuf->ptr = &ioBuf->data[0]+rev;
		ioBuf->limit = &ioBuf->data[0]+ioBuf->len;
	}


}
bool PostScript_Support::RevCheckFileSpace ( XMP_IO* fileRef, IOBuffer* ioBuf, size_t neededLen )
{
	if ( size_t(ioBuf->ptr -  &ioBuf->data[0]) < size_t(neededLen) ) 
	{	// ! Avoid VS.Net compare warnings.
		PostScript_Support::RevRefillBuffer ( fileRef, ioBuf );
	}
	return (size_t(ioBuf->ptr -  &ioBuf->data[0]) >= size_t(neededLen));
}

// =================================================================================================
// SearchBBoxInTrailer
// ===================
//
// Function searches the Bounding Box in the comments after the %Trailer
// this function gets called when the DSC comment BoundingBox: value is
// (atend)
// returns true if atleast one BoundingBox: is found after %Trailer
inline static bool SearchBBoxInTrailer(XMP_IO* fileRef,IOBuffer& ioBuf)
{
	bool bboxfoundintrailer=false;
	if ( !  PostScript_Support::SkipTabsAndSpaces( fileRef, ioBuf ) ) return false;
	if ( ! IsNewline ( *ioBuf.ptr ) ) return false;
	++ioBuf.ptr;
	// Scan for all the %%Trailer outside %%BeginDocument: & %%EndDocument comments
	while(true)
	{
		if ( ! CheckFileSpace ( fileRef, &ioBuf, kPSContainsBeginDocString.length() ) ) return false;
		if ( CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsTrailerString.c_str()), kPSContainsTrailerString.length() )) 
		{
			//found %%Trailer now search for proper %%BoundingBox
			ioBuf.ptr+=kPSContainsTrailerString.length();
			//skip chars after  %%Trailer till newline
			if ( !  PostScript_Support::SkipUntilNewline( fileRef, ioBuf ) ) return false;
			while(true)
			{
				if ( ! CheckFileSpace ( fileRef, &ioBuf, kPSContainsBBoxString.length() ) ) return false;
				//check for  "%%BoundingBox:"
				if (CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsBBoxString.c_str()), kPSContainsBBoxString.length() ) )
				{
					//found  "%%BoundingBox:"
					ioBuf.ptr+=kPSContainsBBoxString.length();
					// Skip leading spaces and tabs.
					if ( !  PostScript_Support::SkipTabsAndSpaces( fileRef, ioBuf ) ) return false;
					if ( IsNewline ( *ioBuf.ptr ) ) return false;	// Reached the end of the %%BoundingBox comment.
					bboxfoundintrailer=true;
					break;
				}
				//skip chars till newline
				if ( !  PostScript_Support::SkipUntilNewline( fileRef, ioBuf ) ) return false;
			}
			if (!bboxfoundintrailer)
				return false;
			else
				break;
		} 
		else if (  CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsBeginDocString.c_str()), kPSContainsBeginDocString.length() ) )
		{
			//"%%BeginDocument:" Found search for "%%EndDocument"
			ioBuf.ptr+=kPSContainsBeginDocString.length();
			//skip chars after "%%BeginDocument:" till newline
			if ( !  PostScript_Support::SkipUntilNewline( fileRef, ioBuf ) ) return false;
			while(true)
			{
				if ( ! CheckFileSpace ( fileRef, &ioBuf, kPSContainsEndDocString.length() ) ) return false;
				//check for  "%%EndDocument"
				if (CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsEndDocString.c_str()), kPSContainsEndDocString.length() ) )
				{
					//found  "%%EndDocument"
					ioBuf.ptr+=kPSContainsEndDocString.length();
					break;
				}
				//skip chars till newline
				if ( !  PostScript_Support::SkipUntilNewline( fileRef, ioBuf ) ) return false;
			}// while to search %%EndDocument

		} //else if to consume a pair of %%BeginDocument: and %%EndDocument
		//skip chars till newline
		if ( !  PostScript_Support::SkipUntilNewline( fileRef, ioBuf ) ) return false;
	}// while for scanning %%BoundingBox after %%Trailer
	if (!bboxfoundintrailer) return false;
	return true;
}

// =================================================================================================
// PostScript_Support::IsValidPSFile
// =================================
//
// Determines if the file is a valid PostScript or EPS file
// Checks done
//		Looks for a valid Poscript header
//		For EPS file checks for a valid Bounding Box comment
bool PostScript_Support::IsValidPSFile(XMP_IO*    fileRef,XMP_FileFormat &format)
{
	IOBuffer ioBuf;
	XMP_Int64 psOffset;
	size_t    psLength;
	XMP_Uns32 fileheader,psMajorVer, psMinorVer,epsMajorVer,epsMinorVer;
	char ch;
	// Check for the binary EPSF preview header.

	fileRef->Rewind();
	if ( ! CheckFileSpace ( fileRef, &ioBuf, 4 ) ) return false;
	fileheader = GetUns32BE ( ioBuf.ptr );

	if ( fileheader == 0xC5D0D3C6 ) 
	{

		if ( ! CheckFileSpace ( fileRef, &ioBuf, 30 ) ) return false;

		psOffset = GetUns32LE ( ioBuf.ptr+4 );	// PostScript offset.
		psLength = GetUns32LE ( ioBuf.ptr+8 );	// PostScript length.

		FillBuffer ( fileRef, psOffset, &ioBuf );	// Make sure buffer starts at psOffset for length check.
		if ( (ioBuf.len < kIOBufferSize) && (ioBuf.len < psLength) ) return false;	// Not enough PostScript.

	}

	// Check the start of the PostScript DSC header comment.

	if ( ! CheckFileSpace ( fileRef, &ioBuf, (kPSFileTag.length() + 3 + 1) ) ) return false;
	if ( ! CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSFileTag.c_str()), kPSFileTag.length() ) ) return false;
	ioBuf.ptr += kPSFileTag.length();

	// Check the PostScript DSC major version number.

	psMajorVer = 0;
	while ( (ioBuf.ptr < ioBuf.limit) && IsNumeric( *ioBuf.ptr ) ) 
	{
		psMajorVer = (psMajorVer * 10) + (*ioBuf.ptr - '0');
		if ( psMajorVer > 1000 ) return false;	// Overflow.
		ioBuf.ptr += 1;
	}
	if ( psMajorVer < 3 ) return false;	// The version must be at least 3.0.

	if ( ! CheckFileSpace ( fileRef, &ioBuf, 3 ) ) return false;
	if ( *ioBuf.ptr != '.' ) return false;	// No minor number.
	ioBuf.ptr += 1;

	// Check the PostScript DSC minor version number.

	psMinorVer = 0;
	while ( (ioBuf.ptr < ioBuf.limit) && IsNumeric( *ioBuf.ptr ) ) 
	{
		psMinorVer = (psMinorVer * 10) + (*ioBuf.ptr - '0');
		if ( psMinorVer > 1000 ) return false;	// Overflow.
		ioBuf.ptr += 1;
	}

	switch( format ) 
	{
		case kXMP_PostScriptFile:
		{
			// Almost done for plain PostScript, check for whitespace.

			if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
			if ( ! IsWhitespace(*ioBuf.ptr) ) return false;
			ioBuf.ptr += 1;
		
			break;
		}
		case kXMP_UnknownFile:
		{
			if ( ! SkipTabsAndSpaces ( fileRef, ioBuf ) ) return false;

			if ( ! CheckFileSpace ( fileRef, &ioBuf, 5 ) ) return false;
			// checked PS header to this point Atkleast a PostScript File
			format=kXMP_PostScriptFile;
			//return true if no "EPSF-" is found as it is a valid PS atleast
			if ( ! CheckBytes ( ioBuf.ptr, Uns8Ptr("EPSF-"), 5 ) ) return true;
		}
		case kXMP_EPSFile:
		{

			if ( ! SkipTabsAndSpaces ( fileRef, ioBuf ) ) return false;
			// Check for the EPSF keyword on the header comment.

			if ( ! CheckFileSpace ( fileRef, &ioBuf, 5+3+1 ) ) return false;
			if ( ! CheckBytes ( ioBuf.ptr, Uns8Ptr("EPSF-"), 5 ) ) return false;
			ioBuf.ptr += 5;

			// Check the EPS major version number.

			epsMajorVer = 0;
			while ( (ioBuf.ptr < ioBuf.limit) &&IsNumeric( *ioBuf.ptr ) ) {
				epsMajorVer = (epsMajorVer * 10) + (*ioBuf.ptr - '0');
				if ( epsMajorVer > 1000 ) return false;	// Overflow.
				ioBuf.ptr += 1;
			}
			if ( epsMajorVer < 3 ) return false;	// The version must be at least 3.0.

			if ( ! CheckFileSpace ( fileRef, &ioBuf, 3 ) ) return false;
			if ( *ioBuf.ptr != '.' ) return false;	// No minor number.
			ioBuf.ptr += 1;

			// Check the EPS minor version number.

			epsMinorVer = 0;
			while ( (ioBuf.ptr < ioBuf.limit) && IsNumeric( *ioBuf.ptr ) ) {
				epsMinorVer = (epsMinorVer * 10) + (*ioBuf.ptr - '0');
				if ( epsMinorVer > 1000 ) return false;	// Overflow.
				ioBuf.ptr += 1;
			}
		
			if ( ! SkipTabsAndSpaces ( fileRef, ioBuf ) ) return false;
			if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
			if ( ! IsNewline( *ioBuf.ptr ) ) return false;
			ch=*ioBuf.ptr;
			ioBuf.ptr += 1;
			if (ch==kCR &&*ioBuf.ptr==kLF)
			{
				if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
				++ioBuf.ptr;
			}


			while ( true ) 
			{
				if ( ! CheckFileSpace ( fileRef, &ioBuf, kPSContainsBBoxString.length() ) ) return false;

				if ( CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSEndCommentString.c_str()), kPSEndCommentString.length() ) //explicit endcommentcheck
					|| *ioBuf.ptr!='%' || !(*(ioBuf.ptr+1)>32 && *(ioBuf.ptr+1)<=126 )) // implicit endcomment check
				{
					// Found "%%EndComments", don't look any further.
					return false;
				} 
				else if ( ! CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsBBoxString.c_str()), kPSContainsBBoxString.length() ) )
				{
					// Not "%%EndComments" or "%%BoundingBox:", skip past the end of this line.
					 if ( ! SkipUntilNewline ( fileRef, ioBuf ) ) return true;
				} 
				else 
				{

					// Found "%%BoundingBox:", look for llx lly urx ury.
					ioBuf.ptr += kPSContainsBBoxString.length();
					//Check for atleast a mandatory space b/w "%%BoundingBox:" and llx lly urx ury
					if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
					if ( ! IsSpaceOrTab ( *ioBuf.ptr ) ) return false;
					ioBuf.ptr++;

					while ( true ) 
					{
						// Skip leading spaces and tabs.
						if ( ! SkipTabsAndSpaces( fileRef, ioBuf ) ) return false;
						if ( IsNewline ( *ioBuf.ptr ) ) return false;	// Reached the end of the %%BoundingBox comment.

						//if the comment is %%BoundingBox: (atend) go past the %%Trailer to check BBox
						bool bboxfoundintrailer=false;
						if (*ioBuf.ptr=='(')
						{
							if ( ! CheckFileSpace ( fileRef, &ioBuf, kPSContainsAtendString.length() ) ) return false;

							if ( CheckBytes ( ioBuf.ptr, Uns8Ptr(kPSContainsAtendString.c_str()), kPSContainsAtendString.length() ) ) 
							{
								// search for Bounding Box Past Trailer
								ioBuf.ptr += kPSContainsAtendString.length();
								bboxfoundintrailer=SearchBBoxInTrailer( fileRef, ioBuf );
							}
						
							if (!bboxfoundintrailer)
								return false;

						}//if (*ioBuf.ptr=='(')

						int noOfIntegers=0;
						// verifies for llx lly urx ury.
						while(true)
						{
							if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
							if(IsPlusMinusSign(*ioBuf.ptr ))
								++ioBuf.ptr;
							bool atleastOneNumeric=false;
							while ( true) 
							{
								if ( ! CheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
								if (!IsNumeric ( *ioBuf.ptr ) ) break;
								++ioBuf.ptr;
								atleastOneNumeric=true;
							}
							if (!atleastOneNumeric) return false;
						
							if ( ! SkipTabsAndSpaces( fileRef, ioBuf ) ) return false;
							noOfIntegers++;
							if ( IsNewline ( *ioBuf.ptr ) ) break;
						}
						if (noOfIntegers!=4) 
							return false;
						format=kXMP_EPSFile;
						return true;
					}	

				}	//Found "%%BoundingBox:"

			}	// Outer marker loop.
		}
		default:
		{
			return false;
		}
	}
	return true;
}


// =================================================================================================
// PostScript_Support::IsSFDFilterUsed
// =================================
//
// Determines Whether the metadata is embedded using the Sub-FileDecode Approach or no
//	In case of Sub-FileDecode filter approach the metaData can be easily extended without
//	the need to inject a new XMP packet before the existing Packet.
// 
bool PostScript_Support::IsSFDFilterUsed(XMP_IO* &fileRef, XMP_Int64 xpacketOffset)
{
	IOBuffer ioBuf;
	fileRef->Rewind();
	fileRef->Seek((xpacketOffset/kIOBufferSize)*kIOBufferSize,kXMP_SeekFromStart);
	if ( ! CheckFileSpace ( fileRef, &ioBuf,xpacketOffset%kIOBufferSize ) ) return false;
	ioBuf.ptr+=(xpacketOffset%kIOBufferSize);
	//skip white spaces
	while(true)
	{
		if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
		if (!IsWhitespace(*ioBuf.ptr)) break;
		--ioBuf.ptr;
	} 
	std::string temp;
	bool filterFound=false;
	while(true)
	{
		if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
		if (*ioBuf.ptr==')')
		{
			--ioBuf.ptr;
			while(true)
			{
				//get the string till '('
				if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false ;
				temp+=*ioBuf.ptr;
				--ioBuf.ptr;
				if (*ioBuf.ptr=='(')
				{
					if(filterFound)
					{
						reverse(temp.begin(), temp.end());
						if(!temp.compare("SubFileDecode"))
							return true;
					}
					if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false ;
					--ioBuf.ptr;
					temp.clear();
					break;
				}
			}
			
			filterFound=false;
		}
		else if(*ioBuf.ptr=='[')
		{
			//end of SubFileDecode Filter parsing
			return false;
		}
		else if(*ioBuf.ptr=='k' )
		{
			if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 4 ) ) return false;
			if(IsWhitespace(*(ioBuf.ptr-4))&& *(ioBuf.ptr-3)=='m'
				&& *(ioBuf.ptr-2)=='a' && *(ioBuf.ptr-1)=='r' )
				//end of SubFileDecode Filter parsing
				return false;
			while(true)//ignore till any special mark
			{
				if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 4 ) ) return false;
				if (IsWhitespace(*(ioBuf.ptr))||*(ioBuf.ptr)=='['||
					*(ioBuf.ptr)=='<' ||*(ioBuf.ptr)=='>') break;
				--ioBuf.ptr;
			}
			filterFound=false;
		}
		else if(*ioBuf.ptr=='<')
		{
			--ioBuf.ptr;
			if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
			if (*(ioBuf.ptr)=='<')
			{
				//end of SubFileDecode Filter parsing
				return false;
			}
			while(true)//ignore till any special mark
			{
				if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 4 ) ) return false;
				if (IsWhitespace(*(ioBuf.ptr))||*(ioBuf.ptr)=='['||
					*(ioBuf.ptr)=='<' ||*(ioBuf.ptr)=='>') break;
				--ioBuf.ptr;
			}
			filterFound=false;
		}
		else if(*ioBuf.ptr=='>')
		{
			--ioBuf.ptr;
			if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
			if (*(ioBuf.ptr)=='>')//ignore the dictionary
			{
				--ioBuf.ptr;
				XMP_Int16 count=1;
				while(true)
				{
					if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 2 ) ) return false;
					if(*(ioBuf.ptr)=='<' && *(ioBuf.ptr-1)=='<')
					{
						count--;
						ioBuf.ptr-=2;
					}
					else if(*(ioBuf.ptr)=='>' && *(ioBuf.ptr-1)=='>')
					{
						count++;
						ioBuf.ptr-=2;
					}
					else
					{
						ioBuf.ptr-=1;
					}
					if(count==0)
						break;
				}
			}
			filterFound=false;
		}
		else
		{
			while(true)
			{
				if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
				temp+=(*ioBuf.ptr);
				--ioBuf.ptr;
				if (*ioBuf.ptr=='/')
				{
					if(filterFound)
					{
						reverse(temp.begin(), temp.end());
						if(!temp.compare("SubFileDecode"))
							return true;
					}
					temp.clear();
					filterFound=false;
					break;
				}
				else if(IsWhitespace(*ioBuf.ptr))
				{
					reverse(temp.begin(), temp.end());
					if(!temp.compare("filter")&&!filterFound)
						filterFound=true;
					else
						filterFound=false;
					temp.clear();
					break;
				}
			}
			if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
				--ioBuf.ptr;
		}
		while(true)
		{
			if ( ! PostScript_Support::RevCheckFileSpace ( fileRef, &ioBuf, 1 ) ) return false;
			if (!IsWhitespace(*ioBuf.ptr)) break;
			--ioBuf.ptr;
		} 
						
	}
	return false;

}


// =================================================================================================
// constructDateTime
// =================================
//
// If input string date is of the format D:YYYYMMDDHHmmSSOHH'mm' and valid
// output format YYYY-MM-DDThh:mm:ssTZD is returned
// 
static void constructDateTime(const std::string &input,std::string& outDate)
{
	std::string date;
	XMP_DateTime datetime;
	std::string verdate;
	size_t start =0;
	if(input[0]=='D' && input[1]==':')
		start=2;
	if (input.length() >=14+start)
	{
		for(int x=0;x<4;x++)
		{
			date+=input[start+x];//YYYY
		}
		date+='-';
		start+=4;
		for(int x=0;x<2;x++)
		{
			date+=input[start+x];//MM
		}
		date+='-';
		start+=2;
		for(int x=0;x<2;x++)
		{
			date+=input[start+x];//DD
		}
		date+='T';
		start+=2;
		for(int x=0;x<2;x++)
		{
			date+=input[start+x];//HH
		}
		
		date+=':';
		start+=2;
		for(int x=0;x<2;x++)
		{
			date+=input[start+x];//MM
		}
		
		date+=':';
		start+=2;
		for(int x=0;x<2;x++)
		{
			date+=input[start+x];//SS
		}
		start+=2;
		if ((input[start]=='+' || input[start]=='-' )&&input.length() ==19+start)
		{
			date+=input[start];
			start++;
			for(int x=0;x<2;x++)
			{
				date+=input[start+x];//hh
			}
			date+=':';
			start+=2;
			for(int x=0;x<2;x++)
			{
				date+=input[start+x];//mm
			}
		}
		else
		{
			date+='Z';
		}
		
		try
		{
			SXMPUtils::ConvertToDate ( date.c_str(),&datetime);
			SXMPUtils::ConvertFromDate (datetime,&verdate);
		}
		catch(...)
		{
			return;
		}
		outDate=verdate;
	}
	
}

// =================================================================================================
// GetNumber
// =========
//
// Extracts number from a char string
// 
static short GetNumber(const char **inString,short noOfChars=SHRT_MAX)
{
	const char* inStr=*inString;
	short number=0;
	while(IsNumeric ( *inStr )&& noOfChars--)
	{
		number=number*10 +inStr[0]-'0';
		inStr++;
	}
	*inString=inStr;
	return number;
}

// =================================================================================================
// tokeniseDateString
// ==================
//
// Parses Date string and tokenizes it for extracting different parts of a date
// 
static bool tokeniseDateString(const char* inString,std::vector<PostScript_Support::DateTimeTokens>  &tokens )
{
	const char* inStr= inString;
	PostScript_Support::DateTimeTokens dttoken;
	//skip leading whitespace
	while ( inStr[0]!='\0' ) 
	{	
		while( IsSpaceOrTab ( *inStr ) || *inStr =='(' 
			||*inStr ==')' ||*inStr==',')
		{
			++inStr;
		}
		if (*inStr=='\0') return true;
		dttoken=PostScript_Support::DateTimeTokens();
		if (IsNumeric(*inStr))
		{
			while(IsNumeric(*inStr)||(IsDelimiter(*inStr)&&dttoken.noOfDelimiter==0)||
				(dttoken.delimiter==*inStr && dttoken.noOfDelimiter!=0))
			{
				if (IsDelimiter(*inStr))
				{
					dttoken.delimiter=inStr[0];
					dttoken.noOfDelimiter++;
				}
				dttoken.token+=*(inStr++);
			}
			tokens.push_back(dttoken);
		}
		else if (IsAlpha(*inStr))
		{
			if(*inStr=='D'&& *(inStr+1)==':')
			{
				inStr+=2;
				while( IsNumeric(*inStr))
				{
					dttoken.token+=*(inStr++);
				}
			}
			else
			{
				while( IsAlpha(*inStr))
				{
					dttoken.token+=*(inStr++);
				}

			}
			tokens.push_back(dttoken);
		}
		else if (*inStr=='-' ||*inStr=='+')
		{
			dttoken.token+=*(inStr++);
			while( IsNumeric(*inStr)||*inStr==':')
			{
				if (*inStr==':')
				{
					dttoken.delimiter=inStr[0];
					dttoken.noOfDelimiter++;
				}
				dttoken.token+=*(inStr++);
			}
			tokens.push_back(dttoken);
		}
		else
		{
			++inStr;
		}
	}
	return true;
}

// =================================================================================================
// SwapMonthDateIfNeeded
// =====================
//
// Swaps month and date value if it creates a possible valid date
// 
static void SwapMonthDateIfNeeded(short &day, short&month)
{
	if(month>12&& day<13)
	{
		short temp=month;
		month=day;
		day=temp;
	}
}

// =================================================================================================
// AdjustYearIfNeeded
// =====================
//
// Guess the year for a two digit year in a date
//
static void AdjustYearIfNeeded(short &year)
{
	if (year<100)
	{
		if (year >40)
		{
			year=1900+year;
		}
		else
		{
			year=2000+year;
		}
	}
}

static bool IsLeapYear ( XMP_Int32 year )
{
	if ( year < 0 ) year = -year + 1;		// Fold the negative years, assuming there is a year 0.
	if ( (year % 4) != 0 ) return false;	// Not a multiple of 4.
	if ( (year % 100) != 0 ) return true;	// A multiple of 4 but not a multiple of 100.
	if ( (year % 400) == 0 ) return true;	// A multiple of 400.
	return false;							// A multiple of 100 but not a multiple of 400.
}
// =================================================================================================
// PostScript_Support::ConvertToDate
// =================================
//
// Converts date string from native of Postscript to YYYY-MM-DDThh:mm:ssTZD if a valid date is identified
//
std::string PostScript_Support::ConvertToDate(const char* inString)
{
	PostScript_Support::Date date(0,0,0,0,0,0);
	std::string dateTimeString; 
	short number=0;
	std::vector<PostScript_Support::DateTimeTokens>   tokenzs;
    tokeniseDateString(inString,tokenzs );
	std::vector<PostScript_Support::DateTimeTokens>:: const_iterator itr=tokenzs.begin();
	for(;itr!=tokenzs.end();itr++)
	{
		if(itr->token[0]=='+' ||itr->token[0]=='-')
		{
			const char *str=itr->token.c_str();
			date.offsetSign=*(str++);
			date.offsetHour=GetNumber(&str,2);
			if (*str==':')str++;
			date.offsetMin=GetNumber(&str,2);
			if (!(date.offsetHour<=12 && date.offsetHour>=0
				&&date.offsetMin>=0 && date.offsetMin<=59))
			{
				date.offsetSign='+';
				date.offsetHour=0;
				date.offsetMin=0;
			}
			else
			{
				date.containsOffset= true;
			}
		}
		else if(itr->noOfDelimiter!=0)
		{//either a date or time token
			if(itr->noOfDelimiter==2 && itr->delimiter=='/')
			{
				if(date.day==0&& date.month==0 && date.year==0)
				{
					const char *str=itr->token.c_str();
					number=GetNumber(&str);
					str++;
					if (number<32)
					{
						date.month=number;
						date.day=GetNumber(&str);
						SwapMonthDateIfNeeded(date.day, date.month);
						str++;
						date.year=GetNumber(&str);
						AdjustYearIfNeeded(date.year);
					}
					else
					{
						date.year=number;
						AdjustYearIfNeeded(date.year);
						date.month=GetNumber(&str);
						str++;
						date.day=GetNumber(&str);
						SwapMonthDateIfNeeded(date.day, date.month);
					}
				}
			}
			else if(itr->noOfDelimiter==2 && itr->delimiter==':')
			{
				const char *str=itr->token.c_str();
				date.hours=GetNumber(&str);
				str++;
				date.minutes=GetNumber(&str);
				str++;
				date.seconds=GetNumber(&str);
				if (date.hours>23|| date.minutes>59|| date.seconds>59)
				{
					date.hours=0;
					date.minutes=0;
					date.seconds=0;
				}
			}
			else if(itr->noOfDelimiter==1 && itr->delimiter==':')
			{
				const char *str=itr->token.c_str();
				date.hours=GetNumber(&str);
				str++;
				date.minutes=GetNumber(&str);
				if (date.hours>23|| date.minutes>59)
				{
					date.hours=0;
					date.minutes=0;
				}
			}
			else if(itr->noOfDelimiter==2 && itr->delimiter=='-')
			{
				const char *str=itr->token.c_str();
				number=GetNumber(&str);
				str++;
				if (number>31)
				{
					date.year=number;
					AdjustYearIfNeeded(date.year);
					date.month=GetNumber(&str);
					str++;
					date.day=GetNumber(&str);
					SwapMonthDateIfNeeded(date.day, date.month);
				}
				else
				{
					date.month=number;
					date.day=GetNumber(&str);
					str++;
					SwapMonthDateIfNeeded(date.day, date.month);
					date.year=GetNumber(&str);
					AdjustYearIfNeeded(date.year);
				}
			}
			else if(itr->noOfDelimiter==2 && itr->delimiter=='.')
			{
				const char *str=itr->token.c_str();
				date.year=GetNumber(&str);
				str++;
				AdjustYearIfNeeded(date.year);
				date.month=GetNumber(&str);
				str++;
				date.day=GetNumber(&str);
				SwapMonthDateIfNeeded(date.day, date.month);
				
			}
		}
		else if (IsAlpha(itr->token[0]))
		{	//this can be a name of the Month
			// name of the day is ignored
			short month=0;
			std::string lowerToken=itr->token;
			std::transform(lowerToken.begin(), lowerToken.end(), lowerToken.begin(), ::tolower);
			if(!lowerToken.compare("jan")||!lowerToken.compare("january"))
			{
				month=1;
			}
			else if (!lowerToken.compare("feb")||!lowerToken.compare("february"))
			{
				month=2;
			}
			else if (!lowerToken.compare("mar")||!lowerToken.compare("march"))
			{
				month=3;
			}
			else if (!lowerToken.compare("apr")||!lowerToken.compare("april"))
			{
				month=4;
			}
			else if (!lowerToken.compare("may"))
			{
				month=5;
			}
			else if (!lowerToken.compare("jun")||!lowerToken.compare("june"))
			{
				month=6;
			}
			else if (!lowerToken.compare("jul")||!lowerToken.compare("july"))
			{
				month=7;
			}
			else if (!lowerToken.compare("aug")||!lowerToken.compare("august"))
			{
				month=8;
			}
			else if (!lowerToken.compare("sep")||!lowerToken.compare("september"))
			{
				month=9;
			}
			else if (!lowerToken.compare("oct")||!lowerToken.compare("october"))
			{
				month=10;
			}
			else if (!lowerToken.compare("nov")||!lowerToken.compare("november"))
			{
				month=11;
			}
			else if (!lowerToken.compare("dec")||!lowerToken.compare("december"))
			{
				month=12;
			}
			else if (!lowerToken.compare("pm"))
			{
				if (date.hours<13)
				{
					date.hours+=12;
				}
			}
			else if (itr->token.length()>14)
			{
				constructDateTime(itr->token,dateTimeString);
			}
			if(month!=0 && date.month==0)
			{
				date.month=month;
				if(date.day==0)
				{
					if(itr!=tokenzs.begin())
					{
						--itr;
						if (itr->noOfDelimiter==0 && IsNumeric(itr->token[0]) )
						{
							const char * str=itr->token.c_str();
							short day= GetNumber(&str);
							if (day<=31 &&day >=1)
							{
								date.day=day;
							}
						}
						++itr;
					}
					if(itr!=tokenzs.end())
					{
						++itr;
						if (itr!=tokenzs.end()&&itr->noOfDelimiter==0 && IsNumeric(itr->token[0]) )
						{
							const char * str=itr->token.c_str();
							short day= GetNumber(&str);
							if (day<=31 &&day >=1)
							{
								date.day=day;
							}
						}
					}
				}
			}
		}
		else if (IsNumeric(itr->token[0]) && date.year==0&&itr->token.length()==4)
		{//this is the year
			const char * str=itr->token.c_str();
			date.year=GetNumber(&str);
		}
		else if (itr->token.length()>=14)
		{
			constructDateTime(itr->token,dateTimeString);
		}
	}
	if (dateTimeString.length()==0)
	{
		char dtstr[100];
		XMP_DateTime datetime;
		if ( date.year < 10000 && date.month < 13 && date.month > 0 && date.day > 0 )
		{
			bool isValidDate=true;
			if ( date.month == 2 )
			{
				if ( IsLeapYear ( date.year ) )
				{
					if ( date.day > 29 )  isValidDate=false;
				}
				else
				{
					if ( date.day > 28 )  isValidDate=false;
				}
			}
			else if ( date.month == 4 || date.month == 6 || date.month == 9
				|| date.month == 11 )
			{
				if ( date.day > 30 )  isValidDate=false;
			}
			else
			{
				if ( date.day > 31 )  isValidDate=false;
			}
			if( isValidDate && ! ( date == PostScript_Support::Date ( 0, 0, 0, 0, 0, 0 ) ) )
			{
				if ( date.containsOffset )
				{
					sprintf(dtstr,"%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d",date.year,date.month,date.day,
							date.hours,date.minutes,date.seconds,date.offsetSign,date.offsetHour,date.offsetMin);
				}
				else
				{
					sprintf(dtstr,"%04d-%02d-%02dT%02d:%02d:%02dZ",date.year,date.month,date.day,
							date.hours,date.minutes,date.seconds);
				}
				try
				{
			
					SXMPUtils::ConvertToDate ( dtstr, &datetime ) ;
					SXMPUtils::ConvertFromDate ( datetime, &dateTimeString ) ;
				}
				catch(...)
				{
				}
			}
		}
	}
	
	return dateTimeString;
}
// =================================================================================================
