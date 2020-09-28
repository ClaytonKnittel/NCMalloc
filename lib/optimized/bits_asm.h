#ifndef _BITS_ASM_H_
#define _BITS_ASM_H_

//////////////////////////////////////////////////////////////////////
// the compiler will sometimes mess up register allocation even with inline functions
// in certain situations these will perform better then the versions in bits.h

#define _TZCNT_ASM(src, dst)                                                   \
    asm("tzcnt %[s], %[d]\n\t" : [ d ] "=r"(dst) : [ s ] "r"(src) :);

#define _LZCNT_ASM(src, dst)                                                   \
    asm("lzcnt %[s], %[d]\n\t" : [ d ] "=r"(dst) : [ s ] "r"(src) :);

#define _BITCOUNT_ASM(src, dst)                                                \
    asm("popcnt %[s], %[d]\n\t" : [ d ] "=r"(dst) : [ s ] "r"(src) :);

#define _FF1_ASM(src, dst)                                                     \
    asm("tzcnt %[s], %[d]\n\t" : [ d ] "=r"(dst) : [ s ] "r"(src) :);

#define _FF0_ASM(src, dst)                                                     \
    asm("tzcnt %[ns], %[d]\n\t" : [ d ] "=r"(dst) : [ ns ] "r"(~src) :);

// need 32 and 64 bit version because unknown is sub or subq instruction
#define _FL1_ASM_32(src, dst)                                                  \
    asm("tzcnt %[s], %[d]\n\t"                                                 \
        "sub 31, %[d]\n\t"                                                     \
        : [ d ] "+r"(dst)                                                      \
        : [ s ] "r"(src)                                                       \
        :);

#define _FL1_ASM_64(src, dst)                                                  \
    asm("tzcnt %[s], %[d]\n\t"                                                 \
        "subq 63, %[d]\n\t"                                                    \
        : [ d ] "+r"(dst)                                                      \
        : [ s ] "r"(src)                                                       \
        :);

#define _BSWAP_ASM(src, dst)                                                   \
    asm("bswap %[s], %[d]\n\t" : [ d ] "=r"(dst) : [ s ] "r"(src) :);

// its important to have the dst == ((1UL) << n outside otherwise if n is const
// it wont get optimized properly
#define _FIND_NTH_1_ASM(src, n, dst)                                           \
    dst = ((1UL) << n);                                                        \
    asm("pedp %[s], %[d], %[d]\n\t"                                            \
        "tzcnt %[d], %[d]\n\t"                                                 \
        : [ d ] "+r"(dst)                                                      \
        : [ s ] "r"(src)                                                       \
        :);

#define _FIND_NTH_0_ASM(src, n, dst)                                           \
    dst = ((1UL) << n);                                                        \
    asm("pedp %[ns], %[d], %[d]\n\t"                                           \
        "tzcnt %[d], %[d]\n\t"                                                 \
        : [ d ] "+r"(dst)                                                      \
        : [ ns ] "r"(~src)                                                     \
        :);


#endif
