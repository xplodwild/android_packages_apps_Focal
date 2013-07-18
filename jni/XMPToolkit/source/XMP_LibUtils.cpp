// =================================================================================================
// Copyright 2009 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"

#include "source/XMP_LibUtils.hpp"

#include "source/UnicodeInlines.incl_cpp"

#include <cstdio>
#include <cstring>

// =================================================================================================

#ifndef TraceThreadLocks
	#define TraceThreadLocks 0
#endif

// -------------------------------------------------------------------------------------------------

extern "C" bool Initialize_LibUtils()
{
	return true;
}

// -------------------------------------------------------------------------------------------------

extern "C" void Terminate_LibUtils(){
	// Nothing to do.
}

// =================================================================================================
// Error notifications
// =================================================================================================

bool GenericErrorCallback::CheckLimitAndSeverity ( XMP_ErrorSeverity severity ) const
{

	if ( this->limit == 0 ) return true;	// Always notify if the limit is zero.
	if ( severity < this->topSeverity ) return false;	// Don't notify, don't count.

	if ( severity > this->topSeverity ) {
		this->topSeverity = severity;
		this->notifications = 0;
	}

	this->notifications += 1;
	return (this->notifications <= this->limit);

}	// GenericErrorCallback::CheckLimitAndSeverity

// =================================================================================================

void GenericErrorCallback::NotifyClient ( XMP_ErrorSeverity severity, XMP_Error & error, XMP_StringPtr filePath /*= 0 */ ) const
{
	bool notifyClient = CanNotify() && !error.IsNotified();
	bool returnAndRecover (severity == kXMPErrSev_Recoverable);

	if ( notifyClient ) {
		error.SetNotified();
		notifyClient = CheckLimitAndSeverity ( severity );
		if ( notifyClient ) {
			returnAndRecover &= ClientCallbackWrapper( filePath, severity, error.GetID(), error.GetErrMsg() );
		}
	}

	if ( ! returnAndRecover ) XMP_Error_Throw ( error );

}

// =================================================================================================
// Thread synchronization locks
// =================================================================================================

XMP_ReadWriteLock::XMP_ReadWriteLock() : beingWritten(false)
{
	#if XMP_DebugBuild && HaveAtomicIncrDecr
		this->lockCount = 0;
		// Atomic counter must be 32 or 64 bits and naturally aligned.
		size_t counterSize = sizeof ( XMP_AtomicCounter );
		size_t counterOffset = XMP_OffsetOf ( XMP_ReadWriteLock, lockCount );
		XMP_Assert ( (counterSize == 4) || (counterSize == 8) );	// Counter must be 32 or 64 bits.
		XMP_Assert ( (counterOffset & (counterSize-1)) == 0 );		// Counter must be naturally aligned.
	#endif
	XMP_BasicRWLock_Initialize ( this->lock );
	#if TraceThreadLocks
		fprintf ( stderr, "Created lock %.8X\n", this );
	#endif
}

// ---------------------------------------------------------------------------------------------

XMP_ReadWriteLock::~XMP_ReadWriteLock()
{
	#if TraceThreadLocks
		fprintf ( stderr, "Deleting lock %.8X\n", this );
	#endif
	#if XMP_DebugBuild && HaveAtomicIncrDecr
		XMP_Assert ( this->lockCount == 0 );
	#endif
	XMP_BasicRWLock_Terminate ( this->lock );
}

// ---------------------------------------------------------------------------------------------

