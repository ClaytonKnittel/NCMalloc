#include <util/arg.h>
#include <util/verbosity.h>

uint32_t run_mask  = 0xf;
uint64_t test_size = (1 << 20);
uint64_t nthread   = (32);
uint64_t nbytes    = (26);

#include <concurrency/rseq/rseq_base.h>
#include <optimized/const_math.h>
#include <timing/thread_helper.h>
#include <timing/timers.h>

#include <allocator/object_allocator.h>

#include <time.h>
#include <unordered_set>


typedef struct nbytes {
    uint64_t padding[2];
} nbytes_t;

using test_t      = nbytes_t;
using allocator_t = alloc::object_allocator<>;

uint64_t          sum        = 0;
uint64_t          total_nsec = 0;
pthread_barrier_t b;


typedef struct targs {
    allocator_t * allocator;
    test_t **     ptrs;
} targs_t;

void *
alloc_only(void * targ) {
    total_nsec = 0;
    sum        = 0;

    init_thread();
    allocator_t * allocator = ((targs_t *)targ)->allocator;
    test_t **     ptrs      = ((targs_t *)targ)->ptrs;

    const uint32_t _test_size = test_size;

    struct timespec start_ts, end_ts;

    uint64_t _sum = 0;
    pthread_barrier_wait(&b);


    timers::gettime(timers::ELAPSE, &start_ts);

    for (uint32_t i = 0; i < _test_size; ++i) {
        test_t * ptr = (test_t *)allocator->_allocate(sizeof(test_t));
        allocator->_valid_addr(ptr);
        if (ptr) {
            memset(ptr, _sum, sizeof(nbytes_t));
            ptrs[_sum] = ptr;
            ++_sum;
        }
    }

    timers::gettime(timers::ELAPSE, &end_ts);


    __atomic_fetch_add(&total_nsec,
                       timers::ts_to_ns(&end_ts) - timers::ts_to_ns(&start_ts),
                       __ATOMIC_RELAXED);

    __atomic_fetch_add(&sum, _sum, __ATOMIC_RELAXED);


    (void)(allocator);
    return NULL;
}


void *
alloc_free(void * targ) {
    total_nsec = 0;
    sum        = 0;

    init_thread();
    allocator_t * allocator = (allocator_t *)targ;

    const uint32_t _test_size = test_size;

    struct timespec start_ts, end_ts;

    uint64_t _sum = 0;
    pthread_barrier_wait(&b);

    timers::gettime(timers::ELAPSE, &start_ts);
    for (uint32_t i = 0; i < _test_size; ++i) {
        test_t * ptr = (test_t *)allocator->_allocate(sizeof(test_t));
        allocator->_valid_addr(ptr);
        if (ptr) {
            memset(ptr, i, sizeof(nbytes_t));
            allocator->_free(ptr);
            ++_sum;
        }
    }
    timers::gettime(timers::ELAPSE, &end_ts);


    __atomic_fetch_add(&total_nsec,
                       timers::ts_to_ns(&end_ts) - timers::ts_to_ns(&start_ts),
                       __ATOMIC_RELAXED);

    __atomic_fetch_add(&sum, _sum, __ATOMIC_RELAXED);


    (void)(allocator);
    return NULL;
}

void *
alloc_free_alloc(void * targ) {
    total_nsec = 0;
    sum        = 0;

    init_thread();
    allocator_t * allocator = ((targs_t *)targ)->allocator;

    uint32_t  idx  = 0;
    test_t ** ptrs = ((targs_t *)targ)->ptrs;


    const uint32_t _test_size = test_size;

    struct timespec start_ts, end_ts;

    uint64_t _sum = 0;

    pthread_barrier_wait(&b);

    for (uint32_t i = 0; i < _test_size; ++i) {
        test_t * ptr = (test_t *)allocator->_allocate(sizeof(test_t));
        allocator->_valid_addr(ptr);
        if (ptr) {
            ptrs[idx++] = ptr;
        }
    }

    pthread_barrier_wait(&b);
    for (uint32_t i = 0; i < idx; ++i) {
        allocator->_free(ptrs[i]);
    }

    pthread_barrier_wait(&b);
    memset(ptrs, 0, sizeof(test_t *) * test_size);
    pthread_barrier_wait(&b);

    timers::gettime(timers::ELAPSE, &start_ts);
    for (uint32_t i = 0; i < _test_size; ++i) {
        test_t * ptr = (test_t *)allocator->_allocate(sizeof(test_t));
        allocator->_valid_addr(ptr);
        if (ptr) {
            memset(ptr, i, sizeof(nbytes_t));
            ptrs[_sum] = ptr;
            ++_sum;
        }
    }

    timers::gettime(timers::ELAPSE, &end_ts);


    __atomic_fetch_add(&total_nsec,
                       timers::ts_to_ns(&end_ts) - timers::ts_to_ns(&start_ts),
                       __ATOMIC_RELAXED);

    __atomic_fetch_add(&sum, _sum, __ATOMIC_RELAXED);


    (void)(allocator);
    return NULL;
}

