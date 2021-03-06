#ifndef _ERROR_HANDLING_H_
#define _ERROR_HANDLING_H_

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <misc/cpp_attributes.h>
#include <misc/macro_helper.h>
// Error* versions are if errno will have been set.
// Die* versions are if not errno related

namespace ERR {

#define ERROR_ASSERT(...)                                                      \
    CAT(ERROR_ASSERT_, NOT_ONE_NARG(__VA_ARGS__))(__VA_ARGS__)

#define DIE_ASSERT(...) CAT(DIE_ASSERT_, NOT_ONE_NARG(__VA_ARGS__))(__VA_ARGS__)

#define ERROR_ASSERT_MANY(X, msg, args...)                                     \
    if (BRANCH_UNLIKELY(!(X))) {                                               \
        ERROR_DIE(msg, ##args);                                                \
    }

#define ERROR_ASSERT_ONE(X)                                                    \
    if (BRANCH_UNLIKELY(!(X))) {                                               \
        ERROR_DIE(NULL);                                                       \
    }

#define DIE_ASSERT_MANY(X, msg, args...)                                       \
    if (BRANCH_UNLIKELY(!(X))) {                                               \
        DIE(msg, ##args);                                                      \
    }

#define DIE_ASSERT_ONE(X)                                                      \
    if (BRANCH_UNLIKELY(!(X))) {                                               \
        DIE(NULL);                                                             \
    }

#define DIE(msg, args...)                                                      \
    ERR::die(__FILE__, __LINE__, msg, ##args);                                 \
    _UNREACHABLE_;

#define ERROR_DIE(msg, args...)                                                \
    ERR::errdie(__FILE__, __LINE__, errno, msg, ##args);                       \
    _UNREACHABLE_;


void EXIT_FUNC errdie(const char * file_name,
                      uint32_t     line_number,
                      int32_t      error_number,
                      const char * msg,
                      ...);

void EXIT_FUNC die(const char * file_name,
                   uint32_t     line_number,
                   const char * msg,
                   ...);

}  // namespace ERR
#endif
