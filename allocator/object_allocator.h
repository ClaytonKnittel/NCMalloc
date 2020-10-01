#ifndef _OBJECT_ALLOCATOR_H_
#define _OBJECT_ALLOCATOR_H_

#include <misc/cpp_attributes.h>

#include <system/mmap_helpers.h>
#include <system/sys_info.h>

#include <concurrency/bitvec_atomics.h>

#include <allocator/obj_slab.h>
#include <allocator/slab_allocation.h>
#include <allocator/slab_manager.h>
#include <allocator/slab_size_classes.h>



#define OBJ_DBG_ASSERT(X)  assert(X)

namespace alloc {


template<typename slab_t>
static constexpr uint64_t
calculate_start(uint64_t mem_region) {
    return cmath::roundup<uint64_t>((uint64_t)mem_region, sizeof(slab_t));
}


template<typename slab_t>
static constexpr uint64_t
calculate_end(uint64_t mem_region, uint64_t region_size) {
    uint64_t misalignment = ((uint64_t)mem_region) % sizeof(slab_t);
    if (misalignment) {
        return calculate_start<slab_t>(mem_region) +
               cmath::rounddown<uint64_t>((region_size - misalignment),
                                          sizeof(slab_t));
    }
    else {
        return mem_region +
               cmath::rounddown<uint64_t>(region_size, sizeof(slab_t));
    }
}


template<typename slab_t, typename slab_manager_t, typename slab_allocator_t>
struct memory_layout {

    slab_manager_t   slab_managers[num_size_classes][NPROCS];
    slab_allocator_t slab_allocator;

    // this is not meant to be particularly efficient to access, mostly for
    // destruction / reset
    const uint64_t raw_region_size;

    memory_layout(void * mem_region, uint64_t region_size)
        : slab_allocator(calculate_start<slab_t>(
              ((uint64_t)mem_region) +
              sizeof(memory_layout<slab_t, slab_manager_t, slab_allocator_t>))),
          raw_region_size(region_size) {}
};

template<uint32_t cache_size_lower_bound = 13,
         typename slab_allocator_t =
             new_memory::shared_memory_slab_allocator<obj_slab>>
struct object_allocator {


    static constexpr uint32_t cache_size =
        (cmath::next_p2<uint32_t>(24 +
                                  sizeof(uint64_t) * cache_size_lower_bound) -
         24) /
        sizeof(uint64_t);


    using slab_t         = obj_slab;
    using slab_manager_t = slab_manager<slab_t, cache_size>;
    using memory_layout_t =
        memory_layout<slab_t, slab_manager_t, slab_allocator_t>;

    // will default to approximately a few gb of unreserve memory
    static constexpr uint64_t default_region_size = ((1UL) << 31);

    static constexpr uint64_t _log_sizeof_slab_manager =
        cmath::ulog2<uint64_t>(sizeof(slab_manager_t));


    memory_layout_t * const m;
    const uint64_t          end;


    object_allocator()
        : object_allocator(mmap_alloc_noreserve(default_region_size),
                           default_region_size) {}

    object_allocator(uint64_t region_size)
        : object_allocator(mmap_alloc_reserve(region_size), region_size) {}