void XMP_ReadWriteLock::Acquire ( bool forWriting )
{
	#if TraceThreadLocks
		fprintf ( stderr, "Acquiring lock %.8X for %s, count %d%s\n",
				  this, (forWriting ? "writing" : "reading"), this->lockCount, (this->beingWritten ? ", being written" : "") );
	#endif

	if ( forWriting ) {
		XMP_BasicRWLock_AcquireForWrite ( this->lock );
		#if XMP_DebugBuild && HaveAtomicIncrDecr
			XMP_Assert ( this->lockCount == 0 );
		#endif
	} else {
		XMP_BasicRWLock_AcquireForRead ( this->lock );
		XMP_Assert ( ! this->beingWritten );
	}
	#if XMP_DebugBuild && HaveAtomicIncrDecr
		XMP_AtomicIncrement ( this->lockCount );
	#endif
	this->beingWritten = forWriting;

	#if TraceThreadLocks
		fprintf ( stderr, "Acquired lock %.8X for %s, count %d%s\n",
				  this, (forWriting ? "writing" : "reading"), this->lockCount, (this->beingWritten ? ", being written" : "") );
	#endif
}

// ---------------------------------------------------------------------------------------------

void XMP_ReadWriteLock::Release()
{
	#if TraceThreadLocks
		fprintf ( stderr, "Releasing lock %.8X, count %d%s\n", this, this->lockCount, (this->beingWritten ? ", being written" : "") );
	#endif

	#if XMP_DebugBuild && HaveAtomicIncrDecr
		XMP_Assert ( this->lockCount > 0 );
		XMP_AtomicDecrement ( this->lockCount );	// ! Do these before unlocking, that might release a waiting thread.
	#endif
	bool forWriting = this->beingWritten;
	this->beingWritten = false;

	if ( forWriting ) {
		XMP_BasicRWLock_ReleaseFromWrite ( this->lock );
	} else {
		XMP_BasicRWLock_ReleaseFromRead ( this->lock );
	}

	#if TraceThreadLocks
		fprintf ( stderr, "Released lock %.8X, count %d%s\n", this, this->lockCount, (this->beingWritten ? ", being written" : "") );
	#endif
}

// =================================================================================================

