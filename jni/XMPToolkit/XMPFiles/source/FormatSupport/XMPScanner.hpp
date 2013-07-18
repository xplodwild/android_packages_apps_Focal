#ifndef __XMPScanner_hpp__
#define __XMPScanner_hpp__

// =================================================================================================
// Copyright 2004 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
//
// Adobe patent application tracking #P435, entitled 'Unique markers to simplify embedding data of
// one format in a file with a different format', inventors: Sean Parent, Greg Gilley.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! This must be the first include.

#include <list>
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

#include "public/include/XMP_Const.h"

// =================================================================================================
// The XMPScanner class is used to scan a stream of input for XMP packets.  A scanner object is
// constructed then fed the input through a series of calls to Scan.  Report may be called at any
// time to get the current knowledge of the input.
//
// A packet starts when a valid header is found and ends when a valid trailer is found.  If the
// header contains a "bytes" attribute, additional whitespace must follow.
//
// *** RESTRICTIONS: The current implementation of the scanner has the the following restrictions:
//		- The input must be presented in order.
//		- Not fully thread safe, don't make concurrent calls to the same XMPScanner object.
// =================================================================================================

class XMPScanner {
public:

	// =============================================================================================
	// The entire input stream is represented as a series of snips.  Each snip defines one portion
	// of the input stream that either has not been seen, has been seen and contains no packets, is
	// exactly one packet, or contains the start of an unfinished packet.  Adjacent snips with the
	// same state are merged, so the number of snips is always minimal.
	//
	// A newly constructed XMPScanner object has one snip covering the whole input with a state
	// of "not seen".  A block of input that contains a full XMP packet is split into 3 parts: a
	// (possibly empty) raw input snip, the packet, and another (possibly empty) raw input snip.  A
	// block of input that contains the start of an XMP packet is split into two snips, a (possibly
	// empty) raw input snip and the packet start; the following snip must be a "not seen" snip.
	//
	// It is possible to have ill-formed packets.  These have a syntactically valid header and
	// trailer, but some semantic error.  For example, if the "bytes" attribute length does not span
	// to the end of the trailer, or if the following packet begins within trailing padding.
	
	enum {
		eNotSeenSnip,		// This snip has not been seen yet.
		ePendingSnip,		// This snip is an input buffer being processed.
		eRawInputSnip,		// This snip is raw input, it doesn't contain any part of an XMP packet.
		eValidPacketSnip,	// This snip is a complete, valid XMP packet.
		ePartialPacketSnip,	// This snip contains the start of a possible XMP packet.
		eBadPacketSnip		// This snip contains a complete, but semantically incorrect XMP packet.
	};
	typedef XMP_Uns8	SnipState;
	
	enum {	// The values allow easy testing for 16/32 bit and big/little endian.
		eChar8Bit			= 0,
		eChar16BitBig		= 2,
		eChar16BitLittle	= 3,
		eChar32BitBig		= 4,
		eChar32BitLittle	= 5
	};
	typedef XMP_Uns8	CharacterForm;

	enum {
		eChar16BitMask			= 2,	// These constant shouldn't be used directly, they are mainly
		eChar32BitMask			= 4,	// for the CharFormIsXyz macros below.
		eCharLittleEndianMask	= 1
	};
	
	#define CharFormIs16Bit(f)			( ((int)(f) & XMPScanner::eChar16BitMask) != 0 )
	#define CharFormIs32Bit(f)			( ((int)(f) & XMPScanner::eChar32BitMask) != 0 )
	
	#define CharFormIsBigEndian(f)		( ((int)(f) & XMPScanner::eCharLittleEndianMask) == 0 )
	#define CharFormIsLittleEndian(f)	( ((int)(f) & XMPScanner::eCharLittleEndianMask) != 0 )
	
	struct SnipInfo {

		XMP_Int64		fOffset;		// The byte offset of this snip within the input stream.
		XMP_Int64		fLength;		// The length in bytes of this snip.
		SnipState		fState;			// The state of this snip.
		bool			fOutOfOrder;	// If true, this snip was seen before the one in front of it.
		char			fAccess;		// The read-only/read-write access from the end attribute.
		CharacterForm	fCharForm;		// How the packet is divided into characters.
		const char *	fEncodingAttr;	// The value of the encoding attribute, if any, with nulls removed.
		XMP_Int64		fBytesAttr;		// The value of the bytes attribute, -1 if not present.

