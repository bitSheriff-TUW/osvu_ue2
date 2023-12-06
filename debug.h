/**
 * @file debug.h
 * @author  Benjamin Mandl (12220853)
 * @date 2023-10-18
 */

#ifndef DEBUG_H_
#define DEBUG_H_
#include <stdio.h>

    #ifdef DEBUG

    /** @brief Macro for debug pid output. */
    #define debug_pid(fmt, ...) \
    (void) fprintf(stderr, "[%s:%d@%s PID:%d] " fmt "\n", \
    __FILE__,  __LINE__, __func__,  getpid(), ##__VA_ARGS__)

    /** @brief Macro for debug output. */
    #define debug(fmt, ...) \
    (void) fprintf(stderr, "[%s:%d@%s] " fmt "\n", \
    __FILE__,  __LINE__, __func__, ##__VA_ARGS__)

    #else

    #define debug_pid(fmt, ...) /* NOP */
    #define debug(fmt, ...) /* NOP */


    #endif 
#endif  // DEBUG_H_
