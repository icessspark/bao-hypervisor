//
// Created by tonny on 8/7/20.
//

#ifndef BAO_PRINTF_H
#define BAO_PRINTF_H

#include <stdarg.h>

int printf(const char *fmt, ...);

#define panic(_)   \
    do {           \
        printf(_); \
        while (1)  \
            ;      \
    } while (0)

#endif  // BAO_PRINTF_H