    object_allocator(void * mem, uint64_t region_size)
        : m((memory_layout_t * const)mem),
          end(calculate_end<slab_t>(((uint64_t)m) + sizeof(memory_layout_t),
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
        return end -
               calculate_start<slab_t>(((uint64_t)m) + sizeof(memory_layout_t));
    }

    void
    reset() {
        const uint64_t region_size = get_raw_region_size();
        memset((void *)m, 0, region_size);
        new (m) memory_layout_t(m, region_size);
    }

    uint64_t CONST_ATTR
    max_objects() const {
        const uint64_t slabs_start = (uint64_t)(m + 1);
        const uint64_t nslabs      = slabs_start / sizeof(obj_slab);
        return nslabs * obj_slab::capacity;
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
    _send_slab(slab_t * slab, const uint32_t size_idx) {

        OBJ_DBG_ASSERT(slab->next == NULL);
        static_assert(slab_t::next_offset == OBJ_SLAB_NEXT_OFFSET);

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
              [ m_clobber ] "=&m" (*(m->slab_managers))
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
              [ m_clobber ] "=&m" (*(m->slab_managers))
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
              [ m_clobber ] "=&m" (*(m->slab_managers))
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
                      [ m_clobber ] "=&m" (*(m->slab_managers))
                    : [ sm_base ] "r" (m->slab_managers[size_idx]),
                      [ LOG_SIZEOF_SM ] "i" (_log_sizeof_slab_manager)
                    : "cc");
        // clang-format on
        return ret;
    }

    uint64_t
    try_push(uint64_t ptr, const uint32_t size_idx) {
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
                    :
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


    void *
    _allocate_inner(const uint32_t size_idx) {
        while (1) {
            const uint64_t start_cpu = get_start_cpu();
            IMPOSSIBLE_COND(start_cpu >= NPROCS);

            slab_manager_t * sm = m->slab_managers[size_idx] + start_cpu;
            slab_t *         _available_slabs_head = sm->available_slabs_head;

            OBJ_DBG_ASSERT(
                (((uint64_t)_available_slabs_head) % sizeof(obj_slab)) == 0);
            OBJ_DBG_ASSERT((((uint64_t)(sm->available_slabs_head)) %
                            sizeof(obj_slab)) == 0);
            OBJ_DBG_ASSERT((((uint64_t)(sm->available_slabs_tail)) %
                            sizeof(obj_slab)) == 0);


            if (BRANCH_UNLIKELY(_available_slabs_head == NULL)) {
                if (m->slab_allocator.out_of_memory(end)) {
                    return NULL;
                }

                slab_t * new_slab = m->slab_allocator._new();
                OBJ_DBG_ASSERT((((uint64_t)new_slab) % sizeof(obj_slab)) == 0);

                if ((new_slab) >= ((slab_t *)end)) {
                    return NULL;
                }
                new ((void * const)new_slab) slab_t(idx_to_size(size_idx));

                OBJ_DBG_ASSERT(new_slab != NULL);
                OBJ_DBG_ASSERT(new_slab->next == NULL);
                _send_slab(new_slab, size_idx);

            }
            else {

                uint64_t ret = _available_slabs_head->_allocate(start_cpu);

                if (BRANCH_LIKELY(ret < slab_t::SUCCESS_BOUND)) {
                    return (void *)(_available_slabs_head->payload +
                                    idx_to_size(size_idx) * ret);
                }
                else if (ret != slab_t::FAILURE::MIGRATED) {
                    if (!sm->_cas_set_next_available_slab(
                            start_cpu,
                            _available_slabs_head)) {

                        OBJ_DBG_ASSERT(sm->available_slabs_head !=
                                       _available_slabs_head);
                        OBJ_DBG_ASSERT(_available_slabs_head->state ==
                                       slab_t::OWNED);
                        if (!_available_slabs_head->_try_release()) {
                            _available_slabs_head->next = NULL;
                            _send_slab(_available_slabs_head, size_idx);
                            OBJ_DBG_ASSERT(_available_slabs_head->state ==
                                           slab_t::OWNED);
                        }
                    }
                }
            }
        }
    }

    void *
    _allocate(const uint32_t size) {
        const uint32_t size_idx = size_to_idx(size);
        uint64_t       ptr      = try_pop(size_idx);
        if (ptr > ((1UL) << _log_sizeof_slab_manager)) {
            return (void *)ptr;
        }
        return _allocate_inner(size_idx);
    }

    slab_t *
    addr_to_slab(void * addr) {
        return (slab_t *)(sizeof(slab_t) * (((uint64_t)addr) / sizeof(slab_t)));
    }


    void
    _free(void * addr) {
        slab_t *       slab     = addr_to_slab(addr);
        const uint32_t size     = slab->block_size;
        const uint32_t size_idx = size_to_idx(size);

        if (!try_push((uint64_t)addr, size_idx)) {
            return;
        }

        OBJ_DBG_ASSERT((((uint64_t)slab) % sizeof(obj_slab)) == 0);

        if (slab->_free(((uint64_t)addr) - ((uint64_t)slab))) {
            if (slab->_set_owned()) {
                OBJ_DBG_ASSERT(slab->state == slab_t::OWNED);
                slab->next = NULL;
                _send_slab(slab, size_idx);
            }
        }
    }

    void
    _valid_addr(void * addr) {
        if (addr) {
            uint64_t addr_minus_start = (((uint64_t)addr) % sizeof(slab_t));
            assert(addr_minus_start >= slab_t::payload_offset);
            assert(((uint64_t)addr) >
                   (calculate_start<slab_t>((uint64_t)(m + 1))));
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
        fprintf(stderr,
                "[%p ... %p]\n",
                (void *)calculate_start<slab_t>(
                    (((uint64_t)m + sizeof(memory_layout_t)))),
                (void *)calculate_end<slab_t>(
                    ((uint64_t)m + sizeof(memory_layout_t)),
                    get_slab_region_size()));
        for (uint32_t i = 0; i < NPROCS; ++i) {
            for (uint32_t _i = 1; _i < 2; ++_i) {
                fprintf(stderr, "[%d]", i);
                m->slab_managers[_i][i].print_status_full();
            }
        }
    }
};

}  // namespace alloc

#undef OBJ_DBG_ASSERT

#endif
