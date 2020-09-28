#ifndef RECOMPUTE_SYS_INFO_VALUES  // flag so that can recompute with make flag
#if __has_include("PRECOMPUTED_SYS_INFO.h")  // have the values
#include "PRECOMPUTED_SYS_INFO.h"            // will include _SYS_INFO_H_
#endif
#endif


#ifndef _SYS_INFO_H_  // this will only pass if PRECOMPUTED_SYS_INFO.h does not
                      // exist
#define _SYS_INFO_H_

#include <misc/error_handling.h>
#include <misc/macro_helper.h>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>


namespace sysi {

enum THP { ALWAYS = 0, MADVISE = 1, NEVER = 2 };


typedef struct cache_info {
    uint32_t sets;
    uint32_t assos;
    uint32_t size;
} cache_info_t;


static void create_defs(const char * outfile);
static void find_precomputed_header_file(char * path);
static void read_proc_cpu(uint32_t * phys_core_info,
                          uint32_t * vm_bit_info,
                          uint32_t * phys_m_bit_info);
static void read_cache_info(cache_info_t * l1_dinfo,
                            cache_info_t * l1_iinfo,
                            cache_info_t * l2_uinfo,
                            cache_info_t * l3_uinfo);
static void read_kernel_header(uint64_t * page_offset_out,
                               uint64_t * start_kernel_map_out);


#ifndef NPROCS
#define NPROCS sysconf(_SC_NPROCESSORS_ONLN)
#endif

#ifndef NCORES
#define NCORES sysi::compute_ncores()

static uint32_t
compute_ncores() {
    uint32_t ncores;
    read_proc_cpu(&ncores, NULL, NULL);
    return ncores;
}

#endif

#ifndef PHYS_CORE
#define PHYS_CORE(X) sysi::compute_phys_core(X)

static uint32_t
compute_phys_core(uint32_t logical_core_num) {
    uint32_t ret;
    char     buf[8] = { 0 };
    char *   end    = buf;

    char core_info_file[128];
    sprintf(core_info_file,
            "/sys/devices/system/cpu/cpu%d/topology/core_id",
            logical_core_num);
    FILE * fp = fopen(core_info_file, "r");
    ERROR_ASSERT(fp != NULL,
                 "Error unable to open core info file:\n\"%s\"\n",
                 core_info_file);

    ERROR_ASSERT(fgets(buf, 7, fp) != NULL,
                 "Error unable to read core info from file:\n\"%s\"\n",
                 core_info_file);

    ret = (uint32_t)strtol(buf, &end, 10);
    DIE_ASSERT(
        (end != buf),
        "Error unable get integer type content from file contents:\n\"%s\"\n",
        buf);
    fclose(fp);

    return ret;
}
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#endif

#ifndef VM_NBITS
#define VM_NBITS sysi::compute_vm_nbits()


static uint32_t
compute_vm_nbits() {
    uint32_t vm_nbits;
    read_proc_cpu(NULL, &vm_nbits, NULL);
    return vm_nbits;
}

#endif

#ifndef PHYS_M_NBITS
#define PHYS_M_NBITS sysi::compute_phys_m_nbits()

static uint32_t
compute_phys_m_nbits() {
    uint32_t phys_m_nbits;
    read_proc_cpu(NULL, NULL, &phys_m_nbits);
    return phys_m_nbits;
}

#endif

#ifndef PAGE_OFFSET
#define PAGE_OFFSET sysi::compute_page_offset()

static uint64_t
compute_page_offset() {
    uint64_t page_offset;
    read_kernel_header(&page_offset, NULL);
    return page_offset;
}

#endif

#ifndef KERNEL_MEM_START
#define KERNEL_MEM_START sysi::compute_kernel_mem_start()

static uint64_t
compute_kernel_mem_start() {
    uint64_t kernel_mem_start;
    read_kernel_header(NULL, &kernel_mem_start);
    return kernel_mem_start;
}


#endif

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE sysi::compute_cache_line_size()


static uint32_t
compute_cache_line_size() {
    uint32_t ret;
    char     buf[8] = { 0 };
    char *   end    = buf;

    const char * cache_line_info_file =
        "/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size";
    FILE * fp = fopen(cache_line_info_file, "r");
    ERROR_ASSERT(fp != NULL,
                 "Error unable to open cache line file:\n\"%s\"\n",
                 cache_line_info_file);

    ERROR_ASSERT(fgets(buf, 7, fp) != NULL,
                 "Error unable to read cache line size from file:\n\"%s\"\n",
                 cache_line_info_file);

    ret = (uint32_t)strtol(buf, &end, 10);
    DIE_ASSERT(
        (end != buf),
        "Error unable get integer type content from file contents:\n\"%s\"\n",
        buf);
    fclose(fp);

    return ret;
}


#endif


#ifndef L1_DCACHE_SIZE
#define L1_DCACHE_SIZE sysi::compute_l1_dcache_size()

static uint32_t
compute_l1_dcache_size() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(&ci_out, NULL, NULL, NULL);
    return ci_out.size;
}
#endif


