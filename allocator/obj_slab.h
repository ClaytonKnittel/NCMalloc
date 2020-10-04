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

#define OBJ_SLAB_ASSERT(X) assert(X)

//#define CACHE_MOST_RECENT

#ifdef CACHE_MOST_RECENT
#define OBJ_SLAB_NEXT_OFFSET 528
#else
#define OBJ_SLAB_NEXT_OFFSET 520
#endif


struct obj_slab {


    // constants
    static constexpr uint64_t vec_size       = 64;
    static constexpr uint64_t num_vecs       = 64;
    static constexpr uint64_t capacity       = num_vecs * vec_size;
    static constexpr uint64_t payload_size   = capacity * 8;
    static constexpr uint64_t next_offset    = 520;
    static constexpr uint64_t payload_offset = 1280;

    enum { OWNED = 0, UNOWNED = 1 };

    enum noalias_byte : uint8_t {};


    // FULL is any value >= capacity
    enum FAILURE { MIGRATED = capacity, FULL = capacity + 1 };
    static constexpr uint64_t SUCCESS_BOUND = FAILURE::MIGRATED;


    // allocation bits
    uint64_t available_vecs;
    uint64_t available_slots[num_vecs];


    // next obj_slab for whatever list its in. This is modified by owning core
    obj_slab * next;

    // core owned memory
    const uint32_t block_size;


    // shared memory

    // state is primarily modified in conjunction with freed_vecs so best to
    // store together.


    // freeing bits

    uint64_t freed_vecs L2_LOAD_ALIGN;
    uint64_t            state;
    uint64_t            freed_slots[num_vecs];

    noalias_byte payload[payload_size] L2_LOAD_ALIGN;


    ~obj_slab() = default;
    obj_slab(const uint32_t _block_size) : block_size(_block_size) {
        const uint32_t nblocks = payload_size / _block_size;
        const uint32_t nslots  = (nblocks + 63) / 64;


        available_vecs = ((nslots >= 64) ? 0 : ((1UL) << nslots)) - 1;
                
        // fill all slots but last one that may start partially full
        memset(available_slots, -1, (nslots - 1) * sizeof(uint64_t));

        // calculate partial fullness and set
        if (nblocks % 64) {
            available_slots[nslots - 1] = ((1UL) << (nblocks % 64)) - 1;
        }
        else {
            available_slots[nslots - 1] = ~(0UL);
        }
    }


    uint64_t
    _allocate(const uint32_t start_cpu) {

        OBJ_SLAB_ASSERT((((uint64_t)this) % sizeof(obj_slab)) == 0);
        OBJ_SLAB_ASSERT((((uint64_t)this) % sizeof(obj_slab)) == 0);
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
            "movq (%[_this]), %[temp_av]\n\t"
            
            "jmp 5f\n\t"

            "7:\n\t"

            // this appears to be faster than:
            "blsrq %[temp_av], %[temp_av]\n\t"

        
            // this is a completely valid state to be migrated out after
            // (all we have really done is cleaned up v1 vector a bit)
            // because we can be migrated out here we don't check/set if
            // temp_as is full as that could lead to invalid state in v1
            "movq %[temp_av], (%[_this])\n\t"

            
            "5:\n\t"
            // this is } in while loop starting at 5:

                
            // idx = temp_v
            "mov %[FULL], %[idx]\n\t"
        
            // The reason we can't do this after notq %[idx] is because
            // 0 is a valid idx to return whereas -1 is not
            // (also why setting idx before the comparison)
            "testq %[temp_av], %[temp_av]\n\t"
            "jz 9f\n\t"
        
            // idx_av = tzcnt(idx) (find first one)
            "tzcntq %[temp_av], %[idx_av]\n\t"

            // temp_as = v[idx_av + 1]
            "movq 8(%[_this], %[idx_av], 8), %[temp_as]\n\t"

            // test if temp_as is full
            "testq %[temp_as], %[temp_as]\n\t"
            "jz 7b\n\t" // 7b is btsq %[idx_outer], %[temp_av], fall into 5b

            "movq %[temp_as], %[idx]\n\t"
            "blsrq %[temp_as], %[temp_as]\n\t"
                       
            // commit
            "movq %[temp_as], 8(%[_this], %[idx_av], 8)\n\t"
            

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
              [ MIGRATED ] "i" (MIGRATED),
              [ FULL ] "i" (FULL),
              [ start_cpu ] "r" (start_cpu)
            : "cc");
        // clang-format on

        return idx;
    }

    // this function is quasi atomic.
    uint32_t
    _try_release() {

        uint64_t reclaimed_vecs = freed_vecs;


        if (reclaimed_vecs == 0) {
            // if nothing free this is unowned memory and will be
            // claimed by first free

            OBJ_SLAB_ASSERT(state == OWNED);
            state = UNOWNED;
            return 1;
        }

        atomic_unset(&freed_vecs, reclaimed_vecs);
        uint64_t index_iterator = reclaimed_vecs;

        // incase the compiler doesn't track this...
        IMPOSSIBLE_COND(index_iterator == 0);

        // another option is to this iteratively. For slabs will a lot
        // of frees this will be faster. As well it will reduce free
        // time as only 1 write will be necessary
        do {

            uint32_t idx = bits::find_first_one<uint64_t>(index_iterator);
            index_iterator &= (index_iterator - 1);
            IMPOSSIBLE_COND(idx >= 64);

            uint64_t reclaimed_slots = freed_slots[idx];
            atomic_unset(freed_slots + idx, reclaimed_slots);

            // since full in a sense we own all of the allocation vectors
            available_slots[idx] = reclaimed_slots;

        } while (index_iterator);

        available_vecs = reclaimed_vecs;

        return 0;
    }


    uint32_t
    _free(uint64_t addr_minus_start) {
        IMPOSSIBLE_COND(addr_minus_start < payload_offset);
        IMPOSSIBLE_COND(addr_minus_start - payload_offset >= payload_size);

        // div instruction :(
        uint64_t position_idx =
            (addr_minus_start - payload_offset) / block_size;

        IMPOSSIBLE_COND(position_idx >= capacity);


        uint64_t vec_idx  = position_idx / num_vecs;
        uint64_t slot_idx = position_idx & (vec_size - 1);

        IMPOSSIBLE_COND(vec_idx >= 64);
        IMPOSSIBLE_COND(slot_idx >= 64);


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
            return state == UNOWNED;
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
                "State  : %lx\n\t"
                "Avail  : 0x%016lx\n\t"
                "Free   : 0x%016lx\n\t"
                "}\n",
                this,
                this,
                sizeof(obj_slab),
                ((uint64_t)this) % sizeof(obj_slab),
                state,
                available_vecs,
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
                sizeof(obj_slab),
                ((uint64_t)this) % sizeof(obj_slab),
                next,
                state,
                available_vecs,
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

static_assert(obj_slab::next_offset == offsetof(obj_slab, next));
static_assert(obj_slab::payload_offset == offsetof(obj_slab, payload));

#undef OBJ_SLAB_ASSERT

#endif
