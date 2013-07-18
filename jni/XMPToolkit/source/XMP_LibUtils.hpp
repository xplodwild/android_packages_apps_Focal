#ifndef __XMP_LibUtils_hpp__
#define __XMP_LibUtils_hpp__ 1

// =================================================================================================
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! Must be the first include.
#include "public/include/XMP_Const.h"

#include <map>
#include <string>
#include <vector>

#if XMP_DebugBuild
	#include <cassert>
#endif

#if XMP_WinBuild
	#ifndef snprintf
		#define snprintf _snprintf
	#endif
#endif

// =================================================================================================
// Basic types, constants
// ======================

#define kTab ((char)0x09)
#define kLF ((char)0x0A)
#define kCR ((char)0x0D)

#if XMP_WinBuild
	#define kDirChar '\\'
#else
	#define kDirChar '/'
#endif

typedef std::string XMP_VarString;

#define EliminateGlobal(g) delete ( g ); g = 0

extern "C" bool Initialize_LibUtils();
extern "C" void Terminate_LibUtils();

#define IgnoreParam(p)	(void)p

// The builtin offsetof macro sometimes violates C++ data member rules.
#define XMP_OffsetOf(struct,field)	( (char*)(&((struct*)0x100)->field) - (char*)0x100 )

// =================================================================================================
// Support for exceptions and asserts
// ==================================

#define AnnounceThrow(msg)		/* Do nothing. */
#define AnnounceCatch(msg)		/* Do nothing. */

#define XMP_Throw(msg,id)	{ AnnounceThrow ( msg ); throw XMP_Error ( id, msg ); }