#ifndef L1_DCACHE_SETS
#define L1_DCACHE_SETS sysi::compute_l1_dcache_sets()

static uint32_t
compute_l1_dcache_sets() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(&ci_out, NULL, NULL, NULL);
    return ci_out.sets;
}
#endif

#ifndef L1_DCACHE_ASSOS
#define L1_DCACHE_ASSOS sysi::compute_l1_dcache_assos()

static uint32_t
compute_l1_dcache_assos() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(&ci_out, NULL, NULL, NULL);
    return ci_out.assos;
}
#endif


#ifndef L1_ICACHE_SIZE
#define L1_ICACHE_SIZE sysi::compute_l1_icache_size()

static uint32_t
compute_l1_icache_size() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(NULL, &ci_out, NULL, NULL);
    return ci_out.size;
}
#endif

#ifndef L1_ICACHE_SETS
#define L1_ICACHE_SETS sysi::compute_l1_icache_sets()

static uint32_t
compute_l1_icache_sets() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(NULL, &ci_out, NULL, NULL);
    return ci_out.sets;
}
#endif


#ifndef L1_ICACHE_ASSOS
#define L1_ICACHE_ASSOS sysi::compute_l1_icache_assos()

static uint32_t
compute_l1_icache_assos() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(NULL, &ci_out, NULL, NULL);
    return ci_out.assos;
}
#endif


#ifndef L2_UCACHE_SIZE
#define L2_UCACHE_SIZE sysi::compute_l2_ucache_size()

static uint32_t
compute_l2_ucache_size() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(NULL, NULL, &ci_out, NULL);
    return ci_out.size;
}
#endif


#ifndef L2_UCACHE_SETS
#define L2_UCACHE_SETS sysi::compute_l2_ucache_sets()

static uint32_t
compute_l2_ucache_sets() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(NULL, NULL, &ci_out, NULL);
    return ci_out.sets;
}
#endif


#ifndef L2_UCACHE_ASSOS
#define L2_UCACHE_ASSOS sysi::compute_l2_ucache_assos()

static uint32_t
compute_l2_ucache_assos() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(NULL, NULL, &ci_out, NULL);
    return ci_out.assos;
}
#endif


#ifndef L3_UCACHE_SIZE
#define L3_UCACHE_SIZE sysi::compute_l3_ucache_size()

static uint32_t
compute_l3_ucache_size() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(NULL, NULL, NULL, &ci_out);
    return ci_out.size;
}
#endif


#ifndef L3_UCACHE_SETS
#define L3_UCACHE_SETS sysi::compute_l3_ucache_sets()

static uint32_t
compute_l3_ucache_sets() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(NULL, NULL, NULL, &ci_out);
    return ci_out.sets;
}
#endif

#ifndef L3_UCACHE_ASSOS
#define L3_UCACHE_ASSOS sysi::compute_l3_ucache_assos()

static uint32_t
compute_l3_ucache_assos() {
    cache_info_t ci_out = {.sets = 0, .assos = 0, .size = 0 };
    read_cache_info(NULL, NULL, NULL, &ci_out);
    return ci_out.assos;
}
#endif

#ifndef TRANSPARENT_HUGE_PAGE_CFG
#define TRANSPARENT_HUGE_PAGE_CFG sysi::compute_transparent_huge_page_config()

