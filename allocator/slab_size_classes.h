#ifndef _SLAB_SIZE_CLASSES_H_
#define _SLAB_SIZE_CLASSES_H_

#include <stdint.h>
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define CONST_ATTR    __attribute__((const))

static constexpr uint32_t num_size_classes             = 11;
static constexpr uint32_t slab_sizes[num_size_classes] = { 8,   16,  24, 32,
                                                           48,  64,  80, 96,
                                                           112, 128, 144 };

// todo, test performance of no branch options
constexpr uint32_t ALWAYS_INLINE CONST_ATTR
idx_to_size(const uint32_t idx) {
    return slab_sizes[idx];
}

constexpr uint32_t ALWAYS_INLINE CONST_ATTR
size_to_idx(const uint32_t size) {
    return (size <= 24) ? (((size + 7) / 8) - 1) : (((size + 15) / 16) + 1);
}

constexpr uint32_t ALWAYS_INLINE CONST_ATTR
size_to_idx_nb(const uint32_t size) {
    uint32_t base = 3;
    if (size > 24) {
        ++base;
    }
    return ((size + ((1 << base) - 1)) >> base) + (2 * (base / 2)) - 3;
}

constexpr uint32_t ALWAYS_INLINE CONST_ATTR
round_size(const uint32_t size) {
    return (size <= 24) ? ((size + 7) & (~7)) : ((size + 15) & (~15));
}

constexpr uint32_t ALWAYS_INLINE CONST_ATTR
round_size_nb(const uint32_t size) {
    uint32_t base = 8;
    if (size > 24) {
        base = 2 * base;
    }
    return size & (~(base - 1));
}


#endif