#if XMP_DebugBuild
#define XMP_Throw_Verbose(msg,e,id)						\
{														\
	char tmpMsg[255];									\
	snprintf(tmpMsg, sizeof(tmpMsg), #msg "( %d )", e);	\
	XMP_Throw( tmpMsg, id);								\
}
#else
	#define XMP_Throw_Verbose(msg,e,id) XMP_Throw(msg, id)
#endif

class GenericErrorCallback {
public:
	// Abstract base class for XMPCore and XMPFiles internal error notification support. Needed so
	// that the XMLParserAdapter (used by both XMPCore and XMPFiles) can send error notifications,
	// and so that utility parts of just XMPCore or XMPFiles can avoid dependence on XMPCore.hpp or
	// XMPFiles.hpp if that is appropriate.

	XMP_Uns32					limit;
	mutable XMP_Uns32			notifications;
	mutable XMP_ErrorSeverity	topSeverity;

	GenericErrorCallback() : notifications(0), limit(1), topSeverity(kXMPErrSev_Recoverable) {};
	virtual ~GenericErrorCallback() {};

	void Clear() { this->notifications = 0; this->limit = 1; this->topSeverity = kXMPErrSev_Recoverable; };

	bool CheckLimitAndSeverity (XMP_ErrorSeverity severity ) const;

	// Const so they can be used with const XMPMeta and XMPFiles objects.
	void NotifyClient ( XMP_ErrorSeverity severity, XMP_Error & error, XMP_StringPtr filePath = 0 ) const;

	virtual bool CanNotify ( ) const = 0;
	virtual bool ClientCallbackWrapper ( XMP_StringPtr filePath, XMP_ErrorSeverity severity, XMP_Int32 cause, XMP_StringPtr messsage ) const = 0;

};

#define XMP_Error_Throw(error)	{ AnnounceThrow (error.GetErrMsg()); throw error; }


// -------------------------------------------------------------------------------------------------

#define _MakeStr(p) #p
#define _NotifyMsg(n,c,f,l)	#n " failed: " #c " in " f " at line " _MakeStr(l)
#define _ExplicitMsg(msg,c,e) #e " " #msg ": " #c

#define XMP_Validate(c,msg,e)									\
	if ( ! (c) ) {												\
		const char * validate_msg = _ExplicitMsg ( msg, c, e );	\
		XMP_Throw ( validate_msg, e );							\
	}

// This statement is needed in XMP_Assert definition to reduce warnings from
// static analysis tool in Visual Studio. Defined here, as platform fork not
// possible within macro definition below
#if XMP_WinBuild
	#define analysis_assume(c) __analysis_assume( c );
#else
	#define analysis_assume(c) ((void) 0)
#endif

#if ! XMP_DebugBuild
	#define XMP_Assert(c)	((void) 0)
#else
		#define XMP_Assert(c)	assert ( c )
#endif

	#define XMP_Enforce(c)																	\
		if ( ! (c) ) {																		\
			const char * assert_msg = _NotifyMsg ( XMP_Enforce, (c), __FILE__, __LINE__ );	\
			XMP_Throw ( assert_msg , kXMPErr_EnforceFailure );								\
		}
// =================================================================================================
// Thread synchronization locks
// ============================

// About XMP and thread synchronization
//
// A variety of choices are provided for thread synchronization. Exactly one method must be chosen
// by defining the appropriate symbol to 1.
//
// * UseNoLock - This choice turns the synchronization functions into no-ops. It must only be used
//   by single threaded clients, or clients providing their own control at a higher level.
//
// * UseGlobalLibraryLock - This choice uses a single per-library lock. The result is thread safe
//   but unfriendly behavior, no true concurrency. This should only be used as a debugging fallback.
//
// * UseBoostLock - This choice uses the Boost shared_mutex mechanism. It has the advantage of being
//   robust and being available on pretty much all platforms. It has the disadvantage of requiring
//   the developer to download, integrate, and build the Boost thread library.
//
// * UsePThreadLock - This choice uses the POSIX pthread rwlock mechanism. It has the advantage of
//   being robust and being available on any modern UNIX platform, including Mac OS X.
//
// * UseWinSlimLock - This choice uses the Windows slim reader/writer mechanism. It is robust but
//   only available on Vista and newer versions of Windows, it is not available on XP.
//
// * UseHomeGrownLock - This choice uses local code plus lower level synchronization primitives. It
//   has the advantage of being usable on all platforms, and having exposed and tunable policy. It
//   has the disadvantage of possibly being less robust than Boost or the O/S provided mechanisms.
//   The lower level synchronization primitives are pthread mutex and condition for UNIX (including
//   Mac OS X). For Windows there is a choice of critical section and condition variable for Vista
//   and newer; or critical section, event, and semaphore for XP and newer.

#define UseHomeGrownLock 1

// -------------------------------------------------------------------------------------------------
// A basic exclusive access mutex and atomic increment/decrement operations.

#if XMP_WinBuild

	#include <Windows.h>

	#define HaveAtomicIncrDecr 1
	typedef LONG XMP_AtomicCounter;

	#define XMP_AtomicIncrement(x)	InterlockedIncrement ( &(x) )
	#define XMP_AtomicDecrement(x)	InterlockedDecrement ( &(x) )

	typedef CRITICAL_SECTION XMP_BasicMutex;
	
	#define InitializeBasicMutex(mutex)	{ InitializeCriticalSection ( &mutex ); }
	#define TerminateBasicMutex(mutex)	{ DeleteCriticalSection ( &mutex ); }
	#define AcquireBasicMutex(mutex)	{ EnterCriticalSection ( &mutex ); }
	#define ReleaseBasicMutex(mutex)	{ LeaveCriticalSection ( &mutex ); }

#elif XMP_MacBuild | XMP_iOSBuild

	#include <pthread.h>
	#include <libkern/OSAtomic.h>

	#define HaveAtomicIncrDecr 1
	typedef int32_t XMP_AtomicCounter;

	#define XMP_AtomicIncrement(x)	OSAtomicIncrement32 ( &(x) )
	#define XMP_AtomicDecrement(x)	OSAtomicDecrement32 ( &(x) )

	typedef pthread_mutex_t XMP_BasicMutex;

	#define InitializeBasicMutex(mutex)	{ int err = pthread_mutex_init ( &mutex, 0 ); XMP_Enforce ( err == 0 ); }
	#define TerminateBasicMutex(mutex)	{ int err = pthread_mutex_destroy ( &mutex ); XMP_Enforce ( err == 0 ); }
	#define AcquireBasicMutex(mutex)	{ int err = pthread_mutex_lock ( &mutex ); XMP_Enforce ( err == 0 ); }
	#define ReleaseBasicMutex(mutex)	{ int err = pthread_mutex_unlock ( &mutex ); XMP_Enforce ( err == 0 ); }

#elif XMP_UNIXBuild

	#include <pthread.h>

	// Atomic increment/decrement intrinsics should be in gcc 4.1, but Solaris seems to lack them.
	#ifndef HaveAtomicIncrDecr
		#define HaveAtomicIncrDecr 1
	#endif
	#if HaveAtomicIncrDecr
		typedef XMP_Uns32 XMP_AtomicCounter;
		#define XMP_AtomicIncrement(x)	__sync_add_and_fetch ( &(x), 1 )
		#define XMP_AtomicDecrement(x)	__sync_sub_and_fetch ( &(x), 1 )
	#endif

	typedef pthread_mutex_t XMP_BasicMutex;

	#define InitializeBasicMutex(mutex)	{ int err = pthread_mutex_init ( &mutex, 0 ); XMP_Enforce ( err == 0 ); }
	#define TerminateBasicMutex(mutex)	{ int err = pthread_mutex_destroy ( &mutex ); XMP_Enforce ( err == 0 ); }
	#define AcquireBasicMutex(mutex)	{ int err = pthread_mutex_lock ( &mutex ); XMP_Enforce ( err == 0 ); }
	#define ReleaseBasicMutex(mutex)	{ int err = pthread_mutex_unlock ( &mutex ); XMP_Enforce ( err == 0 ); }

#endif

class XMP_AutoMutex {
public:
	XMP_AutoMutex ( XMP_BasicMutex * _mutex ) : mutex(_mutex) { AcquireBasicMutex ( *this->mutex ); }
	~XMP_AutoMutex() { this->Release(); }
	void Release() { if ( this->mutex != 0 ) ReleaseBasicMutex ( *this->mutex ); this->mutex = 0; }
private:
	XMP_BasicMutex * mutex;
	XMP_AutoMutex() {};	// ! Must not be used.
};

// -------------------------------------------------------------------------------------------------
// Details for the various locking mechanisms.

#if UseNoLock

	typedef void* XMP_BasicRWLock;	// For single threaded clients that want maximum performance.
	
	#define XMP_BasicRWLock_Initialize(lck)			/* Do nothing. */
	#define XMP_BasicRWLock_Terminate(lck)			/* Do nothing. */

	#define XMP_BasicRWLock_AcquireForRead(lck)		/* Do nothing. */
	#define XMP_BasicRWLock_AcquireForWrite(lck)	/* Do nothing. */

	#define XMP_BasicRWLock_ReleaseFromRead(lck)	/* Do nothing. */
	#define XMP_BasicRWLock_ReleaseFromWrite(lck)	/* Do nothing. */

#elif UseGlobalLibraryLock
	
	extern XMP_BasicMutex sLibraryLock;

	typedef void* XMP_BasicRWLock;	// Use the old thread-unfriendly per-DLL mutex.
	
	#define XMP_BasicRWLock_Initialize(lck)			/* Do nothing. */
	#define XMP_BasicRWLock_Terminate(lck)			/* Do nothing. */

	#define XMP_BasicRWLock_AcquireForRead(lck)		/* Do nothing. */
	#define XMP_BasicRWLock_AcquireForWrite(lck)	/* Do nothing. */

	#define XMP_BasicRWLock_ReleaseFromRead(lck)	/* Do nothing. */
	#define XMP_BasicRWLock_ReleaseFromWrite(lck)	/* Do nothing. */

#elif UseBoostLock

	#include <boost/thread/shared_mutex.hpp>
	typedef boost::shared_mutex XMP_BasicRWLock;
	
	#define XMP_BasicRWLock_Initialize(lck)			/* Do nothing. */
	#define XMP_BasicRWLock_Terminate(lck)			/* Do nothing. */

	#define XMP_BasicRWLock_AcquireForRead(lck)		lck.lock_shared()
	#define XMP_BasicRWLock_AcquireForWrite(lck)	lck.lock()

	#define XMP_BasicRWLock_ReleaseFromRead(lck)	lck.unlock_shared()
	#define XMP_BasicRWLock_ReleaseFromWrite(lck)	lck.unlock()

#elif UsePThreadLock

	#include <pthread.h>
	typedef pthread_rwlock_t XMP_BasicRWLock;
	
	#define XMP_BasicRWLock_Initialize(lck)				\
		{ int err = pthread_rwlock_init ( &lck, 0 );	\
		  if ( err != 0 ) XMP_Throw ( "Initialize pthread rwlock failed", kXMPErr_ExternalFailure ); }
	#define XMP_BasicRWLock_Terminate(lck)	\
		{ int err = pthread_rwlock_destroy ( &lck ); XMP_Assert ( err == 0 ); }

	#define XMP_BasicRWLock_AcquireForRead(lck)		\
		{ int err = pthread_rwlock_rdlock ( &lck );	\
		  if ( err != 0 ) XMP_Throw ( "Acquire pthread read lock failed", kXMPErr_ExternalFailure ); }
	#define XMP_BasicRWLock_AcquireForWrite(lck)	\
		{ int err = pthread_rwlock_wrlock ( &lck );	\
		  if ( err != 0 ) XMP_Throw ( "Acquire pthread write lock failed", kXMPErr_ExternalFailure ); }

	#define XMP_BasicRWLock_ReleaseFromRead(lck)	\
		{ int err = pthread_rwlock_unlock ( &lck );	\
		  if ( err != 0 ) XMP_Throw ( "Release pthread read lock failed", kXMPErr_ExternalFailure ); }
	#define XMP_BasicRWLock_ReleaseFromWrite(lck)	\
		{ int err = pthread_rwlock_unlock ( &lck );	\
		  if ( err != 0 ) XMP_Throw ( "Release pthread write lock failed", kXMPErr_ExternalFailure ); }

#elif UseWinSlimLock

	#include <Windows.h>
	typedef SRWLOCK XMP_BasicRWLock;
	
	#define XMP_BasicRWLock_Initialize(lck)			InitializeSRWLock ( &lck )
	#define XMP_BasicRWLock_Terminate(lck)			/* Do nothing. */

	#define XMP_BasicRWLock_AcquireForRead(lck)		AcquireSRWLockShared ( &lck )
	#define XMP_BasicRWLock_AcquireForWrite(lck)	AcquireSRWLockExclusive ( &lck )

	#define XMP_BasicRWLock_ReleaseFromRead(lck)	ReleaseSRWLockShared ( &lck )
	#define XMP_BasicRWLock_ReleaseFromWrite(lck)	ReleaseSRWLockExclusive ( &lck )

#elif UseHomeGrownLock

	class XMP_HomeGrownLock;
	typedef XMP_HomeGrownLock XMP_BasicRWLock;
	
	#define XMP_BasicRWLock_Initialize(lck)			/* Do nothing. */
	#define XMP_BasicRWLock_Terminate(lck)			/* Do nothing. */
	#define XMP_BasicRWLock_AcquireForRead(lck)		lck.AcquireForRead()
	#define XMP_BasicRWLock_AcquireForWrite(lck)	lck.AcquireForWrite()
	#define XMP_BasicRWLock_ReleaseFromRead(lck)	lck.ReleaseFromRead()
	#define XMP_BasicRWLock_ReleaseFromWrite(lck)	lck.ReleaseFromWrite()
	
	#if XMP_MacBuild | XMP_UNIXBuild | XMP_iOSBuild

		#include <pthread.h>

		typedef pthread_cond_t  XMP_BasicQueue;

	#elif XMP_WinBuild
	
		#include <Windows.h>
		#ifndef BuildLocksForWinXP
			#define BuildLocksForWinXP 1
		#endif

		#if ! BuildLocksForWinXP
			typedef CONDITION_VARIABLE XMP_BasicQueue;	// ! Requires Vista or newer.
		#else
			class XMP_WinXP_HGQueue {
			public:
				XMP_WinXP_HGQueue();
				~XMP_WinXP_HGQueue();
				void Wait ( XMP_BasicMutex & queueMutex );
				void ReleaseOne();
				void ReleaseAll();
			private:
				HANDLE queueEvent;
				volatile XMP_Uns32 waitCount;	// ! Does not need to be XMP_AtomicCounter.
				volatile bool releaseAll;
			};
			typedef XMP_WinXP_HGQueue XMP_BasicQueue;
		#endif

	#endif
	
	class XMP_HomeGrownLock {
	public:
		XMP_HomeGrownLock();
		~XMP_HomeGrownLock();
		void AcquireForRead();
		void AcquireForWrite();
		void ReleaseFromRead();
		void ReleaseFromWrite();
	private:
		XMP_BasicMutex queueMutex;	// Used to protect queueing operations.
		XMP_BasicQueue readerQueue, writerQueue;
		volatile XMP_Uns32 lockCount, readersWaiting, writersWaiting;	// ! Does not need to be XMP_AtomicCounter.
		volatile bool beingWritten;
	};

#else

	#error "No locking mechanism chosen"

#endif

class XMP_ReadWriteLock {	// For the lock objects, use XMP_AutoLock to do the locking.
public:
	XMP_ReadWriteLock();
	~XMP_ReadWriteLock();
	void Acquire ( bool forWriting );
	void Release();
private:
	XMP_BasicRWLock lock;
	#if XMP_DebugBuild && HaveAtomicIncrDecr
		volatile XMP_AtomicCounter lockCount;	// ! Only for debug checks, must be XMP_AtomicCounter.
	#endif
	volatile bool beingWritten;
};

#define kXMP_ReadLock	false
#define kXMP_WriteLock	true

class XMP_AutoLock {
public:
	XMP_AutoLock ( const XMP_ReadWriteLock * _lock, bool forWriting, bool cond = true ) : lock(0)
		{
			if ( cond ) {
				// The cast below is needed because the _lock parameter might come from something
				// like "const XMPMeta &", which would make the lock itself const. But we need to
				// modify the lock (to acquire and release) even if the owning object is const.
				this->lock = (XMP_ReadWriteLock*)_lock;
				this->lock->Acquire ( forWriting );
			}
		}
	~XMP_AutoLock() { this->Release(); }
	void Release() { if ( this->lock != 0 ) this->lock->Release(); this->lock = 0; }
private:
	XMP_ReadWriteLock * lock;
	XMP_AutoLock() {};	// ! Must not be used.
};

// =================================================================================================
// Support for wrappers
// ====================

#define AnnounceStaticEntry(proc)			/* Do nothing. */
#define AnnounceObjectEntry(proc,rwMode)	/* Do nothing. */

#define AnnounceExit()	/* Do nothing. */

// -------------------------------------------------------------------------------------------------

#if UseGlobalLibraryLock
	#define AcquireLibraryLock(lck)	XMP_AutoMutex libLock ( &lck )
#else
	#define AcquireLibraryLock(lck)	/* nothing */
#endif

#define XMP_ENTER_NoLock(Proc)		\
	AnnounceStaticEntry ( Proc );	\
	try {							\
		wResult->errMessage = 0;

#define XMP_ENTER_Static(Proc)				\
	AnnounceStaticEntry ( Proc );			\
	AcquireLibraryLock ( sLibraryLock );	\
	try {									\
		wResult->errMessage = 0;

#define XMP_ENTER_ObjRead(XMPClass,Proc)				\
	AnnounceObjectEntry ( Proc, "reader" );				\
	AcquireLibraryLock ( sLibraryLock );				\
	const XMPClass & thiz = *((XMPClass*)xmpObjRef);	\
	XMP_AutoLock objLock ( &thiz.lock, kXMP_ReadLock );	\
	try {												\
		wResult->errMessage = 0;

#define XMP_ENTER_ObjWrite(XMPClass,Proc)					\
	AnnounceObjectEntry ( Proc, "writer" );					\
	AcquireLibraryLock ( sLibraryLock );					\
	XMPClass * thiz = (XMPClass*)xmpObjRef;					\
	XMP_AutoLock objLock ( &thiz->lock, kXMP_WriteLock );	\
	try {													\
		wResult->errMessage = 0;

#define XMP_EXIT			\
	XMP_CATCH_EXCEPTIONS	\
	AnnounceExit();

#define XMP_EXIT_NoThrow						\
	} catch ( ... )	{							\
		AnnounceCatch ( "no-throw catch-all" );	\
		/* Do nothing. */						\
	}											\
	AnnounceExit();

#define XMP_CATCH_EXCEPTIONS										\
	} catch ( XMP_Error & xmpErr ) {								\
		wResult->int32Result = xmpErr.GetID(); 						\
		wResult->ptrResult   = (void*)"XMP";						\
		wResult->errMessage  = xmpErr.GetErrMsg();					\
		if ( wResult->errMessage == 0 ) wResult->errMessage = "";	\
		AnnounceCatch ( wResult->errMessage );						\
	} catch ( std::exception & stdErr ) {							\
		wResult->int32Result = kXMPErr_StdException; 				\
		wResult->errMessage  = stdErr.what(); 						\
		if ( wResult->errMessage == 0 ) wResult->errMessage = "";	\
		AnnounceCatch ( wResult->errMessage );						\
	} catch ( ... ) {												\
		wResult->int32Result = kXMPErr_UnknownException; 			\
		wResult->errMessage  = "Caught unknown exception";			\
		AnnounceCatch ( wResult->errMessage );						\
	}

#if XMP_DebugBuild
	#define RELEASE_NO_THROW	/* empty */
#else
	#define RELEASE_NO_THROW	throw()
#endif

// =================================================================================================
// Data structure dumping utilities
// ================================

#define IsHexDigit(ch)		( (('0' <= (ch)) && ((ch) <= '9')) || (('A' <= (ch)) && ((ch) <= 'F')) )
#define HexDigitValue(ch)	( (((ch) - '0') < 10) ? ((ch) - '0') : ((ch) - 'A' + 10) )

static const char * kTenSpaces = "          ";
#define OutProcPadding(pad)	{ size_t padLen = (pad); 												\
							  for ( ; padLen >= 10; padLen -= 10 ) OutProcNChars ( kTenSpaces, 10 );	\
							  for ( ; padLen > 0; padLen -= 1 ) OutProcNChars ( " ", 1 ); }


#define OutProcNewline()	{ XMP_Status status = (*outProc) ( refCon, "\n", 1 );  if ( status != 0 ) return; }

#define OutProcNChars(p,n)	{ XMP_Status status = (*outProc) ( refCon, (p), (n) );  if ( status != 0 ) return; }

#define OutProcLiteral(lit)	{ XMP_Status _status = (*outProc) ( refCon, (lit), (XMP_StringLen)strlen(lit) );  if ( _status != 0 ) return; }

#define OutProcString(str)	{ XMP_Status _status = (*outProc) ( refCon, (str).c_str(), (XMP_StringLen)(str).size() );  if ( _status != 0 ) return; }

#define OutProcDecInt(num)	{ snprintf ( buffer, sizeof(buffer), "%ld", (long)(num) ); /* AUDIT: Using sizeof for snprintf length is safe */	\
							  buffer[sizeof(buffer) -1] = 0; /* AUDIT warning C6053: Make sure buffer is terminated */							\
							  XMP_Status _status = (*outProc) ( refCon, buffer, (XMP_StringLen)strlen(buffer) );  if ( _status != 0 ) return; }

#define OutProcHexInt(num)	{ snprintf ( buffer, sizeof(buffer), "%lX", (long)(num) ); /* AUDIT: Using sizeof for snprintf length is safe */	\
							  buffer[sizeof(buffer) -1] = 0; /* AUDIT warning C6053: Make sure buffer is terminated */							\
							  XMP_Status _status = (*outProc) ( refCon, buffer, (XMP_StringLen)strlen(buffer) );  if ( _status != 0 ) return; }