static uint32_t
contains_braces(char * buf) {
    uint32_t contains = 0;
    while (*buf) {
        if (*buf == ']' || *buf == '[') {
            contains++;
        }
        ++buf;
    }
    return contains;
}

static uint32_t
compute_transparent_huge_page_config() {
    const uint32_t buf_len = 64;
    const char *   huge_page_config_path =
        "/sys/kernel/mm/transparent_hugepage/enabled";

    char buf[buf_len] = "";

    FILE * fp = fopen(huge_page_config_path, "r");
    ERROR_ASSERT(fp != NULL);

    ERROR_ASSERT(fgets(buf, buf_len, fp) != NULL);
    DIE_ASSERT(buf[buf_len - 2] == 0,
               "Buf too small to read entire hugepage config file\n");

    uint32_t parse_braces = contains_braces(buf);
    DIE_ASSERT(parse_braces == 2 || parse_braces == 0,
               "Error: unknown format in transparent huge page config file\n");

    char * parse_start = buf;
    if (parse_braces) {
        uint32_t i;
        for (i = 0; (i < buf_len) && buf[i]; ++i) {
            if (buf[i] == '[') {
                parse_start = &(buf[++i]);
                --parse_braces;
                break;
            }
        }
        for (; (i < buf_len) && buf[i]; ++i) {
            if (buf[i] == ']') {
                buf[i] = 0;
                --parse_braces;
                break;
            }
        }
        DIE_ASSERT(!parse_braces);
    }
    if (!strncmp(parse_start, "always", strlen("always"))) {
        return THP::ALWAYS;
    }
    else if (!strncmp(parse_start, "madvise", strlen("madvise"))) {
        return THP::MADVISE;
    }
    else if (!strncmp(parse_start, "never", strlen("never"))) {
        return THP::NEVER;
    }
    DIE("No valid config found\n");
}
#endif


static uint64_t
get_ac_value(char * buf) {
    uint32_t start_hex_val = 0;
    uint32_t end_hex_val   = 0;
    for (uint32_t i = 0; buf[i]; ++i) {
        if (!strncmp(buf + i, "_AC(0x", strlen("_AC(0x"))) {
            start_hex_val = i + (strlen("_AC(0x"));
        }
        else if (start_hex_val && buf[i] == ',') {
            end_hex_val = i;
            buf[i]      = 0;
        }
    }
    DIE_ASSERT(start_hex_val != 0 && end_hex_val != 0,
               "Unable to get value from line: %s\n",
               buf);

    char *   end;
    uint64_t ret = strtoull(buf + start_hex_val, &end, 16);
    DIE_ASSERT(end != buf + start_hex_val,
               "Error reading hex value as int from: %s\n",
               buf + start_hex_val);

    return ret;
}

