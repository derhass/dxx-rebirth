#include "curlutil.h"
#include "json/reader.h"
#include "json/writer.h"

#include <cstring>
#include <vector>

// Some static stuff, because I'm not sure where we should store it :V
static CURLM *g_pCurl = NULL;
struct curl_slist *g_pHeaders = NULL;

// Our private cURL object
typedef struct
{
	std::vector<uint8_t> vData;
	unsigned iPos;
	dxx_http_callback cb;
} dxx_curl_handle;

// Read callback for cURL
static size_t read_cb( void *pData, size_t iSize, size_t iCount, void *pHandle )
{
	// Get the handle
	dxx_curl_handle *pPrivate = (dxx_curl_handle *)pHandle;

	// Enough room?
	unsigned iSpace = pPrivate->vData.size() - pPrivate->iPos;

	if( iSpace < iSize * iCount )
		pPrivate->vData.resize( pPrivate->iPos + iSize * iCount );

	// Pad the info
	memcpy( &pPrivate->vData[pPrivate->iPos], pData, iSize * iCount );
	pPrivate->iPos += iSize * iCount;
	return iSize * iCount;
}

bool CurlUtil::Init()
{
	// Start out easy
	int iRet = curl_global_init( CURL_GLOBAL_SSL );

	// Sanity
	if( iRet != 0 )
	{
		printf( "Error initializing cURL!\n" );
		return false;
	}

	// Create the handle
	g_pCurl = curl_multi_init();

	// Sanity
	if( g_pCurl == NULL )
	{
		printf( "Error initializing cURL handle!\n" );
		return false;
	}

	// Set the headers up
	g_pHeaders = curl_slist_append( g_pHeaders, "Accept: */*" );
	g_pHeaders = curl_slist_append( g_pHeaders, "Content-Type: application/json" );

	return true;
}

void CurlUtil::Deinit()
{
	// Clean up cURL
	curl_multi_cleanup( g_pCurl );
	curl_slist_free_all( g_pHeaders );
	curl_global_cleanup();
}

void CurlUtil::Get( const std::string &sPath, dxx_http_callback cb )
{
	// Create a handle
	CURL *pCurl = curl_easy_init();

	// Sanity
	if( pCurl == NULL )
	{
		printf( "Error opening cURL handle!\n" );
		return;
	}

	// Create our handle
	dxx_curl_handle handle;

	// Set some options
	curl_easy_setopt( pCurl, CURLOPT_URL, sPath.c_str() );
	curl_easy_setopt( pCurl, CURLOPT_HTTPHEADER, g_pHeaders );
	curl_easy_setopt( pCurl, CURLOPT_WRITEFUNCTION, read_cb );
	curl_easy_setopt( pCurl, CURLOPT_ACCEPT_ENCODING, "gzip;q=1.0" );

	// Note, I am hosting the tracker for free
	// I'm not paying extra for the SSL on dxxtracker.reenigne.{net,ca} or tracker.dxx-rebirth.com
	curl_easy_setopt( pCurl, CURLOPT_SSL_VERIFYPEER, 0 );
	curl_easy_setopt( pCurl, CURLOPT_SSL_VERIFYHOST, 0 );

	// Add it to the queue
	CURLMcode iRet = curl_multi_add_handle( g_pCurl, pCurl );

	// Sanity
	if( iRet != CURLM_OK )
		printf( "Failed to add GET request: %s\n", curl_multi_strerror( iRet ) );
}

void CurlUtil::Post( const std::string &sPath, Json::Value &data, dxx_http_callback cb )
{
	// Create the cURL object
	CURL *pCurl = curl_easy_init();

	// Sanity
	if( pCurl == NULL )
	{
		printf( "Can't create cURL handle!\n" );
		return;
	}

	// Build private data
	Json::FastWriter writer;
	std::string sData = writer.write( data );

	// Private handle
	dxx_curl_handle *pHandle = new dxx_curl_handle;
	pHandle->cb = cb;
	pHandle->iPos = 0;

	// Set the options
	curl_easy_setopt( pCurl, CURLOPT_URL, sPath.c_str() );
	curl_easy_setopt( pCurl, CURLOPT_HTTPHEADER, g_pHeaders );
	curl_easy_setopt( pCurl, CURLOPT_POST, 1 );
	curl_easy_setopt( pCurl, CURLOPT_POSTFIELDS, sData.c_str() );
	curl_easy_setopt( pCurl, CURLOPT_WRITEFUNCTION, read_cb );
	curl_easy_setopt( pCurl, CURLOPT_WRITEDATA, pHandle );
	curl_easy_setopt( pCurl, CURLOPT_PRIVATE, pHandle );
	curl_easy_setopt( pCurl, CURLOPT_ACCEPT_ENCODING, "gzip;q=1.0" );

	// Note, I am hosting the tracker for free
	// I'm not paying extra for the SSL on dxxtracker.reenigne.{net,ca} or tracker.dxx-rebirth.com
	curl_easy_setopt( pCurl, CURLOPT_SSL_VERIFYPEER, 0 );
	curl_easy_setopt( pCurl, CURLOPT_SSL_VERIFYHOST, 0 );

	// Add it to the queue
	CURLMcode iRet = curl_multi_add_handle( g_pCurl, pCurl );

	// Sanity
	if( iRet != CURLM_OK )
		printf( "Failed to add POST request: %s\n", curl_multi_strerror( iRet ) );
}

void CurlUtil::Tick()
{
	// The number of transfers still running
	int iRunning = 0;

	// Now, iterate through 'em in the linked list
	CURLMsg *pInfo = curl_multi_info_read( g_pCurl, &iRunning );

	while( pInfo != NULL )
	{
		// Test if it's done
		if( pInfo->msg != CURLMSG_DONE )
		{
			pInfo = curl_multi_info_read( g_pCurl, &iRunning );
			continue;
		}

		// We know it's finished, do some stuff
		char *pPrivate = NULL;
		curl_easy_getinfo( pInfo->easy_handle, CURLINFO_PRIVATE, &pPrivate );

		// Convert it to a tasty tasty vector
		dxx_curl_handle *pHandle = (dxx_curl_handle *)pPrivate;

		// Make some Json value out of this
		Json::Reader reader;
		Json::Value val;
		const char *pBuf = (const char *)&pHandle->vData[0];

		const char *pStart = pBuf;
		const char *pEnd = &pBuf[pHandle->vData.size() - 1];

		// Handle dis, do magic
		if( reader.parse( pStart, pEnd, val ) )
			pHandle->cb( val );
		else
			printf( "Error parsing json data from server!\n" );

		// Clean up
		delete pHandle;

		// Curl stuff too
		curl_multi_remove_handle( g_pCurl, pInfo->easy_handle );
		curl_easy_cleanup( pInfo->easy_handle );

		// Next on the list :D
		pInfo = curl_multi_info_read( g_pCurl, &iRunning );
	}

	// Perform miracles (disclaimer: miracles may not live up to miracle claim)
	CURLMcode iRet = curl_multi_perform( g_pCurl, &iRunning );

	// Miracle badness?
	if( iRet != CURLM_OK )
		printf( "Error ticking (wat) %s\n", curl_multi_strerror( iRet ) );
}
