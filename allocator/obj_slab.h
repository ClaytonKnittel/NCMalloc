#ifndef _OBJ_SLAB_H_
#define _OBJ_SLAB_H_

// sys headers
#include <stdio.h>
//

#include <concurrency/bitvec_atomics.h>
#include <concurrency/rseq/rseq_base.h>

#include <misc/cpp_attributes.h>
#include <optimized/bits.h>
#include <optimized/const_math.h>
#include <system/sys_info.h>

#include <allocator/slab_traits.h>

#define OBJ_SLAB_ASSERT(X)  // assert(X)


#define OBJ_SLAB_NEXT_OFFSET 8


struct obj_slab_base_data {
    // 4096 is maximum possible capacity
    enum FAILURE { MIGRATED = 4096, FULL = 4097 };
    static constexpr uint64_t SUCCESS_BOUND = FAILURE::MIGRATED;

    enum { OWNED = 0, UNOWNED = 1 };



    const uint64_t       in_memory_chunk_size;
    obj_slab_base_data * next;
    uint64_t             available_vecs;
    uint64_t             available_slots[0];

    // allocate and try_release, and set_state do not rely on template
    // parameters. Since the layout is the same for all functions best to only
    // have 1 version in the code cache. Might be good to verify the compiler
    // doesn't automatically do this if it detects no dif on different versions

    obj_slab_base_data(const uint64_t chunk_size)
        : in_memory_chunk_size(chunk_size) {}

    uint64_t
    _allocate(const uint32_t start_cpu) {

        OBJ_SLAB_ASSERT((((uint64_t)this) % slab_size) == 0);
        OBJ_SLAB_ASSERT((((uint64_t)this) % slab_size) == 0);
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // NOLINT
        // temporaries for storing either available_vecs and available_slots
        // respectively
        uint64_t temp_av, temp_as;  // NOLINT

        // temporary to store idx in temp_v1 for indexing available_slots array
        uint64_t idx_av;  // NOLINT

        // output: [0 - 4095] -> valid slot, [~0] -> obj_slab full, [capacity]
        // migrated
        uint64_t idx;  // NOLINT

#pragma GCC diagnostic push
#pragma GCC diagnostic push
        // clang-format off
        asm volatile(

            RSEQ_INFO_DEF(32)
            RSEQ_CS_ARR_DEF()

            // any register will do



            // start critical section
            "1:\n\t"
            RSEQ_PREP_CS_DEF(%[temp_av])
            
            "mov %[MIGRATED], %[idx]\n\t"
        
            // check if migrated        
            RSEQ_CMP_CUR_VS_START_CPUS()
            // if migrated goto 2:
            "jnz 9f\n\t"            

            // if not migrated temp_v = *v
            "movq 16(%[_this]), %[temp_av]\n\t"
            "jmp 5f\n\t"

            "7:\n\t"
            "blsrq %[temp_av], %[temp_av]\n\t"

        
            // this is a completely valid state to be migrated out after
            // (all we have really done is cleaned up v1 vector a bit)
            // because we can be migrated out here we don't check/set if
            // temp_as is full as that could lead to invalid state in v1
            "movq %[temp_av], 16(%[_this])\n\t"

            
            "5:\n\t"
            // this is } in while loop starting at 5:


            // prep return before checking if slab is full
            "movq %[FULL], %[idx]\n\t"
            "testq %[temp_av], %[temp_av]\n\t"
            "jz 9f\n\t"
        
            "tzcntq %[temp_av], %[idx_av]\n\t"
            "movq 24(%[_this], %[idx_av], 8), %[temp_as]\n\t"

            "testq %[temp_as], %[temp_as]\n\t"
            "jz 7b\n\t" // 7b is blsrq %[idx_outer], %[temp_av], fall into 5b

            "movq %[temp_as], %[idx]\n\t"
            "blsrq %[temp_as], %[temp_as]\n\t"
                       
            // commit
            "movq %[temp_as], 24(%[_this], %[idx_av], 8)\n\t"
            

            // end critical section
            "2:\n\t"

            // calculate index outside of critical section

            // note it might be worth it to include this in the
            // critical section. tzcnt is only p0 and the fact path
            // requires a 3 arg LEA also on p0 for return. Question is
            // whether the serialization is worth potentially avoiding
            // a preemption
            "tzcntq %[idx], %[idx]\n\t"
            "salq $6, %[idx_av]\n\t"
            "addq %[idx_av], %[idx]\n\t"

            // label for full / migrated jump
            "9:\n\t"
            RSEQ_START_ABORT_DEF()
            // given that the critical section is fairly involved
            // it may be worth it to put this in the same code section
            // as critical section for faster aborts
            "jmp 1b\n\t"
            RSEQ_END_ABORT_DEF()

            : [ idx] "=&r" (idx),
              [ idx_av ] "=&r" (idx_av),
              [ temp_as ] "=&r" (temp_as),
              [ temp_av ] "=&r" (temp_av),
              [ av_clobber ] "=&m" (available_vecs),
              [ as_clobber ] "=&m" (available_slots)
            : [ _this ] "r" (this),              
              [ MIGRATED ] "i" (obj_slab_base_data::MIGRATED),
              [ FULL ] "i" (obj_slab_base_data::FULL),
              [ start_cpu ] "r" (start_cpu)
            : "cc");
        // clang-format on
        return idx;
    }