static void
read_kernel_header(uint64_t * page_offset_out,
                   uint64_t * start_kernel_map_out) {
    const char * kconfig_fmt_path = "/boot/config-%s";
    const char * kheader_fmt_path =
        "/usr/src/linux-headers-%s/arch/x86/include/asm/%s";
    struct utsname kinfo;
    ERROR_ASSERT(!uname(&kinfo), "Error getting kernel information\n");

    const uint32_t buf_len = 256;
    char           kconfig_path[buf_len];
    sprintf(kconfig_path, kconfig_fmt_path, kinfo.release);

    FILE * fp_config = NULL;

    fp_config = fopen(kconfig_path, "r");
    ERROR_ASSERT(fp_config != NULL,
                 "Error opening kernel config path at: %s\n",
                 kconfig_path);

    int32_t is_dynamic_mem_layout = -1;
    int32_t is_5level_addr        = -1;

    char buf[buf_len] = "";
    while (fgets(buf, buf_len, fp_config)) {
        if (!strncmp("CONFIG_DYNAMIC_MEMORY_LAYOUT=y",
                     buf,
                     strlen("CONFIG_DYNAMIC_MEMORY_LAYOUT=y"))) {
            is_dynamic_mem_layout = 1;
        }
        else if (!strncmp("CONFIG_DYNAMIC_MEMORY_LAYOUT=n",
                          buf,
                          strlen("CONFIG_DYNAMIC_MEMORY_LAYOUT=n"))) {
            is_dynamic_mem_layout = 0;
        }

        if (!strncmp("CONFIG_X86_5LEVEL", buf, strlen("CONFIG_X86_5LEVEL"))) {
            is_5level_addr = 1;
        }
        else if (!strncmp("# CONFIG_X86_5LEVEL",
                          buf,
                          strlen("# CONFIG_X86_5LEVEL"))) {
            is_5level_addr = 0;
        }
    }

    DIE_ASSERT(is_dynamic_mem_layout != -1,
               "Unable to find DYNAMIC_MEMORY_LAYOUT config\n");
    DIE_ASSERT(is_5level_addr != -1, "Unable to find 5LEVEL config\n");

    fclose(fp_config);


    FILE * fp_header = NULL;
    char   kheader_path[buf_len];

    // get PAGE_OFFSET          -> page_types.h
    // get __START_KERNEL_map   -> page_64.h
    // get __PAGE_OFFSET        -> page_64_types.h

    // the logic we are trying to mimic in the end is:
    /*
    static inline unsigned long __phys_addr_nodebug(unsigned long x)
    {
        unsigned long y = x - __START_KERNEL_map;

        x = y + ((x > y) ? phys_base : (__START_KERNEL_map - PAGE_OFFSET));

        return x;
    }
    */
    // x > y is only true if x > __START_KERNEL_map
    // this should not be true for any vm returned by mmap
    // so all we need is:
    // return (x - _START_KERNEL_map) + (__START_KERNEL_map - PAGE_OFFSET)
    // i.e
    // return x - PAGE_OFFSET
    // getting __START_KERNEL_map for debugging (turn on an assert)

    sprintf(kheader_path, kheader_fmt_path, kinfo.release, "page_types.h");
    fp_header = fopen(kheader_path, "r");
    ERROR_ASSERT(fp_header != NULL,
                 "Error opening kernel header path at: %s\n",
                 kheader_path);

    int32_t verified_page_offset = 0;
    while (fgets(buf, buf_len, fp_header)) {
        if (!strncmp("#define PAGE_OFFSET",
                     buf,
                     strlen("#define PAGE_OFFSET"))) {
            uint32_t line_len = strlen(buf);
            uint32_t cs_len   = strlen("__PAGE_OFFSET");

            for (uint32_t i = 0; i < line_len - cs_len; ++i) {
                if (!strncmp(buf + i, "__PAGE_OFFSET", cs_len)) {
                    verified_page_offset = 1;
                    break;
                }
            }
            break;
        }
    }
    fclose(fp_header);
    DIE_ASSERT(verified_page_offset,
               "Unable to find PAGE_OFFSET at %s\n",
               kheader_path);

    sprintf(kheader_path, kheader_fmt_path, kinfo.release, "page_64_types.h");
    fp_header = fopen(kheader_path, "r");
    ERROR_ASSERT(fp_header != NULL,
                 "Error opening kernel header path at: %s\n",
                 kheader_path);


    verified_page_offset = 0;

    uint64_t start_kernel_map = 0;
    uint64_t page_offset      = 0;

    while (fgets(buf, buf_len, fp_header)) {
        if (is_5level_addr &&
            !strncmp("#define __PAGE_OFFSET_BASE_L5",
                     buf,
                     strlen("#define __PAGE_OFFSET_BASE_L5"))) {
            page_offset = get_ac_value(buf);
        }
        else if (!is_5level_addr &&
                 !strncmp("#define __PAGE_OFFSET_BASE_L4",
                          buf,
                          strlen("#define __PAGE_OFFSET_BASE_L4"))) {
            page_offset = get_ac_value(buf);
        }
        else if (!strncmp("#define __START_KERNEL_map",
                          buf,
                          strlen("#define __START_KERNEL_map"))) {
            start_kernel_map = get_ac_value(buf);
        }
        else if (is_dynamic_mem_layout &&
                 !strncmp("#define __PAGE_OFFSET",
                          buf,
                          strlen("#define __PAGE_OFFSET")) &&
                 buf[strlen("#define __PAGE_OFFSET")] != '_') {
            uint32_t line_len = strlen(buf);
            uint32_t cs_len   = strlen("page_offset_base");
            for (uint32_t i = 0; i < line_len - cs_len; ++i) {
                if (!strncmp(buf + i,
                             "page_offset_base",
                             strlen("page_offset_base"))) {
                    verified_page_offset = 1;
                }
            }
        }
        else if (!is_dynamic_mem_layout &&
                 !strncmp("#define __PAGE_OFFSET",
                          buf,
                          strlen("#define __PAGE_OFFSET")) &&
                 buf[strlen("#define __PAGE_OFFSET")] != '_') {
            uint32_t line_len = strlen(buf);
            uint32_t cs_len   = strlen("__PAGE_OFFSET_BASE_L4");
            for (uint32_t i = 0; i < line_len - cs_len; ++i) {
                if (!strncmp(buf + i,
                             "__PAGE_OFFSET_BASE_L4",
                             strlen("__PAGE_OFFSET_BASE_L4"))) {
                    verified_page_offset = 1;
                }
            }
        }
    }
    fclose(fp_header);

    DIE_ASSERT(verified_page_offset,
               "Error: unable to verify __PAGE_OFFSET source in: %s\n",
               kheader_path);
    DIE_ASSERT(start_kernel_map != 0,
               "Error: unable to find start_kernel_map value\n");
    DIE_ASSERT(page_offset != 0, "Error: unable to find page_offset value\n");

    if (page_offset_out != NULL) {
        *page_offset_out = page_offset;
    }
    if (start_kernel_map_out != NULL) {
        *start_kernel_map_out = start_kernel_map;
    }
}