		SnipInfo() :
			fOffset ( 0 ),
			fLength ( 0 ),
			fState ( eNotSeenSnip ),
			fOutOfOrder ( false ),
			fAccess ( ' ' ),
			fCharForm ( eChar8Bit ),
			fEncodingAttr ( "" ),
			fBytesAttr( -1 )
		{ }

		SnipInfo ( SnipState state, XMP_Int64 offset, XMP_Int64 length ) :
			fOffset ( offset ),
			fLength ( length ),
			fState ( state ),
			fOutOfOrder ( false ),
			fAccess ( ' ' ),
			fCharForm ( eChar8Bit ),
			fEncodingAttr ( "" ),
			fBytesAttr( -1 )
		{ }
	
	};
	
	typedef std::vector<SnipInfo>	SnipInfoVector;

	XMPScanner ( XMP_Int64 streamLength );
	// Constructs a new XMPScanner object for a stream with the given length.
	
	~XMPScanner();

	long GetSnipCount();
	// Returns the number of snips that the stream has been divided into.
	
 	bool StreamAllScanned();
 	// Returns true if all of the stream has been seen.
	
	void Scan ( const void * bufferOrigin, XMP_Int64 bufferOffset, XMP_Int64 bufferLength );
	// Scans the given part of the input, incorporating it in to the known snips.
	// The bufferOffset is the offset of this block of input relative to the entire stream.
	// The bufferLength is the length in bytes of this block of input.

	void Report ( SnipInfoVector & snips );
	// Produces a report of what is known about the input stream. 

	class ScanError : public std::logic_error {
	public:
		ScanError() throw() : std::logic_error ( "" ) {}
		explicit ScanError ( const char * message ) throw() : std::logic_error ( message ) {}
		virtual ~ScanError() throw() {}
	};

private:	// XMPScanner
	
	class PacketMachine;

	class InternalSnip {
	public:

		SnipInfo	fInfo;							// The public info about this snip.
		std::auto_ptr<PacketMachine>	fMachine;	// The state machine for "active" snips.
		
		InternalSnip ( XMP_Int64 offset, XMP_Int64 length );
		InternalSnip ( const InternalSnip & );
		~InternalSnip ();

	};	// InternalSnip

	typedef std::list<InternalSnip>		InternalSnipList;
	typedef InternalSnipList::iterator	InternalSnipIterator;

	class PacketMachine {
	public:
		
		XMP_Int64		fPacketStart;	// Byte offset relative to the entire stream.
		XMP_Int32		fPacketLength;	// Length in bytes to the end of the trailer processing instruction.
		XMP_Int32		fBytesAttr;		// The value of the bytes attribute, -1 if not present.
		std::string		fEncodingAttr;	// The value of the encoding attribute, if any, with nulls removed.
		CharacterForm	fCharForm;		// How the packet is divided into characters.
		char			fAccess;		// The read-only/read-write access from the end attribute.
		bool			fBogusPacket;	// True if the packet has an error such as a bad "bytes" attribute value.
		
		void ResetMachine();

		enum TriState {
			eTriNo,
			eTriMaybe,
			eTriYes
		};

		TriState FindNextPacket();
		
		void AssociateBuffer ( XMP_Int64 bufferOffset, const void * bufferOrigin, XMP_Int64 bufferLength );
		
		PacketMachine ( XMP_Int64 bufferOffset, const void * bufferOrigin, XMP_Int64 bufferLength );
		~PacketMachine();
	
	private:	// PacketMachine
	
		PacketMachine() {};	// ! Hide the default constructor.
	
		enum RecognizerKind {

			eFailureRecognizer,			// Not really recognizers, special states to end one buffer's processing.
			eSuccessRecognizer,

			eLeadInRecognizer,			// Anything up to the next '<'.
			eHeadStartRecorder,			// Save the starting offset, count intervening nulls.
			eHeadStartRecognizer,		// The literal string "?xpacket begin=".