    static constexpr ALWAYS_INLINE CONST_ATTR uint64_t *
                                              freed_vecs_to_freed_slots(uint64_t * const freed_vec_ptr) {
        return freed_vec_ptr + 2;
    }

    static constexpr ALWAYS_INLINE CONST_ATTR uint64_t *
                                              freed_vecs_to_state(uint64_t * const freed_vec_ptr) {
        return freed_vec_ptr + 1;
    }

    // this function is quasi atomic.
    uint32_t
    _try_release(uint64_t * const freed_vecs) {
        uint64_t reclaimed_vecs = *freed_vecs;
        if (reclaimed_vecs == 0) {
            // if nothing free this is unowned memory and will be
            // claimed by first free

            OBJ_SLAB_ASSERT(state == obj_slab_base_data::OWNED);
            *freed_vecs_to_state(freed_vecs) = obj_slab_base_data::UNOWNED;
            return 1;
        }

        atomic_unset(freed_vecs, reclaimed_vecs);
        uint64_t index_iterator = reclaimed_vecs;

        // incase the compiler doesn't track this...
        IMPOSSIBLE_COND(index_iterator == 0);

        // another option is to this iteratively. For slabs will a lot
        // of frees this will be faster. As well it will reduce free
        // time as only 1 write will be necessary
        do {

            uint32_t idx = bits::find_first_one<uint64_t>(index_iterator);
            index_iterator &= (index_iterator - 1);

            uint64_t reclaimed_slots =
                freed_vecs_to_freed_slots(freed_vecs)[idx];
            atomic_unset(freed_vecs_to_freed_slots(freed_vecs) + idx,
                         reclaimed_slots);

            // since full in a sense we own all of the allocation vectors
            available_slots[idx] = reclaimed_slots;

        } while (index_iterator);

        available_vecs = reclaimed_vecs;
        return 0;
    }
};

template<uint64_t chunk_size>
struct obj_slab {
    static constexpr uint64_t vec_size = 8 * sizeof(uint64_t);
    static constexpr uint64_t num_vecs = obj_slab_traits<chunk_size>::num_slot_vecs;
    static constexpr uint64_t capacity = obj_slab_traits<chunk_size>::capacity;
    static constexpr uint64_t capacity_bytes = capacity * chunk_size;

    static constexpr uint64_t objects_offset =
        offsetof(obj_slab<chunk_size>, objects);
    
    


    // FULL is any value >= capacity

    // core owned memory
    obj_slab_base_data fixed_data;

    // allocation bits
    uint64_t available_slots[num_vecs];

    // all size classes will have [in_memory_chunk_size, next, available_vecs]
    // as first 24 bytes


    // shared memory

    uint64_t freed_vecs L2_LOAD_ALIGN;
    uint64_t            state;
    uint64_t            freed_slots[num_vecs];

    uint8_t objects[capacity_bytes] L2_LOAD_ALIGN;