static uint32_t
read_cache_dir(char * dir_path, cache_info_t * ci_out) {
#define UCACHE 1
#define DCACHE 2
#define ICACHE 3

    if (ci_out == NULL) {
        return 0;
    }

    uint32_t       sets = 0, assos = 0, size = 0, level = 0, type = 0;
    const uint32_t buf_len      = 64;
    char           buf[buf_len] = "";
    char *         end          = NULL;

    FILE *   fp                = NULL;
    uint32_t dir_path_base_len = strlen(dir_path);

    // read level
    memcpy(dir_path + dir_path_base_len, "/level", strlen("/level"));
    dir_path[dir_path_base_len + strlen("/level")] = 0;

    fp = fopen(dir_path, "r");
    ERROR_ASSERT(fp != NULL, "Error opening cache info file: %s\n", dir_path);
    ERROR_ASSERT(fgets(buf, buf_len, fp) != NULL,
                 "Error read reading cache info file: %s\n",
                 dir_path);
    level = strtol(buf, &end, 10);
    DIE_ASSERT(end != buf, "Error parsing %s as int\n", buf);
    fclose(fp);

    // read sets
    memcpy(dir_path + dir_path_base_len,
           "/number_of_sets",
           strlen("/number_of_sets"));
    dir_path[dir_path_base_len + strlen("/number_of_sets")] = 0;

    fp = fopen(dir_path, "r");
    ERROR_ASSERT(fp != NULL, "Error opening cache info file: %s\n", dir_path);
    ERROR_ASSERT(fgets(buf, buf_len, fp) != NULL,
                 "Error read reading cache info file: %s\n",
                 dir_path);
    sets = strtol(buf, &end, 10);
    DIE_ASSERT(end != buf, "Error parsing %s as int\n", buf);
    fclose(fp);

    // read size
    memcpy(dir_path + dir_path_base_len, "/size", strlen("/size"));
    dir_path[dir_path_base_len + strlen("/size")] = 0;

    fp = fopen(dir_path, "r");
    ERROR_ASSERT(fp != NULL, "Error opening cache info file: %s\n", dir_path);
    ERROR_ASSERT(fgets(buf, buf_len, fp) != NULL,
                 "Error read reading cache info file: %s\n",
                 dir_path);
    size = strtol(buf, &end, 10) * 1024;
    DIE_ASSERT(end != buf, "Error parsing %s as int\n", buf);
    fclose(fp);

    // read type
    memcpy(dir_path + dir_path_base_len, "/type", strlen("/type"));
    dir_path[dir_path_base_len + strlen("/type")] = 0;

    fp = fopen(dir_path, "r");
    ERROR_ASSERT(fp != NULL, "Error opening cache info file: %s\n", dir_path);
    ERROR_ASSERT(fgets(buf, buf_len, fp) != NULL,
                 "Error read reading cache info file: %s\n",
                 dir_path);


    if (!strncmp("Unified", buf, strlen("Unified"))) {
        type = UCACHE;
    }
    else if (!strncmp("Data", buf, strlen("Data"))) {
        type = DCACHE;
    }
    else if (!strncmp("Instruction", buf, strlen("Instruction"))) {
        type = ICACHE;
    }
    DIE_ASSERT(type != 0, "Error unknown type read from: %s\n", dir_path);
    fclose(fp);

    // read assos
    memcpy(dir_path + dir_path_base_len,
           "/ways_of_associativity",
           strlen("/ways_of_associativity"));
    dir_path[dir_path_base_len + strlen("/ways_of_associativity")] = 0;

    fp = fopen(dir_path, "r");
    ERROR_ASSERT(fp != NULL, "Error opening cache info file: %s\n", dir_path);
    ERROR_ASSERT(fgets(buf, buf_len, fp) != NULL,
                 "Error read reading cache info file: %s\n",
                 dir_path);
    assos = strtol(buf, &end, 10);
    DIE_ASSERT(end != buf, "Error parsing %s as int\n", buf);
    fclose(fp);

    ci_out->sets  = sets;
    ci_out->size  = size;
    ci_out->assos = assos;

    return level | ((type << 16));
}


