#ifndef TRACKER_UTIL_H
#define TRACKER_UTIL_H

#ifdef USE_TRACKER

#include "curlutil.h"

namespace TrackerUtil
{
	// Request a games list
	void ReqGames( dxx_http_callback cb );

	// Post a new game to the tracker
	void RegisterGame( Json::Value &data, dxx_http_callback cb );

	// Remove a game from the tracker
	void UnregisterGame( Json::Value &data, dxx_http_callback cb );
}

#endif // USE_TRACKER

#endif // TRACKER_UTIL_H
