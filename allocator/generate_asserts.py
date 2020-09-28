#! /usr/bin/env python3

import sys

sizes = [
    8, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240,
    256, 272, 288, 304, 320, 336, 352, 368, 384, 400, 416, 432, 448, 464, 480,
    496, 512
]


n_asserts = 4
assert_comments = [
    "// asserting size is within permitted bound",
    "// asserting num_slot_vecs has enough indexes for capacity",
    "// asserting calculate maximum possible capacity",
    "// asserting meta_data_size indeed represents all meta data (note + 3 is only for available but this is being safe)"
]
assert_templates = [
    "static_assert(obj_slab_traits<[SIZE]>::capacity * [SIZE] + obj_slab_traits<[SIZE]>::meta_data_size <= slab_size);",
    "static_assert(obj_slab_traits<[SIZE]>::capacity / 64 <= obj_slab_traits<[SIZE]>::num_slot_vecs);",
    "static_assert((slab_size - (obj_slab_traits<[SIZE]>::capacity * [SIZE] + obj_slab_traits<[SIZE]>::meta_data_size)) < [SIZE]);",
    "static_assert(obj_slab_traits<[SIZE]>::meta_data_size >= (L2_CACHE_LOAD_SIZE * (((L2_CACHE_LOAD_SIZE - 1) + (8 * (2 + obj_slab_traits<[SIZE]>::num_slot_vecs))) / L2_CACHE_LOAD_SIZE)) + (L2_CACHE_LOAD_SIZE * (((L2_CACHE_LOAD_SIZE - 1) + (8 * (3 + obj_slab_traits<[SIZE]>::num_slot_vecs))) / L2_CACHE_LOAD_SIZE)));"
]


print("// Asserts generate by: " + sys.argv[0] + "\n")
for i in range(0, n_asserts):
    print(assert_comments[i])
    print("// " + assert_templates[i] + "\n")

print("\n\n")

for size in sizes:
    for assert_template in assert_templates:
        print(assert_template.replace("[SIZE]", str(size)))
    print("\n")