static void
read_cache_info(cache_info_t * l1_dinfo,
                cache_info_t * l1_iinfo,
                cache_info_t * l2_uinfo,
                cache_info_t * l3_uinfo) {

    const char * cache_info_fmt_path =
        "/sys/devices/system/cpu/cpu0/cache/index%d";
    const uint32_t buf_len = 256;
    char           cache_info_path[buf_len];

    cache_info_t ci_out;
    for (uint32_t i = 0;; ++i) {
        sprintf(cache_info_path, cache_info_fmt_path, i);

        struct stat ci_sb;
        if (stat(cache_info_path, &ci_sb) != 0 || !S_ISDIR(ci_sb.st_mode)) {
            break;
        }

        uint32_t ret = read_cache_dir(cache_info_path, &ci_out);

        uint32_t level_info = ret & 0xffff;
        uint32_t type_info  = ret >> 16;
        if (type_info == UCACHE) {
            if (level_info == 2) {
                if (l2_uinfo != NULL) {
                    memcpy(l2_uinfo, &ci_out, sizeof(cache_info_t));
                }
            }
            else if (level_info == 3) {
                if (l3_uinfo != NULL) {
                    memcpy(l3_uinfo, &ci_out, sizeof(cache_info_t));
                }
            }
            else {
                DIE_ASSERT(0,
                           "No support for more than 3 levels of cache or "
                           "unified L1 cache at this time\n");
            }
        }
        else if (type_info == DCACHE) {
            DIE_ASSERT(level_info == 1,
                       "No support for multi level data cache at this time\n");
            if (l1_dinfo != NULL) {
                memcpy(l1_dinfo, &ci_out, sizeof(cache_info_t));
            }
        }
        else if (type_info == ICACHE) {
            DIE_ASSERT(
                level_info == 1,
                "No support for multi level instruction cache at this time\n");
            if (l1_iinfo != NULL) {
                memcpy(l1_iinfo, &ci_out, sizeof(cache_info_t));
            }
        }
    }

#undef UCACHE
#undef DCACHE
#undef ICACHE
}


