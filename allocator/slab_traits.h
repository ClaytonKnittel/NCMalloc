#ifndef _SLAB_TRAITS_H_
#define _SLAB_TRAITS_H_

#include <misc/cpp_attributes.h>

static constexpr uint64_t slab_size = 32768;

// 8,16,24,32,48,64,80,96,112,128,144,160,176,192,208,224,240,256
static constexpr uint64_t num_size_classes = 18;


template<uint64_t chunk_size>
constexpr uint64_t ALWAYS_INLINE CONST_ATTR
consteval_size_to_idx() {
    if constexpr (chunk_size == 8) {
        return 0;
    }
    else if constexpr (chunk_size == 16) {
        return 1;
    }
    else if constexpr (chunk_size == 24) {
        return 2;
    }
    else {
        return 1 + (chunk_size / 16);
    }
}

constexpr uint64_t ALWAYS_INLINE CONST_ATTR
size_to_idx(const uint64_t size) {
    return (size < 32) ? ((size / 8) - 1) : ((size / 16) + 1);
}

constexpr uint64_t ALWAYS_INLINE CONST_ATTR
round_size(const uint64_t size) {
    return (size < 32) ? ((size + 7) & (~(7UL))) : ((size + 15) & (~(15UL)));
}

static constexpr uint64_t size_idx_to_meta_data[num_size_classes] = {
    1152, 768, 512, 512, 256, 256, 256, 256, 256,
    256,  256, 256, 256, 256, 256, 256, 256, 256
};

static constexpr uint64_t size_idx_to_free_offset[num_size_classes] = {
    640, 384, 256, 256, 128, 128, 128, 128, 128,
    128, 128, 128, 128, 128, 128, 128, 128, 128
};

static constexpr uint64_t size_idx_to_capacity[num_size_classes] = {
    3952, 2000, 1344, 1008, 677, 508, 406, 338, 290,
    254,  225,  203,  184,  165, 156, 145, 135, 127
};


template<uint64_t chunk_size>
struct obj_slab_traits;


// these smaller sizes have a bit less trivial math because the meta data size
// is variable as well so just hard coding for now at least
template<>
struct obj_slab_traits<8> {
    static constexpr uint64_t capacity       = 3952;
    static constexpr uint64_t num_slot_vecs  = 62;
    static constexpr uint64_t meta_data_size = 1152;
};


template<>
struct obj_slab_traits<16> {
    static constexpr uint64_t capacity       = 2000;
    static constexpr uint64_t num_slot_vecs  = 32;
    static constexpr uint64_t meta_data_size = 768;
};

template<>
struct obj_slab_traits<24> {
    static constexpr uint64_t capacity       = 1344;
    static constexpr uint64_t num_slot_vecs  = 21;
    static constexpr uint64_t meta_data_size = 512;
};

template<>
struct obj_slab_traits<32> {
    static constexpr uint64_t capacity       = 1008;
    static constexpr uint64_t num_slot_vecs  = 16;
    static constexpr uint64_t meta_data_size = 512;
};

template<uint64_t chunk_size>
struct obj_slab_traits {
    static constexpr uint64_t capacity       = (slab_size - 256) / chunk_size;
    static constexpr uint64_t num_slot_vecs  = (capacity + 63) / 64;
    static constexpr uint64_t meta_data_size = 256;
};


// Asserts generate by: ./generate_asserts.py

// asserting size is within permitted bound
// static_assert(obj_slab_traits<[SIZE]>::capacity * [SIZE] +
// obj_slab_traits<[SIZE]>::meta_data_size <= slab_size);

// asserting num_slot_vecs has enough indexes for capacity
// static_assert(obj_slab_traits<[SIZE]>::capacity / 64 <=
// obj_slab_traits<[SIZE]>::num_slot_vecs);

// asserting calculate maximum possible capacity
// static_assert((slab_size - (obj_slab_traits<[SIZE]>::capacity * [SIZE] +
// obj_slab_traits<[SIZE]>::meta_data_size)) < [SIZE]);

