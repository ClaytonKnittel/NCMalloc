#ifndef _SLAB_MANAGER_H_
#define _SLAB_MANAGER_H_


#include <concurrency/rseq/rseq_base.h>

#include <misc/cpp_attributes.h>
#include <system/sys_info.h>

#include <allocator/free_cache.h>
#include <allocator/obj_slab.h>
#include <allocator/slab_allocation.h>

#define SM_DBG_ASSERT(X)   assert(X)


template<typename slab_t, uint32_t cache_size = 13>
struct slab_manager {

    enum FAILURE { ANY = 1 };
    static constexpr uint64_t SUCCESS_BOUND = ANY;

    // maybe drop entire free list and just use head and tail of available list
    // and append free slabs to tail...
    slab_t *               available_slabs_head;
    slab_t *               available_slabs_tail;
    free_cache<cache_size> fc;


    uint32_t
    _cas_set_next_available_slab(const uint32_t start_cpu,
                                 slab_t *       expected_slab) {

        static_assert(slab_t::next_offset == OBJ_SLAB_NEXT_OFFSET);
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized" // NOLINT
        uint64_t temp_ptr;
#pragma GCC diagnostic push
#pragma GCC diagnostic push

        // clang-format off
        asm volatile goto(
            RSEQ_INFO_DEF(32)
            RSEQ_CS_ARR_DEF()




            "1:\n\t"
            // any register will do
            RSEQ_PREP_CS_DEF(%[temp_ptr])
            
            // check if migrated        
            RSEQ_CMP_CUR_VS_START_CPUS()
            // if migrated goto abort:
            "jnz %l[failure]\n\t"
            
            "movq (%[_this]), %[temp_ptr]\n\t"

            "cmpq %[temp_ptr], %[expected_slab]\n\t"
            "jne %l[failure]\n\t"
            
            // available_slabs = slab;

            "movq " V_TO_STR(OBJ_SLAB_NEXT_OFFSET) "(%[temp_ptr]), %[temp_ptr]\n\t"
            "movq %[temp_ptr], (%[_this])\n\t"
            
            "2:\n\t"

            RSEQ_START_ABORT_DEF()
            "jmp 1b\n\t"
            RSEQ_END_ABORT_DEF()
            
            :
            : [ expected_slab ] "r" (expected_slab),
              [ temp_ptr ] "r" (temp_ptr),
              [ _this ] "r"(this),
              [ start_cpu ] "r"(start_cpu)
            : "cc", "memory"
            : failure);
        // clang-format on

        return 0;
    failure:
        return ANY;
    }


    void
    print_status_recap() {
        fprintf(stderr,
                "Available Slabs [ %p ... %p ]\n\t->",
                available_slabs_head,
                available_slabs_tail);

        slab_t * tmp = available_slabs_head;
        while (tmp) {
            fprintf(stderr, " %p ->", tmp);
            tmp = tmp->next;
        }
        fprintf(stderr, " nil\n");
    }

    void
    print_status_full() {
        fprintf(stderr,
                "Available Slabs [ %p ... %p ]\n\t->",
                available_slabs_head,
                available_slabs_tail);

        slab_t * tmp = available_slabs_head;

        while (tmp) {
            tmp->print_state_full();
            tmp = tmp->next;
        }
        fprintf(stderr, "nil\n");
    }
} L2_LOAD_ALIGN;

#undef SM_DBG_ASSERT

#endif
