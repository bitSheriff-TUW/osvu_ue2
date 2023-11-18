/**
 * @file debug.h
 * @author  Benjamin Mandl (12220853)
 * @date 2023-10-18
 */

#ifdef DEBUG
#include <stdio.h>

#define debug(fmt, ...) (void)fprintf(stderr, "[%s:%d@%s] " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else

#define debug(fmt, ...) /* NOP */

#endif
