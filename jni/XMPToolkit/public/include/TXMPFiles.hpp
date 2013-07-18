#ifndef __TXMPFiles_hpp__
#define __TXMPFiles_hpp__    1

#if ( ! __XMP_hpp__ )
    #error "Do not directly include, use XMP.hpp"
#endif

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

// =================================================================================================
/// \file TXMPFiles.hpp
/// \brief API for access to the main (document-level) metadata in a file_.
///
/// The Adobe XMP Toolkit's file handling component, XMPFiles, is a front end to a set of
/// format-specific file handlers that support file I/O for XMP. The file handlers implement smart,
/// efficient support for those file formats for which the means to embed XMP is defined in the XMP
/// Specification. Where possible, this support allows:
///   \li Injection    of XMP where none currently exists
///   \li Expansion of XMP without regard to existing padding
///   \li Reconciliation of the XMP and other legacy forms of metadata.
///
/// \c TXMPFiles is designed for use by clients interested in the metadata and not in the primary
/// file content; the Adobe Bridge application is a typical example. \c TXMPFiles is not intended to
/// be appropriate for files authored by an application; that is, those files for which the
/// application has explicit knowledge of the file format.
// =================================================================================================


// =================================================================================================
/// \class TXMPFiles TXMPFiles.hpp
/// \brief API for access to the main (document-level) metadata in a file.
///
/// \c TXMPFiles is a template class that provides the API for the Adobe XMP Toolkit's XMPFiles
/// component. This provides convenient access to the main, or document level, XMP for a file. Use
/// it to obtain metadata from a file, which you can then manipulate with the XMP Core component
/// (the classes \c TXMPMeta, \c TXMPUtils, and \c TXMPIterator); and to write new or changed
/// metadata back out to a file.
///
/// The functions allow you to open a file, read and write the metadata, then close the file.
/// While open, portions of the file might be maintained in RAM data structures. Memory
/// usage can vary considerably depending onfile format and access options.
///
/// A file can be opened for read-only or read-write access, with typical exclusion for both
/// modes. Errors result in the throw of an \c XMPError exception.
///
/// \c TXMPFiles is the template class. It must be instantiated with a string class such as
/// \c std::string. Read the Toolkit Overview for information about the overall architecture of the XMP
/// API, and the documentation for \c XMP.hpp for specific instantiation instructions.
///
/// Access these functions through the concrete class, \c SXMPFiles.
// =================================================================================================


#if XMP_StaticBuild    // ! Client XMP_IO objects can only be used in static builds.
    #include "XMP_IO.hpp"
#endif


template <class tStringObj>
class TXMPFiles {

public:

    // =============================================================================================
    /// \name Initialization and termination
    /// @{
    ///
    /// A \c TXMPFiles object must be initialized before use and can be terminated when done.

    // ---------------------------------------------------------------------------------------------
    /// @brief \c GetVersionInfo() retrieves version information for the XMPFiles component.
    ///
    /// Can be called before \c #Initialize(). This function is static; make the call directly from
    /// the concrete class (\c SXMPFiles).
    ///
    /// @param versionInfo [out] A buffer in which to return the version information.

    static void GetVersionInfo ( XMP_VersionInfo * versionInfo );

    // ---------------------------------------------------------------------------------------------
    /// @brief Initializes the XMPFiles library; must be called before creating an \c SXMPFiles object.
    ///
    /// The main action is to activate the available smart file handlers. Must be called before
    /// using any methods except \c GetVersionInfo().
    ///
    /// This function is static; make the call directly from the concrete class (\c SXMPFiles).
    ///
    /// @return True on success.

    static bool Initialize();

    // ---------------------------------------------------------------------------------------------
    /// @brief Initializes the XMPFiles library; must be called before creating an \c SXMPFiles object.
    ///
    /// This overload of TXMPFiles::Initialize() accepts option bits to customize the initialization
    /// actions. At this time no option is defined.
    ///
    /// The main action is to activate the available smart file handlers. Must be called before
    /// using any methods except \c GetVersionInfo().
    ///
    /// This function is static; make the call directly from the concrete class (\c SXMPFiles).
    ///
    /// @param options Option flags to control the initialization actions.
    ///
    /// @return True on success.

    static bool Initialize ( XMP_OptionBits options );

    // ---------------------------------------------------------------------------------------------
    /// @brief Initializes the XMPFiles library; must be called before creating an \c SXMPFiles object.
    ///
    /// This overload of TXMPFiles::Initialize() accepts plugin directory and name of the plug-ins 
    /// as a comma separated list to load the file handler plug-ins. If plugins == NULL, then all
    /// plug-ins present in the plug-in directory will be loaded.
    ///
    /// The main action is to activate the available smart file handlers. Must be called before
    /// using any methods except \c GetVersionInfo().
    ///
    /// This function is static; make the call directly from the concrete class (\c SXMPFiles).
    ///
    /// @param pluginFolder Pugin directorty to load the file handler plug-ins.
    /// @param plugins Comma sepearted list of plug-ins which should be loaded from the plug-in directory. 
    /// If plugin == NULL, then all plug-ins availbale in the plug-in directory will be loaded.
    ///
    /// @return True on success.

