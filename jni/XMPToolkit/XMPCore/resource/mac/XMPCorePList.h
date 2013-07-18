#include "../../../public/include/XMP_Version.h"
#include "../../../build/XMP_BuildInfo.h"

#if NDEBUG	// Can't use XMP_Environment.h, it seems to mess up the PList compiler.
	#define kConfig Release
	#define kDebugSuffix
	#define kBasicVersion XMPCORE_API_VERSION
#else
	#define kConfig Debug
	#define kDebugSuffix (debug)
	#define kBasicVersion XMPCORE_API_VERSION kDebugSuffix
#endif

