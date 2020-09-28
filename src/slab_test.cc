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


using allocator_t = alloc::object_allocator<13>;

uint64_t          sum        = 0;
uint64_t          total_nsec = 0;
pthread_barrier_t b;


void *
alloc_only(void * targ) {
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
        void * ptr = allocator->_allocate(1);
        allocator->_valid_addr(ptr);
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

    nbytes = ((1UL) << nbytes);
    allocator_t allocator(nbytes);

    thelp::thelper th;


    th.spawn_n(nthread, alloc_only, thelp::pin_policy::FIRST_N, &allocator, 0);
    th.join_all();

    fprintf(stderr,
            "Alloc Only:\n"
            "Number of Success Ops  : %lu / %lu -> %.3lf\n"
            "Nano Seconds Per Op    : %.3lf\n",
            sum,
            (nthread * test_size),
            ((double)sum) / (nthread * test_size),
            ((double)total_nsec) / (nthread * test_size));
}