    static bool Initialize ( const char* pluginFolder, const char* plugins = NULL );

    // ---------------------------------------------------------------------------------------------
    /// @brief Initializes the XMPFiles library; must be called before creating an \c SXMPFiles object.
    ///
    /// This overload of TXMPFiles::Initialize( XMP_OptionBits options ) accepts plugin directory and 
    /// name of the plug-ins as a comma separated list to load the file handler plug-ins. 
    /// If plugins == NULL, then all plug-ins present in the plug-in directory will be loaded.
    ///
    /// The main action is to activate the available smart file handlers. Must be called before
    /// using any methods except \c GetVersionInfo().
    ///
    /// This function is static; make the call directly from the concrete class (\c SXMPFiles).
    ///
    /// @param options Option flags to control the initialization actions.
    /// @param pluginFolder Pugin directorty to load the file handler plug-ins.
    /// @param plugins Comma sepearted list of plug-ins which should be loaded from the plug-in directory. 
    /// If plugin == NULL, then all plug-ins availbale in the plug-in directory will be loaded.
    ///
    /// @return True on success.

    static bool Initialize ( XMP_OptionBits options, const char* pluginFolder, const char* plugins = NULL );

    // ---------------------------------------------------------------------------------------------
    /// @brief Terminates use of the XMPFiles library.
    ///
    /// Optional. Deallocates global data structures created by intialization. Its main action is to
    /// deallocate heap-allocated global storage, for the benefit of client leak checkers.
    ///
    /// This function is static; make the call directly from the concrete class (\c SXMPFiles).

    static void Terminate();

    /// @}

    // =============================================================================================
    /// \name Constructors and destructor
    /// @{
    ///
    /// The default constructor initializes an object that is associated with no file. The alternate
    /// constructors call \c OpenFile().

    // ---------------------------------------------------------------------------------------------
    /// @brief Default constructor initializes an object that is associated with no file.

    TXMPFiles();

    // ---------------------------------------------------------------------------------------------
    /// @brief Destructor; typical virtual destructor.
    ///
    /// The destructor does not call \c CloseFile(); pending updates are lost when the destructor is run.
    ///
    /// @see \c OpenFile(), \c CloseFile()

    virtual ~TXMPFiles() throw();

    // ---------------------------------------------------------------------------------------------
    /// @brief Alternate constructor associates the new \c XMPFiles object with a specific file.
    ///
    /// Calls \c OpenFile() to open the specified file after performing a default construct.
    ///
    /// @param filePath    The path for the file, specified as a nul-terminated UTF-8 string.
    ///
    /// @param format A format hint for the file, if known.
    ///
    /// @param openFlags Options for how the file is to be opened (for read or read/write, for
    /// example). Use a logical OR of these bit-flag constants:
    ///
    ///   \li \c #kXMPFiles_OpenForRead
    ///   \li \c #kXMPFiles_OpenForUpdate
    ///   \li \c #kXMPFiles_OpenOnlyXMP
    ///   \li \c #kXMPFiles_OpenStrictly
    ///   \li \c #kXMPFiles_OpenUseSmartHandler
    ///   \li \c #kXMPFiles_OpenUsePacketScanning
    ///   \li \c #kXMPFiles_OpenLimitedScanning
    ///
    /// @return The new \c TXMPFiles object.

    TXMPFiles ( XMP_StringPtr  filePath,
                XMP_FileFormat format = kXMP_UnknownFile,
                XMP_OptionBits openFlags = 0 );

    // ---------------------------------------------------------------------------------------------
    /// @brief Alternate constructor associates the new \c XMPFiles object with a specific file,
    /// using a string object.
    ///
    /// Overloads the basic form of the function, allowing you to pass a string object
    /// for the file path. It is otherwise identical; see details in the canonical form.

    TXMPFiles ( const tStringObj & filePath,
                XMP_FileFormat     format = kXMP_UnknownFile,
                XMP_OptionBits     openFlags = 0 );

    // ---------------------------------------------------------------------------------------------
    /// @brief Copy constructor
    ///
    /// Increments an internal reference count but does not perform a deep copy.
    ///
    /// @param original The existing \c TXMPFiles object to copy.
    ///
    /// @return The new \c TXMPFiles object.

    TXMPFiles ( const TXMPFiles<tStringObj> & original );

    // ---------------------------------------------------------------------------------------------
    /// @brief Assignment operator
    ///
    /// Increments an internal reference count but does not perform a deep copy.
    ///
    /// @param rhs The existing \c TXMPFiles object.

