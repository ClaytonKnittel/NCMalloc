#ifndef _OBJECT_ALLOCATOR_H_
#define _OBJECT_ALLOCATOR_H_

#include <misc/cpp_attributes.h>

#include <system/mmap_helpers.h>
#include <system/sys_info.h>

#include <concurrency/bitvec_atomics.h>

#include <allocator/obj_slab.h>
#include <allocator/slab_allocation.h>
#include <allocator/slab_manager.h>


#define OBJ_DBG_ASSERT(X)  // assert(X)

namespace alloc {

static constexpr uint64_t
calculate_start(uint64_t mem_region) {
    return cmath::roundup<uint64_t>((uint64_t)mem_region, slab_size);
}

static constexpr uint64_t
calculate_end(uint64_t mem_region, uint64_t region_size) {
    uint64_t misalignment = ((uint64_t)mem_region) % slab_size;
    if (misalignment) {
        return calculate_start(mem_region) +
               cmath::rounddown<uint64_t>((region_size - misalignment),
                                          slab_size);
    }
    else {
        return mem_region + cmath::rounddown<uint64_t>(region_size, slab_size);
    }
}

// forward declaration for _call_free
template<uint32_t cache_size_lower_bound>
struct object_allocator;

template<uint32_t cache_size_lower_bound, uint64_t chunk_size>
void _call_free(object_allocator<cache_size_lower_bound> * obj_allocator,
                uint64_t                                   addr);


template<uint32_t num_size_classes,
         typename slab_manager_t,
         typename slab_allocator_t>
struct memory_layout {

    // the reason for partition first by size class then by core is because for
    // a given operation size class will stay fixed but index within core may
    // need to be recalculated due to preemption
    slab_manager_t   slab_managers[num_size_classes][NPROCS];
    slab_allocator_t slab_allocator;

    // this is not meant to be particularly efficient to access, mostly for
    // destruction / reset
    const uint64_t raw_region_size;

    memory_layout(void * mem_region, uint64_t region_size)
        : slab_allocator(
              calculate_start(((uint64_t)mem_region) +
                              sizeof(memory_layout<num_size_classes,
                                                   slab_manager_t,
                                                   slab_allocator_t>))),
          raw_region_size(region_size) {}
};


template<uint32_t cache_size_lower_bound = 13>
struct object_allocator {


    typedef void (*free_wrapper)(object_allocator<cache_size_lower_bound> *,
                                 uint64_t);
    const free_wrapper call_free[num_size_classes] = {
        &_call_free<cache_size_lower_bound, 8>,
        &_call_free<cache_size_lower_bound, 16>,
        &_call_free<cache_size_lower_bound, 24>,
        &_call_free<cache_size_lower_bound, 32>,
        &_call_free<cache_size_lower_bound, 48>,
        &_call_free<cache_size_lower_bound, 64>,
        &_call_free<cache_size_lower_bound, 80>,
        &_call_free<cache_size_lower_bound, 96>,
        &_call_free<cache_size_lower_bound, 112>,
        &_call_free<cache_size_lower_bound, 128>,
        &_call_free<cache_size_lower_bound, 144>,
        &_call_free<cache_size_lower_bound, 160>,
        &_call_free<cache_size_lower_bound, 176>,
        &_call_free<cache_size_lower_bound, 192>,
        &_call_free<cache_size_lower_bound, 208>,
        &_call_free<cache_size_lower_bound, 224>,
        &_call_free<cache_size_lower_bound, 240>,
        &_call_free<cache_size_lower_bound, 256>
    };


    // this is the cache for each size class in the slab managers
    static constexpr uint32_t cache_size =
        (cmath::next_p2<uint32_t>(24 +
                                  sizeof(uint64_t) * cache_size_lower_bound) -
         24) /
        sizeof(uint64_t);

    using slab_t           = obj_slab_base_data;
    using slab_manager_t   = slab_manager<slab_t, cache_size>;
    using slab_allocator_t = new_memory::shared_memory_slab_allocator;

