#ifndef __PostScript_Support_hpp__
#define __PostScript_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2012 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"
#include "source/XIO.hpp"


#define MAX_NO_MARK 100
#define IsNumeric( ch ) ( ch >='0' && ch<='9' )
#define Uns8Ptr(p) ((XMP_Uns8 *) (p))
#define IsPlusMinusSign(ch)  ( ch =='+' || ch=='-' )
#define IsDateDelimiter( ch ) ( ((ch) == '/') || ((ch) == '-') || ((ch) == '.') )
#define IsTimeDelimiter( ch ) ( ((ch) == ':') )
#define IsDelimiter(ch) (IsDateDelimiter( ch ) || IsTimeDelimiter( ch ))
#define IsAlpha(ch) ((ch>=97 &&ch <=122) || (ch>=65 && ch<=91))


enum {
	kPSHint_NoMarker  = 0,
	kPSHint_NoMain    = 1,
	kPSHint_MainFirst = 2,
	kPSHint_MainLast  = 3
};

enum UpdateMethod{
	kPS_None			= 0,
	kPS_Inplace			= 1,
	kPS_ExpandSFDFilter = 2,
	kPS_InjectNew		= 3
};

enum TokenFlag{
	// --------------------
	// Flags for native Metadata  and DSC commnents in EPS format

	/// No Native MetaData
	kPS_NoData			=   0x00000001,
	/// Document Creator tool
	kPS_CreatorTool		=   0x00000002,
	/// Document Creation Date
	kPS_CreateDate		=   0x00000004,
	/// Document Modify Date
	kPS_ModifyDate		=   0x00000008,
	/// Document Creator/Author
	kPS_Creator			=   0x00000010,
	/// Document Title
	kPS_Title			=   0x00000020,
	/// Document Desciption
	kPS_Description		=   0x00000040,
	/// Document Subject/Keywords
	kPS_Subject			=   0x00000080,
	/// ADO_ContainsXMP hint
	kPS_ADOContainsXMP	=   0x00000100,
	/// End Comments
	kPS_EndComments		=   0x00000200,
	/// Begin Prolog
	kPS_BeginProlog		=   0x00000400,
	/// End Prolog
	kPS_EndProlog	    =   0x00000800,
	/// Begin Setup
	kPS_BeginSetup	    =   0x00001000,
	/// End Setup
	kPS_EndSetup	    =   0x00002000,
	/// Page
	kPS_Page	        =   0x00004000,
	/// End Page Comments
	kPS_EndPageComments	=   0x00008000,
	/// Begin Page SetUp
	kPS_BeginPageSetup	=   0x00010000,
	/// End Page SetUp
	kPS_EndPageSetup	=   0x00020000,
	/// Trailer
	kPS_Trailer			=   0x00040000,
	/// EOF
	kPS_EOF				=   0x00080000,
	/// End PostScript
	kPS_EndPostScript	=   0x00100000,
	/// Max Token
	kPS_MaxToken		=   0x00200000
};

enum NativeMetadataIndex{
	// --------------------
	// Index native Metadata ina PS file
	kPS_dscCreator			= 0, 
	kPS_dscCreateDate		= 1,
	kPS_dscFor				= 2,
	kPS_dscTitle			= 3,
	kPS_docInfoCreator		= 4,
	kPS_docInfoCreateDate	= 5,
	kPS_docInfoModDate		= 6,
	kPS_docInfoAuthor		= 7,
	kPS_docInfoTitle		= 8,
	kPS_docInfoSubject		= 9,
	kPS_docInfoKeywords		= 10,
	kPS_MaxNativeIndexValue 
};

static XMP_Uns64 nativeIndextoFlag[]={	kPS_CreatorTool,
										kPS_CreateDate,
										kPS_Creator,
										kPS_Title,
										kPS_CreatorTool,
										kPS_CreateDate,
										kPS_ModifyDate,
										kPS_Creator,
										kPS_Title,
										kPS_Description,
										kPS_Subject
									};

