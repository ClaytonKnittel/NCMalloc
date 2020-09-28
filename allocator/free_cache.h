#ifndef _FREE_CACHE_H_
#define _FREE_CACHE_H_

#include <concurrency/rseq/rseq_base.h>

#include <misc/cpp_attributes.h>
#include <system/sys_info.h>


template<uint32_t cache_size>
struct free_cache {
    uint32_t current_idx;
    uint64_t ptrs[cache_size];

};

#endif
