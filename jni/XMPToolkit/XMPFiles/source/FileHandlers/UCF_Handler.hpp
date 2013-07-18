#ifndef __UCF_Handler_hpp__
#define __UCF_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"

// =================================================================================================
/// \file UCF_Handler.hpp
//
//  underlying math:
//    __   0   ______    0    ______  __
//            |  A   |       |  A   |
//            |      |       |      |
//         al |      |  (a2l)|      |
//         x  |------|   b2  |------|
//         xl |  X   |       |  B   |_
//         b  |------|  (b2l)|      | |
//            |  B   |   x2  |------| | B2 could also be
//         bl |      |   x2l |  X2  | | _after_ X2
//         cd |------|   cd2 |------|<'
//            |//CD//|       |//CD2/|
//        cdx |------|  cdx2 |------|
//        cdxl|------|  cdx2l|------|
//         cdl|//////|   cd2l|//////|
//         z  |------|     z2|------|
//            |      |       |      |
//        [zl]|  Z   |   z2l |  Z2  |
//         h  |------|   h2  |------|
//    fl      |  H   |       |  H2  |  f2l
//    __   hl |______|  (h2l)|______|  __
//
//    fl  file length pre (2 = post)
//    numCf  number of content files prior to injection
//    numCf2            "            post
//
//
//    l	length variable, all else offset
//    [ ] variable is not needed
//    ( ) variable is identical to left
//    a   content files prior to xmp (possibly: all)
//    b   content files behind xmp (possibly: 0)
//    x   xmp packet (possibly: 0)
//    cd  central directory
//    h   end of central directory
//
//    z   zip64 record and locator (if existing)
//
//    general rules:
//    the bigger A, the less rewrite effort.
//    (also within the CD)
//    putting XMP at the end maximizes A.
//
//    bool previousXMP == x!=0
//
//    (x==0) == (cdx==0) == (xl==0) == (cdxl==0)
//
//    std::vector<XMP_Uns32> cdOffsetsPre;
//
//    -----------------
//    asserts:
//( 1)  a == a2 == 0,	making these variables obsolete
//( 2)  a2l == al,		this block is not touched
//( 3)  b2 <= b,		b is only moved closer to the beginning of file
//( 4)  b2l == bl,		b does not change in size
//( 5)  x2 >= x,		b is only moved further down in the file
//
//( 6) x != 0, x2l != 0, cd != 0, cdl != 0
//    				none of these blocks is at the beginning ('mimetype' by spec),
//    				nor is any of them zero byte long
//( 7) h!=0, hl >= 22	header is not at the beginning, minimum size 22
//
//    file size computation:
//( 8) al + bl + xl +cdl +hl = fl
//( 9) al + bl + x2l+cd2l+hl = fl2
//
//(10) ( x==0 ) <=> ( cdx == 0 )
//    				if there's a packet in the pre-file, or there isn't
//(11) (x==0)	=> xl=0
//(12) (cdx==0)=> cdx=0
//
//(13) x==0 ==> b,bl,b2,b2l==0
//    				if there is no pre-xmp, B does not exist
//(14) x!=0 ==> al:=x, b:=x+xl, bl:=cd-b
//
//    zip 64:
//(15)  zl and z2l are basically equal, except _one_ of them is 0 :
//
//(16)  b2l is indeed never different t
//
//    FIXED_SIZE means the fixed (minimal) portion of a struct
//    TOTAL_SIZE indicates, that this struct indeed has a fixed, known total length
//
// =================================================================================================

extern XMPFileHandler* UCF_MetaHandlerCTor ( XMPFiles * parent );

extern bool UCF_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
                              XMP_IO*    fileRef,
                              XMPFiles *     parent );

static const XMP_OptionBits kUCF_HandlerFlags = (
						kXMPFiles_CanInjectXMP     |
						kXMPFiles_CanExpand        |
						kXMPFiles_CanRewrite       |
						/* kXMPFiles_PrefersInPlace | removed, only reasonable for formats where difference is significant */
						kXMPFiles_AllowsOnlyXMP    |
						kXMPFiles_ReturnsRawPacket |
						// *** kXMPFiles_AllowsSafeUpdate |
						kXMPFiles_NeedsReadOnlyPacket //UCF/zip has checksums...
						);