    using memory_layout_t =
        memory_layout<num_size_classes, slab_manager_t, slab_allocator_t>;

    // will default to approximately a few gb of unreserve memory
    static constexpr uint64_t default_region_size = ((1UL) << 33);

    static constexpr uint64_t _log_sizeof_slab_manager =
        cmath::ulog2<uint64_t>(sizeof(slab_manager_t));


    // actual member data here (I need to learn inheritence amirite?)
    memory_layout_t * const m;
    const uint64_t          end;


    object_allocator()
        : object_allocator(mmap_alloc_noreserve(default_region_size),
                           default_region_size) {}

    object_allocator(uint64_t region_size)
        : object_allocator(mmap_alloc_reserve(region_size), region_size) {}


    object_allocator(void * mem, uint64_t region_size)
        : m((memory_layout_t * const)mem),
          end(calculate_end(((uint64_t)m) + sizeof(memory_layout_t),
                            region_size)) {
        new (m) memory_layout_t(m, region_size);
    }


    ~object_allocator() {
        madv_free((void *)m, get_raw_region_size());
    }

    uint64_t ALWAYS_INLINE PURE_ATTR
    get_raw_region_size() const {
        return m->raw_region_size;
    }

    uint64_t ALWAYS_INLINE PURE_ATTR
    get_slab_region_size() const {
        return end - calculate_start(((uint64_t)m) + sizeof(memory_layout_t));
    }

    void
    reset() {
        const uint64_t region_size = get_raw_region_size();
        memset((void *)m, 0, region_size);
        new (m) memory_layout_t(m, region_size);
    }


    uint32_t ALWAYS_INLINE PURE_ATTR
    in_range(void * p) const {
        return ((uint64_t)p) > ((uint64_t)m) && ((uint64_t)p) < end;
    }