    void operator= ( const TXMPFiles<tStringObj> & rhs );

    // ---------------------------------------------------------------------------------------------
    /// @brief Reconstructs a \c TXMPFiles object from an internal reference.
    ///
    /// This constructor creates a new \c TXMPFiles object that refers to the underlying reference
    /// object of an existing \c TXMPFiles object. Use to safely pass \c SXMPFiles references across
    /// DLL boundaries.
    ///
    /// @param xmpFilesObj The underlying reference object, obtained from some other XMP object
    /// with \c TXMPFiles::GetInternalRef().
    ///
    /// @return The new object.

    TXMPFiles ( XMPFilesRef xmpFilesObj );

    // ---------------------------------------------------------------------------------------------
    /// @brief GetInternalRef() retrieves an internal reference that can be safely passed across DLL
    /// boundaries and reconstructed.
    ///
    /// Use with the reconstruction constructor to safely pass \c SXMPFiles references across DLL
    /// boundaries where the clients might have used different string types when instantiating
    /// \c TXMPFiles.
    ///
    /// @return The internal reference.
    ///
    /// @see \c TXMPMeta::GetInternalRef() for usage.

    XMPFilesRef GetInternalRef();

    /// @}

    // =============================================================================================
    /// \name File handler information
    /// @{
    ///
    /// Call this static function from the concrete class, \c SXMPFiles, to obtain information about
    /// the file handlers for the XMPFiles component.

    // ---------------------------------------------------------------------------------------------
    /// @brief  GetFormatInfo() reports what features are supported for a specific file format.
    ///
    /// The file handlers for different file formats vary considerably in what features they
    /// support. Support depends on both the general capabilities of the format and the
    /// implementation of the handler for that format.
    ///
    ///This function is static; make the call directly from the concrete class (\c SXMPFiles).
    ///
    /// @param format The file format whose support flags are desired.
    ///
    /// @param handlerFlags [out] A buffer in which to return a logical OR of option bit flags.
    /// The following constants are defined:
    ///
    ///   \li \c #kXMPFiles_CanInjectXMP - Can inject first-time XMP into an existing file.
    ///   \li \c #kXMPFiles_CanExpand - Can expand XMP or other metadata in an existing file.
    ///   \li \c #kXMPFiles_CanRewrite - Can copy one file to another, writing new metadata (as in SaveAs)
    ///   \li \c #kXMPFiles_CanReconcile - Supports reconciliation between XMP and other forms.
    ///   \li \c #kXMPFiles_AllowsOnlyXMP - Allows access to just the XMP, ignoring other forms.
    ///   This is only meaningful if \c #kXMPFiles_CanReconcile is set.
    ///   \li \c #kXMPFiles_ReturnsRawPacket - File handler returns raw XMP packet information and string.
    ///
    /// Even if \c #kXMPFiles_ReturnsRawPacket is set, the returned packet information might have an
    /// offset of -1 to indicate an unknown offset. While all file handlers should be able to return
    /// the raw packet, some might not know the offset of the packet within the file. This is
    /// typical in cases where external libraries are used. These cases might not even allow return
    /// of the raw packet.
    ///
    /// @return True if the format has explicit "smart" support, false if the format is handled by
    /// the default packet scanning plus heuristics. */


    static bool GetFormatInfo ( XMP_FileFormat   format,
                                XMP_OptionBits * handlerFlags = 0 );

    /// @}

    // =============================================================================================
    /// \name File operations
    /// @{
    ///
    /// These functions allow you to open, close, and query files.

    // ---------------------------------------------------------------------------------------------
    /// @brief \c CheckFileFormat() tries to determine the format of a file.
    ///
    /// Tries to determine the format of a file, returning an \c #XMP_FileFormat value. Uses the
    /// same logic as \c OpenFile() to select a smart handler.
    ///
    /// @param filePath The path for the file, appropriate for the local operating system. Passed as
    /// a nul-terminated UTF-8 string. The path is the same as would be passed to \c OpenFile.
    ///
    /// @return The file's format if a smart handler would be selected by \c OpenFile(), otherwise
    /// \c #kXMP_UnknownFile.

    static XMP_FileFormat CheckFileFormat ( XMP_StringPtr filePath );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c CheckPackageFormat() tries to determine the format of a "package" folder.
    ///
    /// Tries to determine the format of a package, given the name of the top-level folder. Returns
    /// an \c #XMP_FileFormat value. Examples of recognized packages include the video formats P2,
    /// XDCAM, or Sony HDV. These packages contain collections of "clips", stored as multiple files
    /// in specific subfolders.
    ///
    /// @param folderPath The path for the top-level folder, appropriate for the local operating
    /// system. Passed as a nul-terminated UTF-8 string. This is not the same path you would pass to
    /// \c OpenFile(). For example, the top-level path for a package might be ".../MyMovie", while
    /// the path to a file you wish to open would be ".../MyMovie/SomeClip".
    ///
    /// @return The package's format if it can be determined, otherwise \c #kXMP_UnknownFile.