enum {	// data descriptor
	// may or may not have a signature: 0x08074b50
	kUCF_DD_crc32				=   0,
	kUCF_DD_sizeCompressed		=   4,
	kUCF_DD_sizeUncompressed	=   8,
};

class UCF_MetaHandler : public XMPFileHandler
{
public:
	UCF_MetaHandler ( XMPFiles * _parent );
	~UCF_MetaHandler();

	void CacheFileData();
	void ProcessXMP();

	void UpdateFile ( bool doSafeUpdate );
    void WriteTempFile ( XMP_IO* tempRef );

protected:
	const static XMP_Uns16 xmpFilenameLen = 21;
	const static char* xmpFilename;

private:
	class Zip64EndOfDirectory {
	private:

	public:
		const static XMP_Uns16 o_sig				=	0;	// 0x06064b50
		const static XMP_Uns16 o_size				=	4;	// of this, excluding leading 12 bytes
				// == FIXED_SIZE -12, since we're never creating the extensible data sector...
		const static XMP_Uns16 o_VersionMade		=	12;
		const static XMP_Uns16 o_VersionNeededExtr	=	14;
		const static XMP_Uns16 o_numDisk			=	16;	// force 0
		const static XMP_Uns16 o_numCDDisk			=	20; // force 0
		const static XMP_Uns16 o_numCFsThisDisk		=	24;
		const static XMP_Uns16 o_numCFsTotal		=	32; // force equal
		const static XMP_Uns16 o_sizeOfCD			=	40; // (regular one, not Z64)
		const static XMP_Uns16 o_offsetCD			=	48; //           "

		const static XMP_Int32 FIXED_SIZE = 56;
		char  fields[FIXED_SIZE];

		const static XMP_Uns32 ID = 0x06064b50;

		Zip64EndOfDirectory( XMP_Int64 offsetCD, XMP_Int64 sizeOfCD, XMP_Uns64 numCFs )
		{
			memset(fields,'\0',FIXED_SIZE);

			PutUns32LE(ID				,&fields[o_sig] );
			PutUns64LE(FIXED_SIZE - 12,	&fields[o_size] );	//see above
			PutUns16LE(	45	,&fields[o_VersionMade] );
			PutUns16LE(	45	,&fields[o_VersionNeededExtr] );
			// fine at 0:	o_numDisk
			// fine at 0:	o_numCDDisk
			PutUns64LE( numCFs, &fields[o_numCFsThisDisk] );
			PutUns64LE( numCFs, &fields[o_numCFsTotal] );
			PutUns64LE( sizeOfCD,	&fields[o_sizeOfCD] );
			PutUns64LE( offsetCD,	&fields[o_offsetCD] );
		}

		void write(XMP_IO* file)
		{
			XMP_Validate( ID == GetUns32LE( &this->fields[o_sig] ), "invalid header on write", kXMPErr_BadFileFormat );
			file ->Write ( fields , FIXED_SIZE  );
		}

	};

	class Zip64Locator {
	public:
		const static XMP_Uns16 o_sig				=	0;	// 0x07064b50
		const static XMP_Uns16 o_numDiskZ64CD		=	4;	// force 0
		const static XMP_Uns16 o_offsZ64EOD			=   8;
		const static XMP_Uns16 o_numDisks			=   16; // set 1, tolerate 0

		const static XMP_Int32 TOTAL_SIZE			=	20;
		char  fields[TOTAL_SIZE];

		const static XMP_Uns32 ID = 0x07064b50;

