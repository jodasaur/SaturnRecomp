#include "host_time.h"
#include <assert.h>
#include <stdio.h>
#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
static volatile sig_atomic_t interrupted;
static void on_alarm(int signal_number)
{
    (void)signal_number;
    interrupted = 1;
}
#endif

int main(void)
{
    uint64_t start = host_monotonic_ns(), previous = start;
    for (unsigned i = 0; i < 10000; i++) {
        uint64_t now = host_monotonic_ns();
        assert(now >= previous);
        previous = now;
    }
    host_sleep_ms(0);
    start = host_monotonic_ns();
    host_sleep_ms(20);
#ifdef _WIN32
    /* Sleep rounds to the system timer interval. */
    assert(host_monotonic_ns() - start >= UINT64_C(10000000));
#else
    assert(host_monotonic_ns() - start >= UINT64_C(20000000));
#endif
#ifndef _WIN32
    {
        struct sigaction action = {0}, old_action;
        action.sa_handler = on_alarm;
        sigemptyset(&action.sa_mask);
        assert(sigaction(SIGALRM, &action, &old_action) == 0);
        alarm(1);
        start = host_monotonic_ns();
        host_sleep_ms(1100);
        assert(interrupted);
        assert(host_monotonic_ns() - start >= UINT64_C(1100000000));
        alarm(0);
        assert(sigaction(SIGALRM, &old_action, NULL) == 0);
    }
#endif
    puts("host_time: monotonic nanoseconds and sleep passed");
    return 0;
}