static const std::string kPSFileTag    = "%!PS-Adobe-";
static const std::string kPSContainsXMPString = "%ADO_ContainsXMP:";
static const std::string kPSContainsBBoxString = "%%BoundingBox:";
static const std::string kPSContainsBeginDocString = "%%BeginDocument:";
static const std::string kPSContainsEndDocString = "%%EndDocument";
static const std::string kPSContainsTrailerString = "%%Trailer";
static const std::string kPSContainsCreatorString = "%%Creator:";
static const std::string kPSContainsCreateDateString = "%%CreationDate:";
static const std::string kPSContainsForString = "%%For:";
static const std::string kPSContainsTitleString = "%%Title:";
static const std::string kPSContainsAtendString = "(atend)";
static const std::string kPSEndCommentString  = "%%EndComments";	// ! Assumed shorter than kPSContainsXMPString.
static const std::string kPSContainsDocInfoString  = "/DOCINFO";	
static const std::string kPSContainsPdfmarkString  = "pdfmark";	
static const std::string kPS_XMPHintMainFirst="%ADO_ContainsXMP: MainFirst\n";
static const std::string kPS_XMPHintMainLast="%ADO_ContainsXMP: MainLast\n";

// For new xpacket injection into the EPS file is done in Postscript using the pdfmark operator
// There are different conventions described for EPS and PS files in XMP Spec part 3.
// The tokens kEPS_Injectdata1, kEPS_Injectdata2 and kEPS_Injectdata3 are used to
// embedd xpacket in EPS files.the xpacket is written inbetween kEPS_Injectdata1 and kEPS_Injectdata2.
// The tokens kPS_Injectdata1 and kPS_Injectdata2 are used to embedd xpacket in DSC compliant PS files
// The code inside the tokens is taken from examples in XMP Spec part 3 
// section 2.6.2 PS, EPS (PostScript® and Encapsulated PostScript)
static const std::string kEPS_Injectdata1="\n/currentdistillerparams where\n"
"{pop currentdistillerparams /CoreDistVersion get 5000 lt} {true} ifelse\n"
"{userdict /EPSHandler1_pdfmark /cleartomark load put\n"
"userdict /EPSHandler1_ReadMetadata_pdfmark {flushfile cleartomark} bind put}\n"
"{ userdict /EPSHandler1_pdfmark /pdfmark load put\n"
"userdict /EPSHandler1_ReadMetadata_pdfmark {/PUT pdfmark} bind put } ifelse\n"
"[/NamespacePush EPSHandler1_pdfmark\n"
"[/_objdef {eps_metadata_stream} /type /stream /OBJ EPSHandler1_pdfmark\n"
"[{eps_metadata_stream} 2 dict begin\n"
"/Type /Metadata def /Subtype /XML def currentdict end /PUT EPSHandler1_pdfmark\n"
"[{eps_metadata_stream}\n"
"currentfile 0 (% &&end EPS XMP packet marker&&)\n"
"/SubFileDecode filter EPSHandler1_ReadMetadata_pdfmark\n";

static const std::string kPS_Injectdata1="\n/currentdistillerparams where\n"
"{pop currentdistillerparams /CoreDistVersion get 5000 lt} {true} ifelse\n"
"{userdict /PSHandler1_pdfmark /cleartomark load put\n"
"userdict /PSHandler1_ReadMetadata_pdfmark {flushfile cleartomark} bind put}\n"
"{ userdict /PSHandler1_pdfmark /pdfmark load put\n"
"userdict /PSHandler1_ReadMetadata_pdfmark {/PUT pdfmark} bind put } ifelse\n"
"[/NamespacePush PSHandler1_pdfmark\n"
"[/_objdef {ps_metadata_stream} /type /stream /OBJ PSHandler1_pdfmark\n"
"[{ps_metadata_stream} 2 dict begin\n"
"/Type /Metadata def /Subtype /XML def currentdict end /PUT PSHandler1_pdfmark\n"
"[{ps_metadata_stream}\n"
"currentfile 0 (% &&end PS XMP packet marker&&)\n"
"/SubFileDecode filter PSHandler1_ReadMetadata_pdfmark\n";