    ~obj_slab() = default;
    obj_slab() : fixed_data(chunk_size) {
        // since initializing is necessary due to partial vectors for some size
        // classes might as well use 1s to indicate available so we can drop the
        // notq instructions in the assembly
        if constexpr (num_vecs * vec_size == capacity) {
            memset(available_slots, -1, num_vecs * sizeof(uint64_t));
        }
        else {
            memset(available_slots, -1, (num_vecs - 1) * sizeof(uint64_t));
            available_slots[num_vecs - 1] =
                ((1UL) << (vec_size * num_vecs - capacity)) - 1;
        }
        fixed_data.available_vecs = ((1UL) << num_vecs) - 1;
    }


    uint32_t
    _free(uint64_t addr_minus_start) {
        IMPOSSIBLE_COND(addr_minus_start < objects_offset);

        uint64_t position_idx =
            (addr_minus_start - objects_offset) / chunk_size;

        IMPOSSIBLE_COND(position_idx >= capacity);


        uint64_t vec_idx  = position_idx / num_vecs;
        uint64_t slot_idx = position_idx & (vec_size - 1);

        IMPOSSIBLE_COND(vec_idx >= vec_size);
        IMPOSSIBLE_COND(slot_idx >= vec_size);

        if (freed_slots[vec_idx] != 0) {
            atomic_bit_set(freed_slots + vec_idx, slot_idx);

            // definetly not first free so owned

            return 0;
        }
        else {
            atomic_bit_set(freed_slots + vec_idx, slot_idx);
            atomic_bit_set(&freed_vecs, vec_idx);

            // possible that this is unowned in which case the freeing
            // core will claim
            return state == obj_slab_base_data::UNOWNED;
        }
    }

    uint32_t
    _set_owned() {
        return atomic_bit_unset_ret((uint64_t *)(&state), 0);
    }

    void
    print_state_recap() {
        fprintf(stderr,
                "obj_slab {\n\t"
                "Start  : %p (%p %% %lu == %lu)\n\t"
                "State  : %x\n\t"
                "Avail  : 0x%016lx\n\t"
                "Free   : 0x%016lx\n\t"
                "}\n",
                this,
                this,
                sizeof(obj_slab<chunk_size>),
                ((uint64_t)this) % sizeof(obj_slab<chunk_size>),
                state,
                fixed_data.available_vecs,
                freed_vecs);
    }

    void
    print_state_full() {
        fprintf(stderr,
                "obj_slab {\n\t"
                "Start  : %p (%p %% %lu == %lu)\n\t"
                "Next   : %p\n\t"
                "State  : %lx\n\t"
                "Avail  : %016lx\n\n\t"
                "       \t[%016lx",
                this,
                this,
                sizeof(obj_slab<chunk_size>),
                ((uint64_t)this) % sizeof(obj_slab<chunk_size>),
                fixed_data.next,
                state,
                fixed_data.available_vecs,
                available_slots[0]);
        for (uint32_t i = 1; i < num_vecs; ++i) {

            if ((i % 4) == 0) {
                fprintf(stderr, "\n       \t\t ");
                fprintf(stderr, "%016lx", available_slots[i]);
            }
            else {
                fprintf(stderr, ", %016lx", available_slots[i]);
            }
        }
        fprintf(stderr,
                "]\n\n\t"
                "Free   : %016lx\n\n\t"
                "       \t[%016lx",
                freed_vecs,
                freed_slots[0]);
        for (uint32_t i = 1; i < num_vecs; ++i) {

            if ((i % 4) == 0) {
                fprintf(stderr, "\n       \t\t ");
                fprintf(stderr, "%016lx", freed_slots[i]);
            }
            else {
                fprintf(stderr, ", %016lx", freed_slots[i]);
            }
        }
        fprintf(stderr, "]\n\t}\n");
    }

} L2_LOAD_ALIGN;


template<uint64_t chunk_size>
void
_call_new(void * dst) noexcept {
    (new (dst) obj_slab<chunk_size>());
}

typedef void (*new_wrapper)(void *);
static constexpr new_wrapper call_new[num_size_classes] = {
    &_call_new<8>,   &_call_new<16>,  &_call_new<24>,  &_call_new<32>,
    &_call_new<48>,  &_call_new<64>,  &_call_new<80>,  &_call_new<96>,
    &_call_new<112>, &_call_new<128>, &_call_new<144>, &_call_new<160>,
    &_call_new<176>, &_call_new<192>, &_call_new<208>, &_call_new<224>,
    &_call_new<240>, &_call_new<256>
};


#undef OBJ_SLAB_ASSERT

#endif