#if UseHomeGrownLock

	#if XMP_MacBuild | XMP_UNIXBuild | XMP_iOSBuild

		// -----------------------------------------------------------------------------------------

		// About pthread mutexes and conditions:
		//
		// The mutex protecting the condition must be locked before waiting for the condition. A
		// thread can wait for a condition to be signaled by calling the pthread_cond_wait
		// subroutine. The subroutine atomically unlocks the mutex and blocks the calling thread
		// until the condition is signaled. When the call returns, the mutex is locked again.

		#define InitializeBasicMutex(mutex)	{ int err = pthread_mutex_init ( &mutex, 0 ); XMP_Enforce ( err == 0 ); }
		#define TerminateBasicMutex(mutex)	{ int err = pthread_mutex_destroy ( &mutex ); XMP_Enforce ( err == 0 ); }

		#define AcquireBasicMutex(mutex)	{ int err = pthread_mutex_lock ( &mutex ); XMP_Enforce ( err == 0 ); }
		#define ReleaseBasicMutex(mutex)	{ int err = pthread_mutex_unlock ( &mutex ); XMP_Enforce ( err == 0 ); }

		#define InitializeBasicQueue(queue)	{ int err = pthread_cond_init ( &queue, 0 ); XMP_Enforce ( err == 0 ); }
		#define TerminateBasicQueue(queue)	{ int err = pthread_cond_destroy ( &queue ); XMP_Enforce ( err == 0 ); }

		#define WaitOnBasicQueue(queue,mutex)	{ int err = pthread_cond_wait ( &queue, &mutex ); XMP_Enforce ( err == 0 ); }
		#define ReleaseOneBasicQueue(queue)		{ int err = pthread_cond_signal ( &queue ); XMP_Enforce ( err == 0 ); }
		#define ReleaseAllBasicQueue(queue)		{ int err = pthread_cond_broadcast ( &queue ); XMP_Enforce ( err == 0 ); }

		// -----------------------------------------------------------------------------------------

	#elif XMP_WinBuild

		// -----------------------------------------------------------------------------------------

		#define InitializeBasicMutex(mutex)	{ InitializeCriticalSection ( &mutex ); }
		#define TerminateBasicMutex(mutex)	{ DeleteCriticalSection ( &mutex ); }

		#define AcquireBasicMutex(mutex)	{ EnterCriticalSection ( &mutex ); }
		#define ReleaseBasicMutex(mutex)	{ LeaveCriticalSection ( &mutex ); }

		#if ! BuildLocksForWinXP

			// About Win32 condition variables (not on XP):
			//
			// Condition variables enable threads to atomically release a lock and enter the
			// sleeping state. They can be used with critical sections or slim reader/writer (SRW)
			// locks. Condition variables support operations that "wake one" or "wake all" waiting
			// threads. After a thread is woken, it re-acquires the lock it released when the thread
			// entered the sleeping state.

			#define InitializeBasicQueue(queue)	{ InitializeConditionVariable ( &queue ); }
			#define TerminateBasicQueue(queue)	/* Do nothing. */

			#define WaitOnBasicQueue(queue,mutex)	\
				{ BOOL ok = SleepConditionVariableCS ( &queue, &mutex, INFINITE /* timeout */ ); XMP_Enforce ( ok ); }

			#define ReleaseOneBasicQueue(queue)	{ WakeConditionVariable ( &queue ); }
			#define ReleaseAllBasicQueue(queue)	{ WakeAllConditionVariable ( &queue ); }

		#else

			// Need to create our own queue for Windows XP. This is not a general queue, it depends
			// on the usage inside XMP_HomeGrownLock where the queueMutex guarantees that the
			// queueing operations are done single threaded.

			#define InitializeBasicQueue(queue)	/* Do nothing. */
			#define TerminateBasicQueue(queue)	/* Do nothing. */

			#define WaitOnBasicQueue(queue,mutex)	{ queue.Wait ( mutex ); }
			#define ReleaseOneBasicQueue(queue)		{ queue.ReleaseOne(); }
			#define ReleaseAllBasicQueue(queue)		{ queue.ReleaseAll(); }

			// -------------------------------------------------------------------------------------

			XMP_WinXP_HGQueue::XMP_WinXP_HGQueue() : queueEvent(0), waitCount(0), releaseAll(false)
			{
				this->queueEvent = CreateEvent ( NULL, FALSE, TRUE, NULL );	// Auto reset, initially clear.
				XMP_Enforce ( this->queueEvent != 0 );
			}

			// -------------------------------------------------------------------------------------

			XMP_WinXP_HGQueue::~XMP_WinXP_HGQueue()
			{
				CloseHandle ( this->queueEvent );
			}

			// -------------------------------------------------------------------------------------

			void XMP_WinXP_HGQueue::Wait ( XMP_BasicMutex & queueMutex )
			{
				++this->waitCount;	// ! Does not need atomic increment, protected by queue mutex.
				ReleaseBasicMutex ( queueMutex );
				DWORD status = WaitForSingleObject ( this->queueEvent, INFINITE );
				if ( status != WAIT_OBJECT_0 ) XMP_Throw ( "Failure from WaitForSingleObject", kXMPErr_ExternalFailure );
				AcquireBasicMutex ( queueMutex );
				--this->waitCount;	// ! Does not need atomic decrement, protected by queue mutex.

				if ( this->releaseAll ) {
					if ( this->waitCount == 0 ) {
						this->releaseAll = false;
					} else {
						BOOL ok = SetEvent ( this->queueEvent );
						if ( ! ok ) XMP_Throw ( "Failure from SetEvent", kXMPErr_ExternalFailure );
					}
				}
			}

			// -------------------------------------------------------------------------------------

			void XMP_WinXP_HGQueue::ReleaseOne()
			{
				XMP_Assert ( ! this->releaseAll );
				BOOL ok = SetEvent ( this->queueEvent );
				if ( ! ok ) XMP_Throw ( "Failure from SetEvent", kXMPErr_ExternalFailure );
			}

			// -------------------------------------------------------------------------------------

			void XMP_WinXP_HGQueue::ReleaseAll()
			{
				this->releaseAll = true;
				BOOL ok = SetEvent ( this->queueEvent );
				if ( ! ok ) XMP_Throw ( "Failure from SetEvent", kXMPErr_ExternalFailure );
			}

		#endif

		// -----------------------------------------------------------------------------------------

	#endif

	// =============================================================================================

	XMP_HomeGrownLock::XMP_HomeGrownLock() : lockCount(0), readersWaiting(0), writersWaiting(0), beingWritten(false)
	{
		InitializeBasicMutex ( this->queueMutex );
		InitializeBasicQueue ( this->writerQueue );
		InitializeBasicQueue ( this->readerQueue );
	}

	// =============================================================================================

	XMP_HomeGrownLock::~XMP_HomeGrownLock()
	{
		TerminateBasicMutex ( this->queueMutex );
		TerminateBasicQueue ( this->writerQueue );
		TerminateBasicQueue ( this->readerQueue );
	}

	// =============================================================================================

	void XMP_HomeGrownLock::AcquireForRead()
	{
		XMP_AutoMutex autoMutex ( &this->queueMutex );

		++this->readersWaiting;	// ! Does not need atomic increment, protected by queue mutex.
		while ( (this->beingWritten) || (this->writersWaiting > 0) ) {
			// Don't allow more readers if writers are waiting.
			WaitOnBasicQueue ( this->readerQueue, this->queueMutex );
		}
		--this->readersWaiting;	// ! Does not need atomic decrement, protected by queue mutex.
		XMP_Assert ( ! this->beingWritten );

		++this->lockCount;	// ! Does not need atomic increment, protected by queue mutex.
	}

	// =============================================================================================

	void XMP_HomeGrownLock::AcquireForWrite()
	{
		XMP_AutoMutex autoMutex ( &this->queueMutex );

		++this->writersWaiting;	// ! Does not need atomic increment, protected by queue mutex.
		while ( this->lockCount > 0 ) {
			WaitOnBasicQueue ( this->writerQueue, this->queueMutex );
		}
		--this->writersWaiting;	// ! Does not need atomic decrement, protected by queue mutex.
		XMP_Assert ( (! this->beingWritten) && (this->lockCount == 0) );

		++this->lockCount;	// ! Does not need atomic increment, protected by queue mutex.
		this->beingWritten = true;
	}

	// =============================================================================================

	void XMP_HomeGrownLock::ReleaseFromRead()
	{
		XMP_AutoMutex autoMutex ( &this->queueMutex );

		XMP_Assert ( (! this->beingWritten) && (this->lockCount > 0) );
		--this->lockCount;	// ! Does not need atomic decrement, protected by queue mutex.

		if ( this->writersWaiting > 0 ) {
			ReleaseOneBasicQueue ( this->writerQueue );
		} else if ( this->readersWaiting > 0 ) {
			ReleaseAllBasicQueue ( this->readerQueue );
		}

	}

	// =============================================================================================

	void XMP_HomeGrownLock::ReleaseFromWrite()
	{
		XMP_AutoMutex autoMutex ( &this->queueMutex );

		XMP_Assert ( this->beingWritten && (this->lockCount == 1) );
		--this->lockCount;	// ! Does not need atomic decrement, protected by queue mutex.
		this->beingWritten = false;

		if ( this->writersWaiting > 0 ) {
			ReleaseOneBasicQueue ( this->writerQueue );
		} else if ( this->readersWaiting > 0 ) {
			ReleaseAllBasicQueue ( this->readerQueue );
		}
	}

	// =============================================================================================