    uint32_t ALWAYS_INLINE PURE_ATTR
    in_range(uint64_t p) const {
        return p > ((uint64_t)m) && p < end;
    }


#define SEND_SLAB_BRANCHES 1
    void
    _send_slab(const uint32_t size_idx, slab_t * slab) {
        OBJ_DBG_ASSERT(slab->next == NULL);

#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // NOLINT
        uint64_t temp_ptr, sm;                          // NOLINT

#if SEND_SLAB_BRANCHES == 0
        uint64_t offset, next_reg = OBJ_SLAB_NEXT_OFFSET;
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic push

#if SEND_SLAB_BRANCHES == 0
        // clang-format off
        asm volatile(
            RSEQ_INFO_DEF(32)
            RSEQ_CS_ARR_DEF()


            "1:\n\t"
            // any register will do
            RSEQ_PREP_CS_DEF(%[temp_ptr])
            
            "xorq %[offset], %[offset]\n\t"
            
            "movl %%fs:__rseq_abi@tpoff+4, %k[sm]\n\t"
            "salq %[LOG_SIZEOF_SM], %[sm]\n\t"
            "addq %[sm_base], %[sm]\n\t"

            "cmpq $0, (%[sm])\n\t"
            "leaq 8(%[sm]), %[temp_ptr]\n\t"
            
            "cmovneq %[next_reg], %[offset]\n\t"
            "cmovneq %[temp_ptr], %[sm]\n\t"
            "cmovneq (%[temp_ptr]), %[temp_ptr]\n\t"
            
            "movq %[slab], (%[temp_ptr], %[offset])\n\t"
            "movq %[slab], (%[sm])\n\t"
            
            "2:\n\t"

            RSEQ_START_ABORT_DEF()
            "jmp 1b\n\t"
            RSEQ_END_ABORT_DEF()

            : [ temp_ptr ] "=&r" (temp_ptr),
              [ offset ] "=&r" (offset),           
              [ sm ] "=&r" (sm),
              [ slab_clobber ] "=&m" (*slab),
              [ sm_clobber ] "=&m" (*(m->slab_managers[size_idx]))
            : [ slab ] "r" (slab),
              [ next_reg ] "r" (next_reg),
              [ sm_base ] "r" (m->slab_managers[size_idx]),
              [ LOG_SIZEOF_SM ] "i" (_log_sizeof_slab_manager)
            : "cc");
#elif SEND_SLAB_BRANCHES == 1
        
        asm volatile(
            RSEQ_INFO_DEF(32)
            RSEQ_CS_ARR_DEF()


            "1:\n\t"
            // any register will do
            RSEQ_PREP_CS_DEF(%[temp_ptr])
                    
            "movl %%fs:__rseq_abi@tpoff+4, %k[sm]\n\t"
            "salq %[LOG_SIZEOF_SM], %[sm]\n\t"
            "addq %[sm_base], %[sm]\n\t"

            "cmpq $0, (%[sm])\n\t"
            "leaq 8(%[sm]), %[temp_ptr]\n\t"
            "je 6f\n\t"

            "movq %[temp_ptr], %[sm]\n\t"
            "movq (%[temp_ptr]), %[temp_ptr]\n\t"
            "addq $" V_TO_STR(OBJ_SLAB_NEXT_OFFSET) ", %[temp_ptr]\n\t"

            "6:\n\t"
            "movq %[slab], (%[temp_ptr])\n\t"
            "movq %[slab], (%[sm])\n\t"
            
            "2:\n\t"

            RSEQ_START_ABORT_DEF()
            "jmp 1b\n\t"
            RSEQ_END_ABORT_DEF()

            : [ temp_ptr ] "=&r" (temp_ptr),
              [ sm ] "=&r" (sm),
              [ sm_clobber ] "=&m" (*(m->slab_managers[size_idx]))
            : [ slab ] "r" (slab),
              [ sm_base ] "r" (m->slab_managers[size_idx]),
              [ LOG_SIZEOF_SM ] "i" (_log_sizeof_slab_manager)
            : "cc");
#else
        asm volatile(
            RSEQ_INFO_DEF(32)
            RSEQ_CS_ARR_DEF()


            "1:\n\t"
            // any register will do
            RSEQ_PREP_CS_DEF(%[temp_ptr])

            
            "movl %%fs:__rseq_abi@tpoff+4, %k[sm]\n\t"
            "salq %[LOG_SIZEOF_SM], %[sm]\n\t"
            "addq %[sm_base], %[sm]\n\t"

            "cmpq $0, (%[sm])\n\t"
            "leaq 8(%[sm]), %[temp_ptr]\n\t"
            
            "jne 6f\n\t"
            "movq %[slab], (%[temp_ptr])\n\t"
            "jmp 7f\n\t"
            
            "6:\n\t"
            "movq %[temp_ptr], %[sm]\n\t"
            "movq (%[temp_ptr]), %[temp_ptr]\n\t"
            "movq %[slab], " V_TO_STR(OBJ_SLAB_NEXT_OFFSET) "(%[temp_ptr])\n\t"
            "7:\n\t"
            "movq %[slab], (%[sm])\n\t"
            "2:\n\t"

            RSEQ_START_ABORT_DEF()
            "jmp 1b\n\t"
            RSEQ_END_ABORT_DEF()

            : [ temp_ptr ] "=&r" (temp_ptr),
              [ sm ] "=&r" (sm),
              [ sm_clobber ] "=&m" (*(m->slab_managers[size_idx]))
            : [ slab ] "r" (slab),
              [ sm_base ] "r" (m->slab_managers[size_idx]),
              [ LOG_SIZEOF_SM ] "i" (_log_sizeof_slab_manager)
            : "cc");
        // clang-format on
#endif
    }