    static XMP_FileFormat CheckPackageFormat ( XMP_StringPtr folderPath );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c GetFileModDate() returns the last modification date of all files that are returned
    /// by \c GetAssociatedResources()
    ///
    /// Returns the most recent O/S file modification date of all associated files. In the typical case
    /// of a single file containing embedded XMP, returned date value is the modification date of the 
    /// same file. For sidecar and folder based video packages, returned date value is the modification 
    /// date of that associated file which was updated last.
    ///
    /// @param filePath A path exactly as would be passed to \c OpenFile.
    /// 
    /// @param modDate A required pointer to return the last modification date.
    ///
    /// @param format A format hint as would be passed to \c OpenFile.
    ///
    /// @param options An optional set of option flags. The only defined one is \c kXMPFiles_ForceGivenHandler,
    /// used to shortcut the handler selection logic if the caller is certain of the format.
    ///
    /// @return Returns true if the file path is valid to select a smart handler, false for an
    /// invalid path or if fallback packet scanning would be selected. 

    static bool GetFileModDate ( XMP_StringPtr    filePath,
                                 XMP_DateTime *   modDate,
                                 XMP_FileFormat * format = 0,
                                 XMP_OptionBits   options = 0 );
	

    // ---------------------------------------------------------------------------------------------
    /// @brief \c GetAssociatedResources() returns a list of files and folders associated to filePath.
    ///
    /// \c GetAssociatedResources is provided to locate all files that are associated to the given
    /// filePath such as sidecar-based XMP or folder-based video packages.If a smart 
    /// handler can be selected (not fallback packet scanning) then a list of file/folder paths is
    /// returned for the related files that can be safely copied/imported to a different location,
    /// keeping intact metadata(XMP and non-XMP),content and the necessary folder structure of the 
    /// format. The necessary folder structure here is the structure that is needed to uniquely 
    /// identify a folder-based format.The filePath and format parameters are exactly as would be 
    /// used for OpenFile. In the simple embedded XMP case just one path is returned. In the simple
    /// sidecar case one or two paths will be returned, one if there is no sidecar XMP and two if 
    /// sidecar XMP exists. For folder-based handlers paths to all associated files is returned, 
    /// including the files and folders necessary to identify the format.In general, all the returned
    /// paths are existent.In case of folder based video formats the first associated resource in the 
	/// resourceList is the root folder.
    ///
    /// @param filePath A path exactly as would be passed to \c OpenFile.
    /// 
    /// @param resourceList Address of a vector of strings to receive all associated resource paths.
    ///
    /// @param format A format hint as would be passed to \c OpenFile.
    ///
    /// @param options An optional set of option flags. The only defined one is \c kXMPFiles_ForceGivenHandler,
    /// used to shortcut the handler selection logic if the caller is certain of the format.
    ///
    /// @return Returns true if the file path is valid to select a smart handler, false for an
    /// invalid path or if fallback packet scanning would be selected. Can also return false for
    /// unexpected errors that prevent knowledge of the file usage.
    
    static bool GetAssociatedResources ( XMP_StringPtr            filePath,
                                         std::vector<tStringObj>* resourceList,
                                         XMP_FileFormat           format = kXMP_UnknownFile, 
                                         XMP_OptionBits           options = 0);

    // ---------------------------------------------------------------------------------------------
    /// @brief \c IsMetadataWritable() returns true if metadata can be updated for the given media path.
    ///
    /// \c IsMetadataWritable is provided to check if metadata can be updated or written to the format.In  
    /// the case of folder-based video formats only if all the metadata files can be written to, true is 
    /// returned.In other words, false is returned for a partial-write state of metadata files in
    /// folder-based media formats. 
    ///
    /// @param filePath A path exactly as would be passed to \c OpenFile.
    ///
    /// @param writable A pointer to the result flag. Is true if the metadata can be updated in the format,
    /// otherwise false.
    ///
    /// @param format A format hint as would be passed to \c OpenFile.
    ///
    /// @param options An optional set of option flags. The only defined one is \c kXMPFiles_ForceGivenHandler,
    /// used to shortcut the handler selection logic if the caller is certain of the format.
    ///
    /// @return Returns true if the file path is valid to select a smart handler, false for an
    /// invalid path or if fallback packet scanning would be selected. 
    
