#ifdef USE_TRACKER
#include "trackerutil.h"
#include "args.h"
#include "multi.h"
#include "vers_id.h"

#include <cstdio>

// Tracker API version
#define DXX_TRACKER_API_VERSION "v1"

void TrackerUtil::ReqGames( dxx_http_callback cb )
{
	// URL to hit
	char sUrl[256];
	sprintf( sUrl, "http://%s/api/%s/games?game=%s&major=%d&minor=%d&micro=%d",
		GameArg.MplTrackerHost,
		DXX_TRACKER_API_VERSION,
		DXX_NAME_NUMBER,
		DXX_VERSION_MAJORi,
		DXX_VERSION_MINORi,
		DXX_VERSION_MICROi
	);

	// Send off the request
	CurlUtil::Get( sUrl, cb );
}

void TrackerUtil::RegisterGame( Json::Value &data, dxx_http_callback cb )
{
	// The URL to hit
	char sUrl[256];
	sprintf( sUrl, "http://%s/api/%s/games/new",
		GameArg.MplTrackerHost,
		DXX_TRACKER_API_VERSION
	);

	// Pass it off
	CurlUtil::Post( sUrl, data, cb );
}

#endif // USE_TRACKER
