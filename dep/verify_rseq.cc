#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <concurrency/rseq/rseq_base.h>
#include <misc/error_handling.h>

const uint32_t retries = 10;

int
main() {
    DIE_ASSERT(rseq_refcount == 0,
               "Error: Invalid refcount for verifying rseq\n");
    init_thread();

    uint32_t i;
    for (i = 0; rseq_refcount == 0 && i < retries; ++i) {
        usleep(1000);
        init_thread();
    }
    DIE_ASSERT(rseq_refcount != 0,
               "Error: unable to intialize rseq after: %d attempts\n",
               retries);
    DIE_ASSERT(i < retries, "Error: impossible condition met\n");

    DIE_ASSERT(
        get_cur_cpu() <= sysconf(_SC_NPROCESSORS_ONLN),
        "Error: Invalid cpu returned after initialization\n");

    for (i = 0; i < sysconf(_SC_NPROCESSORS_ONLN); ++i) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(i, &set);
        ERROR_ASSERT(sched_setaffinity(0, sizeof(cpu_set_t), &set) == 0,
                     "Error: Unable to set cpu\n");
        uint32_t timeout = 1000;
        while ((uint32_t)sched_getcpu() != i && --timeout) {
            usleep(1000);
        }
        DIE_ASSERT(timeout, "Error: process not migrating to set core\n");
        DIE_ASSERT(get_start_cpu() == get_cur_cpu(),
                   "Error: start cpu != cur_cpu\n");
        DIE_ASSERT(get_start_cpu() == i,
                   "Error: rseq returning incorrect cpu\n");
    }

    return 0;
}