    static bool IsMetadataWritable (XMP_StringPtr  filePath,
                                   bool *         writable,    
                                   XMP_FileFormat format = kXMP_UnknownFile,
                                   XMP_OptionBits options = 0 );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c OpenFile() opens a file for metadata access.
    ///
    /// Opens a file for the requested forms of metadata access. Opening the file at a minimum
    /// causes the raw XMP packet to be read from the file. If the file handler supports legacy
    /// metadata reconciliation then legacy metadata is also read, unless \c #kXMPFiles_OpenOnlyXMP
    /// is passed.
    ///
    /// If the file is opened for read-only access (passing \c #kXMPFiles_OpenForRead), the disk
    /// file is closed immediately after reading the data from it; the \c XMPFiles object, however,
    /// remains in the open state. You must call \c CloseFile() when finished using it. Other
    /// methods, such as \c GetXMP(), can only be used between the \c OpenFile() and \c CloseFile()
    /// calls. The \c XMPFiles destructor does not call \c CloseFile(); if you call it without
    /// closing, any pending updates are lost.
    ///
    /// If the file is opened for update (passing \c #kXMPFiles_OpenForUpdate), the disk file
    /// remains open until \c CloseFile() is called. The disk file is only updated once, when
    /// \c CloseFile() is called, regardless of how many calls are made to \c PutXMP().
    ///
    /// Typically, the XMP is not parsed and legacy reconciliation is not performed until \c GetXMP()
    /// is called, but this is not guaranteed. Specific file handlers might do earlier parsing of
    /// the XMP. Delayed parsing and early disk file close for read-only access are optimizations
    /// to help clients implementing file browsers, so that they can access the file briefly
    /// and possibly display a thumbnail, then postpone more expensive XMP processing until later.
    ///
    /// @param filePath The path for the file, appropriate for the local operating system. Passed as
    /// a nul-terminated UTF-8 string.
    ///
    /// @param format The format of the file. If the format is unknown (\c #kXMP_UnknownFile) the
    /// format is determined from the file content. The first handler to check is guessed from the
    /// file's extension. Passing a specific format value is generally just a hint about what file
    /// handler to try first (instead of the one based on the extension). If
    /// \c #kXMPFiles_OpenStrictly is set, then any format other than \c #kXMP_UnknownFile requires
    /// that the file actually be that format; otherwise an exception is thrown.
    ///
    /// @param openFlags A set of option flags that describe the desired access. By default (zero)
    /// the file is opened for read-only access and the format handler decides on the level of
    /// reconciliation that will be performed. A logical OR of these bit-flag constants:
    ///
    ///   \li \c #kXMPFiles_OpenForRead - Open for read-only access.
    ///   \li \c #kXMPFiles_OpenForUpdate - Open for reading and writing.
    ///   \li \c #kXMPFiles_OpenOnlyXMP - Only the XMP is wanted, no reconciliation.
    ///   \li \c #kXMPFiles_OpenStrictly - Be strict about locating XMP and reconciling with other
    ///   forms. By default, a best effort is made to locate the    correct XMP and to reconcile XMP
    ///   with other forms (if reconciliation is done). This option forces stricter rules, resulting
    ///   in exceptions for errors. The definition of strictness is specific to each handler, there
    ///   might be no difference.
    ///   \li \c #kXMPFiles_OpenUseSmartHandler - Require the use of a smart handler.
    ///   \li \c #kXMPFiles_OpenUsePacketScanning - Force packet scanning, do not use a smart handler.
    ///
    /// @return True if the file is succesfully opened and attached to a file handler. False for
    /// anticipated problems, such as passing \c #kXMPFiles_OpenUseSmartHandler but not having an
    /// appropriate smart handler. Throws an exception for serious problems.

    bool OpenFile ( XMP_StringPtr  filePath,
                    XMP_FileFormat format = kXMP_UnknownFile,
                    XMP_OptionBits openFlags = 0 );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c OpenFile() opens a file for metadata access, using a string object
    ///
    /// Overloads the basic form of the function, allowing you to pass a string object for the file
    /// path. It is otherwise identical; see details in the canonical form.

    bool OpenFile ( const tStringObj & filePath,
                    XMP_FileFormat     format = kXMP_UnknownFile,
                    XMP_OptionBits     openFlags = 0 );

    #if XMP_StaticBuild    // ! Client XMP_IO objects can only be used in static builds.
    // ---------------------------------------------------------------------------------------------
    /// @brief \c OpenFile() opens a client-provided XMP_IO object for metadata access.
    ///
    /// Alternative to the basic form of the function, allowing you to pass an XMP_IO object for
    /// client-managed I/O.
    ///

    bool OpenFile ( XMP_IO *       clientIO,
                    XMP_FileFormat format = kXMP_UnknownFile,
                    XMP_OptionBits openFlags = 0 );
    #endif

