#ifndef _BITS_H_
#define _BITS_H_
#include <stdint.h>
#include <x86intrin.h>
#include <type_traits>


#include <misc/cpp_attributes.h>


namespace bits {

template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
bitcount(const T v) {
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        return _mm_popcnt_u64(v);
    }
    else {
        return _mm_popcnt_u32(v);
    }
}


template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
tzcnt(const T v) {
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        // 64 if v == 0
        return _tzcnt_u64(v);
    }
    else {
        // 32 if v == 0;
        return _tzcnt_u32(v);
    }
}

template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
lzcnt(const T v) {
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        // 64 if v == 0
        return _lzcnt_u64(v);
    }
    else {
        // 32 if v == 0
        return _lzcnt_u32(v);
    }
}

template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
pdep(const T mask, const T v) {
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        return _pdep_u64(mask, v);
    }
    else {
        return _pdep_u32(mask, v);
    }
}


template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
find_first_one(const T v) {
    return tzcnt<T>(v);
}

template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
find_first_zero(const T v) {
    return tzcnt<T>(~v);
}

// return -1 if empty
template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
find_last_one(const T v) {
    return (8 * sizeof(T) - 1) - lzcnt<T>(v);
}

template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
find_last_zero(const T v) {
    return (8 * sizeof(T) - 1) - lzcnt<T>(~v);
}


// the bswap might be better off being replace with movbe asm (if it ever
// becomes important)
template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
bswap(const T v) {
    if constexpr (sizeof(T) == sizeof(uint16_t)) {
        return __builtin_bswap16(v);
    }
    else if constexpr (sizeof(T) == sizeof(uint32_t)) {
        return __builtin_bswap32(v);
    }
    else {
        return __builtin_bswap64(v);
    }
}


template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
next_p2(const T v) {
    return v == 1 ? 0 : (((T)1) << (8 * sizeof(T) - lzcnt<T>(v - 1)));
}


template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
to_mask(const T v) {
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        return ((1UL) << v) - 1;
    }
    else {
        return (1 << v) - 1;
    }
}

template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
nth_bit(const T v, const T n) {
    return (v >> n) & 0x1;
}

template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
get_nth_set_bit(const T v, const T n) {
    return tzcnt<T>(pdep<T>(((T)1) << n, v));
}

template<typename T>
constexpr T ALWAYS_INLINE CONST_ATTR
get_nth_unset_bit(const T v, const T n) {
    return tzcnt<T>(pdep<T>(((T)1) << n, ~v));
}

}  // namespace bits
#endif
