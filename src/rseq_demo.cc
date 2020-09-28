#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x86intrin.h>
#include <pthread.h>

#include <concurrency/rseq/rseq_base.h>
#include <timing/thread_helper.h>

void
rseq_add(uint64_t * arr) {

    uint64_t cpu;
    
    // clang-format off
    asm volatile(
        
        RSEQ_INFO_DEF(32)
        RSEQ_CS_ARR_DEF() 

        // start critical section
        "1:\n\t"
        RSEQ_PREP_CS_DEF(%[cpu])

        "movl %%fs:__rseq_abi@tpoff+4, %k[cpu]\n\t"
        // cpu *= 128
        "salq $7, %[cpu]\n\t"
        
        "addq $1, (%[arr], %[cpu], 1)\n\t"

        // end critical section
        "2:\n\t"

        RSEQ_START_ABORT_DEF()
        "jmp 1b\n\t"
        RSEQ_END_ABORT_DEF()
        
        : [ cpu ] "=&r" (cpu),
          [ arr_clobber ] "=m" (*arr)
        : [ arr ] "r" (arr)
        : "cc" );
    // clang-format on
}

pthread_barrier_t b;
uint64_t          total;
uint64_t          cycles;

#define NTHREAD 32
#define TEST_SIZE 100000UL
#define FUN atomic_adder

void *
rseq_adder(void * targ) {
    init_thread();
    total  = 0;
    cycles = 0;

    uint64_t * arr = (uint64_t *)targ;
    pthread_barrier_wait(&b);

    
    uint64_t start = _rdtsc();
    for (uint64_t i = 0; i < TEST_SIZE; ++i) {
        rseq_add(arr);
    }
    uint64_t end = _rdtsc();

    __atomic_fetch_add(&cycles, end - start, __ATOMIC_RELAXED);
    return NULL;
}


void *
atomic_adder(void * targ) {
    init_thread();
    total  = 0;
    cycles = 0;

    uint64_t * arr = (uint64_t *)targ;
    pthread_barrier_wait(&b);


    uint64_t start = _rdtsc();
    for (uint64_t i = 0; i < TEST_SIZE; ++i) {
        uint32_t start_cpu = get_start_cpu();
        __atomic_fetch_add(arr + 16 * start_cpu, 1, __ATOMIC_RELAXED);
    }
    uint64_t end = _rdtsc();

    __atomic_fetch_add(&cycles, end - start, __ATOMIC_RELAXED);
    return NULL;
}


int
main() {
    uint64_t * arr = (uint64_t *)calloc(128, 8);

    thelp::thelper th;

    pthread_barrier_init(&b, NULL, NTHREAD);
    th.spawn_n(NTHREAD,
               FUN,
               thelp::pin_policy::FIRST_N,
               arr,
               0);
    th.join_all();

    for(uint32_t i = 0; i < 128; ++i) {
        total +=arr[i];
    }

    double t = cycles;
    t /= (NTHREAD * TEST_SIZE);
    fprintf(stderr,
            "Correct    : %lu == %lu\n"
            "Cycles     : %.3lf\n",
            total, NTHREAD * TEST_SIZE,
            t);
            
}
