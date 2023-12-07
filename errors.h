#pragma once

/**
 * @file errors.h
 * @author  Benjamin Mandl (12220853)
 * @date 2023-11-07
 */

#include <stdint.h>

typedef uint16_t error_t;


#define ERROR_OK 0U                 /*<! @brief No Error */
#define ERROR_FORK_FAILED 0x01U     /*<! @brief Fork Failed */
#define ERROR_PIPE_FAILED 0x02U     /*<! @brief Pipe Failed */
#define ERROR_PARAM 0x04U           /*<! @brief Param Error */
#define ERROR_CIRBUF_EMPTY 0x08U    /*<! @brief Circular Empty */
#define ERROR_CIRBUF_FULL 0x10U     /*<! @brief Circular Full */
#define ERROR_SEMAPHORE 0x20U       /*<! @brief Semaphore Error */
#define ERROR_NULLPTR 0x40U         /*<! @brief Nullpointer Error */
#define ERROR_SHMEM 0x80U           /*<! @brief Shared Memory Error */
#define ERROR_SIGINT 0x100U         /*<! @brief Signal Happend */
#define ERROR_LIMIT 0x200U          /*<! @brief Limit was reached */