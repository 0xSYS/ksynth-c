#ifdef _WIN32
#include <windows.h>
#include <time.h>

#include "utils.h"

static LARGE_INTEGER rendering_time_start, rendering_time_end;
static double rendering_time_freq;



void init_timer()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    rendering_time_freq = (double)freq.QuadPart;
}

void get_time(struct timespec* ts)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    ts->tv_sec = (time_t)(now.QuadPart / rendering_time_freq);
    ts->tv_nsec = (long)(((now.QuadPart % (LONGLONG)rendering_time_freq) * 1e9) / rendering_time_freq);
}
#else
#include <time.h>

#define init_timer() ((void)0)
#define get_time(ts) clock_gettime(CLOCK_MONOTONIC, (ts))
#endif