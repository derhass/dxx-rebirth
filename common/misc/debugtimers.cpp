/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/* DEBUGTIMERS functionality is only enabled if USE_DEBUGTIMERS is defined.
 * This will insert timing probes into certain points of the render loop,
 * and will write the data to a logfile. This is meant for debugging
 * performance issues. 
 *
 * The timing probes will also use GL_ARB_timer_query (if available)
 * to get precise GPU timings.
 */

#ifdef USE_DEBUGTIMERS

#include "debugtimers.h"

#include <time.h>
#include <stdio.h>
#include <SDL/SDL.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef OGL
#include "ogl_extensions.h"
#endif

#include "console.h"
#include "maths.h"
#include "game.h" // for FrameTime

#include <vector>

#define DEBUGTIMERS_FRAMES 10

typedef unsigned long timestamp_t; /* using microseconds */

typedef enum {
	TIMESTAMP_CPU=0,
	TIMESTAMP_GL_GPU,
	TIMESTAMP_GL_CPU,
	TIMESTAMP_COUNT
} ts_type_t;

typedef struct {
	timestamp_t ts[TIMESTAMP_COUNT];
#ifdef OGL
	GLuint gl_query_obj;
#endif
} benchpoint_t;

#define DEBUGTIMERS_FRAMESTAT_GBL 3
#define DEBUGTIMERS_FRAMESTAT_CPU (BENCHPOINT_COUNT)
#define DEBUGTIMERS_FRAMESTAT_GL  (BENCHPOINT_COUNT)
#define DEBUGTIMERS_FRAMESTAT_LAT (BENCHPOINT_COUNT)
#define DEBUGTIMERS_FRAMESTAT_TOTAL (DEBUGTIMERS_FRAMESTAT_GBL + DEBUGTIMERS_FRAMESTAT_CPU + DEBUGTIMERS_FRAMESTAT_GL + DEBUGTIMERS_FRAMESTAT_LAT)
typedef struct {
	timestamp_t ts[DEBUGTIMERS_FRAMESTAT_TOTAL];
} framestat_t;

typedef struct {
	int enabled;
	benchpoint_t point[DEBUGTIMERS_FRAMES][BENCHPOINT_COUNT];
	double factor;
	FILE *f;
	std::vector<framestat_t> buffer;
	unsigned int frame;
	unsigned int pos;
	unsigned int buf_size;
	unsigned int buf_pos;
} debugtimers_t;

static debugtimers_t debugtimers;

static const char *point_name[BENCHPOINT_COUNT]={
	"next",
	"tupd",
	"evnt",
	"calt",
	"proc",
	"rndM",
	"rndC",
	"rndG",
	"rndH",
	"endf",
	"flip"
};

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

extern void
bench_flush()
{
	unsigned int j;
	static int hdr=1;
	int i;

	if (!debugtimers.f) {
		return;
	}

	if (hdr) {
		fprintf(debugtimers.f,"DEBUGTIMERS-V2, ffreq: %f\n",1.0/debugtimers.factor);
		fprintf(debugtimers.f, "frame\tFT\ttotLO");
		for (j=0; j<3; j++) {
			for (int i=0; i<BENCHPOINT_COUNT; i++) {
				char xhdr[3]={'C','G','L'};
				fprintf(debugtimers.f, "\t%c%s",xhdr[j],point_name[i]);
			}
		}
		fputc('\n', debugtimers.f);
		hdr=0;
	}

	con_printf(CON_VERBOSE,"DEBUGTIMERS: writing stats for %u frames",debugtimers.buf_pos);
	for (j=0; j<debugtimers.buf_pos; j++) {
		const framestat_t *fs=&debugtimers.buffer[j];

		for (i=0; i< DEBUGTIMERS_FRAMESTAT_TOTAL; i++) {
			fprintf(debugtimers.f, "%lu\t",fs->ts[i]);
		}
		fputc('\n', debugtimers.f);
	}
	debugtimers.buf_pos=0;
}