    // ---------------------------------------------------------------------------------------------
    /// @brief CloseFile() explicitly closes an opened file.
    ///
    /// Performs any necessary output to the file and closes it. Files that are opened for update
    /// are written to only when closing.
    ///
    /// If the file is opened for read-only access (passing \c #kXMPFiles_OpenForRead), the disk
    /// file is closed immediately after reading the data from it; the \c XMPFiles object, however,
    /// remains in the open state. You must call \c CloseFile() when finished using it. Other
    /// methods, such as \c GetXMP(), can only be used between the \c OpenFile() and \c CloseFile()
    /// calls. The \c XMPFiles destructor does not call \c CloseFile(); if you call it without closing,
    /// any pending updates are lost.
    ///
    /// If the file is opened for update (passing \c #kXMPFiles_OpenForUpdate), the disk file remains
    /// open until \c CloseFile() is called. The disk file is only updated once, when \c CloseFile()
    /// is called, regardless of how many calls are made to \c PutXMP().
    ///
    /// @param closeFlags Option flags for optional closing actions. This bit-flag constant is
    /// defined:
    ///
    ///   \li \c #kXMPFiles_UpdateSafely - Write into a temporary file then swap for crash safety.

    void CloseFile ( XMP_OptionBits closeFlags = 0 );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c GetFileInfo() retrieves basic information about an opened file.
    ///
    /// @param filePath [out] A buffer in which to return the path passed to \c OpenFile(). Can be
    /// null if value is not wanted.
    ///
    /// @param openFlags  [out] A buffer in which to return the option flags passed to
    /// \c OpenFile(). Can be null if value is not wanted.
    ///
    /// @param format [out] A buffer in which to return the file format. Can be null if value is not
    /// wanted.
    /// @param handlerFlags  [out] A buffer in which to return the handler's capability flags. Can
    /// be null if value is not wanted.
    ///
    /// @return True if the file object is in the open state; that is, \c OpenFile() has been called
    /// but \c CloseFile() has not. False otherwise. Even if the file object is open, the actual
    /// disk file might be closed in the host file-system sense; see \c OpenFile().

    bool GetFileInfo ( tStringObj *     filePath = 0,
                       XMP_OptionBits * openFlags = 0,
                       XMP_FileFormat * format = 0,
                       XMP_OptionBits * handlerFlags = 0 );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c SetAbortProc() registers a callback function used to check for a user-signaled abort.
    ///
    /// The specified procedure is called periodically to allow a user to cancel time-consuming
    /// operations. The callback function should return true to signal an abort, which results in an
    /// exception being thrown.
    ///
    /// @param abortProc The callback function.
    ///
    /// @param abortArg A pointer to caller-defined data to pass to the callback function.

    void SetAbortProc ( XMP_AbortProc abortProc,
                        void *        abortArg );

    /// @}

    // =============================================================================================
    /// \name Accessing metadata
    /// @{
    ///
    /// These functions allow you to retrieve XMP metadata from open files, so that you can use the
    /// \c TXMPMeta API to manipulate it. The \c PutXMP() functions update the XMP packet in memory.
    /// Changed XMP is not actually written out to the file until the file is closed.

    // ---------------------------------------------------------------------------------------------
    /// @brief \c GetXMP() retrieves the XMP metadata from an open file.
    ///
    /// The function reports whether XMP is present in the file; you can choose to retrieve any or
    /// all of the parsed XMP, the raw XMP packet,or information about the raw XMP packet. The
    /// options provided when the file was opened determine if reconciliation is done with other
    /// forms of metadata.
    ///
    /// @param xmpObj [out] An XMP object in which to return the parsed XMP metadata. Can be null.
    ///
    /// @param xmpPacket [out] An string object in which to return the raw XMP packet as stored in
    /// the file. Can be null. The encoding of the packet is given in the \c packetInfo. Returns an
    /// empty string if the low level file handler does not provide the raw packet.
    ///
    /// @param packetInfo [out] An string object in which to return the location and form of the raw
    /// XMP in the file. \c #XMP_PacketInfo::charForm and \c #XMP_PacketInfo::writeable reflect the
    /// raw XMP in the file. The parsed XMP property values are always UTF-8. The writeable flag is
    /// taken from the packet trailer; it applies only to "format ignorant" writing. The
    /// \c #XMP_PacketInfo structure always reflects the state of the XMP in the file. The offset,
    /// length, and character form do not change as a result of calling \c PutXMP() unless the file
    /// is also written. Some file handlers might not return location or contents of the raw packet
    /// string. To determine whether one does, check the \c #kXMPFiles_ReturnsRawPacket bit returned
    /// by \c GetFormatInfo(). If the low-level file handler does not provide the raw packet
    /// location, \c #XMP_PacketInfo::offset and \c #XMP_PacketInfo::length are both 0,
    /// \c #XMP_PacketInfo::charForm is UTF-8, and \c #XMP_PacketInfo::writeable is false.
    ///
    /// @return True if the file has XMP, false otherwise.

