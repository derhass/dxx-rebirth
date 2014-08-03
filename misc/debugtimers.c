#include "debugtimers.h"

#include <time.h>
#include <stdio.h>
#include <SDL/SDL.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include "console.h"
#include "maths.h"
#include "game.h" // for FrameTime

#define DEBUGTIMERS_FRAMES 10

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

typedef uint64_t GLuint64;
typedef int64_t GLint64;

#define GL_QUERY_RESULT                   0x8866
#define GL_TIMESTAMP                      0x8E28

typedef void (APIENTRYP PFNGLGENQUERIESARBPROC) (GLsizei n, GLuint *ids);
typedef void (APIENTRYP PFNGLDELETEQUERIESARBPROC) (GLsizei n, const GLuint *ids);
typedef void (APIENTRYP PFNGLGETQUERYIVARBPROC) (GLenum target, GLenum pname, GLint *params);

typedef void (APIENTRYP PFNGLQUERYCOUNTERPROC) (GLuint id, GLenum target);
typedef void (APIENTRYP PFNGLGETQUERYOBJECTUI64VPROC) (GLuint id, GLenum pname, GLuint64 *params);

typedef void (APIENTRYP PFNGLGETINTEGER64VPROC) (GLenum pname, GLint64 *data);

static PFNGLGENQUERIESARBPROC glGenQueriesARBFunc = NULL;
static PFNGLQUERYCOUNTERPROC glQueryCounterFunc = NULL;
static PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64vFunc = NULL;
static PFNGLGETINTEGER64VPROC glGetInteger64vFunc = NULL;


typedef unsigned long timestamp_t; /* using microseconds */

typedef enum {
	TIMESTAMP_CPU=0,
	TIMESTAMP_GL_GPU,
	TIMESTAMP_GL_CPU,
	TIMESTAMP_COUNT
} ts_type_t;

typedef struct {
	timestamp_t ts[TIMESTAMP_COUNT];
	GLuint gl_query_obj;
} benchpoint_t;

typedef struct {
	benchpoint_t point[DEBUGTIMERS_FRAMES][BENCHPOINT_COUNT];
	unsigned int frame;
	unsigned int pos;
	double factor;
	FILE *f;
} debugtimers_t;

static debugtimers_t debugtimers;

static timestamp_t getcurtime(void)
{
	timestamp_t ts;
#ifdef _WIN32
	/* Windows, Query Performance Counter, CPU clock precision */
	LARGE_INTEGER cnt;
	QueryPerformanceCounter(&cnt);
	ts=(timestamp_t)(((double)cnt.QuadPart)*debugtimers.factor);
#else
	/* Unix with new Posix timers, nanosecond precision */
	struct timespec tsp;
	clock_gettime(CLOCK_REALTIME,&tsp);
	ts=((timestamp_t)tsp.tv_sec) * (timestamp_t)1000000 +
	    (timestamp_t)tsp.tv_nsec/(timestamp_t)1000;	
#endif
	return ts;
}

static void bench_finish(benchpoint_t *b)
{
	int i;
	benchpoint_t *bnext;
	unsigned int pnext;
	timestamp_t ft;

	pnext=debugtimers.pos+1;
	if (pnext >= DEBUGTIMERS_FRAMES) {
		pnext-=DEBUGTIMERS_FRAMES;
	}
	bnext=&debugtimers.point[pnext][0];
	ft=(timestamp_t)( f2fl(FrameTime) * 1000000.0f);

	fprintf(debugtimers.f,"%u\t%lu\t%lu\t%lu",debugtimers.frame - DEBUGTIMERS_FRAMES,
		bnext[BENCHPOINT_START].ts[TIMESTAMP_CPU]-b[BENCHPOINT_START].ts[TIMESTAMP_CPU],
		ft,
		b[BENCHPOINT_END].ts[TIMESTAMP_CPU] - b[BENCHPOINT_START].ts[TIMESTAMP_CPU]);
	for (i=0; i<(int)BENCHPOINT_COUNT; i++) {
		GLuint64 glts;
		if (b[i].gl_query_obj) {
			glGetQueryObjectui64vFunc(b[i].gl_query_obj, GL_QUERY_RESULT, &glts);
			b[i].ts[TIMESTAMP_GL_GPU]=(timestamp_t)(glts/1000);
		} else {
			b[i].ts[TIMESTAMP_GL_GPU]=(timestamp_t)0;
			b[i].ts[TIMESTAMP_GL_CPU]=(timestamp_t)0;
		}
	}
	for (i=1; i<(int)BENCHPOINT_COUNT; i++) {
		fprintf(debugtimers.f,"\t%lu",
			b[i].ts[TIMESTAMP_CPU] - b[i-1].ts[TIMESTAMP_CPU]);
	}
	fprintf(debugtimers.f, "\tGL\t%lu",
		bnext[BENCHPOINT_START].ts[TIMESTAMP_GL_GPU]-b[BENCHPOINT_START].ts[TIMESTAMP_GL_GPU]);
	for (i=1; i<(int)BENCHPOINT_COUNT; i++) {
		fprintf(debugtimers.f,"\t%lu",
			b[i].ts[TIMESTAMP_GL_GPU] - b[i-1].ts[TIMESTAMP_GL_GPU]);

	}
	fprintf(debugtimers.f, "\tlat\t%lu",
		bnext[BENCHPOINT_START].ts[TIMESTAMP_GL_CPU]-b[BENCHPOINT_START].ts[TIMESTAMP_GL_CPU]);

	for (i=0; i<(int)BENCHPOINT_COUNT; i++) {
		fprintf(debugtimers.f,"\t%lu",
			b[i].ts[TIMESTAMP_GL_GPU] - b[i].ts[TIMESTAMP_GL_CPU]);
	}
	fputc('\n', debugtimers.f);
}