#endif

// =================================================================================================
// Data structure dumping utilities
// ================================

void
DumpClearString ( const XMP_VarString & value, XMP_TextOutputProc outProc, void * refCon )
{

	char buffer [20];
	bool prevNormal;
	XMP_Status status = 0;

	XMP_StringPtr spanStart, spanEnd;
	XMP_StringPtr valueEnd = &value[0] + value.size();

	spanStart = &value[0];
	while ( spanStart < valueEnd ) {

		// Output the next span of regular characters.
		for ( spanEnd = spanStart; spanEnd < valueEnd; ++spanEnd ) {
			if ( *spanEnd > 0x7F ) break;
			if ( (*spanEnd < 0x20) && (*spanEnd != kTab) && (*spanEnd != kLF) ) break;
		}
		if ( spanStart != spanEnd ) status = (*outProc) ( refCon,  spanStart, (XMP_StringLen)(spanEnd-spanStart) );
		if ( status != 0 ) break;
		spanStart = spanEnd;

		// Output the next span of irregular characters.
		prevNormal = true;
		for ( spanEnd = spanStart; spanEnd < valueEnd; ++spanEnd ) {
			if ( ((0x20 <= *spanEnd) && (*spanEnd <= 0x7F)) || (*spanEnd == kTab) || (*spanEnd == kLF) ) break;
			char space = ' ';
			if ( prevNormal ) space = '<';
			status = (*outProc) ( refCon, &space, 1 );
			if ( status != 0 ) break;
			OutProcHexByte ( *spanEnd );
			prevNormal = false;
		}
		if ( ! prevNormal ) {
			status = (*outProc) ( refCon, ">", 1 );
			if ( status != 0 ) return;
		}
		spanStart = spanEnd;

	}

}	// DumpClearString

