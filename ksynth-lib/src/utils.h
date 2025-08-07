#ifndef UITLS_H
#define UTILS_H

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

//struct timespec {
//    time_t tv_sec;  // seconds
//    long   tv_nsec; // nanoseconds
//};

#endif

#define KSYNTH_API __declspec(dllexport)


void init_timer();
void get_time(struct timespec* ts);

#endif