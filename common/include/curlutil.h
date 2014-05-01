#ifndef CURL_UTIL_H
#define CURL_UTIL_H

#include "json/value.h"
#include <curl/curl.h>

// Boom, interaction; callbacks from cURL requests (data, length)
// XXX I should hand back a json value :V
typedef void (*dxx_http_callback)(Json::Value&);

namespace CurlUtil
{
	// Initialize once per program run
	bool Init();

	// Clean up all memory
	void Deinit();

	// Perform a GET
	void Get( const std::string &sPath, dxx_http_callback cb );

	// Perform a POST
	void Post( const std::string &sPath, Json::Value &data, dxx_http_callback cb );

	// Tick to do a bit of socket work every frame
	void Tick();
}

#endif // CURL_UTIL_H
