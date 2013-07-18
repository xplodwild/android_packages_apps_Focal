// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2012 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"	// ! XMP_Environment.h must be the first included header.

#include "public/include/XMP_Const.h"

#include "source/XMP_LibUtils.hpp"
#include "source/XMP_ProgressTracker.hpp"

// =================================================================================================
// XMP_ProgressTracker::XMP_ProgressTracker
// ========================================

XMP_ProgressTracker::XMP_ProgressTracker ( const CallbackInfo & _cbInfo )
{

	this->Clear();
	if ( _cbInfo.clientProc == 0 ) return;
	XMP_Assert ( _cbInfo.wrapperProc != 0 );
	
	this->cbInfo = _cbInfo;
	if ( this->cbInfo.interval < 0.0 ) this->cbInfo.interval = 1.0;

}	// XMP_ProgressTracker::XMP_ProgressTracker

// =================================================================================================
// XMP_ProgressTracker::BeginWork
// ==============================

void XMP_ProgressTracker::BeginWork ( float _totalWork )
{

	if ( _totalWork < 0.0 ) _totalWork = 0.0;
	this->totalWork = _totalWork;
	this->workDone = 0.0;
	this->workInProgress = true;

	this->startTime = this->prevTime = PerfUtils::NoteThisMoment();
	if ( this->cbInfo.sendStartStop ) this->NotifyClient ( true );

}	// XMP_ProgressTracker::BeginWork

// =================================================================================================
// XMP_ProgressTracker::AddTotalWork
// =================================

void XMP_ProgressTracker::AddTotalWork ( float workIncrement )
{

	if ( workIncrement < 0.0 ) workIncrement = 0.0;
	this->totalWork += workIncrement;
	
}	// XMP_ProgressTracker::AddTotalWork

// =================================================================================================
// XMP_ProgressTracker::AddWorkDone
// ================================

void XMP_ProgressTracker::AddWorkDone ( float workIncrement )
{

	if ( workIncrement < 0.0 ) workIncrement = 0.0;
	this->workDone += workIncrement;
	this->NotifyClient();
	
}	// XMP_ProgressTracker::AddWorkDone

// =================================================================================================
// XMP_ProgressTracker::WorkComplete
// =================================

void XMP_ProgressTracker::WorkComplete ()
{

	if ( this->totalWork == 0.0 ) this->totalWork = 1.0;	// Force non-zero fraction done.
	this->workDone = this->totalWork;
	XMP_Assert ( this->workDone > 0.0 );	// Needed in NotifyClient for start/stop case.

	this->NotifyClient ( this->cbInfo.sendStartStop );
	this->workInProgress = false;

}	// XMP_ProgressTracker::WorkComplete

// =================================================================================================
// XMP_ProgressTracker::Clear
// ==========================

void XMP_ProgressTracker::Clear ()
{

	this->cbInfo.Clear();
	this->workInProgress = false;
	this->totalWork = 0.0;
	this->workDone = 0.0;
	this->startTime = this->prevTime = PerfUtils::kZeroMoment;
	// ! There is no standard zero value for PerfUtils::MomentValue.

}	// XMP_ProgressTracker::Clear

// =================================================================================================
// XMP_ProgressTracker::NotifyClient
// =================================
//
// The math for the time remaining is easy but not immediately obvious. We know the elapsed time and
// fraction of work done:
//
//		elapsedTime = totalTime * fractionDone		or		totalTime = (elapsedTime / fractionDone)
//		remainingTime = totalTime * (1 - fractionDone)
// so:
//		remainingTime = (elapsedTime / fractionDone) * (1 - fractionDone)

void XMP_ProgressTracker::NotifyClient ( bool isStartStop )
{
	XMP_Bool ok = !kXMP_Bool_False;
	float fractionDone = 0.0;
	
	if ( this->cbInfo.clientProc == 0 ) return;
	XMP_Assert ( this->cbInfo.wrapperProc != 0 );
	XMP_Assert ( (this->totalWork >= 0.0) && (this->workDone >= 0.0) && (this->cbInfo.interval >= 0.0) );
	// ! Note that totalWork might be unknown or understimated, and workDone greater than totalWork.
	
	if ( isStartStop ) {

		float totalTime = 0.0;
		if ( this->workDone > 0.0 ) {
			fractionDone = 1.0;	// This is the stop call.
			totalTime = PerfUtils::GetElapsedSeconds ( this->startTime, PerfUtils::NoteThisMoment() );
		}
		ok = (*this->cbInfo.wrapperProc ) ( this->cbInfo.clientProc, this->cbInfo.context,
											totalTime, fractionDone, 0.0 );

	} else {

		PerfUtils::MomentValue currentTime = PerfUtils::NoteThisMoment();
		float elapsedTime = PerfUtils::GetElapsedSeconds ( this->prevTime, currentTime );
		if ( elapsedTime < this->cbInfo.interval ) return;

		float remainingTime = 0.0;
		if ( (this->totalWork > 0.0) && (this->workDone > 0.0) ) {
			fractionDone = this->workDone / this->totalWork;
			if ( fractionDone > 1.0 ) fractionDone = 1.0;
			elapsedTime = PerfUtils::GetElapsedSeconds ( this->startTime, currentTime );
			remainingTime = (elapsedTime / fractionDone) * (1.0 - fractionDone);
		}

		this->prevTime = currentTime;
		ok = (*this->cbInfo.wrapperProc ) ( this->cbInfo.clientProc, this->cbInfo.context,
											elapsedTime, fractionDone, remainingTime );

	}

	if ( ok == kXMP_Bool_False ) XMP_Throw ( "Abort signaled by progress reporting callback", kXMPErr_ProgressAbort );
		
}	// XMP_ProgressTracker::NotifyClient

// =================================================================================================
