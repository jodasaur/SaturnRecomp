/* Host diagnostics and throttling only; never use these for guest clocks.
 * Header-only to preserve the existing standalone PowerShell test builds.
 * POSIX callers must build with _POSIX_C_SOURCE=200809L. */
#ifndef SATURN_HOST_TIME_H
#define SATURN_HOST_TIME_H

#include <stdint.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <time.h>
#endif

static inline uint64_t host_monotonic_ns(void)
{
#ifdef _WIN32
    LARGE_INTEGER count, frequency;
    uint64_t ticks, hz;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0 ||
        !QueryPerformanceCounter(&count)) abort();
    ticks = (uint64_t)count.QuadPart;
    hz = (uint64_t)frequency.QuadPart;
    /* Split whole seconds to avoid overflowing the long-running counter.
     * Floating point is confined to the fractional second. */
    return (ticks / hz) * UINT64_C(1000000000) +
           (uint64_t)((double)(ticks % hz) * 1000000000.0 / (double)hz);
#else
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) abort();
    return (uint64_t)now.tv_sec * UINT64_C(1000000000) + (uint64_t)now.tv_nsec;
#endif
}

static inline void host_sleep_ms(unsigned milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    struct timespec remaining = {
        (time_t)(milliseconds / 1000u),
        (long)(milliseconds % 1000u) * 1000000L
    };
    while (nanosleep(&remaining, &remaining) != 0) {
        if (errno != EINTR) abort();
    }
#endif
}

#endif
