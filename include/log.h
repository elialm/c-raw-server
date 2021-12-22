#ifndef __LOG_H__
#define __LOG_H__

#ifdef DEBUG
#include <stdio.h>
#define PRINTF_DEBUG(format, ...)                           \
    do {                                                    \
        fprintf(stderr, "### DEBUG: %s:%i : ", __FILE__, __LINE__);  \
        fprintf(stderr, format"\n" __VA_OPT__(,) __VA_ARGS__);       \
    } while(0)
#else
#define PRINTF_DEBUG(format, ...) (void)0
#endif

#endif //__LOG_H__