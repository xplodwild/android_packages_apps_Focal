#ifndef __XMP_ProgressTracker_hpp__
#define __XMP_ProgressTracker_hpp__ 1

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

#include "source/PerfUtils.hpp"

// =================================================================================================

class XMP_ProgressTracker {
public:
	
	struct CallbackInfo {

		XMP_ProgressReportWrapper wrapperProc;
		XMP_ProgressReportProc clientProc;
		void * context;
		float  interval;
		bool   sendStartStop;

		void Clear() { this->wrapperProc = 0; this->clientProc = 0;
					   this->context = 0; this->interval = 1.0; this->sendStartStop = false; };
		CallbackInfo() { this->Clear(); };
		CallbackInfo ( XMP_ProgressReportWrapper _wrapperProc, XMP_ProgressReportProc _clientProc,
					   void * _context, float _interval, bool _sendStartStop )
			: wrapperProc(_wrapperProc), clientProc(_clientProc),
			  context(_context), interval(_interval), sendStartStop(_sendStartStop) {};

	};

	XMP_ProgressTracker ( const CallbackInfo & _cbInfo );

	void BeginWork ( float _totalWork = 0.0 );
	void AddTotalWork ( float workIncrement );
	void AddWorkDone ( float workIncrement );
	void WorkComplete();

	bool WorkInProgress() { return this->workInProgress; };

	~XMP_ProgressTracker() {};

private:

	XMP_ProgressTracker() { this->Clear(); };	// Hidden on purpose.

	void Clear();
	void NotifyClient ( bool isStartStop = false );
	
	CallbackInfo cbInfo;
	bool workInProgress;
	float totalWork, workDone;
	PerfUtils::MomentValue startTime, prevTime;

};	// XMP_ProgressTracker

// =================================================================================================

#endif	// __XMP_ProgressTracker_hpp__
