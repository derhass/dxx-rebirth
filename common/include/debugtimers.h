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

#pragma once

#ifdef USE_DEBUGTIMERS

#include <string>

typedef enum {
	BENCHPOINT_START,
	BENCHPOINT_TIMER_UPDATE,
	BENCHPOINT_EVENT_POLL,
	BENCHPOINT_CALC_TIME,
	BENCHPOINT_PROCESS,
	BENCHPOINT_RENDER,
	BENCHPOINT_COCKPIT,
	BENCHPOINT_GAUGES,
	BENCHPOINT_HUD,
	BENCHPOINT_FRAME,
	BENCHPOINT_END,
	BENCHPOINT_COUNT
} benchpoint_desc_t;

extern void bench_init(const std::string& filename, int size);
extern void bench_init_gl();
extern void bench_close_gl();
extern void bench_close();
extern void bench_flush();
extern void bench_start_frame();
extern void bench_end_frame();
extern void bench_point(benchpoint_desc_t bp);

#define BENCH_INIT(x,y) bench_init(x,y)
#define BENCH_INIT_GL(x) bench_init_gl()
#define BENCH_CLOSE_GL(x) bench_close_gl()
#define BENCH_CLOSE(x) bench_close()
#define BENCH_FLUSH(x) bench_flush()
#define BENCH_START_FRAME(x) bench_start_frame()
#define BENCH_END_FRAME(x) bench_end_frame()
#define BENCH_POINT(x) bench_point(x)

#else // USE_DEBUGTIMERS

/* USE_DEBUGTIMERS not set, define macros which do nothing */
#define BENCH_INIT(x,y) ((void)0)
#define BENCH_INIT_GL(x) ((void)0)
#define BENCH_CLOSE_GL(x) ((void)0)
#define BENCH_CLOSE(x) ((void)0)
#define BENCH_FLUSH(x) ((void)0)
#define BENCH_START_FRAME(x) ((void)0)
#define BENCH_END_FRAME(x) ((void)0)
#define BENCH_POINT(x) ((void)0)

#endif // USE_DEBUGTIMERS

