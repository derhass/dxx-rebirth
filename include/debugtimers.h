#ifndef _DEBUGTIMERS_H
#define _DEBUGTIMERS_H

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

extern void bench_init(void);
extern void bench_init_gl(void);
extern void bench_start_frame(void);
extern void bench_end_frame(void);
extern void bench_point(benchpoint_desc_t bp);

#define USE_DEBUGTIMERS

#ifdef USE_DEBUGTIMERS

#define BENCH_INIT(x) bench_init()
#define BENCH_INIT_GL(x) bench_init_gl()
#define BENCH_START_FRAME(x) bench_start_frame()
#define BENCH_END_FRAME(x) bench_end_frame()
#define BENCH_POINT(x) bench_point(x)

#else

#define BENCH_INIT(x) ((void)0)
#define BENCH_INIT_GL(x) ((void)0)
#define BENCH_START_FRAME(x) ((void)0)
#define BENCH_END_FRAME(x) ((void)0)
#define BENCH_POINT(x) ((void)0)

#endif

#endif /* !_DEBUGTIMERS_H */