		Zip64Locator( XMP_Int64 offsetZ64EOD )
		{
			memset(fields,'\0',TOTAL_SIZE);
			PutUns32LE(ID,				&fields[Zip64Locator::o_sig] );
			PutUns32LE(0,				&fields[Zip64Locator::o_numDiskZ64CD] );
			PutUns64LE(offsetZ64EOD,	&fields[Zip64Locator::o_offsZ64EOD] );
			PutUns32LE(1,				&fields[Zip64Locator::o_numDisks] );
		}

		// writes structure to file (starting at current position)
		void write(XMP_IO* file)
		{
			XMP_Validate( ID == GetUns32LE( &this->fields[o_sig] ), "invalid header on write", kXMPErr_BadFileFormat );
			file ->Write ( fields , TOTAL_SIZE  );
		}
	};

	struct EndOfDirectory {
	public:
		const static XMP_Int32 FIXED_SIZE = 22;	//32 bit type is important to not overrun on maxcomment
		const static XMP_Uns32 ID = 0x06054b50;
		const static XMP_Int32 COMMENT_MAX = 0xFFFF;
		//offsets
		const static XMP_Int32 o_CentralDirectorySize = 12;
		const static XMP_Int32 o_CentralDirectoryOffset = 16;
	};

	class FileHeader {
		private:
			//TODO intergrate in clear()
			void release() // avoid terminus free() since subject to a #define (mem-leak-check)
			{
				if (filename)	delete filename;
				if (extraField)	delete extraField;
				filename=0;
				extraField=0;
			}

		public:
			const static XMP_Uns32 SIG = 0x04034b50;
			const static XMP_Uns16 kdataDescriptorFlag = 0x8;

			const static XMP_Uns16 o_sig = 0;
			const static XMP_Uns16 o_extractVersion		=	4;
			const static XMP_Uns16 o_flags				=   6;
			const static XMP_Uns16 o_compression		=   8;
			const static XMP_Uns16 o_lastmodTime		=   10;
			const static XMP_Uns16 o_lastmodDate		=   12;
			const static XMP_Uns16 o_crc32				=   14;
			const static XMP_Uns16 o_sizeCompressed		=   18;
			const static XMP_Uns16 o_sizeUncompressed	=   22;
			const static XMP_Uns16 o_fileNameLength		=   26;
			const static XMP_Uns16 o_extraFieldLength	=   28;
			// total									30

			const static int FIXED_SIZE = 30;
			char  fields[FIXED_SIZE];

			char* filename;
			char* extraField;
			XMP_Uns16 filenameLen;
			XMP_Uns16 extraFieldLen;

			void clear()
			{
				this->release();
				memset(fields,'\0',FIXED_SIZE);
				//arm with minimal default values:
				PutUns32LE(0x04034b50,	&fields[FileHeader::o_sig] );
				PutUns16LE(0x14,		&fields[FileHeader::o_extractVersion] );
			}

			FileHeader() : filename(0),filenameLen(0),extraField(0),extraFieldLen(0)
			{
				clear();
			};

			// reads entire *FileHeader* structure from file (starting at current position)
			void read(XMP_IO* file)
			{
				this->release();

				file ->ReadAll ( fields , FIXED_SIZE  );

				XMP_Uns32 tmp32 = GetUns32LE( &this->fields[FileHeader::o_sig] );
				XMP_Validate( SIG == tmp32, "invalid header", kXMPErr_BadFileFormat );
				filenameLen   = GetUns16LE( &this->fields[FileHeader::o_fileNameLength] );
				extraFieldLen = GetUns16LE( &this->fields[FileHeader::o_extraFieldLength] );

				// nb unlike the CDFileHeader the FileHeader will in practice never have
				// extra fields. Reasoning: File headers never carry (their own) offsets,
				// (un)compressed size of XMP will hardly ever reach 4 GB

				if (filenameLen) {
					filename = new char[filenameLen];
					file->ReadAll ( filename, filenameLen );
				}
				if (extraFieldLen) {
					extraField = new char[extraFieldLen];
					file->ReadAll ( extraField, extraFieldLen );
					// *** NB: this WOULD need parsing for content files that are
					//   compressed or uncompressed >4GB (VERY unlikely for XMP)
				}
			}