    uint64_t
    try_pop(const uint32_t size_idx) {
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // NOLINT
        uint64_t ret, fc_cache;
#pragma GCC diagnostic push
#pragma GCC diagnostic push
        // clang-format off
                asm volatile(
                    RSEQ_INFO_DEF(32)
                    RSEQ_CS_ARR_DEF()

              
                    "1:\n\t"
                    // any register will do
                    RSEQ_PREP_CS_DEF(%[ret])


                    "movl %%fs:__rseq_abi@tpoff+4, %k[fc_cache]\n\t"
                    "salq %[LOG_SIZEOF_SM], %[fc_cache]\n\t"
                    "addq %[sm_base], %[fc_cache]\n\t"
                
                    "movq 16(%[fc_cache]), %[ret]\n\t"
                    "testq %[ret], %[ret]\n\t"
                    "jz 2f\n\t"

                    "movq 16(%[fc_cache], %[ret], 8), %[ret]\n\t"
                
                    "subq $1, 16(%[fc_cache])\n\t"
                    "2:\n\t"

                    RSEQ_START_ABORT_DEF()
                    "jmp 1b\n\t"
                    RSEQ_END_ABORT_DEF()
            
                    : [ ret ] "=&r" (ret),
                      [ fc_cache ] "=&r" (fc_cache),
                      [ sm_clobber ] "=&m" (*(m->slab_managers[size_idx]))
                    : [ sm_base ] "r" (m->slab_managers[size_idx]),
                      [ LOG_SIZEOF_SM ] "i" (_log_sizeof_slab_manager)
                    : "cc");
        // clang-format on
        return ret;
    }

    uint64_t
    try_push(const uint32_t size_idx, uint64_t ptr) {

#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // NOLINT
        uint64_t idx, fc_cache;
#pragma GCC diagnostic push
#pragma GCC diagnostic push
        // clang-format off
                asm volatile goto(
                    RSEQ_INFO_DEF(32)
                    RSEQ_CS_ARR_DEF()


                    "1:\n\t"
                    // any register will do
                    RSEQ_PREP_CS_DEF(%[fc_cache])


                    "movl %%fs:__rseq_abi@tpoff+4, %k[fc_cache]\n\t"
                    "salq %[LOG_SIZEOF_SM], %[fc_cache]\n\t"
                    "addq %[sm_base], %[fc_cache]\n\t"
                
                    "movq 16(%[fc_cache]), %[idx]\n\t"
                    "cmpq %[CACHE_SIZE], %[idx]\n\t"
                    "je %l[no_push]\n\t"

                    "movq %[ptr], 24(%[fc_cache], %[idx], 8)\n\t"
                    "addq $1, 16(%[fc_cache])\n\t"
                    "2:\n\t"

                    RSEQ_START_ABORT_DEF()
                    "jmp 1b\n\t"
                    RSEQ_END_ABORT_DEF()
                    : // no output for asm goto (worth investigating
                      // the cost of this "memory" clobber
                    :[ idx ] "r" (idx),
                     [ fc_cache ] "r" (fc_cache),
                     [ ptr ] "r" (ptr),
                     [ sm_base ] "r" (m->slab_managers[size_idx]),
                     [ LOG_SIZEOF_SM ] "i" (_log_sizeof_slab_manager),
                     [ CACHE_SIZE ] "i" (cache_size)
                    : "cc", "memory"
                    : no_push);
        // clang-format on
        return 0;
    no_push:
        return 1;
    }

    void
    _new_obj_slab(const uint32_t size_idx, void * dst) {
        call_new[size_idx](dst);
    }


