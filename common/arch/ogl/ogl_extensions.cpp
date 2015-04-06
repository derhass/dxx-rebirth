/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/* OpenGL extensions:
 * We use global function pointer for any extension function we want to use.
 */

#include <string.h>

#include <SDL/SDL.h>

#include "ogl_extensions.h"
#include "console.h"

#include "dxxsconf.h"
#include "compiler-array.h"

/* GL 3.2 */
bool ogl_have_3_2=false;
PFNGLGETINTEGER64VPROC glGetInteger64vFunc = NULL;

/* GL_ARB_sync */
bool ogl_have_ARB_sync = false;
PFNGLFENCESYNCPROC glFenceSyncFunc = NULL;
PFNGLDELETESYNCPROC glDeleteSyncFunc = NULL;
PFNGLCLIENTWAITSYNCPROC glClientWaitSyncFunc = NULL;

/* GL_ARB_occlusion_query */
bool ogl_have_ARB_occlusion_query=false;
PFNGLGENQUERIESARBPROC glGenQueriesARBFunc = NULL;
PFNGLDELETEQUERIESARBPROC glDeleteQueriesARBFunc = NULL;

/* GL_ARB_timer_query */
bool ogl_have_ARB_timer_query=false;
PFNGLQUERYCOUNTERPROC glQueryCounterFunc = NULL;
PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64vFunc = NULL;

static array<long, 2> parse_version_str(const char *v)
{
	array<long, 2> version;
	version[0]=1;
	version[1]=0;
	if (v) {
		char *ptr;
		version[0]=strtol(v,&ptr,10);
		if (ptr[0]) 
			version[1]=strtol(ptr+1,NULL,10);
	}
	return version;
}

static bool is_ext_supported(const char *extensions, const char *name)
{
	if (extensions && name) {
		const char *found=strstr(extensions, name);
		if (found) {
			// check that the name is actually complete */
			char c = found[strlen(name)];
			if (c == ' ' || c == 0)
				return true;
		}
	}
	return false;
}

enum support_mode {
	NO_SUPPORT=0,
	SUPPORT_CORE=1,
	SUPPORT_EXT=2
};

static support_mode is_supported(const char *extensions, const array<long, 2> &version, const char *name, long major, long minor)
{
	if ( (version[0] > major) || (version[0] == major && version[1] >= minor))
		return SUPPORT_CORE;

	if (is_ext_supported(extensions, name))
		return SUPPORT_EXT;
	return NO_SUPPORT;
}

void ogl_extensions_init()
{
	const char *version_str = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	if (!version_str) {
		con_printf(CON_URGENT, "no valid OpenGL context when querying GL extensions!");
		return;
	}
	const auto version = parse_version_str(version_str);
	const char *extension_str = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

	/* GL_3_2 */
	if (is_supported(NULL, version, NULL, 3, 2) == SUPPORT_CORE) {
		glGetInteger64vFunc = reinterpret_cast<PFNGLGETINTEGER64VPROC>(SDL_GL_GetProcAddress("glGetInteger64v"));
	}
	if (glGetInteger64vFunc) {
		ogl_have_3_2=true;
		con_printf(CON_VERBOSE, "GL 3.2 available");
	} else {
		con_printf(CON_VERBOSE, "GL 3.2 not available");
	}

	/* GL_ARB_sync */
	if (is_supported(extension_str, version, "GL_ARB_sync", 3, 2)) {
		glFenceSyncFunc = reinterpret_cast<PFNGLFENCESYNCPROC>(SDL_GL_GetProcAddress("glFenceSync"));
		glDeleteSyncFunc = reinterpret_cast<PFNGLDELETESYNCPROC>(SDL_GL_GetProcAddress("glDeleteSync"));
		glClientWaitSyncFunc = reinterpret_cast<PFNGLCLIENTWAITSYNCPROC>(SDL_GL_GetProcAddress("glClientWaitSync"));

	}
	if (glFenceSyncFunc && glDeleteSyncFunc && glClientWaitSyncFunc) {
		ogl_have_ARB_sync=true;
		con_printf(CON_VERBOSE, "GL_ARB_sync available");
	} else {
		con_printf(CON_VERBOSE, "GL_ARB_sync not available");
	}

	/* GL_ARB_occlusion_query */
	support_mode mode=is_supported(extension_str, version, "GL_ARB_occlusion_query", 1, 5);
	if (mode == SUPPORT_CORE) {
		/* use the core version of these functions, without ARB suffix */
		glGenQueriesARBFunc = reinterpret_cast<PFNGLGENQUERIESARBPROC>(SDL_GL_GetProcAddress("glGenQueries"));
		glDeleteQueriesARBFunc = reinterpret_cast<PFNGLDELETEQUERIESARBPROC>(SDL_GL_GetProcAddress("glDeleteQueries"));
	} else if (mode == SUPPORT_EXT) {
		/* use the extension version of these functions, with ARB suffix */
		glGenQueriesARBFunc = reinterpret_cast<PFNGLGENQUERIESARBPROC>(SDL_GL_GetProcAddress("glGenQueriesARB"));
		glDeleteQueriesARBFunc = reinterpret_cast<PFNGLDELETEQUERIESARBPROC>(SDL_GL_GetProcAddress("glDeleteQueriesARB"));
	}
	if (glGenQueriesARBFunc && glDeleteQueriesARBFunc) {
		ogl_have_ARB_occlusion_query=true;
		con_printf(CON_VERBOSE, "GL_ARB_occlusion_query available");
	} else {
		con_printf(CON_VERBOSE, "GL_ARB_occlusion_query not available");
	}

	/* GL_ARB_timer_query */
	if (is_supported(extension_str, version, "GL_ARB_timer_query", 3, 3)) {
		glQueryCounterFunc = reinterpret_cast<PFNGLQUERYCOUNTERPROC>(SDL_GL_GetProcAddress("glQueryCounter"));
		glGetQueryObjectui64vFunc = reinterpret_cast<PFNGLGETQUERYOBJECTUI64VPROC>(SDL_GL_GetProcAddress("glGetQueryObjectui64v"));

	}
	if (glQueryCounterFunc && glGetQueryObjectui64vFunc) {
		ogl_have_ARB_timer_query=true;
		con_printf(CON_VERBOSE, "GL_ARB_timer_query available");
	} else {
		con_printf(CON_VERBOSE, "GL_ARB_timer_query not available");
	}

}
