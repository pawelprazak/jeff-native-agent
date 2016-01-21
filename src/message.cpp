#include "message.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>

void stdout_message(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    (void) vfprintf(stdout, format, ap);
    va_end(ap);
}

void stderr_message(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    (void) vfprintf(stderr, format, ap);
    (void) fflush(stderr);
    va_end(ap);
}

void fatal_error(const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    (void) vfprintf(stderr, format, ap);
    (void) fflush(stderr);
    va_end(ap);
    exit(3);
}