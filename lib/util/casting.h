#ifndef _CASTING_H_
#define _CASTING_H_

#include <misc/cpp_attributes.h>
#include <stdint.h>

namespace cast {

// bitcasting floats to ints and vice versa
uint32_t ALWAYS_INLINE CONST_ATTR
float_to_int(float f) {
    uint32_t i;
    asm("vmovd %[f], %[i]\n\t" : [ i ] "=r"(i) : [ f ] "x"(f) :);
    return i;
}

uint64_t ALWAYS_INLINE CONST_ATTR
float_to_int(double f) {
    uint64_t i;
    asm("vmovq %[f], %[i]\n\t" : [ i ] "=r"(i) : [ f ] "x"(f) :);
    return i;
}

float ALWAYS_INLINE CONST_ATTR
int_to_float(uint32_t i) {
    float f;
    asm("vmovd %[i], %[f]\n\t" : [ f ] "=x"(f) : [ i ] "r"(i) :);
    return f;
}

double ALWAYS_INLINE CONST_ATTR
int_to_float(uint64_t i) {
    double f;
    asm("vmovq %[i], %[f]\n\t" : [ f ] "=x"(f) : [ i ] "r"(i) :);
    return f;
}


}  // namespace cast


#endif
