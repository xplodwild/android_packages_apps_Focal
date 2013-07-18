#ifndef __PerfUtils_hpp__
#define __PerfUtils_hpp__ 1

// =================================================================================================
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "public/include/XMP_Environment.h"

#if XMP_MacBuild
	#include <CoreServices/CoreServices.h>
#elif XMP_WinBuild
	#include <Windows.h>
#elif XMP_UNIXBuild | XMP_iOSBuild
	#include <time.h>
#endif

namespace PerfUtils {

	#if XMP_WinBuild
//		typedef LARGE_INTEGER MomentValue;
		typedef LONGLONG MomentValue;
		static const MomentValue kZeroMoment = 0;
	#elif XMP_UNIXBuild
		typedef struct timespec MomentValue;
		static const MomentValue kZeroMoment = {0, 0};
	#elif XMP_iOSBuild | XMP_MacBuild
		typedef uint64_t MomentValue;
		static const MomentValue kZeroMoment = 0;
	#endif

	const char * GetTimerInfo();

	MomentValue NoteThisMoment();
	
	double GetElapsedSeconds ( MomentValue start, MomentValue finish );

};	// PerfUtils

#endif	// __PerfUtils_hpp__
