#ifdef USE_TRACKER
#include "trackerutil.h"
#include "args.h"
#include "multi.h"

#include <cstdio>

// Tracker API version
#define DXX_TRACKER_API_VERSION "v1"

// Figure out what we're running
#if defined( DXX_BUILD_DESCENT_I )
static unsigned g_iGame = 1;
#elif defined( DXX_BUILD_DESCENT_II )
static unsigned g_iGame = 2;
#else
#  error "What the hell?  We're on crack!  Decide on a descent version!"
#endif

void TrackerUtil::ReqGames( dxx_http_callback cb )
{
	// URL to hit
	// XXX take into account major minor and micro!!!
	char sUrl[256];
	sprintf( sUrl, "http://%s/api/%s/games?game=%d&major=%d&minor=%d&micro=%d",
		GameArg.MplTrackerHost,
		DXX_TRACKER_API_VERSION,
		g_iGame,
		DXX_VERSION_MAJORi,
		DXX_VERSION_MINORi,
		DXX_VERSION_MICROi
	);

	// Send off the request
	CurlUtil::Get( "http://ip.jsontest.com/", cb );
}

#endif // USE_TRACKER
