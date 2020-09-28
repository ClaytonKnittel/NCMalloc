#ifndef _SIGNALS_H_
#define _SIGNALS_H_

#include <misc/error_handling.h>
#include <signal.h>

typedef void (signal_func(int));

void
install_signal(int32_t signum, signal_func f) {
    struct sigaction sa;

    sa.sa_handler = f;
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags = 0;
    ERROR_ASSERT(sigaction(signum, &sa, NULL) == 0);
}

void
reset_signal(int32_t signum) {
    struct sigaction sa;

    sa.sa_handler = SIG_DFL;
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags = 0;
    ERROR_ASSERT(sigaction(signum, &sa, NULL) == 0);
}


void
ignore_signal(int32_t signum) {
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;
    sigemptyset(&(sa.sa_mask));
    sa.sa_flags = 0;
    ERROR_ASSERT(sigaction(signum, &sa, NULL) == 0);
}



#endif
