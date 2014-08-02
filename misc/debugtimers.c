#include "debugtimers.h"

#include <time.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include <GL/gl.h>

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
	CRASSERT(initialized);
	QueryPerformanceCounter(&cnt);
	ts=(timestamp_t)(((double)cnt.QuadPart)*factor);
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

	fprintf(debugtimers.f,"%u\t%lu",debugtimers.frame - DEBUGTIMERS_FRAMES,
		b[BENCHPOINT_END].ts[TIMESTAMP_CPU] - b[BENCHPOINT_START].ts[TIMESTAMP_CPU]);
	for (i=0; i<(int)BENCHPOINT_COUNT; i++) {
		GLuint64 glts;
		glGetQueryObjectui64vFunc(b[i].gl_query_obj, GL_QUERY_RESULT, &glts);
		b[i].ts[TIMESTAMP_GL_GPU]=(timestamp_t)(glts/1000);
	}
	for (i=1; i<(int)BENCHPOINT_COUNT; i++) {
		fprintf(debugtimers.f,"\t%lu",
			b[i].ts[TIMESTAMP_CPU] - b[i-1].ts[TIMESTAMP_CPU]);

	}
	fprintf(debugtimers.f, "\tGL");
	for (i=1; i<(int)BENCHPOINT_COUNT; i++) {
		fprintf(debugtimers.f,"\t%lu",
			b[i].ts[TIMESTAMP_GL_GPU] - b[i-1].ts[TIMESTAMP_GL_GPU]);

	}
	fprintf(debugtimers.f, "\tlat");
	for (i=0; i<(int)BENCHPOINT_COUNT; i++) {
		fprintf(debugtimers.f,"\t%lu",
			b[i].ts[TIMESTAMP_GL_GPU] - b[i].ts[TIMESTAMP_GL_CPU]);

	}
	fprintf(debugtimers.f,"\n");
}

extern void bench_init(void)
{
#ifdef _WIN32
	LARGE_INTEGER freq;
#endif

	debugtimers.frame=0;
	debugtimers.pos=0;

	debugtimers.f=fopen("./debugtimers.txt","wt");

#ifdef _WIN32
	QueryPerformanceFrequency(&freq);
	debugtimers.factor = 1000000.0 / (double)freq.QuadPart;
#endif
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

	glGenQueriesARBFunc(DEBUGTIMERS_FRAMES * BENCHPOINT_COUNT, ids);
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

	glGetInteger64vFunc(GL_TIMESTAMP, &glts);
	b->ts[TIMESTAMP_GL_CPU]=(timestamp_t)(glts/1000);
	
	glQueryCounterFunc(b->gl_query_obj, GL_TIMESTAMP);
}

