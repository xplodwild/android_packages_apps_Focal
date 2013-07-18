#include "../../../public/include/XMP_Version.h"
#include "../../../build/XMP_BuildInfo.h"

#if NDEBUG	// Can't use XMP_Environment.h, it seems to mess up the PList compiler.
	#define kConfig Release
	#define kDebugSuffix
	#define kBasicVersion XMPFILES_API_VERSION
#else
	#define kConfig Debug
	#if IncludeNewHandlers
		#define kDebugSuffix (debug/NewHandlers)
	#else
		#define kDebugSuffix (debug)
	#endif
	#define kBasicVersion XMPFILES_API_VERSION kDebugSuffix
#endif