// asserting meta_data_size indeed represents all meta data (note + 3 is only
// for available but this is being safe)
// static_assert(obj_slab_traits<[SIZE]>::meta_data_size >= (L2_CACHE_LOAD_SIZE
// * (((L2_CACHE_LOAD_SIZE - 1) + (8 * (2 +
// obj_slab_traits<[SIZE]>::num_slot_vecs))) / L2_CACHE_LOAD_SIZE)) +
// (L2_CACHE_LOAD_SIZE * (((L2_CACHE_LOAD_SIZE - 1) + (8 * (3 +
// obj_slab_traits<[SIZE]>::num_slot_vecs))) / L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<8>::capacity * 8 +
                  obj_slab_traits<8>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<8>::capacity / 64 <=
              obj_slab_traits<8>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<8>::capacity * 8 +
                            obj_slab_traits<8>::meta_data_size)) < 8);
static_assert(
    obj_slab_traits<8>::meta_data_size >=
    (L2_CACHE_LOAD_SIZE * (((L2_CACHE_LOAD_SIZE - 1) +
                            (8 * (2 + obj_slab_traits<8>::num_slot_vecs))) /
                           L2_CACHE_LOAD_SIZE)) +
        (L2_CACHE_LOAD_SIZE * (((L2_CACHE_LOAD_SIZE - 1) +
                                (8 * (3 + obj_slab_traits<8>::num_slot_vecs))) /
                               L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<16>::capacity * 16 +
                  obj_slab_traits<16>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<16>::capacity / 64 <=
              obj_slab_traits<16>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<16>::capacity * 16 +
                            obj_slab_traits<16>::meta_data_size)) < 16);
static_assert(obj_slab_traits<16>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<16>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<16>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<32>::capacity * 32 +
                  obj_slab_traits<32>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<32>::capacity / 64 <=
              obj_slab_traits<32>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<32>::capacity * 32 +
                            obj_slab_traits<32>::meta_data_size)) < 32);
static_assert(obj_slab_traits<32>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<32>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<32>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<48>::capacity * 48 +
                  obj_slab_traits<48>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<48>::capacity / 64 <=
              obj_slab_traits<48>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<48>::capacity * 48 +
                            obj_slab_traits<48>::meta_data_size)) < 48);
static_assert(obj_slab_traits<48>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<48>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<48>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<64>::capacity * 64 +
                  obj_slab_traits<64>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<64>::capacity / 64 <=
              obj_slab_traits<64>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<64>::capacity * 64 +
                            obj_slab_traits<64>::meta_data_size)) < 64);
static_assert(obj_slab_traits<64>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<64>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<64>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<80>::capacity * 80 +
                  obj_slab_traits<80>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<80>::capacity / 64 <=
              obj_slab_traits<80>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<80>::capacity * 80 +
                            obj_slab_traits<80>::meta_data_size)) < 80);
static_assert(obj_slab_traits<80>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<80>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<80>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<96>::capacity * 96 +
                  obj_slab_traits<96>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<96>::capacity / 64 <=
              obj_slab_traits<96>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<96>::capacity * 96 +
                            obj_slab_traits<96>::meta_data_size)) < 96);
static_assert(obj_slab_traits<96>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<96>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<96>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<112>::capacity * 112 +
                  obj_slab_traits<112>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<112>::capacity / 64 <=
              obj_slab_traits<112>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<112>::capacity * 112 +
                            obj_slab_traits<112>::meta_data_size)) < 112);
static_assert(obj_slab_traits<112>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<112>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<112>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<128>::capacity * 128 +
                  obj_slab_traits<128>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<128>::capacity / 64 <=
              obj_slab_traits<128>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<128>::capacity * 128 +
                            obj_slab_traits<128>::meta_data_size)) < 128);