			// writes structure to file (starting at current position)
			void write(XMP_IO* file)
			{
				XMP_Validate( SIG == GetUns32LE( &this->fields[FileHeader::o_sig] ), "invalid header on write", kXMPErr_BadFileFormat );

				filenameLen   = GetUns16LE( &this->fields[FileHeader::o_fileNameLength] );
				extraFieldLen = GetUns16LE( &this->fields[FileHeader::o_extraFieldLength] );

				file ->Write ( fields , FIXED_SIZE  );
				if (filenameLen)	file->Write ( filename, filenameLen    );
				if (extraFieldLen)	file->Write ( extraField, extraFieldLen  );
			}

			void transfer(const FileHeader &orig)
			{
				memcpy(fields,orig.fields,FIXED_SIZE);
				if (orig.extraField)
				{
					extraFieldLen=orig.extraFieldLen;
					extraField = new char[extraFieldLen];
					memcpy(extraField,orig.extraField,extraFieldLen);
				}
				if (orig.filename)
				{
					filenameLen=orig.filenameLen;
					filename = new char[filenameLen];
					memcpy(filename,orig.filename,filenameLen);
				}
			};

			void setXMPFilename()
			{
				// only needed for fresh structs, thus enforcing rather than catering to memory issues
				XMP_Enforce( (filenameLen==0) && (extraFieldLen == 0) );
				filenameLen = xmpFilenameLen;
				PutUns16LE(filenameLen,	&fields[FileHeader::o_fileNameLength] );
				filename = new char[xmpFilenameLen];
				memcpy(filename,"META-INF/metadata.xml",xmpFilenameLen);
			}

			XMP_Uns32 sizeHeader()
			{
				return this->FIXED_SIZE + this->filenameLen + this->extraFieldLen;
			}

			XMP_Uns32 sizeTotalCF()
			{
				//*** not zip64 bit safe yet, use only for non-large xmp packet
				return this->sizeHeader() + GetUns32LE( &fields[FileHeader::o_sizeCompressed] );
			}

			~FileHeader()
			{
				this->release();
			};

	}; //class FileHeader

	////// yes, this needs an own class
	////// offsets must be extracted, added, modified,
	////// come&go depending on being >0xffffff
	////class extraField {
	////	private:


	class CDFileHeader {
		private:
			void release()  //*** needed or can go?
			{
				if (filename)	delete filename;
				if (extraField)	delete extraField;
				if (comment)	delete comment;
				filename=0; filenameLen=0;
				extraField=0; extraFieldLen=0;
				comment=0; commentLen=0;
			}

			const static XMP_Uns32 SIG = 0x02014b50;

		public:
			const static XMP_Uns16	o_sig	=   0;	//0x02014b50
			const static XMP_Uns16	o_versionMadeBy		=   4;
			const static XMP_Uns16	o_extractVersion	=	6;
			const static XMP_Uns16	o_flags				=   8;
			const static XMP_Uns16	o_compression		=   10;
			const static XMP_Uns16	o_lastmodTime		=   12;
			const static XMP_Uns16	o_lastmodDate		=   14;
			const static XMP_Uns16	o_crc32				=   16;
			const static XMP_Uns16	o_sizeCompressed	=   20; // 16bit stub
			const static XMP_Uns16	o_sizeUncompressed	=   24; // 16bit stub
			const static XMP_Uns16	o_fileNameLength	=   28;
			const static XMP_Uns16	o_extraFieldLength	=   30;
			const static XMP_Uns16	o_commentLength		=   32;
			const static XMP_Uns16	o_diskNo			=   34;
			const static XMP_Uns16	o_internalAttribs	=   36;
			const static XMP_Uns16	o_externalAttribs	=   38;
			const static XMP_Uns16	o_offsetLocalHeader	=   42;	// 16bit stub
			//					total size is 4+12+12+10+8=46

			const static int FIXED_SIZE = 46;
			char fields[FIXED_SIZE];

