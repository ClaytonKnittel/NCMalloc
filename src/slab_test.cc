#include <util/arg.h>
#include <util/verbosity.h>

uint64_t          test_size = (1 << 20);
uint64_t          nthread   = (32);
pthread_barrier_t b;


#include <concurrency/rseq/rseq_base.h>
#include <optimized/const_math.h>
#include <timing/thread_helper.h>
#include <timing/timers.h>

#include <allocator/object_allocator.h>

#include <container/block_list.h>

#include <time.h>
#include <unordered_set>

using allocator_t = alloc::object_allocator<>;
allocator_t allocator;

struct region {
    const uint32_t   id;
    const uint32_t   size;
    uint32_t * const mem;

    region(const uint32_t _id, const uint32_t _size, uint32_t * const _mem)
        : id(_id), size(_size / sizeof(uint32_t)), mem(_mem) {
        for (uint32_t i = 0; i < size; ++i) {
            mem[i] = _id;
        }
    }

    void
    verify() {
        for (uint32_t i = 0; i < size; ++i) {
            assert(mem[i] == id);
        }
    }

    ~region() {}
};


struct alloc_tester {

    using block_list_t     = container::block_list<region>;
    using block_iterator_t = typename block_list_t::block_iterator_t;
    using single_node_iterator_t =
        typename block_list_t::single_node_iterator_t;
    using node_iterator_t = typename block_list_t::node_iterator_t;

    static constexpr uint64_t batch_destroy_all = (~(0UL));
    static constexpr uint64_t batch_destroy_lo  = 0x3333333333333333;
    static constexpr uint64_t batch_destroy_hi  = 0xcccccccccccccccc;


    block_list_t allocs;
    uint32_t     counter;

    alloc_tester() : counter(0) {
        allocs.zero();
    }

    void
    verify_all() {
        for (node_iterator_t it = allocs.nbegin(); !(it.end()); ++it) {
            it.get()->verify();
        }
    }

    void
    _new(uint32_t size) {
        void * r = allocator._allocate(size);
        if (BRANCH_LIKELY(r != NULL)) {
            region * _r = allocs.insert(region(++counter, size, (uint32_t *)r));
            _r->verify();
        }
    }

    void
    _delete() {
        region * r = allocs.pop();
        r->verify();
        allocator._free(r->mem);
    }

    void
    _delete_batch() {
        for (block_iterator_t it = allocs.bbegin(); !(it.end()); ++it) {
            for (single_node_iterator_t _it = it.nbegin(); !(_it.end());
                 ++_it) {
                _it.get()->verify();
                allocator._free(_it.get()->mem);
            }
            it.get()->drop(batch_destroy_lo);
        }
    }
};

void *
alloc_only(void * targ) {
    (void)(targ);
    init_thread();


    const uint32_t _test_size = test_size;

    alloc_tester t;
    pthread_barrier_wait(&b);


    for (uint32_t i = 0; i < _test_size; ++i) {
        uint32_t size;
        do {
            size = rand() % 144;
        } while (!size);
        t._new(size);
    }
    t.verify_all();
    return NULL;
}

void *
alloc_free(void * targ) {
    (void)(targ);
    init_thread();


    const uint32_t _test_size = test_size;

    alloc_tester t;
    pthread_barrier_wait(&b);


    for (uint32_t i = 0; i < _test_size; ++i) {
        uint32_t size;
        do {
            size = rand() % 144;
        } while (!size);
        t._new(size);
        t._delete();
    }
    t.verify_all();
    return NULL;
}

void *
alloc_free_alloc(void * targ) {
    (void)(targ);
    init_thread();


    const uint32_t _test_size = test_size;

    alloc_tester t;
    pthread_barrier_wait(&b);


    for (uint32_t i = 0; i < _test_size; ++i) {
        uint32_t size;
        do {
            size = rand() % 144;
        } while (!size);
        t._new(size);
        t._delete();
        t._new(size);
    }
    t.verify_all();
    return NULL;
}

void *
alloc_free_half(void * targ) {
    (void)(targ);
    init_thread();


    const uint32_t _test_size = test_size;

    alloc_tester t;
    pthread_barrier_wait(&b);


    for (uint32_t i = 0; i < _test_size; ++i) {
        uint32_t size;
        do {
            size = rand() % 144;
        } while (!size);
        t._new(size);
        if (i % 2) {
            t._delete();
        }
    }
    t.verify_all();
    return NULL;
}


int
main(int argc, char ** argv) {
    PREPARE_PARSER;
    ADD_ARG("-t", "--threads", false, Int, nthread, "Set nthreads");
    ADD_ARG("-n", false, Int, test_size, "Set n calls per thread");
    PARSE_ARGUMENTS;

    ERROR_ASSERT(!pthread_barrier_init(&b, NULL, nthread));

    thelp::thelper th;

    fprintf(stderr, "%-24s", "Alloc Only Test");
    th.spawn_n(nthread, alloc_only, thelp::pin_policy::FIRST_N, NULL, 0);
    th.join_all();
    fprintf(stderr, " - Passed\n");


    fprintf(stderr, "%-24s", "Alloc Free Test");
    th.spawn_n(nthread, alloc_free, thelp::pin_policy::FIRST_N, NULL, 0);
    th.join_all();
    fprintf(stderr, " - Passed\n");

    fprintf(stderr, "%-24s", "Alloc Free Alloc Test");
    th.spawn_n(nthread, alloc_free_alloc, thelp::pin_policy::FIRST_N, NULL, 0);
    th.join_all();
    fprintf(stderr, " - Passed\n");

    fprintf(stderr, "%-24s", "Alloc Free Half Test");
    th.spawn_n(nthread, alloc_free_half, thelp::pin_policy::FIRST_N, NULL, 0);
    th.join_all();
    fprintf(stderr, " - Passed\n");
}