// -------------------------------------------------------------------------------------------------

static void
DumpStringMap ( const XMP_StringMap & map, XMP_StringPtr label, XMP_TextOutputProc outProc, void * refCon )
{
	XMP_cStringMapPos currPos;
	XMP_cStringMapPos endPos = map.end();

	size_t maxLen = 0;
	for ( currPos = map.begin(); currPos != endPos; ++currPos ) {
		size_t currLen = currPos->first.size();
		if ( currLen > maxLen ) maxLen = currLen;
	}

	OutProcNewline();
	OutProcLiteral ( label );
	OutProcNewline();

	for ( currPos = map.begin(); currPos != endPos; ++currPos ) {
		OutProcNChars ( "  ", 2 );
		DumpClearString ( currPos->first, outProc, refCon );
		OutProcPadding ( maxLen - currPos->first.size() );
		OutProcNChars ( " => ", 4 );
		DumpClearString ( currPos->second, outProc, refCon );
		OutProcNewline();
	}

}	// DumpStringMap

// =================================================================================================
// Namespace Tables
// =================================================================================================

XMP_NamespaceTable::XMP_NamespaceTable ( const XMP_NamespaceTable & presets )
{
	XMP_AutoLock presetLock ( &presets.lock, kXMP_ReadLock );

	this->uriToPrefixMap = presets.uriToPrefixMap;
	this->prefixToURIMap = presets.prefixToURIMap;

}	// XMP_NamespaceTable::XMP_NamespaceTable

// =================================================================================================

