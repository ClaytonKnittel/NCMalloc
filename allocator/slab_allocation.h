#ifndef _SLAB_ALLOCATION_H_
#define _SLAB_ALLOCATION_H_

#include <concurrency/rseq/rseq_base.h>
#include <misc/cpp_attributes.h>
#include <optimized/const_math.h>
#include <system/sys_info.h>


namespace new_memory {
enum FAILURE { MIGRATED = 1 };

template<typename slab_t>
struct shared_memory_slab_allocator {
    uint64_t current_slab;

    // this is done at initialization so no need to optimize.
    // Basically we just want current_slab to be aligned to
    // sizeof(slab) so that free can find its slab with modulo
    shared_memory_slab_allocator(uint64_t mem_region)
        : current_slab(mem_region) {}

    uint32_t ALWAYS_INLINE PURE_ATTR
    out_of_memory(uint64_t end) const {
        return (current_slab >= end);
    }

    slab_t *
    _new() {
        return (slab_t *)__atomic_fetch_add(&current_slab,
                                            sizeof(slab_t),
                                            __ATOMIC_RELAXED);
    }
};


}  // namespace new_memory

#endif