static void
read_proc_cpu(uint32_t * phys_core_info,
              uint32_t * vm_bit_info,
              uint32_t * phys_m_bit_info) {
    uint32_t expec_reads =
        (!!vm_bit_info) + (!!phys_m_bit_info) + (!!phys_core_info);
    if (!expec_reads) {
        return;
    }

    const uint32_t buf_len      = 64;
    char           buf[buf_len] = { 0 };
    FILE *         fp           = fopen("/proc/cpuinfo", "r");
    ERROR_ASSERT(fp != NULL, "Error unable to open \"proc/cpuinfo\"\n");

    while (fgets(buf, buf_len, fp)) {
        // skip longer lines basically
        if ((vm_bit_info || phys_m_bit_info) &&
            (!strncmp(buf, "address sizes", strlen("address sizes")))) {
            // line == "address sizes : <INT> bits physical, <INT> bits
            // virtual
            char     waste[16];
            uint32_t _vm_bit_info, _phys_m_bit_info;
            if (sscanf(buf,
                       "%s %s %c %d %s %s %d %s %s",
                       waste,
                       waste,
                       waste,
                       &_phys_m_bit_info,
                       waste,
                       waste,
                       &_vm_bit_info,
                       waste,
                       waste) == 9) {
                if (phys_m_bit_info) {
                    *phys_m_bit_info = _phys_m_bit_info;
                    --expec_reads;
                }
                if (vm_bit_info) {
                    *vm_bit_info = _vm_bit_info;
                    --expec_reads;
                }
                if (!expec_reads) {
                    fclose(fp);
                    return;
                }
            }
        }
        else if (phys_core_info &&
                 (!strncmp(buf, "cpu cores", strlen("cpu cores")))) {
            char waste[16];
            if (sscanf(buf,
                       "%s %s %c %d",
                       waste,
                       waste,
                       waste,
                       phys_core_info) == 4) {
                --expec_reads;
                if (!expec_reads) {
                    fclose(fp);
                    return;
                }
            }
        }
    }
    DIE("Error unable to find all info from proc/cpuinfo\n");
}


static void
find_precomputed_header_file(char * path) {
    const uint32_t buf_len = 128;
    DIR * d = NULL;
    char  cur_dir[buf_len];
    char  to_project_lib[8] = "";
    ERROR_ASSERT(getcwd(cur_dir, buf_len) != NULL,
                 "Error getting current working directory\n");
    d = opendir("build");
    if (d == NULL) {
        strcpy(to_project_lib, "../lib");
    }
    else {
        strcpy(to_project_lib, "lib");
    }
    d = opendir(to_project_lib);
    DIE_ASSERT(d != NULL,
               "Error unable to find lib directory\n\tTested path: \"%s\"\n",
               to_project_lib);
    closedir(d);

    sprintf(path,
            "%s/%s/system/PRECOMPUTED_SYS_INFO.h",
            cur_dir,
            to_project_lib);
}