			// do not bet on any zero-freeness,
			// certainly no zero termination (pascal strings),
			// treat as data blocks
			char* filename;
			char* extraField;
			char* comment;
			XMP_Uns16 filenameLen;
			XMP_Uns16 extraFieldLen;
			XMP_Uns16 commentLen;

			// full, real, parsed 64 bit values
			XMP_Int64 sizeUncompressed;
			XMP_Int64 sizeCompressed;
			XMP_Int64 offsetLocalHeader;

			CDFileHeader() : filename(0),extraField(0),comment(0),filenameLen(0),
				extraFieldLen(0),commentLen(0),sizeUncompressed(0),sizeCompressed(0),offsetLocalHeader(0)
			{
				memset(fields,'\0',FIXED_SIZE);
				//already arm with appropriate values where applicable:
				PutUns32LE(0x02014b50,	&fields[CDFileHeader::o_sig] );
				PutUns16LE(0x14,		&fields[CDFileHeader::o_extractVersion] );
			};

			// copy constructor
			CDFileHeader(const CDFileHeader& orig) : filename(0),extraField(0),comment(0),filenameLen(0),
				extraFieldLen(0),commentLen(0),sizeUncompressed(0),sizeCompressed(0),offsetLocalHeader(0)
			{
				memcpy(fields,orig.fields,FIXED_SIZE);
				if (orig.extraField)
				{
					extraFieldLen=orig.extraFieldLen;
					extraField = new char[extraFieldLen];
					memcpy(extraField , orig.extraField , extraFieldLen);
				}
				if (orig.filename)
				{
					filenameLen=orig.filenameLen;
					filename = new char[filenameLen];
					memcpy(filename , orig.filename , filenameLen);
				}
				if (orig.comment)
				{
					commentLen=orig.commentLen;
					comment = new char[commentLen];
					memcpy(comment , orig.comment , commentLen);
				}

				filenameLen   = orig.filenameLen;
				extraFieldLen = orig.extraFieldLen;
				commentLen    = orig.commentLen;

				sizeUncompressed = orig.sizeUncompressed;
				sizeCompressed = orig.sizeCompressed;
				offsetLocalHeader = orig.offsetLocalHeader;
			}

			// Assignment operator
			CDFileHeader& operator=(const CDFileHeader& obj)
			{
				XMP_Throw("not supported",kXMPErr_Unimplemented);
			}

			// reads entire structure from file (starting at current position)
			void read(XMP_IO* file)
			{
				this->release();

				file->ReadAll ( fields, FIXED_SIZE );
				XMP_Validate( SIG == GetUns32LE( &this->fields[CDFileHeader::o_sig] ), "invalid header", kXMPErr_BadFileFormat );

				filenameLen   = GetUns16LE( &this->fields[CDFileHeader::o_fileNameLength] );
				extraFieldLen = GetUns16LE( &this->fields[CDFileHeader::o_extraFieldLength] );
				commentLen    = GetUns16LE( &this->fields[CDFileHeader::o_commentLength] );

				if (filenameLen) {
					filename = new char[filenameLen];
					file->ReadAll ( filename, filenameLen );
				}
				if (extraFieldLen) {
					extraField = new char[extraFieldLen];
					file->ReadAll ( extraField, extraFieldLen );
				}
				if (commentLen) {
					comment = new char[commentLen];
					file->ReadAll ( comment, commentLen );
				}

				////// GET ACTUAL 64 BIT VALUES //////////////////////////////////////////////
				// get 32bit goodies first, correct later
				sizeUncompressed		= GetUns32LE( &fields[o_sizeUncompressed] );
				sizeCompressed			= GetUns32LE( &fields[o_sizeCompressed] );
				offsetLocalHeader		= GetUns32LE( &fields[o_offsetLocalHeader] );

				XMP_Int32 offset = 0;
				while ( offset < extraFieldLen )
				{
					XMP_Validate( (extraFieldLen - offset) >= 4, "need 4 bytes for next header ID+len", kXMPErr_BadFileFormat);
					XMP_Uns16 headerID = GetUns16LE( &extraField[offset] );
					XMP_Uns16 dataSize = GetUns16LE( &extraField[offset+2] );
					offset += 4;

					XMP_Validate( (extraFieldLen - offset) <= dataSize,
									"actual field lenght not given", kXMPErr_BadFileFormat);
					if ( headerID == 0x1 ) //we only care about "Zip64 extended information extra field"
					{
						XMP_Validate( offset < extraFieldLen, "extra field too short", kXMPErr_BadFileFormat);
						if (sizeUncompressed == 0xffffffff)
						{
							sizeUncompressed = GetUns64LE( &extraField[offset] );
							offset += 8;
						}
						if (sizeCompressed == 0xffffffff)
						{
							sizeCompressed = GetUns64LE( &extraField[offset] );
							offset += 8;
						}
						if (offsetLocalHeader == 0xffffffff)
						{
							offsetLocalHeader = GetUns64LE( &extraField[offset] );
							offset += 8;
						}
					}
					else
					{
						offset += dataSize;
					} // if
				} // while
			} // read()