bool XMP_NamespaceTable::Define ( XMP_StringPtr _uri, XMP_StringPtr _suggPrefix,
								  XMP_StringPtr * prefixPtr, XMP_StringLen * prefixLen )
{
	XMP_AutoLock tableLock ( &this->lock, kXMP_WriteLock );
	bool prefixMatches = false;

	XMP_Assert ( (_uri != 0) && (*_uri != 0) && (_suggPrefix != 0) && (*_suggPrefix != 0) );

	XMP_VarString	uri ( _uri );
	XMP_VarString	suggPrefix ( _suggPrefix );
	if ( suggPrefix[suggPrefix.size()-1] != ':' ) suggPrefix += ':';
	VerifySimpleXMLName ( _suggPrefix, _suggPrefix+suggPrefix.size()-1 );	// Exclude the colon.

	XMP_StringMapPos uriPos = this->uriToPrefixMap.find ( uri );

	if ( uriPos == this->uriToPrefixMap.end() ) {

		// The URI is not yet registered, make sure we use a unique prefix.

		XMP_VarString uniqPrefix ( suggPrefix );
		int  suffix = 0;
		char buffer [32];	// AUDIT: Plenty of room for the "_%d_" suffix.

		while ( true ) {
			if ( this->prefixToURIMap.find ( uniqPrefix ) == this->prefixToURIMap.end() ) break;
			++suffix;
			snprintf ( buffer, sizeof(buffer), "_%d_:", suffix );	// AUDIT: Using sizeof for snprintf length is safe.
			uniqPrefix = suggPrefix;
			uniqPrefix.erase ( uniqPrefix.size()-1 );	// ! Remove the trailing ':'.
			uniqPrefix += buffer;
		}

		// Add the new namespace to both maps.

		XMP_StringPair newNS ( uri, uniqPrefix );
		uriPos = this->uriToPrefixMap.insert ( this->uriToPrefixMap.end(), newNS );

		newNS.first.swap ( newNS.second );
		(void) this->prefixToURIMap.insert ( this->prefixToURIMap.end(), newNS );

	}

	// Return the actual prefix and see if it matches the suggested prefix.

	if ( prefixPtr != 0 ) *prefixPtr = uriPos->second.c_str();
	if ( prefixLen != 0 ) *prefixLen = (XMP_StringLen)uriPos->second.size();

	prefixMatches = ( uriPos->second == suggPrefix );
	return prefixMatches;

}	// XMP_NamespaceTable::Define

// =================================================================================================

bool XMP_NamespaceTable::GetPrefix ( XMP_StringPtr _uri, XMP_StringPtr * prefixPtr, XMP_StringLen * prefixLen ) const
{
	XMP_AutoLock tableLock ( &this->lock, kXMP_ReadLock );
	bool found = false;

	XMP_Assert ( (_uri != 0) && (*_uri != 0) );

	XMP_VarString uri ( _uri );
	XMP_cStringMapPos uriPos = this->uriToPrefixMap.find ( uri );

	if ( uriPos != this->uriToPrefixMap.end() ) {
		if ( prefixPtr != 0 ) *prefixPtr = uriPos->second.c_str();
		if ( prefixLen != 0 ) *prefixLen = (XMP_StringLen)uriPos->second.size();
		found = true;
	}

	return found;

}	// XMP_NamespaceTable::GetPrefix

// =================================================================================================

bool XMP_NamespaceTable::GetURI ( XMP_StringPtr _prefix, XMP_StringPtr * uriPtr, XMP_StringLen * uriLen ) const
{
	XMP_AutoLock tableLock ( &this->lock, kXMP_ReadLock );

	bool found = false;

	XMP_Assert ( (_prefix != 0) && (*_prefix != 0) );

	XMP_VarString prefix ( _prefix );
	if ( prefix[prefix.size()-1] != ':' ) prefix += ':';
	XMP_cStringMapPos prefixPos = this->prefixToURIMap.find ( prefix );

	if ( prefixPos != this->prefixToURIMap.end() ) {
		if ( uriPtr != 0 ) *uriPtr = prefixPos->second.c_str();
		if ( uriLen != 0 ) *uriLen = (XMP_StringLen)prefixPos->second.size();
		found = true;
	}

	return found;

}	// XMP_NamespaceTable::GetURI

// =================================================================================================