    void *
    _inner_allocate(const uint32_t size_idx, const uint64_t size) {
        while (1) {
            const uint64_t start_cpu = get_start_cpu();
            IMPOSSIBLE_COND(start_cpu >= NPROCS);

            slab_manager_t * sm = m->slab_managers[size_idx] + start_cpu;
            slab_t *         _available_slabs_head = sm->available_slabs_head;

            if (BRANCH_UNLIKELY(_available_slabs_head == NULL)) {
                if (m->slab_allocator.out_of_memory(end)) {
                    return NULL;
                }
                slab_t * new_slab = (slab_t *)(m->slab_allocator._new());
                if ((new_slab) >= ((slab_t *)end)) {
                    return NULL;
                }
                _new_obj_slab(size_idx, (void *)new_slab);
                _send_slab(size_idx, new_slab);
            }
            else {

                uint64_t ret = _available_slabs_head->_allocate(start_cpu);
                if (BRANCH_LIKELY(ret < slab_t::SUCCESS_BOUND)) {
                    return (void *)(((uint64_t)_available_slabs_head) +
                                    size * ret +
                                    size_idx_to_meta_data[size_idx]);
                }
                else if (ret != slab_t::FAILURE::MIGRATED) {
                    if (!sm->_cas_set_next_available_slab(
                            start_cpu,
                            _available_slabs_head)) {

                        if (!_available_slabs_head->_try_release(
                                (uint64_t *)((uint64_t)_available_slabs_head) +
                                size_idx_to_free_offset[size_idx])) {

                            _available_slabs_head->next = NULL;
                            _send_slab(size_idx, _available_slabs_head);
                            OBJ_DBG_ASSERT(_available_slabs_head->state ==
                                           slab_t::OWNED);
                        }
                    }
                }
            }
        }
    }

    void *
    _allocate(const uint64_t size) {
        // break into 2 functions at spot we would like
        // inlined. Important to have the size_to_idx part inlined
        // because if we lucky and its constant thats done for us
        const uint64_t rounded_size = round_size(size);
        const uint32_t size_idx     = size_to_idx(rounded_size);
        uint64_t       ptr          = try_pop(size_idx);
        if (ptr > ((1UL) << _log_sizeof_slab_manager)) {
            return (void *)ptr;
        }
        return _inner_allocate(size_idx, rounded_size);
    }

    uint64_t
    addr_to_slab(uint64_t addr) {
        return addr & (~(slab_size - 1));
    }

    uint64_t
    addr_to_size(uint64_t addr) {
        obj_slab_base_data * slab_data =
            (obj_slab_base_data *)addr_to_slab(addr);
        return slab_data->in_memory_chunk_size;
    }

    template<uint64_t size>
    void
    _size_free(uint64_t addr) {
        const uint32_t size_idx = size_to_idx(size);
        if (!try_push(size_idx, addr)) {
            return;
        }

        obj_slab<size> * slab = (obj_slab<size> *)addr_to_slab(addr);
        if (slab->_free(addr - ((uint64_t)slab))) {
            if (slab->_set_owned()) {
                slab->fixed_data.next = NULL;
                _send_slab(size_idx, &(slab->fixed_data));
            }
        }
    }

    void
    _free(uint64_t addr) {
        const uint64_t size     = addr_to_size(addr);
        const uint32_t size_idx = size_to_idx(size);

        call_free[size_idx](this, addr);
    }


    void
    _valid_addr(void * addr) {
        if (addr) {
            assert(((uint64_t)addr) > (calculate_start((uint64_t)(m + 1))));
            assert(((uint64_t)addr) < end);
        }
    }

    void
    print_status_recap() {
        for (uint32_t i = 0; i < NPROCS; ++i) {
            fprintf(stderr, "[%d]", i);
            m->slab_managers[i].print_status_recap();
        }
    }

    void
    print_status_full() {
        fprintf(
            stderr,
            "[%p ... %p]\n",
            (void *)calculate_start((((uint64_t)m + sizeof(memory_layout_t)))),
            (void *)calculate_end(((uint64_t)m + sizeof(memory_layout_t)),
                                  get_slab_region_size()));
        for (uint32_t i = 0; i < NPROCS; ++i) {
            fprintf(stderr, "[%d]", i);
            m->slab_managers[i].print_status_full();
        }
    }
};

// _call_free content (once object_allocator is fully defined)
template<uint32_t cache_size_lower_bound, uint64_t chunk_size>
void
_call_free(object_allocator<cache_size_lower_bound> * obj_allocator,
           uint64_t                                   addr) {
    obj_allocator->template _size_free<chunk_size>(addr);
}


}  // namespace alloc

#undef OBJ_DBG_ASSERT

#endif