			// writes structure to file (starting at current position)
			void write(XMP_IO* file)
			{
				//// WRITE BACK REAL 64 BIT VALUES, CREATE EXTRA FIELD ///////////////
				//may only wipe extra field after obtaining all Info from it
				if (extraField)	delete extraField;
					extraFieldLen=0;

				if ( ( sizeUncompressed  > 0xffffffff ) ||
					 ( sizeCompressed    > 0xffffffff ) ||
					 ( offsetLocalHeader > 0xffffffff )  )
				{
					extraField = new char[64]; // actual maxlen is 32
					extraFieldLen = 4; //first fields are for ID, size
					if ( sizeUncompressed > 0xffffffff )
					{
						PutUns64LE( sizeUncompressed, &extraField[extraFieldLen] );
						extraFieldLen += 8;
						sizeUncompressed = 0xffffffff;
					}
					if ( sizeCompressed > 0xffffffff )
					{
						PutUns64LE( sizeCompressed, &extraField[extraFieldLen] );
						extraFieldLen += 8;
						sizeCompressed = 0xffffffff;
					}
					if ( offsetLocalHeader > 0xffffffff )
					{
						PutUns64LE( offsetLocalHeader, &extraField[extraFieldLen] );
						extraFieldLen += 8;
						offsetLocalHeader = 0xffffffff;
					}

					//write ID, dataSize
					PutUns16LE( 0x0001, &extraField[0] );
					PutUns16LE( extraFieldLen-4, &extraField[2] );
					//extraFieldSize
					PutUns16LE( extraFieldLen, &this->fields[CDFileHeader::o_extraFieldLength] );
				}

				// write out 32-bit ('ff-stubs' or not)
				PutUns32LE( (XMP_Uns32)sizeUncompressed, &fields[o_sizeUncompressed] );
				PutUns32LE( (XMP_Uns32)sizeCompressed, &fields[o_sizeCompressed] );
				PutUns32LE( (XMP_Uns32)offsetLocalHeader, &fields[o_offsetLocalHeader] );

				/// WRITE /////////////////////////////////////////////////////////////////
				XMP_Enforce( SIG == GetUns32LE( &this->fields[CDFileHeader::o_sig] ) );

				file ->Write ( fields , FIXED_SIZE  );
				if (filenameLen)	file->Write ( filename   , filenameLen    );
				if (extraFieldLen)	file->Write ( extraField , extraFieldLen  );
				if (commentLen)		file->Write ( extraField , extraFieldLen  );
			}

			void setXMPFilename()
			{
				if (filename) delete filename;
				filenameLen = xmpFilenameLen;
				filename = new char[xmpFilenameLen];
				PutUns16LE(filenameLen,	&fields[CDFileHeader::o_fileNameLength] );
				memcpy(filename,"META-INF/metadata.xml",xmpFilenameLen);
			}

