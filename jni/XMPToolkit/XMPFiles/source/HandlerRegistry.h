// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2011 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#ifndef _HANDLERREGISTRY_h_
#define _HANDLERREGISTRY_h_

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"
#include "public/include/XMP_IO.hpp"

#include "XMPFiles/source/XMPFiles_Impl.hpp"
#include "source/XMPFiles_IO.hpp"

#include "XMPFiles/source/FormatSupport/IFF/IChunkBehavior.h"
#include "XMPFiles/source/FormatSupport/IFF/ChunkPath.h"
#include "source/Endian.h"

namespace Common
{

/**
	File handler data
 */
struct XMPFileHandlerInfo 
{
	XMP_FileFormat		format;
	XMP_OptionBits		flags;
	void*				checkProc;
	XMPFileHandlerCTor	handlerCTor;

	XMPFileHandlerInfo() : format(0), flags(0), checkProc(0), handlerCTor(0) 
	{};
	
	XMPFileHandlerInfo( XMP_FileFormat _format, 
						XMP_OptionBits _flags,
						CheckFileFormatProc _checkProc, 
						XMPFileHandlerCTor _handlerCTor )
		: format(_format), flags(_flags), checkProc((void*)_checkProc), handlerCTor(_handlerCTor) 
	{};
	
	XMPFileHandlerInfo( XMP_FileFormat _format, 
						XMP_OptionBits _flags,
						CheckFolderFormatProc _checkProc, 
						XMPFileHandlerCTor _handlerCTor )
		: format(_format), flags(_flags), checkProc((void*)_checkProc), handlerCTor(_handlerCTor) 
	{};
};

/**
	The singleton class HandlerRegistry is responsible to manage all file handler.
	It registers file handlers during initialization time and provides functionality
	to select a file handler based on a given file format.
*/

class HandlerRegistry
{
public:
	static HandlerRegistry&		getInstance();
	static void					terminate();

#if EnableDynamicMediaHandlers
	static XMP_FileFormat		checkTopFolderName( const std::string & rootPath );
	static XMP_FileFormat		checkParentFolderNames( const std::string& rootPath,
														const std::string& gpName,
														const std::string& parentName, 
														const std::string& leafName );
#endif

public:
	/**
	 * Register all file handler
	 */
	void				initialize();

	/**
	 * Register a single folder based file handler.
	 *
	 * @param format			File format identifier
	 * @param flags				Flags
	 * @param checkProc			Check format function pointer
	 * @param handlerCTor		Factory function pointer
	 * @param replaceExisting	Replace an already existing handler
	 */
	bool				registerFolderHandler( XMP_FileFormat			format,
											   XMP_OptionBits			flags,
											   CheckFolderFormatProc	checkProc,
											   XMPFileHandlerCTor		handlerCTor,
											   bool						replaceExisting = false );

	/**
	 * Register a single normal file handler.
	 *
	 * @param format			File format identifier
	 * @param flags				Flags
	 * @param checkProc			Check format function pointer
	 * @param handlerCTor		Factory function pointer
	 * @param replaceExisting	Replace an already existing handler
	 */
	bool				registerNormalHandler( XMP_FileFormat			format,
											   XMP_OptionBits			flags,
											   CheckFileFormatProc		checkProc,
											   XMPFileHandlerCTor		handlerCTor,
											   bool						replaceExisting = false );

	/**
	 * Register a single owning file handler.
	 *
	 * @param format			File format identifier
	 * @param flags				Flags
	 * @param checkProc			Check format function pointer
	 * @param handlerCTor		Factory function pointer
	 * @param replaceExisting	Replace an already existing handler
	 */
	bool				registerOwningHandler( XMP_FileFormat			format,
											   XMP_OptionBits			flags,
											   CheckFileFormatProc		checkProc,
											   XMPFileHandlerCTor		handlerCTor,
											   bool						replaceExisting = false );