static void bench_finish(benchpoint_t *b)
{
	int i;
	benchpoint_t *bnext;
	unsigned int pnext;
	timestamp_t ft;
	framestat_t *fs;

	pnext=debugtimers.pos+1;
	if (pnext >= DEBUGTIMERS_FRAMES) {
		pnext-=DEBUGTIMERS_FRAMES;
	}
	bnext=&debugtimers.point[pnext][0];
	ft=(timestamp_t)( f2fl(FrameTime) * 1000000.0f);

#ifdef OGL
	/* collect the GL timer query */
	GLuint64 glts;
	for (i=0; i<(int)BENCHPOINT_COUNT; i++) {
		if (b[i].gl_query_obj) {
			glGetQueryObjectui64vFunc(b[i].gl_query_obj, GL_QUERY_RESULT, &glts);
			b[i].ts[TIMESTAMP_GL_GPU]=(timestamp_t)(glts/1000);
		} else {
			b[i].ts[TIMESTAMP_GL_GPU]=(timestamp_t)0;
			b[i].ts[TIMESTAMP_GL_CPU]=(timestamp_t)0;
		}
	}
	/* also collect the first one of the next frame */
	if (bnext[BENCHPOINT_START].gl_query_obj) {
		glGetQueryObjectui64vFunc(bnext[BENCHPOINT_START].gl_query_obj, GL_QUERY_RESULT, &glts);
		bnext[BENCHPOINT_START].ts[TIMESTAMP_GL_GPU]=(timestamp_t)(glts/1000);
	} else {
		bnext[BENCHPOINT_START].ts[TIMESTAMP_GL_GPU]=(timestamp_t)0;
		bnext[BENCHPOINT_START].ts[TIMESTAMP_GL_CPU]=(timestamp_t)0;
	}
#else // OGL
	for (i=0; i<(int)BENCHPOINT_COUNT; i++) {
		b[i].ts[TIMESTAMP_GL_GPU]=(timestamp_t)0;
		b[i].ts[TIMESTAMP_GL_CPU]=(timestamp_t)0;
	}
	/* also collect the first one of the next frame */
	bnext[BENCHPOINT_START].ts[TIMESTAMP_GL_GPU]=(timestamp_t)0;
	bnext[BENCHPOINT_START].ts[TIMESTAMP_GL_CPU]=(timestamp_t)0;
#endif // OGL

	/* dump the times to the buffer */
	fs=&debugtimers.buffer[debugtimers.buf_pos];

	/* GBL PART */
	fs->ts[0]=(timestamp_t) (debugtimers.frame - DEBUGTIMERS_FRAMES);
	fs->ts[1]=ft;
	fs->ts[2]=b[BENCHPOINT_END].ts[TIMESTAMP_CPU] - b[BENCHPOINT_START].ts[TIMESTAMP_CPU];

	/* headers for each part: the offset to the next frame as global reference */
	fs->ts[DEBUGTIMERS_FRAMESTAT_GBL] =
		bnext[BENCHPOINT_START].ts[TIMESTAMP_CPU]-b[BENCHPOINT_START].ts[TIMESTAMP_CPU];

	fs->ts[DEBUGTIMERS_FRAMESTAT_GBL + DEBUGTIMERS_FRAMESTAT_CPU] =
		bnext[BENCHPOINT_START].ts[TIMESTAMP_GL_GPU]-b[BENCHPOINT_START].ts[TIMESTAMP_GL_GPU];

	fs->ts[DEBUGTIMERS_FRAMESTAT_GBL + DEBUGTIMERS_FRAMESTAT_CPU + DEBUGTIMERS_FRAMESTAT_GL] =
		bnext[BENCHPOINT_START].ts[TIMESTAMP_GL_CPU]-b[BENCHPOINT_START].ts[TIMESTAMP_GL_CPU];

	/* the idividual values per part */
	for (i=1; i<(int)BENCHPOINT_COUNT; i++) {
		fs->ts[DEBUGTIMERS_FRAMESTAT_GBL + i ] = b[i].ts[TIMESTAMP_CPU] - b[i-1].ts[TIMESTAMP_CPU];
		fs->ts[DEBUGTIMERS_FRAMESTAT_GBL + DEBUGTIMERS_FRAMESTAT_CPU + i] =
		       	b[i].ts[TIMESTAMP_GL_CPU] - b[i-1].ts[TIMESTAMP_GL_CPU];
		fs->ts[DEBUGTIMERS_FRAMESTAT_GBL + DEBUGTIMERS_FRAMESTAT_CPU + DEBUGTIMERS_FRAMESTAT_GL + i] =
			b[i].ts[TIMESTAMP_GL_GPU] - b[i].ts[TIMESTAMP_GL_CPU];
	}

	if (++debugtimers.buf_pos >= debugtimers.buf_size) {
		/* dump it to the file */
		bench_flush();
	}
}