			eBOMRecognizer,				// Recognize and record the quoted byte order marker.

			eIDTagRecognizer,			// The literal string " id=".
			eIDOpenRecognizer,			// The opening quote for the ID.
			eIDValueRecognizer,			// The literal string "W5M0MpCehiHzreSzNTczkc9d".
			eIDCloseRecognizer,			// The closing quote for the ID.

			eAttrSpaceRecognizer_1, 	// The space before an attribute.
			eAttrNameRecognizer_1,		// The name of an attribute.
			eAttrValueRecognizer_1,		// The equal sign and quoted string value for an attribute.
			eAttrValueRecorder_1,		// Record the value of an attribute.

			eHeadEndRecognizer,			// The string literal "?>".			
			
			eBodyRecognizer,			// The packet body, anything up to the next '<'.

			eTailStartRecognizer,		// The string literal "?xpacket end=".
			eAccessValueRecognizer,		// Recognize and record the quoted r/w access mode.

			eAttrSpaceRecognizer_2, 	// The space before an attribute.
			eAttrNameRecognizer_2,		// The name of an attribute.
			eAttrValueRecognizer_2,		// The equal sign and quoted string value for an attribute.
			eAttrValueRecorder_2,		// Record the value of an attribute.

			eTailEndRecognizer,			// The string literal "?>".
			ePacketEndRecognizer,		// Look for trailing padding, check and record the packet size.
			eCloseOutRecognizer,		// Look for final nulls for little endian multibyte characters.
			
			eRecognizerCount

		};
		
		XMP_Int64		fBufferOffset;	// The offset of the data buffer within the input stream.
		const char *	fBufferOrigin;	// The starting address of the data buffer for this snip.
		const char *	fBufferPtr;		// The current postion in the data buffer.
		const char *	fBufferLimit;	// The address one past the last byte in the data buffer.

		RecognizerKind	fRecognizer;	// Which recognizer is currently active.
		signed long		fPosition;		// The internal position within a string literal, etc.
		unsigned char	fBytesPerChar;	// The number of bytes per logical character, 1, 2, or 4.
		unsigned char	fBufferOverrun;	// Non-zero if suspended while skipping intervening nulls.
		char			fQuoteChar;		// The kind of quote seen at the start of a quoted value.
		std::string		fAttrName;		// The name for an arbitrary attribute (other than "begin" and "id").
		std::string		fAttrValue;		// The value for an arbitrary attribute (other than "begin" and "id").
	
		void SetNextRecognizer ( RecognizerKind nextRecognizer );
				
		typedef TriState (* RecognizerProc) ( PacketMachine *, const char * );
	
		static TriState
		FindLessThan ( PacketMachine * ths, const char * which );
	
		static TriState
		MatchString ( PacketMachine * ths, const char * literal );
	
		static TriState
		MatchChar ( PacketMachine * ths, const char * literal );
	
		static TriState
		MatchOpenQuote ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		MatchCloseQuote ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		CaptureAttrName ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		CaptureAttrValue ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		RecordStart ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		RecognizeBOM ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		RecordHeadAttr ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		CaptureAccess ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		RecordTailAttr ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		CheckPacketEnd ( PacketMachine * ths, const char * /* unused */ );
	
		static TriState
		CheckFinalNulls ( PacketMachine * ths, const char * /* unused */ );
		
		struct RecognizerInfo {
			RecognizerProc	proc;
			RecognizerKind	successNext;
			RecognizerKind	failureNext;
			const char *	literal;
		};
		
	};	// PacketMachine
	
	XMP_Int64			fStreamLength;
	InternalSnipList	fInternalSnips;

	void
	SplitInternalSnip ( InternalSnipIterator snipPos, XMP_Int64 relOffset, XMP_Int64 newLength );

	InternalSnipIterator
	MergeInternalSnips ( InternalSnipIterator firstPos, InternalSnipIterator secondPos );

	InternalSnipIterator
	PrevSnip ( InternalSnipIterator snipPos );

	InternalSnipIterator
	NextSnip ( InternalSnipIterator snipPos );

	#if DEBUG
		void DumpSnipList ( const char * title );
	#endif
	
};	// XMPScanner

#endif	// __XMPScanner_hpp__
