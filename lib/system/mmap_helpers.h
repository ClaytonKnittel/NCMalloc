#ifndef _MMAP_HELPERS_H_
#define _MMAP_HELPERS_H_

#include <sys/mman.h>

#include <misc/error_handling.h>
#include <system/sys_info.h>

namespace MMAP {

#define mmap_alloc_hugepage(length)                                            \
    MMAP::_mmap_hugepage(NULL,                                                 \
                         length,                                               \
                         (PROT_READ | PROT_WRITE),                             \
                         (MAP_ANONYMOUS | MAP_PRIVATE),                        \
                         (-1),                                                 \
                         0,                                                    \
                         __FILE__,                                             \
                         __LINE__)


#define mmap_alloc_reserve(length)                                             \
    safe_mmap(NULL,                                                            \
              length,                                                          \
              (PROT_READ | PROT_WRITE),                                        \
              (MAP_ANONYMOUS | MAP_PRIVATE),                                   \
              (-1),                                                            \
              0)

#define mmap_alloc_noreserve(length)                                           \
    safe_mmap(NULL,                                                            \
              length,                                                          \
              (PROT_READ | PROT_WRITE),                                        \
              (MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE),                   \
              (-1),                                                            \
              0)

#define mmap_alloc_pagein_reserve(length, npages)                              \
    MMAP::mmap_pagein(                                                         \
        (uint8_t * const)safe_mmap(NULL,                                       \
                                   length,                                     \
                                   (PROT_READ | PROT_WRITE),                   \
                                   (MAP_ANONYMOUS | MAP_PRIVATE),              \
                                   (-1),                                       \
                                   0),                                         \
        npages)

#define mmap_alloc_pagein_noreserve(length, npages)                            \
    MMAP::mmap_pagein((uint8_t * const)safe_mmap(                              \
                          NULL,                                                \
                          length,                                              \
                          (PROT_READ | PROT_WRITE),                            \
                          (MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE),       \
                          (-1),                                                \
                          0),                                                  \
                      npages)


#define safe_mmap(addr, length, prot_flags, mmap_flags, fd, offset)            \
    MMAP::_safe_mmap((addr),                                                   \
                     (length),                                                 \
                     (prot_flags),                                             \
                     (mmap_flags),                                             \
                     (fd),                                                     \
                     (offset),                                                 \
                     __FILE__,                                                 \
                     __LINE__)

#define safe_munmap(addr, length)                                              \
    MMAP::_safe_munmap(addr, length, __FILE__, __LINE__)


#define weak_ram_range(addr, length)                                           \
    MMAP::_weak_madvise(addr, length, MADV_RANDOM, __FILE__, __LINE__);

#define strong_ram_range(addr, length)                                         \
    MMAP::_strong_madvise(addr, length, MADV_RANDOM, __FILE__, __LINE__);


#define weak_prepage(addr, length)                                             \
    MMAP::_weak_madvise(addr, length, MADV_WILLNEED, __FILE__, __LINE__);

#define strong_prepage(addr, length)                                           \
    MMAP::_strong_madvise(addr, length, MADV_WILLNEED, __FILE__, __LINE__);


#define weak_hugepage(addr, length)                                            \
    MMAP::_weak_madvise(addr, length, MADV_HUGEPAGE, __FILE__, __LINE__);

#define strong_hugepage(addr, length)                                          \
    MMAP::_strong_madvise(addr, length, MADV_HUGEPAGE, __FILE__, __LINE__);

#define madv_free(addr, length)                                                \
    MMAP::_strong_madvise(addr, length, MADV_DONTNEED, __FILE__, __LINE__);


void *
_safe_mmap(void *        addr,
           uint64_t      length,
           int32_t       prot_flags,
           int32_t       mmap_flags,
           int32_t       fd,
           int32_t       offset,
           const char *  fname,
           const int32_t ln) {
    void * p = mmap(addr, length, prot_flags, mmap_flags, fd, offset);
    ERROR_ASSERT(p != MAP_FAILED,
                 "%s:%d mmap(%p, %lu...) failed\n",
                 fname,
                 ln,
                 addr,
                 length);
    return p;
}


void
_safe_munmap(void *        addr,
             uint64_t      length,
             const char *  fname,
             const int32_t ln) {
    ERROR_ASSERT(munmap(addr, length) != -1,
                 "%s:%d munmap(%p, %lu) failed\n",
                 fname,
                 ln,
                 addr,
                 length);
}

void
_strong_madvise(void *        addr,
                uint64_t      length,
                uint32_t      advise,
                const char *  fname,
                const int32_t ln) {
    int32_t r;
    do {
        r = madvise(addr, length, advise);
    } while (r == (-1) && errno == EAGAIN);
    ERROR_ASSERT(r == 0,
                 "%s:%d madvise(%p, %lu, %x) failed\n",
                 fname,
                 ln,
                 addr,
                 length,
                 advise);
}

void
_weak_madvise(void *        addr,
              uint64_t      length,
              int32_t       advise,
              const char *  fname,
              const int32_t ln) {
    int32_t r = madvise(addr, length, advise);
    ERROR_ASSERT((r != (-1)) || errno == EAGAIN,
                 "%s:%d madvise(%p, %lu, %x) failed\n",
                 fname,
                 ln,
                 addr,
                 length,
                 advise);
}

void *
_mmap_hugepage(void *        addr,
               uint64_t      length,
               int32_t       prot_flags,
               int32_t       mmap_flags,
               int32_t       fd,
               int32_t       offset,
               const char *  fname,
               const int32_t ln) {
    void * p =
        mmap(addr, length, prot_flags, mmap_flags | MAP_HUGETLB, fd, offset);
    if (p == MAP_FAILED) {
        p = mmap(addr, length, prot_flags, mmap_flags, fd, offset);
        ERROR_ASSERT(p != MAP_FAILED, "%s:%d second mmap failed\n", fname, ln);

        _weak_madvise(p, length, MADV_HUGEPAGE, fname, ln);
    }
    return p;
}


}  // namespace MMAP

#endif