extern void bench_init(const std::string& filename, int size)
{
	int i,j,k;

	debugtimers.enabled=0;
	debugtimers.frame=0;
	debugtimers.pos=0;
	debugtimers.f=NULL;
	debugtimers.buf_size=0;

	if (filename.empty() || size < 1) {
		return;
	}

	debugtimers.enabled=1;
	debugtimers.buf_size=static_cast<unsigned int>(size);
	debugtimers.f=fopen(filename.c_str(),"wt");

	con_printf(CON_NORMAL,"enabling DEBUGTIMERS, writing to '%s', blocksize: %u", filename.c_str(), debugtimers.buf_size);
	debugtimers.factor = -1.0;
#ifdef _WIN32
	LARGE_INTEGER freq;
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

	debugtimers.buffer.resize(debugtimers.buf_size);
	debugtimers.buf_pos=0;
}

extern void bench_init_gl()
{
#ifdef OGL
	if (!debugtimers.enabled)
		return;

	GLuint ids[DEBUGTIMERS_FRAMES * BENCHPOINT_COUNT];
	int i,j;


	if (ogl_have_ARB_timer_query) {
		con_printf(CON_VERBOSE,"DEBUGTIMERS: GL timers available :)");
		glGenQueriesARBFunc(DEBUGTIMERS_FRAMES * BENCHPOINT_COUNT, ids);
	} else {
		con_printf(CON_URGENT,"DEBUGTIMERS: GL timers not available :(");
		for (i=0; i<DEBUGTIMERS_FRAMES*BENCHPOINT_COUNT ; i++) {
			ids[i]=0;
		}
	}
	for (i=0; i<DEBUGTIMERS_FRAMES; i++) {
		for (j=0; j<BENCHPOINT_COUNT; j++) {
			debugtimers.point[i][j].gl_query_obj=ids[i*BENCHPOINT_COUNT+j];
		}
	}
#endif
}

extern void bench_close_gl()
{
#ifdef OGL
	if (!debugtimers.enabled)
		return;

	for (int i=0; i<DEBUGTIMERS_FRAMES; i++) {
		for (int j=0; j<BENCHPOINT_COUNT; j++) {
			if (debugtimers.point[i][j].gl_query_obj) {
				glDeleteQueriesARBFunc(1, &debugtimers.point[i][j].gl_query_obj);
				debugtimers.point[i][j].gl_query_obj=0;
			}
		}
	}
#endif

}

extern void bench_close()
{
	if (!debugtimers.enabled)
		return;

	bench_flush();
	if (debugtimers.f) {
		fclose(debugtimers.f);
		debugtimers.f=NULL;
	}
	debugtimers.buffer.resize(0);
	debugtimers.enabled=0;
}
	
extern void bench_start_frame()
{
	if (!debugtimers.enabled)
		return;

	bench_point(BENCHPOINT_START);
}
extern void bench_end_frame()
{
	if (!debugtimers.enabled)
		return;

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
	if (!debugtimers.enabled)
		return;

	benchpoint_t *b=& debugtimers.point[debugtimers.pos][bp];
	b->ts[TIMESTAMP_CPU]=getcurtime();

#ifdef OGL
	GLint64 glts;
	if (b->gl_query_obj) {
		glGetInteger64vFunc(GL_TIMESTAMP, &glts);
		b->ts[TIMESTAMP_GL_CPU]=(timestamp_t)(glts/1000);
	
		glQueryCounterFunc(b->gl_query_obj, GL_TIMESTAMP);
	}
#endif // OGL
}

#endif // USE_DEBUGTIMERS

