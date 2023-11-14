#pragma once

/**
 * @file  common.h
 * @author  Benjamin Mandl (12220853)
 * @date 2023-11-07
 * @brief Stuff that every module needs
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "errors.h"

/** @brief Name of the shared memory file */
#define SHAREDMEM_FILE "sharedMem"

/*!
 * @struct shared_mem_flags_t
 * @brief   Bundle for all runtime and configuration flags
 *
 * @details
 *
 **/
typedef struct
{
    bool genActive; /*!< Flag that the generatros should be active */
} shared_mem_flags_t;

typedef struct
{
    shared_mem_flags_t flags;

} shared_mem_t;

/*!
 * @brief       Emit Error
 *
 * @details     This internal method is used to print an error message and exit the application
 *              after, because it won't be able to function like intended.
 *
 * @param       msg         String which will be printed to stderr
 * @param       retCode     Error code which will be printed (only if the code is not 0)
 *
 **/
inline void emit_error(char* msg, error_t retCode)
{
    // only print the error code if some was attached
    if (ERROR_OK != retCode)
    {
        fprintf(stderr, "%s code: %d\n", msg, retCode);
    } else
    {
        fprintf(stderr, "%s\n", msg);
    }
    exit(EXIT_FAILURE);
}
