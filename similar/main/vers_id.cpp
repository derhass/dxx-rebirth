#include "vers_id.h"

#ifndef DESCENT_VERSION_EXTRA
#define DESCENT_VERSION_EXTRA	"v" VERSION
#endif

const char g_descent_version[40] = "D" DXX_NAME_NUMBER "X-Rebirth " DESCENT_VERSION_EXTRA;
const char g_descent_build_datetime[21] = __DATE__ " " __TIME__;

#ifdef RECORD_BUILD_ENVIRONMENT
#define RECORD_BUILD_VARIABLE(X)	extern const char g_descent_##X[];	\
	const char g_descent_##X[] = #X "=" DESCENT_##X

RECORD_BUILD_ENVIRONMENT;
#endif
