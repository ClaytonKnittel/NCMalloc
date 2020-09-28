#ifndef _SYS_INFO_H_
#define _SYS_INFO_H_
#define HAVE_CONST_SYS_INFO

#include <stdint.h>

// Number of processors (this is cores includes hyperthreads)
#define NPROCS 8

// Number of physical cores
#define NCORES 4

// Logical to physical core lookup
#define PHYS_CORE(X) get_phys_core(X)
uint32_t inline __attribute__((always_inline)) __attribute__((const))
get_phys_core(const uint32_t logical_core_num) {
	static const constexpr uint32_t core_map[NPROCS] = { 0, 1, 2, 3, 0, 1, 2, 3 };
	return core_map[logical_core_num];
}

// Transparent Huge Page Configuration
enum THP { ALWAYS = 0, MADVISE = 1, NEVER = 2 };
#define TRANSPARENT_HUGE_PAGE_CFG THP::MADVISE

// Virtual memory page size
#define PAGE_SIZE 4096

// Virtual memory address space bits
#define VM_NBITS 48

// Physical memory address space bits
#define PHYS_M_NBITS 39

// Physical memory page offset (for vm -> pm) for kmalloc
#define PAGE_OFFSET (0xffff888000000000UL)

// Where physical memory of kernel (mostly for debugging kmalloc stuff)
#define KERNEL_MEM_START (0xffffffff80000000UL)

// cache line size (should be same for L1, L2, and L3)
#define CACHE_LINE_SIZE 64

// l2 cache loads double cache lines at a time
#define L2_CACHE_LOAD_SIZE 128


//////////////////////////////////////////////////////////////////////
// More specific cache info starts here
//////////////////////////////////////////////////////////////////////

#define L1_DCACHE_SIZE 32768
#define L1_DCACHE_SETS 64
#define L1_DCACHE_ASSOS 8

#define L1_ICACHE_SIZE 32768
#define L1_ICACHE_SETS 64
#define L1_ICACHE_ASSOS 8

#define L2_UCACHE_SIZE 262144
#define L2_UCACHE_SETS 1024
#define L2_UCACHE_ASSOS 4

#define L3_UCACHE_SIZE 8388608
#define L3_UCACHE_SETS 8192
#define L3_UCACHE_ASSOS 16

#endif