static const std::string kEPS_Injectdata2="\n% &&end EPS XMP packet marker&&\n"
"[/Document\n"
"1 dict begin /Metadata {eps_metadata_stream} def\n"
"currentdict end /BDC EPSHandler1_pdfmark\n"
"[/NamespacePop EPSHandler1_pdfmark\n";


static const std::string kPS_Injectdata2="\n% &&end PS XMP packet marker&&\n"
"[{Catalog} {ps_metadata_stream} /Metadata PSHandler1_pdfmark\n"
"[/NamespacePop PSHandler1_pdfmark\n";

static const std::string  kEPS_Injectdata3="\n/currentdistillerparams where\n"
	"{pop currentdistillerparams /CoreDistVersion get 5000 lt} {true} ifelse\n"
	"{userdict /EPSHandler1_pdfmark /cleartomark load put}\n"
	"{ userdict /EPSHandler1_pdfmark /pdfmark load put} ifelse\n"
	"[/EMC EPSHandler1_pdfmark\n";


namespace PostScript_Support
{
	struct Date
	{
		short day;
		short month;
		short year;
		short hours;
		short minutes;
		short seconds;
		bool containsOffset;
		char offsetSign;
		short offsetHour;
		short offsetMin;
		Date(short pday=1,short pmonth=1,short pyear=1900,short phours=0,
			short pminutes=0,short pseconds=0):day(pday),month(pmonth),
			year(pyear),hours(phours),minutes(pminutes),seconds(pseconds),
			containsOffset(false),offsetSign('+'),offsetHour(0),offsetMin(0)
		{
		}
		bool operator==(const Date &a)
		{
			return this->day==a.day &&
				this->month==a.month &&
				this->year==a.year &&
				this->hours==a.hours &&
				this->minutes==a.minutes &&
				this->seconds==a.seconds &&
				this->containsOffset==a.containsOffset &&
				this->offsetSign==a.offsetSign &&
				this->offsetHour==a.offsetHour &&
				this->offsetMin==a.offsetMin;
		}
	};
	struct DateTimeTokens
	{
		std::string token;
		short noOfDelimiter;
		char delimiter;
		DateTimeTokens(std::string ptoken="",short pnoOfDelimiter=0,char pdelimiter=0):
		token(ptoken),noOfDelimiter(pnoOfDelimiter),delimiter(pdelimiter)
		{
		}
	};

	//function to parse strings and get date out of it
	std::string ConvertToDate(const char* inString);
	// These helpers are similar to RefillBuffer and CheckFileSpace with the difference that the it traverses
	// the file stream in reverse order
	void RevRefillBuffer ( XMP_IO* fileRef, IOBuffer* ioBuf );
	bool RevCheckFileSpace ( XMP_IO* fileRef, IOBuffer* ioBuf, size_t neededLen );

	// function to detect character codes greater than 127 in a string
	bool HasCodesGT127(const std::string & value);
	
	// function moves the file pointer ahead such that it skips all tabs and spaces 
	bool  SkipTabsAndSpaces(XMP_IO* file,IOBuffer& ioBuf);
	
	// function moves the file pointer ahead such that it skips all characters until a newline 
	bool SkipUntilNewline(XMP_IO* file,IOBuffer& ioBuf);
	
	// function to detect character codes greater than 127 in a string
	bool IsValidPSFile(XMP_IO*    fileRef,XMP_FileFormat &format);
	
	// Determines Whether the metadata is embedded using the Sub-FileDecode Approach or no
	bool IsSFDFilterUsed(XMP_IO* &fileRef, XMP_Int64 xpacketOffset);

} // namespace PostScript_Support

#endif	// __PostScript_Support_hpp__