extern void bench_init(void)
{
#ifdef _WIN32
	LARGE_INTEGER freq;
#endif
	int i,j,k;

	debugtimers.frame=0;
	debugtimers.pos=0;

	debugtimers.f=fopen("./debugtimers.txt","wt");

	con_printf(CON_URGENT,"Enabling DEBUGTIMERS hack\n");
#ifdef _WIN32
	QueryPerformanceFrequency(&freq);
	debugtimers.factor = 1000000.0 / (double)freq.QuadPart;
#endif

	for (i=0; i<DEBUGTIMERS_FRAMES; i++) {
		for (j=0; j<BENCHPOINT_COUNT; j++) {
			for (k=0;k<TIMESTAMP_COUNT; k++) {
				debugtimers.point[i][j].ts[k]=(timestamp_t)0;
			}
		}
	}
}

extern void bench_init_gl(void)
{
	GLuint ids[DEBUGTIMERS_FRAMES * BENCHPOINT_COUNT];
	int i,j;

	/* get the extension pointers
	 * we only get the bare miniumum we need... */
	glGenQueriesARBFunc = (PFNGLGENQUERIESARBPROC)SDL_GL_GetProcAddress("glGenQueriesARB");
	glQueryCounterFunc = (PFNGLQUERYCOUNTERPROC)SDL_GL_GetProcAddress("glQueryCounter");
	glGetQueryObjectui64vFunc = (PFNGLGETQUERYOBJECTUI64VPROC)SDL_GL_GetProcAddress("glGetQueryObjectui64v");
	glGetInteger64vFunc = (PFNGLGETINTEGER64VPROC)SDL_GL_GetProcAddress("glGetInteger64v");

	if (glGenQueriesARBFunc && glQueryCounterFunc && glGetQueryObjectui64vFunc && glGetInteger64vFunc) {
		con_printf(CON_URGENT,"DEBUGTIMERS: GL timers available :)\n");
		glGenQueriesARBFunc(DEBUGTIMERS_FRAMES * BENCHPOINT_COUNT, ids);
	} else {
		con_printf(CON_URGENT,"DEBUGTIMERS: GL timers not available :(\n");
		for (i=0; i<DEBUGTIMERS_FRAMES*BENCHPOINT_COUNT ; i++) {
			ids[i]=0;
		}
	}
	for (i=0; i<DEBUGTIMERS_FRAMES; i++) {
		for (j=0; j<BENCHPOINT_COUNT; j++) {
			debugtimers.point[i][j].gl_query_obj=ids[i*BENCHPOINT_COUNT+j];
		}
	}
}
	
extern void bench_start_frame(void)
{
	bench_point(BENCHPOINT_START);
}
extern void bench_end_frame(void)
{
	bench_point(BENCHPOINT_END);

	if (++debugtimers.pos >= DEBUGTIMERS_FRAMES) {
		debugtimers.pos=0;
	}
	if (++debugtimers.frame >= DEBUGTIMERS_FRAMES) {
		/* collect the stuff from DEBUGTIMERS_FRAMES ago (which is index "pos") */
		bench_finish(&debugtimers.point[debugtimers.pos][0]);
	}
}

extern void bench_point(benchpoint_desc_t bp)
{
	GLint64 glts;

	benchpoint_t *b=& debugtimers.point[debugtimers.pos][bp];
	b->ts[TIMESTAMP_CPU]=getcurtime();

	if (b->gl_query_obj) {
		glGetInteger64vFunc(GL_TIMESTAMP, &glts);
		b->ts[TIMESTAMP_GL_CPU]=(timestamp_t)(glts/1000);
	
		glQueryCounterFunc(b->gl_query_obj, GL_TIMESTAMP);
	}
}

