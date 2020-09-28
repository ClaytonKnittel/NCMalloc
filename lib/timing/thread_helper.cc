#include <timing/thread_helper.h>
#include <util/verbosity.h>


namespace thelp {


void
thelper::spawn_n(uint32_t   nthreads,
                 tfunc      f,
                 pin_policy pp,
                 void *     data_start,
                 uint64_t   data_incr) {

    cur_nthreads = nthreads;
    if (nthreads > cap_nthreads) {
        if (tids != NULL) {
            free(tids);
        }
        tids         = (pthread_t *)calloc(nthreads, sizeof(pthread_t));
        ERROR_ASSERT(tids != NULL);
        cap_nthreads = nthreads;
    }

    pthread_attr_t attr;
    ERROR_ASSERT(!pthread_attr_init(&attr));

    // this is arbitrary. generally large enough and decreases thread spawn
    // overhead DRASTICALLY
    ERROR_ASSERT(!pthread_attr_setstacksize(&attr, stack_size));

    // this is relatively expensive but I guess the price you pay for generic
    // programming. Could replace cpu_set_t with a uint64_t but this is a mile
    // from the critical path so nbd imo
    cpu_set_t full_cpu_set;
    CPU_ZERO(&full_cpu_set);
    for (uint32_t i = 0; i < NPROCS; ++i) {
        CPU_SET(i, &full_cpu_set);
    }

    cpu_set_t cset;
    CPU_ZERO(&cset);


    for (uint32_t i = 0; i < nthreads; ++i) {
        // there is probably a more efficient way of doing the cpu pinning, but
        // again its not that important to find...
        if (pp == pin_policy::FIRST_N) {
            if (i < NPROCS) {
                CPU_SET(i, &cset);
                ERROR_ASSERT(!pthread_attr_setaffinity_np(&attr,
                                                          sizeof(cpu_set_t),
                                                          &cset));
                CPU_CLR(i, &cset);
            }
            else {
                ERROR_ASSERT(!pthread_attr_setaffinity_np(&attr,
                                                          sizeof(cpu_set_t),
                                                          &full_cpu_set));
            }
        }
        else if (pp == pin_policy::RR) {
            CPU_SET(i % NPROCS, &cset);
            ERROR_ASSERT(
                !pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cset));
            CPU_CLR(i % NPROCS, &cset);
        }
        else {
            ERROR_ASSERT(!pthread_attr_setaffinity_np(&attr,
                                                      sizeof(cpu_set_t),
                                                      &full_cpu_set));
        }

        ERROR_ASSERT(
            !pthread_create(tids + i,
                            &attr,
                            f,
                            (void *)((uint64_t)data_start + i * data_incr)),
            "Error creating thread: %d\n",
            i);
    }

    ERROR_ASSERT(!pthread_attr_destroy(&attr));
}

void
thelper::join_all() const {
    if (cur_nthreads == 0) {
        errv_print("Trying to join 0 threads\n");
        return;
    }

    for (uint32_t i = 0; i < cur_nthreads; ++i) {
        ERROR_ASSERT(!pthread_join(tids[i], NULL));
    }
}

}  // namespace thelp
