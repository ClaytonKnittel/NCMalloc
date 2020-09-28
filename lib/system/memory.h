#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <misc/cpp_attributes.h>
#include <misc/macro_helper.h>
#include <optimized/bits.h>
#include <system/sys_info.h>

// for kmalloc this converts vm -> pm
constexpr uint64_t ALWAYS_INLINE CONST_ATTR
vm_to_pm(const uint64_t vm_addr) {
    return vm_addr - PAGE_OFFSET;
}


constexpr uint32_t ALWAYS_INLINE CONST_ATTR
l1_dcache_set(const uint64_t addr) {
    return (addr / CACHE_LINE_SIZE) & (L1_DCACHE_SETS - 1);
}

constexpr uint32_t ALWAYS_INLINE CONST_ATTR
l1_icache_set(const uint64_t addr) {
    return (addr / CACHE_LINE_SIZE) & (L1_ICACHE_SETS - 1);
}


constexpr uint32_t ALWAYS_INLINE CONST_ATTR
l2_ucache_set(const uint64_t addr) {
    return (addr / CACHE_LINE_SIZE) & (L2_UCACHE_SETS - 1);
}

constexpr uint32_t ALWAYS_INLINE CONST_ATTR
l3_ucache_set(const uint64_t addr) {
    return (addr / CACHE_LINE_SIZE) & (L3_UCACHE_SETS - 1);
}


//////////////////////////////////////////////////////////////////////
// true constexpr
constexpr uint32_t
l1_dcache_critical_stride() {
    return L1_DCACHE_SIZE / (L1_DCACHE_ASSOS * L1_DCACHE_SETS);
}

constexpr uint32_t
l1_icache_critical_stride() {
    return L1_ICACHE_SIZE / (L1_ICACHE_ASSOS * L1_ICACHE_SETS);
}

constexpr uint32_t
l2_ucache_critical_stride() {
    return L2_UCACHE_SIZE / (L2_UCACHE_ASSOS * L2_UCACHE_SETS);
}

constexpr uint32_t
l3_ucache_critical_stride() {
    return L3_UCACHE_SIZE / (L3_UCACHE_ASSOS * L3_UCACHE_SETS);
}



#endif