#define OutProcHexByte(num)	{ snprintf ( buffer, sizeof(buffer), "%.2X", (num) ); /* AUDIT: Using sizeof for snprintf length is safe */	\
							  XMP_Status _status = (*outProc) ( refCon, buffer, (XMP_StringLen)strlen(buffer) );  if ( _status != 0 ) return; }

static const char * kIndent = "   ";
#define OutProcIndent(lev)	{ for ( size_t i = 0; i < (lev); ++i ) OutProcNChars ( kIndent, 3 ); }

void DumpClearString ( const XMP_VarString & value, XMP_TextOutputProc outProc, void * refCon );

// =================================================================================================
// Namespace Tables
// ================
typedef std::vector <XMP_VarString> XMP_StringVector;
typedef XMP_StringVector::iterator XMP_StringVectorPos;
typedef XMP_StringVector::const_iterator XMP_StringVectorCPos;

typedef std::pair < XMP_VarString, XMP_VarString >	XMP_StringPair;

typedef std::map < XMP_VarString, XMP_VarString > XMP_StringMap;
typedef XMP_StringMap::iterator       XMP_StringMapPos;
typedef XMP_StringMap::const_iterator XMP_cStringMapPos;

class XMP_NamespaceTable {
public:

	XMP_NamespaceTable() {};
	XMP_NamespaceTable ( const XMP_NamespaceTable & presets );
	virtual ~XMP_NamespaceTable() {};

    bool Define ( XMP_StringPtr uri, XMP_StringPtr suggPrefix,
    			  XMP_StringPtr * prefixPtr, XMP_StringLen * prefixLen);

    bool GetPrefix ( XMP_StringPtr uri, XMP_StringPtr * prefixPtr, XMP_StringLen * prefixLen ) const;
    bool GetURI    ( XMP_StringPtr prefix, XMP_StringPtr * uriPtr, XMP_StringLen * uriLen ) const;

    void Dump ( XMP_TextOutputProc outProc, void * refCon ) const;

private:

	XMP_ReadWriteLock lock;
	XMP_StringMap uriToPrefixMap, prefixToURIMap;

};


// Right now it supports only ^, $ and \d, in future we should use it as a wrapper over
// regex object once mac and Linux compilers start supporting them.

class XMP_RegExp {
public:
	XMP_RegExp ( XMP_StringPtr regExp )
	{
		if ( regExp )
			regExpStr = regExp;
	}

	XMP_Bool Match ( XMP_StringPtr s );

private:
	XMP_VarString regExpStr;
};

// =================================================================================================

#endif	// __XMP_LibUtils_hpp__