    bool GetXMP ( SXMPMeta *       xmpObj = 0,
                  tStringObj *     xmpPacket = 0,
                  XMP_PacketInfo * packetInfo = 0 );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c PutXMP() updates the XMP metadata in this object without writing out the file.
    ///
    /// This function supplies new XMP for the file. However, the disk file is not written until the
    /// object is closed with \c CloseFile(). The options provided when the file was opened
    /// determine if reconciliation is done with other forms of metadata.
    ///
    /// @param xmpObj The new metadata as an XMP object.

    void PutXMP ( const SXMPMeta & xmpObj );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c PutXMP() updates the XMP metadata in this object without writing out the file,
    /// using a string object for input.
    ///
    /// Overloads the basic form of the function, allowing you to pass the metadata as a string object
    /// instead of an XMP object. It is otherwise identical; see details in the canonical form.
    ///
    /// @param xmpPacket The new metadata as a string object containing a complete XMP packet.

    void PutXMP ( const tStringObj & xmpPacket );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c PutXMP() updates the XMP metadata in this object without writing out the file,
       /// using a string object and optional length.
    ///
    /// Overloads the basic form of the function, allowing you to pass the metadata as a string object
    /// instead of an XMP object. It is otherwise identical; see details in the canonical form.
    ///
    /// @param xmpPacket The new metadata as a <tt>const char *</tt> string containing an XMP packet.
    ///
    /// @param xmpLength Optional. The number of bytes in the string. If not supplied, the string is
    /// assumed to be nul-terminated.

    void PutXMP ( XMP_StringPtr xmpPacket,
                  XMP_StringLen xmpLength = kXMP_UseNullTermination );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c CanPutXMP() reports whether this file can be updated with a specific XMP packet.
    ///
    /// Use to determine if the file can probably be updated with a given set of XMP metadata. This
    /// depends on the size of the packet, the options with which the file was opened, and the
    /// capabilities of the handler for the file format. The function obtains the length of the
    /// serialized packet for the provided XMP, but does not keep it or modify it, and does not
    /// cause the file to be written when closed. This is implemented roughly as follows:
    ///
    /// <pre>
    /// bool CanPutXMP ( XMP_StringPtr xmpPacket )
    /// {
    ///    XMP_FileFormat format;
    ///    this->GetFileInfo ( 0, &format, 0 );
    ///
    ///    XMP_OptionBits formatFlags;
    ///    GetFormatInfo ( format, &formatFlags );
    ///
    ///    if ( (formatFlags & kXMPFiles_CanInjectXMP) && (formatFlags & kXMPFiles_CanExpand) ) return true;
    ///
    ///    XMP_PacketInfo packetInfo;
    ///    bool hasXMP = this->GetXMP ( 0, 0, &packetInfo );
    ///
    ///    if ( ! hasXMP ) {
    ///       if ( formatFlags & kXMPFiles_CanInjectXMP ) return true;
    ///    } else {
    ///       if ( (formatFlags & kXMPFiles_CanExpand) ||
    ///            (packetInfo.length >= strlen(xmpPacket)) ) return true;
    ///    }
    ///    return false;
    /// }
    /// </pre>
    ///
    /// @param xmpObj The proposed new metadata as an XMP object.

    bool CanPutXMP ( const SXMPMeta & xmpObj );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c CanPutXMP() reports whether this file can be updated with a specific XMP packet,
    /// passed in a string object.
    ///
    /// Overloads the basic form of the function, allowing you to pass the metadata as a string object
    /// instead of an XMP object. It is otherwise identical; see details in the canonical form.
    ///
    /// @param xmpPacket The proposed new metadata as a string object containing an XMP packet.

    bool CanPutXMP ( const tStringObj & xmpPacket );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c CanPutXMP() reports whether this file can be updated with a specific XMP packet,
    /// passed in a string object.
    ///
    /// Overloads the basic form of the function, allowing you to pass the metadata as a string object
    /// instead of an XMP object. It is otherwise identical; see details in the canonical form.
    ///
    /// @param xmpPacket The proposed new metadata as a <tt>const char *</tt> string containing an XMP packet.
    ///
    /// @param xmpLength Optional. The number of bytes in the string. If not supplied, the string
    /// is assumed to be nul-terminated.

    bool CanPutXMP ( XMP_StringPtr xmpPacket,
                     XMP_StringLen xmpLength = kXMP_UseNullTermination );

    /// @}

    // =============================================================================================
    /// \name Progress notifications
    /// @{
    ///
    /// These functions allow track the progress of file operations. Initially only file updates are
    /// tracked, these all occur within calls to SXMPFiles::CloseFile. There are no plans to track
    /// other operations at this time. Tracking support must be added to specific file handlers,
    /// there are no guarantees about which handlers will have support. To simplify the logic only
    /// file writes will be estimated and measured.

