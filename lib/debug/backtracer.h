#ifndef _BACKTRACER_H_
#define _BACKTRACER_H_

#include <misc/cpp_attributes.h>
#include <misc/error_handling.h>

#include <system/signals.h>
#include <system/sys_info.h>

#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define __FN__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#ifdef DBG_MODE
#define DBG_PUSH_FRAME(bytes)                                                  \
    if (tframes) {                                                             \
        tframes->push_frame(__LINE__,                                          \
                            __rseq_abi.cpu_id_start,                           \
                            (uint64_t)(bytes),                                 \
                            __FN__);                                           \
    }

#define DBG_REGISTER() all_frames.register_thread()
#define DBG_INIT()     all_frames.init()
#else
#define DBG_PUSH_FRAME(bytes)
#define DBG_REGISTER()
#define DBG_INIT()
#endif


struct frame {
    uint64_t timestamp;
    uint64_t bytes;
    uint32_t core;
    uint32_t ln;
    char     fn[16];

    void
    to_string(uint64_t i, uint64_t tid, char * strout) {
        sprintf(strout,
                "[%010lu] -- %04lu -- [%012lu][%02d] %12.12s:%04d [ %016lx ]",
                tid,
                i,
                timestamp,
                core,
                fn,
                ln,
                bytes);
    }
};


template<uint64_t n = 128>
struct thread_frames {
    uint64_t idx;
    uint64_t tid;
    frame    frames[n];


    void ALWAYS_INLINE
    push_frame(const uint32_t     ln,
               const uint32_t     core,
               const uint64_t     bytes,
               const char * const fn) {

        uint32_t f_idx      = (idx++) % n;
        frames[f_idx].core  = core;
        frames[f_idx].ln    = ln;
        frames[f_idx].bytes = bytes;
        memcpy(frames[f_idx].fn, fn, std::min<uint64_t>(strlen(fn), 15));
        frames[f_idx].timestamp = _rdtsc();
    }

    void
    show_frames() {
        const uint32_t buflen = 128;
        char           outbuf[buflen];
        if (idx < n) {
            for (uint64_t i = 0; i < idx; ++i) {
                frames[i].to_string(i, tid, outbuf);
                fprintf(stderr, "%s\n", outbuf);
            }
        }
        else {
            for (uint64_t i = 1; i <= n; ++i) {
                frames[(i + idx) % n].to_string(i - 1, tid, outbuf);
                fprintf(stderr, "%s\n", outbuf);
            }
        }
    }
} L2_LOAD_ALIGN;

#ifdef DBG_MODE
__thread thread_frames<128> * tframes;
#endif

void show_and_exit(int signum);
template<uint64_t n = 128>
struct dbg_tracer {
    uint64_t           tidx;
    uint64_t           hard_lock;
    thread_frames<n> * frames;

    void
    init() {
        // over using memory but w.e
        frames = (thread_frames<n> *)calloc(1024, sizeof(thread_frames<n>));
        ERROR_ASSERT(frames != NULL);

        tidx      = 0;
        hard_lock = 0;
        install_signal(SIGINT, show_and_exit);
    }

    void
    register_thread() {
        uint64_t start = __atomic_fetch_add(&tidx, 1, __ATOMIC_RELAXED);
        DIE_ASSERT(start < 1024, "Error: out of debug memory\n");
#ifdef DBG_MODE
        tframes      = frames + start;
        tframes->tid = pthread_self();
#endif
    }

    void
    show() {
        thread_frames<n> * cur_frame;
        for (uint64_t i = 0; i < tidx; ++i) {
            cur_frame = frames + i;
            cur_frame->show_frames();
        }
    }
};

#ifdef DBG_MODE
dbg_tracer<128> all_frames;
#endif


void
show_and_exit(int signum) {
    fprintf(stderr, "Handling Signal(%d): %lx\n\n", signum, pthread_self());
#ifdef DBG_MODE
    all_frames.show();
#endif
    exit(-1);
    (void)(signum);
}

#endif