static void
create_defs(const char * outfile) {
    const uint32_t buf_len = 256;
    FILE * fp = NULL;
    if (outfile == NULL || (!strcmp(outfile, ""))) {
        char default_path[buf_len];
        find_precomputed_header_file(default_path);
        fp = fopen(default_path, "w+");
        ERROR_ASSERT(fp != NULL,
                     "Error unable to open file at:\n\"%s\"\n",
                     default_path);
    }
    else {
        fp = fopen(outfile, "w+");
        ERROR_ASSERT(fp != NULL,
                     "Error unable to open file at:\n\"%s\"\n",
                     outfile);
    }


    fprintf(fp, "#ifndef _SYS_INFO_H_\n");
    fprintf(fp, "#define _SYS_INFO_H_\n");
    fprintf(fp, "#define HAVE_CONST_SYS_INFO\n");
    fprintf(fp, "\n");
    fprintf(fp, "#include <stdint.h>\n");
    fprintf(fp, "\n");
    fprintf(fp,
            "// Number of processors (this is cores includes hyperthreads)\n");
    fprintf(fp, "#define NPROCS %d\n", (uint32_t)NPROCS);
    fprintf(fp, "\n");
    fprintf(fp, "// Number of physical cores\n");
    fprintf(fp, "#define NCORES %d\n", NCORES);
    fprintf(fp, "\n");

    fprintf(fp, "// Logical to physical core lookup\n");
    fprintf(fp, "#define PHYS_CORE(X) get_phys_core(X)\n");
    fprintf(fp,
            "uint32_t inline __attribute__((always_inline)) "
            "__attribute__((const))\n");
    fprintf(fp, "get_phys_core(const uint32_t logical_core_num) {\n");
    fprintf(fp,
            "\tstatic const constexpr uint32_t core_map[NPROCS] = { %d",
            PHYS_CORE(0));
    for (uint32_t i = 1; i < NPROCS; ++i) {
        fprintf(fp, ", %d", PHYS_CORE(i));
    }
    fprintf(fp, " };\n");
    fprintf(fp, "\treturn core_map[logical_core_num];\n");
    fprintf(fp, "}\n\n");


    fprintf(fp, "// Transparent Huge Page Configuration\n");

    uint32_t _thp_cfg = TRANSPARENT_HUGE_PAGE_CFG;

    fprintf(fp,
            "enum THP { ALWAYS = %d, MADVISE = %d, NEVER = %d };\n",
            THP::ALWAYS,
            THP::MADVISE,
            THP::NEVER);
    fprintf(fp,
            "#define TRANSPARENT_HUGE_PAGE_CFG %s\n",
            _thp_cfg == THP::ALWAYS
                ? "THP::ALWAYS"
                : (_thp_cfg == THP::MADVISE ? "THP::MADVISE" : "THP::NEVER"));
    fprintf(fp, "\n");
    fprintf(fp, "// Virtual memory page size\n");
    fprintf(fp, "#define PAGE_SIZE %d\n", (uint32_t)PAGE_SIZE);
    fprintf(fp, "\n");
    fprintf(fp, "// Virtual memory address space bits\n");
    fprintf(fp, "#define VM_NBITS %d\n", VM_NBITS);
    fprintf(fp, "\n");
    fprintf(fp, "// Physical memory address space bits\n");
    fprintf(fp, "#define PHYS_M_NBITS %d\n", PHYS_M_NBITS);
    fprintf(fp, "\n");
    fprintf(fp, "// Physical memory page offset (for vm -> pm) for kmalloc\n");
    fprintf(fp, "#define PAGE_OFFSET (0x%lxUL)\n", PAGE_OFFSET);
    fprintf(fp, "\n");
    fprintf(fp,
            "// Where physical memory of kernel (mostly for debugging kmalloc "
            "stuff)\n");
    fprintf(fp, "#define KERNEL_MEM_START (0x%lxUL)\n", KERNEL_MEM_START);
    fprintf(fp, "\n");

    fprintf(fp, "// cache line size (should be same for L1, L2, and L3)\n");
    fprintf(fp, "#define CACHE_LINE_SIZE %d\n", CACHE_LINE_SIZE);
    fprintf(fp, "\n");
    fprintf(fp, "// l2 cache loads double cache lines at a time\n");
    fprintf(fp, "#define L2_CACHE_LOAD_SIZE %d\n", 2 * CACHE_LINE_SIZE);
    fprintf(fp, "\n");
    fprintf(fp,
            "\n////////////////////////////////////////////////////////////////"
            "//////\n");
    fprintf(fp, "// More specific cache info starts here\n");
    fprintf(fp,
            "//////////////////////////////////////////////////////////////////"
            "////\n\n");
    fprintf(fp, "#define L1_DCACHE_SIZE %d\n", L1_DCACHE_SIZE);
    fprintf(fp, "#define L1_DCACHE_SETS %d\n", L1_DCACHE_SETS);
    fprintf(fp, "#define L1_DCACHE_ASSOS %d\n", L1_DCACHE_ASSOS);
    fprintf(fp, "\n");
    fprintf(fp, "#define L1_ICACHE_SIZE %d\n", L1_ICACHE_SIZE);
    fprintf(fp, "#define L1_ICACHE_SETS %d\n", L1_ICACHE_SETS);
    fprintf(fp, "#define L1_ICACHE_ASSOS %d\n", L1_ICACHE_ASSOS);
    fprintf(fp, "\n");
    fprintf(fp, "#define L2_UCACHE_SIZE %d\n", L2_UCACHE_SIZE);
    fprintf(fp, "#define L2_UCACHE_SETS %d\n", L2_UCACHE_SETS);
    fprintf(fp, "#define L2_UCACHE_ASSOS %d\n", L2_UCACHE_ASSOS);
    fprintf(fp, "\n");
    fprintf(fp, "#define L3_UCACHE_SIZE %d\n", L3_UCACHE_SIZE);
    fprintf(fp, "#define L3_UCACHE_SETS %d\n", L3_UCACHE_SETS);
    fprintf(fp, "#define L3_UCACHE_ASSOS %d\n", L3_UCACHE_ASSOS);
    fprintf(fp, "\n");
    fprintf(fp, "#endif\n");
    fclose(fp);
}


}  // namespace sysi

#endif