void *
alloc_free_half(void * targ) {
    total_nsec = 0;
    sum        = 0;

    init_thread();
    allocator_t * allocator = (allocator_t *)targ;

    const uint32_t _test_size = test_size;

    struct timespec start_ts, end_ts;

    uint64_t _sum = 0;
    pthread_barrier_wait(&b);

    timers::gettime(timers::ELAPSE, &start_ts);
    for (uint32_t i = 0; i < _test_size; ++i) {
        test_t * ptr = (test_t*)allocator->_allocate(sizeof(test_t));
        allocator->_valid_addr(ptr);
        if (ptr && (i % 2)) {
            memset(ptr, i, sizeof(nbytes_t));
            allocator->_free(ptr);
        }
        if (ptr) {
            ++_sum;
        }
    }
    timers::gettime(timers::ELAPSE, &end_ts);


    __atomic_fetch_add(&total_nsec,
                       timers::ts_to_ns(&end_ts) - timers::ts_to_ns(&start_ts),
                       __ATOMIC_RELAXED);

    __atomic_fetch_add(&sum, _sum, __ATOMIC_RELAXED);


    (void)(allocator);
    return NULL;
}


int
main(int argc, char ** argv) {
    PREPARE_PARSER;
    ADD_ARG("-t", "--threads", false, Int, nthread, "Set nthreads");
    ADD_ARG("-n", false, Int, test_size, "Set n calls per thread");
    ADD_ARG("-b", false, Int, nbytes, "Number of bytes for allocator");
    ADD_ARG("-r", false, Int, run_mask, "Bit mask of tests to run");
    PARSE_ARGUMENTS;

    ERROR_ASSERT(!pthread_barrier_init(&b, NULL, nthread));

    allocator_t allocator;

    thelp::thelper th;

    if (run_mask & (1 << 0)) {
        test_t ** ptrs =
            (test_t **)calloc(test_size * nthread, sizeof(test_t *));

        targs_t * targs = (targs_t *)calloc(nthread, sizeof(targs_t));
        for (uint32_t i = 0; i < nthread; ++i) {
            (targs + i)->allocator = &allocator;
            (targs + i)->ptrs      = ptrs + i * test_size;
        }

        th.spawn_n(nthread,
                   alloc_only,
                   thelp::pin_policy::FIRST_N,
                   targs,
                   sizeof(targs_t));
        th.join_all();
        free(targs);
        fprintf(stderr,
                "Alloc Only:\n"
                "Number of Success Ops  : %lu / %lu -> %.3lf\n"
                "Nano Seconds Per Op    : %.3lf\n",
                sum,
                (nthread * test_size),
                ((double)sum) / (nthread * test_size),
                ((double)total_nsec) / (nthread * test_size));

        fprintf(stderr, "Correctness Testing Alloc\n");
        std::unordered_set<uint64_t> tset;
        uint64_t                     nptrs = test_size * nthread;

        uint64_t hits = 0;
        for (uint64_t i = 0; i < nptrs; ++i) {
            if (ptrs[i]) {
                allocator._valid_addr(ptrs[i]);
                nbytes_t nb;
                memset(&nb, hits, sizeof(nbytes_t));
                uint32_t count_res  = tset.count((uint64_t)(ptrs[i]));
                uint32_t memcmp_res = memcmp(ptrs[i], &nb, sizeof(nbytes_t));
                if (count_res != 0 || memcmp_res != 0) {
                    fprintf(stderr,
                            "\n\n----------------------- ERROR START "
                            "--------------------\n\n");
                    fprintf(stderr, "%d|%d\n", count_res, memcmp_res);

                    fprintf(stderr,
                            "%lu (%lu): %lx == %lx, %lx == %lx (%ld == "
                            "%ld == "
                            "%ld)\n",
                            i,
                            i % test_size,
                            nb.padding[0],
                            ptrs[i]->padding[0],
                            nb.padding[1],
                            ptrs[i]->padding[1],
                            hits & 0xff,
                            ptrs[i]->padding[0] & 0xff,
                            nb.padding[0] & 0xff);

                    test_t * match = NULL;
                    uint64_t _i;
                    for (_i = 0; _i < i; ++_i) {
                        if (ptrs[_i] == ptrs[i]) {
                            match = ptrs[_i];
                            break;
                        }
                    }
                    fprintf(stderr,
                            "(%ld, %ld, %ld, %p)  == (%ld, %ld, %ld, %p)\n",
                            i,
                            i / test_size,
                            i % test_size,
                            ptrs[i],
                            _i,
                            _i / test_size,
                            _i % test_size,
                            match);
                    
                    allocator.addr_to_slab(ptrs[i])->print_state_full();

                    fprintf(stderr,
                            "\nBig Print\n");
                    allocator.print_status_full();

                    fprintf(stderr,
                            "\n\n----------------------- ERROR END "
                            "--------------------\n\n");
                }

                tset.insert((uint64_t)(ptrs[i]));
                ++hits;
            }
            if((((i + 1) % test_size) == 0)) {
                hits = 0;
            }
        }
        fprintf(stderr, "Test Complete\n");
        //        assert(sum == ((1UL) << 26));
        free(ptrs);

        allocator.reset();
    }
    if (run_mask & (1 << 1)) {
        th.spawn_n(nthread,
                   alloc_free,
                   thelp::pin_policy::FIRST_N,
                   (void *)(&allocator),
                   0);
        th.join_all();
        fprintf(stderr,
                "Alloc Free:\n"
                "Number of Success Ops  : %lu / %lu -> %.3lf\n"
                "Nano Seconds Per Op    : %.3lf\n",
                sum,
                (nthread * test_size),
                ((double)sum) / (nthread * test_size),
                ((double)total_nsec) / (nthread * test_size));

        allocator.reset();
    }

    if (run_mask & (1 << 2)) {
        th.spawn_n(nthread,
                   alloc_free_half,
                   thelp::pin_policy::FIRST_N,
                   (void *)(&allocator),
                   0);
        th.join_all();
        fprintf(stderr,
                "Alloc Free Half:\n"
                "Number of Success Ops  : %lu / %lu -> %.3lf\n"
                "Nano Seconds Per Op    : %.3lf\n",
                sum,
                (nthread * test_size),
                ((double)sum) / (nthread * test_size),
                ((double)total_nsec) / (nthread * test_size));


        allocator.reset();
    }
    if (run_mask & (1 << 3)) {
        test_t ** ptrs =
            (test_t **)calloc(test_size * nthread, sizeof(test_t *));

        targs_t * targs = (targs_t *)calloc(nthread, sizeof(targs_t));
        for (uint32_t i = 0; i < nthread; ++i) {
            (targs + i)->allocator = &allocator;
            (targs + i)->ptrs      = ptrs + i * test_size;
        }

        th.spawn_n(nthread,
                   alloc_free_alloc,
                   thelp::pin_policy::FIRST_N,
                   (void *)(targs),
                   sizeof(targs_t));
        th.join_all();
        free(targs);
        fprintf(stderr,
                "Alloc Free Alloc:\n"
                "Number of Success Ops  : %lu / %lu -> %.3lf\n"
                "Nano Seconds Per Op    : %.3lf\n",
                sum,
                (nthread * test_size),
                ((double)sum) / (nthread * test_size),
                ((double)total_nsec) / (nthread * test_size));

        fprintf(stderr, "Correctness Testing Alloc_Free_Alloc\n");
        std::unordered_set<uint64_t> tset;
        uint32_t                     nptrs = test_size * nthread;
        for (uint64_t i = 0; i < nptrs; ++i) {
            if (ptrs[i]) {
                allocator._valid_addr(ptrs[i]);
                nbytes_t nb;
                memset(&nb, (i % test_size), sizeof(nbytes_t));
                uint32_t count_res  = tset.count((uint64_t)(ptrs[i]));
                uint32_t memcmp_res = memcmp(ptrs[i], &nb, sizeof(nbytes_t));
                if (count_res != 0 || memcmp_res != 0) {
                    fprintf(stderr, "%d|%d\n", count_res, memcmp_res);

                    fprintf(stderr,
                            "%lu (%lu): %lx == %lx, %lx == %lx "
                            "(%ld == %ld == "
                            "%ld)\n",
                            i,
                            i % test_size,
                            nb.padding[0],
                            ptrs[i]->padding[0],
                            nb.padding[1],
                            ptrs[i]->padding[1],
                            i & 0xff,
                            ptrs[i]->padding[0] & 0xff,
                            nb.padding[0] & 0xff);

                    test_t * match = NULL;
                    uint64_t _i;
                    for (_i = 0; _i < i; ++_i) {
                        if (ptrs[_i] == ptrs[i]) {
                            match = ptrs[_i];
                            break;
                        }
                    }
                    fprintf(stderr,
                            "(%ld, %ld, %ld, %p)  == (%ld, %ld, %ld, %p)\n",
                            i,
                            i / test_size,
                            i % test_size,
                            ptrs[i],
                            _i,
                            _i / test_size,
                            _i % test_size,
                            match);
                }
                tset.insert((uint64_t)(ptrs[i]));
            }
        }
        fprintf(stderr, "Test Complete\n");
        free(ptrs);
    }
}