static_assert(obj_slab_traits<128>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<128>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<128>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<144>::capacity * 144 +
                  obj_slab_traits<144>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<144>::capacity / 64 <=
              obj_slab_traits<144>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<144>::capacity * 144 +
                            obj_slab_traits<144>::meta_data_size)) < 144);
static_assert(obj_slab_traits<144>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<144>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<144>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<160>::capacity * 160 +
                  obj_slab_traits<160>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<160>::capacity / 64 <=
              obj_slab_traits<160>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<160>::capacity * 160 +
                            obj_slab_traits<160>::meta_data_size)) < 160);
static_assert(obj_slab_traits<160>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<160>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<160>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<176>::capacity * 176 +
                  obj_slab_traits<176>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<176>::capacity / 64 <=
              obj_slab_traits<176>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<176>::capacity * 176 +
                            obj_slab_traits<176>::meta_data_size)) < 176);
static_assert(obj_slab_traits<176>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<176>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<176>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<192>::capacity * 192 +
                  obj_slab_traits<192>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<192>::capacity / 64 <=
              obj_slab_traits<192>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<192>::capacity * 192 +
                            obj_slab_traits<192>::meta_data_size)) < 192);
static_assert(obj_slab_traits<192>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<192>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<192>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<208>::capacity * 208 +
                  obj_slab_traits<208>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<208>::capacity / 64 <=
              obj_slab_traits<208>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<208>::capacity * 208 +
                            obj_slab_traits<208>::meta_data_size)) < 208);
static_assert(obj_slab_traits<208>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<208>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<208>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<224>::capacity * 224 +
                  obj_slab_traits<224>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<224>::capacity / 64 <=
              obj_slab_traits<224>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<224>::capacity * 224 +
                            obj_slab_traits<224>::meta_data_size)) < 224);
static_assert(obj_slab_traits<224>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<224>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<224>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<240>::capacity * 240 +
                  obj_slab_traits<240>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<240>::capacity / 64 <=
              obj_slab_traits<240>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<240>::capacity * 240 +
                            obj_slab_traits<240>::meta_data_size)) < 240);
static_assert(obj_slab_traits<240>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<240>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<240>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<256>::capacity * 256 +
                  obj_slab_traits<256>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<256>::capacity / 64 <=
              obj_slab_traits<256>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<256>::capacity * 256 +
                            obj_slab_traits<256>::meta_data_size)) < 256);
static_assert(obj_slab_traits<256>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<256>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<256>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<272>::capacity * 272 +
                  obj_slab_traits<272>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<272>::capacity / 64 <=
              obj_slab_traits<272>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<272>::capacity * 272 +
                            obj_slab_traits<272>::meta_data_size)) < 272);
static_assert(obj_slab_traits<272>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<272>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<272>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<288>::capacity * 288 +
                  obj_slab_traits<288>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<288>::capacity / 64 <=
              obj_slab_traits<288>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<288>::capacity * 288 +
                            obj_slab_traits<288>::meta_data_size)) < 288);
static_assert(obj_slab_traits<288>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<288>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<288>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<304>::capacity * 304 +
                  obj_slab_traits<304>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<304>::capacity / 64 <=
              obj_slab_traits<304>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<304>::capacity * 304 +
                            obj_slab_traits<304>::meta_data_size)) < 304);
static_assert(obj_slab_traits<304>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<304>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<304>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<320>::capacity * 320 +
                  obj_slab_traits<320>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<320>::capacity / 64 <=
              obj_slab_traits<320>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<320>::capacity * 320 +
                            obj_slab_traits<320>::meta_data_size)) < 320);
static_assert(obj_slab_traits<320>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<320>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<320>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<336>::capacity * 336 +
                  obj_slab_traits<336>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<336>::capacity / 64 <=
              obj_slab_traits<336>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<336>::capacity * 336 +
                            obj_slab_traits<336>::meta_data_size)) < 336);
