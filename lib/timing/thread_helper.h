#ifndef _THREAD_HELPER_H_
#define _THREAD_HELPER_H_

#include <system/sys_info.h>
#include <misc/error_handling.h>

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

namespace thelp {

typedef void * (*tfunc)(void *);

enum pin_policy { NONE = 0, FIRST_N = 1, RR = 2 };

struct thelper {
    static constexpr const uint32_t stack_size = (1 << 20);
    
    uint32_t    cur_nthreads;
    uint32_t    cap_nthreads;
    pthread_t * tids;

    thelper() {
        cur_nthreads = 0;
        cap_nthreads = 0;
        tids = NULL;
    }
    ~thelper() {
        if(tids) {
            free(tids);
        }
    }

    void spawn_n(uint32_t    nthreads,
                 tfunc       f,
                 pin_policy  pp         = pin_policy::NONE,
                 void *      data_start = NULL,
                 uint64_t    data_incr  = 0);
    void join_all() const;
};

}  // namespace thelp

#endif