			XMP_Int64 size()
			{
				XMP_Int64 r = this->FIXED_SIZE + this->filenameLen + this->commentLen;
				// predict serialization size
				if (  (sizeUncompressed > 0xffffffff)||(sizeCompressed > 0xffffffff)||(offsetLocalHeader>0xffffffff) )
				{
					r += 4;		//extra fields necessary
					if (sizeUncompressed > 0xffffffff) r += 8;
					if (sizeCompressed > 0xffffffff) r += 8;
					if (offsetLocalHeader > 0xffffffff) r += 8;
				}
				return r;
			}

			~CDFileHeader()
			{
				this->release();
			};
	}; // class CDFileHeader

	class EndOfCD {
	private:
		const static XMP_Uns32 SIG = 0x06054b50;
		void UCFECD_Free()
		{
			if(commentLen) delete comment;
			commentLen = 0;
		}
	public:
		const static XMP_Int32 o_Sig = 0;
		const static XMP_Int32 o_CdNumEntriesDisk = 8;  // same-same for UCF, since single-volume
		const static XMP_Int32 o_CdNumEntriesTotal = 10;// must update both
		const static XMP_Int32 o_CdSize = 12;
		const static XMP_Int32 o_CdOffset = 16;
		const static XMP_Int32 o_CommentLen = 20;

		const static int FIXED_SIZE = 22;
		char fields[FIXED_SIZE];

		char* comment;
		XMP_Uns16 commentLen;

		EndOfCD() : comment(0), commentLen(0)
		{
			//nothing
		};

		void read (XMP_IO* file)
		{
			UCFECD_Free();

			file->ReadAll ( fields, FIXED_SIZE );
			XMP_Validate( this->SIG == GetUns32LE( &this->fields[o_Sig] ), "invalid header", kXMPErr_BadFileFormat );

			commentLen = GetUns16LE( &this->fields[o_CommentLen] );
			if(commentLen)
			{
				comment = new char[commentLen];
				file->ReadAll ( comment, commentLen );
			}
		};

		void write(XMP_IO* file)
		{
			XMP_Enforce( this->SIG == GetUns32LE( &this->fields[o_Sig] ) );
			commentLen   = GetUns16LE( &this->fields[o_CommentLen] );
			file ->Write ( fields , FIXED_SIZE  );
			if (commentLen)
				file->Write ( comment, commentLen );
		}

		~EndOfCD()
		{
			if (comment)	delete comment;
		};
	}; //class EndOfCD

	////////////////////////////////////////////////////////////////////////////////////
	// EMBEDDING MATH
	//
	// a = content files before xmp (always 0 thus ommited)
	// b/b2 = content files behind xmp (before/after injection)
	// x/x2 = offset xmp content header + content file (before/after injection)
	// cd/cd = central directory
	// h/h2 = end of central directory record
	XMP_Int64	b,b2,x,x2,cd,cd2,cdx,cdx2,z,z2,h,h2,
	// length thereof ('2' only where possibly different)
	// using XMP_Int64 here also for length (not XMP_Int32),
	// to be prepared for zip64, our LFA functions might need things in multiple chunks...
				al,bl,xl,x2l,cdl,cd2l,cdxl,cdx2l,z2l,hl,fl,f2l;
	XMP_Uns16	numCF,numCF2;

	bool wasCompressed;	// ..before, false if no prior xmp
	bool compressXMP;   // compress this time?
	bool inPlacePossible;
	/* bool isZip64; <=> z2 != 0 */

	FileHeader		xmpFileHeader;
	CDFileHeader	xmpCDHeader;

	XMP_StringPtr uncomprPacketStr;
	XMP_StringLen uncomprPacketLen;
	XMP_StringPtr finalPacketStr;
	XMP_StringLen finalPacketLen;
	std::vector<CDFileHeader> cdEntries;
	EndOfCD endOfCD;
	void writeOut( XMP_IO* sourceFile, XMP_IO* targetFile, bool isRewrite, bool isInPlace);

};	// UCF_MetaHandler

// =================================================================================================

#endif /* __UCF_Handler_hpp__ */