    // ---------------------------------------------------------------------------------------------
    /// @brief \c SetDefaultProgressCallback() sets a global default for progress tracking. This is
    /// used as a default for XMPFiles (library) objects created after the default is set. This does
    /// not affect the callback for new SXMPFiles (client) objects with an existing XMPFiles object.
    ///
    /// @param proc The client's callback function. Can be zero to disable notifications.
    ///
    /// @param context A pointer used to carry client-private context.
    ///
    /// @param interval The desired number of seconds between notifications. Ideally the first
    /// notification is sent after this interval, then at each following multiple of this interval.
    ///
    /// @param sendStartStop A Boolean value indicating if initial and final notifications are
    /// wanted in addition to those at the reporting intervals.

    static void SetDefaultProgressCallback ( XMP_ProgressReportProc proc, void * context = 0,
                                             float interval = 1.0, bool sendStartStop = false );

    // ---------------------------------------------------------------------------------------------
    /// @brief \c SetProgressCallback() sets the progress notification callback for the associated
    /// XMPFiles (library) object.
    ///
    /// @param proc The client's callback function. Can be zero to disable notifications.
    ///
    /// @param context A pointer used to carry client-private context.
    ///
    /// @param interval The desired number of seconds between notifications. Ideally the first
    /// notification is sent after this interval, then at each following multiple of this interval.
    ///
    /// @param sendStartStop A Boolean value indicating if initial and final notifications are
    /// wanted in addition to those at the reporting intervals.

    void SetProgressCallback ( XMP_ProgressReportProc proc, void * context = 0,
                               float interval = 1.0, bool sendStartStop = false );
                                             
    /// @}

    // =============================================================================================
    // Error notifications
    // ===================

    // ---------------------------------------------------------------------------------------------
    /// \name Error notifications
    /// @{
    ///
    /// From the beginning through version 5.5, XMP Toolkit errors result in throwing an \c XMP_Error
    /// exception. For the most part exceptions were thrown early and thus API calls aborted as soon
    /// as an error was detected. Starting in version 5.5, support has been added for notifications
    /// of errors arising in calls to \c TXMPFiles functions.
    ///
    /// A client can register an error notification callback function for a \c TXMPFile object. This
    /// can be done as a global default or individually to each object. The global default applies
    /// to all objects created after it is registered. Within the object there is no difference
    /// between the global default or explicitly registered callback. The callback function returns
    /// a \c bool value indicating if recovery should be attempted (true) or an exception thrown
    /// (false). If no callback is registered, a best effort at recovery and continuation will be
    /// made with an exception thrown if recovery is not possible.
    ///
    /// The number of notifications delivered for a given TXMPFiles object can be limited. This is
    /// intended to reduce chatter from multiple or cascading errors. The limit is set when the
    /// callback function is registered. This limits the number of notifications of the highest
    /// severity delivered or less. If a higher severity error occurs, the counting starts again.
    /// The limit and counting can be reset at any time, see \c ResetErrorCallbackLimit.

    //  --------------------------------------------------------------------------------------------
    /// @brief SetDefaultErrorCallback() registers a global default error notification callback.
    ///
    /// @param proc The client's callback function.
    ///
    /// @param context Client-provided context for the callback.
    ///
    /// @param limit A limit on the number of notifications to be delivered.

    static void SetDefaultErrorCallback ( XMPFiles_ErrorCallbackProc proc, void* context = 0, XMP_Uns32 limit = 1 );

    //  --------------------------------------------------------------------------------------------
    /// @brief SetErrorCallback() registers an error notification callback.
    ///
    /// @param proc The client's callback function.
    ///
    /// @param context Client-provided context for the callback.
    ///
    /// @param limit A limit on the number of notifications to be delivered.

    void SetErrorCallback ( XMPFiles_ErrorCallbackProc proc, void* context = 0, XMP_Uns32 limit = 1 );

    //  --------------------------------------------------------------------------------------------
    /// @brief ResetErrorCallbackLimit() resets the error notification limit and counting. It has no
    /// effect if an error notification callback function is not registered. 
    ///
    /// @param limit A limit on the number of notifications to be delivered.

    void ResetErrorCallbackLimit ( XMP_Uns32 limit = 1 );

    /// @}

    // =============================================================================================

private:

    XMPFilesRef xmpFilesRef;

    // These are used as callbacks from the library code to the client when returning values that
    // involve heap allocations. This ensures the allocations occur within the client.
    static void SetClientString ( void * clientPtr, XMP_StringPtr valuePtr, XMP_StringLen valueLen );
    static void SetClientStringVector ( void * clientPtr, XMP_StringPtr* arrayPtr, XMP_Uns32 stringCount );

};    // class TXMPFiles

// =================================================================================================

#endif // __TXMPFiles_hpp__