void XMP_NamespaceTable::Dump ( XMP_TextOutputProc outProc, void * refCon ) const
{
	XMP_AutoLock tableLock ( &this->lock, kXMP_ReadLock );

	XMP_cStringMapPos p2uEnd = this->prefixToURIMap.end();	// ! Move up to avoid gcc complaints.
	XMP_cStringMapPos u2pEnd = this->uriToPrefixMap.end();

	DumpStringMap ( this->prefixToURIMap, "Dumping namespace prefix to URI map", outProc, refCon );

	if ( this->prefixToURIMap.size() != this->uriToPrefixMap.size() ) {
		OutProcLiteral ( "** bad namespace map sizes **" );
		XMP_Throw ( "Fatal namespace map problem", kXMPErr_InternalFailure );
	}

	for ( XMP_cStringMapPos nsLeft = this->prefixToURIMap.begin(); nsLeft != p2uEnd; ++nsLeft ) {

		XMP_cStringMapPos nsOther = this->uriToPrefixMap.find ( nsLeft->second );
		if ( (nsOther == u2pEnd) || (nsLeft != this->prefixToURIMap.find ( nsOther->second )) ) {
			OutProcLiteral ( "  ** bad namespace URI **  " );
			DumpClearString ( nsLeft->second, outProc, refCon );
			break;
		}

		for ( XMP_cStringMapPos nsRight = nsLeft; nsRight != p2uEnd; ++nsRight ) {
			if ( nsRight == nsLeft ) continue;	// ! Can't start at nsLeft+1, no operator+!
			if ( nsLeft->second == nsRight->second ) {
				OutProcLiteral ( "  ** duplicate namespace URI **  " );
				DumpClearString ( nsLeft->second, outProc, refCon );
				break;
			}
		}

	}

	for ( XMP_cStringMapPos nsLeft = this->uriToPrefixMap.begin(); nsLeft != u2pEnd; ++nsLeft ) {

		XMP_cStringMapPos nsOther = this->prefixToURIMap.find ( nsLeft->second );
		if ( (nsOther == p2uEnd) || (nsLeft != this->uriToPrefixMap.find ( nsOther->second )) ) {
			OutProcLiteral ( "  ** bad namespace prefix **  " );
			DumpClearString ( nsLeft->second, outProc, refCon );
			break;
		}

		for ( XMP_cStringMapPos nsRight = nsLeft; nsRight != u2pEnd; ++nsRight ) {
			if ( nsRight == nsLeft ) continue;	// ! Can't start at nsLeft+1, no operator+!
			if ( nsLeft->second == nsRight->second ) {
				OutProcLiteral ( "  ** duplicate namespace prefix **  " );
				DumpClearString ( nsLeft->second, outProc, refCon );
				break;
			}
		}

	}

}	// XMP_NamespaceTable::Dump

// =================================================================================================
static XMP_Bool matchdigit ( XMP_StringPtr text ) {
	if ( *text >= '0' && *text <= '9' )
		return true;
	return false;
}

/* matchhere: search for regexp at beginning of text */
static XMP_Bool matchhere ( XMP_StringPtr regexp, XMP_StringPtr text ) {
	if ( regexp[0] == '\0' )
		return true;
	if ( regexp[0] == '\\' && regexp[1] == 'd' ) {
		if ( matchdigit(text) )
			return matchhere ( regexp+2, text+1 );
		else
			return false;
	}

	if ( regexp[0] == '$' && regexp[1] == '\0' )
		return *text == '\0';

	if ( *text != '\0' && regexp[0] == *text )
		return matchhere ( regexp+1, text+1 );
	return 0;
}

/* match: search for regexp anywhere in text */
static XMP_Bool match ( XMP_StringPtr regexp, XMP_StringPtr text ) {
	if ( regexp[0] == '^' )
		return matchhere ( regexp+1, text );
	do {    /* must look even if string is empty */
		if ( matchhere ( regexp, text ) )
			return true;
	} while ( *text++ != '\0' );
	return false;
}

XMP_Bool XMP_RegExp::Match ( XMP_StringPtr s )
{
	if ( regExpStr.size() == 0 )
		return true;
	if ( s == NULL )
		return false;
	return match ( this->regExpStr.c_str(), s );
}
// =================================================================================================