static_assert(obj_slab_traits<336>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<336>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<336>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<352>::capacity * 352 +
                  obj_slab_traits<352>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<352>::capacity / 64 <=
              obj_slab_traits<352>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<352>::capacity * 352 +
                            obj_slab_traits<352>::meta_data_size)) < 352);
static_assert(obj_slab_traits<352>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<352>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<352>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<368>::capacity * 368 +
                  obj_slab_traits<368>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<368>::capacity / 64 <=
              obj_slab_traits<368>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<368>::capacity * 368 +
                            obj_slab_traits<368>::meta_data_size)) < 368);
static_assert(obj_slab_traits<368>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<368>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<368>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<384>::capacity * 384 +
                  obj_slab_traits<384>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<384>::capacity / 64 <=
              obj_slab_traits<384>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<384>::capacity * 384 +
                            obj_slab_traits<384>::meta_data_size)) < 384);
static_assert(obj_slab_traits<384>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<384>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<384>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<400>::capacity * 400 +
                  obj_slab_traits<400>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<400>::capacity / 64 <=
              obj_slab_traits<400>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<400>::capacity * 400 +
                            obj_slab_traits<400>::meta_data_size)) < 400);
static_assert(obj_slab_traits<400>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<400>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<400>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<416>::capacity * 416 +
                  obj_slab_traits<416>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<416>::capacity / 64 <=
              obj_slab_traits<416>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<416>::capacity * 416 +
                            obj_slab_traits<416>::meta_data_size)) < 416);
static_assert(obj_slab_traits<416>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<416>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<416>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<432>::capacity * 432 +
                  obj_slab_traits<432>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<432>::capacity / 64 <=
              obj_slab_traits<432>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<432>::capacity * 432 +
                            obj_slab_traits<432>::meta_data_size)) < 432);
static_assert(obj_slab_traits<432>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<432>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<432>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<448>::capacity * 448 +
                  obj_slab_traits<448>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<448>::capacity / 64 <=
              obj_slab_traits<448>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<448>::capacity * 448 +
                            obj_slab_traits<448>::meta_data_size)) < 448);
static_assert(obj_slab_traits<448>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<448>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<448>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<464>::capacity * 464 +
                  obj_slab_traits<464>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<464>::capacity / 64 <=
              obj_slab_traits<464>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<464>::capacity * 464 +
                            obj_slab_traits<464>::meta_data_size)) < 464);
static_assert(obj_slab_traits<464>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<464>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<464>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<480>::capacity * 480 +
                  obj_slab_traits<480>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<480>::capacity / 64 <=
              obj_slab_traits<480>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<480>::capacity * 480 +
                            obj_slab_traits<480>::meta_data_size)) < 480);
static_assert(obj_slab_traits<480>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<480>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<480>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<496>::capacity * 496 +
                  obj_slab_traits<496>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<496>::capacity / 64 <=
              obj_slab_traits<496>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<496>::capacity * 496 +
                            obj_slab_traits<496>::meta_data_size)) < 496);
static_assert(obj_slab_traits<496>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<496>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<496>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));


static_assert(obj_slab_traits<512>::capacity * 512 +
                  obj_slab_traits<512>::meta_data_size <=
              slab_size);
static_assert(obj_slab_traits<512>::capacity / 64 <=
              obj_slab_traits<512>::num_slot_vecs);
static_assert((slab_size - (obj_slab_traits<512>::capacity * 512 +
                            obj_slab_traits<512>::meta_data_size)) < 512);
static_assert(obj_slab_traits<512>::meta_data_size >=
              (L2_CACHE_LOAD_SIZE *
               (((L2_CACHE_LOAD_SIZE - 1) +
                 (8 * (2 + obj_slab_traits<512>::num_slot_vecs))) /
                L2_CACHE_LOAD_SIZE)) +
                  (L2_CACHE_LOAD_SIZE *
                   (((L2_CACHE_LOAD_SIZE - 1) +
                     (8 * (3 + obj_slab_traits<512>::num_slot_vecs))) /
                    L2_CACHE_LOAD_SIZE)));

#endif
