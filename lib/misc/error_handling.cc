#include <misc/error_handling.h>

void EXIT_FUNC
ERR::errdie(const char * file_name,
            uint32_t     line_number,
            int32_t      error_number,
            const char * msg,
            ...) {
    fprintf(stderr, "%s:%d: ", file_name, line_number);
    va_list ap;
    if (msg) {
        va_start(ap, msg);
        vfprintf(stderr,  // NOLINT
                 msg,     // NOLINT
                 ap);     // NOLINT /* This warning is a clang-tidy bug */
        va_end(ap);
    }
    fprintf(stderr, "\t%d:%s\n", error_number, strerror(error_number));
    exit(-1);
}

void EXIT_FUNC
ERR::die(const char * file_name, uint32_t line_number, const char * msg, ...) {
    fprintf(stderr, "%s:%d: ", file_name, line_number);
    va_list ap;
    if (msg) {
        va_start(ap, msg);
        vfprintf(stderr,  // NOLINT
                 msg,     // NOLINT
                 ap);     // NOLINT /* This warning is a clang-tidy bug */
        va_end(ap);
    }
    exit(-1);
}