	// Remove a handler. Does nothing if no such handler exists.
	void removeHandler ( XMP_FileFormat format );
	
	/**
	 * Get file format identifier for filename extension.
	 *
	 * @param fileExt			Filename extension
	 * @param addIfNotFound		If true if handler doesn't exists then add now
	 */
	XMP_FileFormat		getFileFormat( const std::string & fileExt, bool addIfNotFound = false );

	/**
	 * Get handler flags for file format.
	 *
	 * @param format	File format identifier
	 * @param flags		Return handler flag
	 * @return			True on success
	 */
	bool				getFormatInfo( XMP_FileFormat format, XMP_OptionBits* flags = NULL );

	/**
	 * Get handler information for passed format. 
	 * The returned file handler is the default handler. I.e. that handler
	 * that is used when called from outside using the XMPFiles API.
	 *
	 * @param format	File format identifier
	 * @return			Return handler info if available, otherwise NULL
	 */
	XMPFileHandlerInfo*	getHandlerInfo( XMP_FileFormat format );

	/**
	 * Get file handler information of the standard file handler for the
	 * file format identifier.
	 * If there is a replacement for this format then the standard handler
	 * is the replaced handler. Otherwise the standard handler and the 
	 * default handler are the same.
	 *
	 * @param format	File format identifier
	 * @return			Return handler info if available, otherwise NULL
	 */
	XMPFileHandlerInfo*	getStandardHandlerInfo( XMP_FileFormat format );

	/**
	 * Return true if there is a replacement for the file format
	 */
	bool isReplaced( XMP_FileFormat format );

	/**
	 * Select file handler based on passed information and setup XMPFiles instance with related data.
	 *
	 * @param session		XMPFiles instance
	 * @param clientPath	Path to file
	 * @param format		File format identifier
	 * @param openFlags		Flags
	 * @return				File handler structure
	 */
	XMPFileHandlerInfo*	selectSmartHandler( XMPFiles* session, XMP_StringPtr clientPath, XMP_FileFormat format, XMP_OptionBits openFlags );

private:
	/**
	 * Return default file handler for file format identifier or filename extension
	 *
	 * @param format	File format identifier
	 * @param fileExt	Filename extension
	 * @return			File handler structure for passed format/extension
	 */
	XMPFileHandlerInfo* pickDefaultHandler ( XMP_FileFormat format, const std::string & fileExt );

#if EnableDynamicMediaHandlers
	/**
	 * Try to find folder based file handler.
	 *
	 * @param format		File format identifier
	 * @param rootPath		Path to root folder
	 * @param gpName		Grand parent folder name
	 * @param parentName	Parent folder name
	 * @param leafName		File name
	 * @param parentObj		XMPFiles instance
	 * @return				File handler structure
	 */
	XMPFileHandlerInfo* tryFolderHandlers( XMP_FileFormat format,
										   const std::string& rootPath,
										   const std::string& gpName,
										   const std::string& parentName,
										   const std::string& leafName,
										   XMPFiles* parentObj );
#endif

private:
	/**
	 * ctor/dtor
	 */
	 HandlerRegistry();
	~HandlerRegistry();

private:
	typedef std::map <XMP_FileFormat, XMPFileHandlerInfo>	XMPFileHandlerTable;
	typedef XMPFileHandlerTable::iterator					XMPFileHandlerTablePos;
	typedef std::pair <XMP_FileFormat, XMPFileHandlerInfo>	XMPFileHandlerTablePair;

	XMPFileHandlerTable*	mFolderHandlers;	// The directory-oriented handlers.
	XMPFileHandlerTable*	mNormalHandlers;	// The normal file-oriented handlers.
	XMPFileHandlerTable*	mOwningHandlers;	// The file-oriented handlers that "own" the file.

	XMPFileHandlerTable*	mReplacedHandlers;	// All file handler that where replaced by a later one

	static HandlerRegistry*	sInstance;			// singleton instance
};

} // Common

#endif	// _HANDLERREGISTRY_h_
