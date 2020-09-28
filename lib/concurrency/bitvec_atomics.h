#ifndef _BITVEC_ATOMICS_H_
#define _BITVEC_ATOMICS_H_

#include <misc/cpp_attributes.h>
#include <stdint.h>

static void ALWAYS_INLINE
atomic_xor(uint64_t * const v_loc, const uint64_t xor_bits) {
    __atomic_fetch_xor(v_loc, xor_bits, __ATOMIC_RELAXED);
}

static void ALWAYS_INLINE
atomic_or(uint64_t * const v_loc, const uint64_t or_bits) {
    __atomic_fetch_or(v_loc, or_bits, __ATOMIC_RELAXED);
}

static void ALWAYS_INLINE
atomic_and(uint64_t * const v_loc, const uint64_t and_bits) {
    __atomic_fetch_and(v_loc, and_bits, __ATOMIC_RELAXED);
}

static void ALWAYS_INLINE
atomic_unset(uint64_t * const v_loc, const uint64_t unset_bits) {
    __atomic_fetch_and(v_loc, ~unset_bits, __ATOMIC_RELAXED);
}

static void ALWAYS_INLINE
atomic_bit_set(uint64_t * const v_loc, const uint64_t set_bit) {
    __atomic_fetch_or(v_loc, (1UL) << set_bit, __ATOMIC_RELAXED);
}


static uint32_t ALWAYS_INLINE
atomic_bit_set_ret(uint64_t * const v_loc, const uint64_t bit) {
    uint8_t ret = 0;
    asm volatile("lock; btsq %[bit], (%[v]); setc %b[ret]\n\t"
                 : [ ret ] "=r"(ret),  [ v_clobber ] "=m"(*v_loc)
                 : [ bit ] "ri"(bit), [ v ] "r"(v_loc)
                 : "flags");
    return ret;
}

static uint32_t ALWAYS_INLINE
atomic_bit_unset_ret(uint64_t * const v_loc, const uint64_t bit) {
    uint8_t ret = 0;

    asm volatile("lock; btrq %[bit], (%[v]); setc %b[ret]\n\t"
                 : [ ret ] "=r"(ret), [ v_clobber ] "=m"(*v_loc)
                 : [ bit ] "ri"(bit), [ v ] "r"(v_loc)
                 : "flags");
    return ret;
}

static uint32_t ALWAYS_INLINE
atomic_bit_swap_ret(uint64_t * const v_loc, const uint64_t bit) {
    uint8_t ret = 0;
    asm volatile("lock; btcq %[bit], (%[v]); setc %b[ret]\n\t"
                 : [ ret ] "=r"(ret), [ v_clobber ] "=m"(*v_loc)
                 : [ bit ] "ri"(bit), [ v ] "r"(v_loc)
                 : "flags");
    return ret;
}


#endif
