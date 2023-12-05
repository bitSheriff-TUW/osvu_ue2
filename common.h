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

/* **** SHARED MEMORY **** */
#include <fcntl.h> /* For O_* constants */
#include <semaphore.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */

#define SHAREDMEM_FILE "/12220853_sharedMem" /*!< Name of the shared memory file */
#define CIRBUF_BUFSIZE 32                    // TODO: better size
#define DELIMITER_VERTEX UINT16_MAX          /*!< Vertex for the delimiter, delimiter edge is defined by a loop to this vertex */

#define SEM_NAME_MUTEX "/12220853_sem_mutex"
#define SEM_NAME_EMPTY "/12220853_sem_empty"
#define SEM_NAME_FULL "/12220853_sem_full"

/*!
 * @struct edge_t
 * @brief  Structre to store edges (unidirected)
 **/
typedef struct
{
    uint16_t start; /*!< start vertex */
    uint16_t end;   /*!< end vertex */

} edge_t;

/*!
 * @struct shared_mem_flags_t
 * @brief   Bundle for all runtime and configuration flags
 *
 * @details
 *
 **/
typedef struct
{
    bool genActive; /*!< Flag that the generators should be active */
} shared_mem_flags_t;

typedef struct
{
    ssize_t head;               /*!< Index to the head (write end) */
    ssize_t tail;               /*!< Index to the tail (read end) */
    edge_t buf[CIRBUF_BUFSIZE]; /*!< actual memory of the circular buffer */
} shared_mem_circbuf_t;

typedef struct
{
    shared_mem_flags_t flags; /*!< All flags needed for the shared memory */

    shared_mem_circbuf_t circbuf; /*!< Bundle for the circular buffer */

} shared_mem_t;

// FIXME: remove if not needed
// typedef struct {
//
//     int16_t fd; /*!< file desrciptor */
//     void* pRawAddr; /*!< raw address of the mapped shared mem */
//     shared_mem_t* pSharedMem; /*!< casted pointer to the shared memory */
// }IF_shared_mem_t;

/*!
 * @struct sems_t
 * @brief  Structure of needed semaphores
 *
 * @details Bundle of semaphores
 *
 **/
typedef struct
{
    sem_t* mutex_write;  /*!< Mutex for the generator (writing to circular buffer) */
    sem_t* buffer_empty; /*!< semaphore to handle emptiness */
    sem_t* buffer_full;  /*!< sempaphore to handle fullness */
} sems_t;

/* **** FUNCTIONS **** */
void emit_error(char* msg, error_t retCode);
error_t circular_buffer_read(shared_mem_circbuf_t* pCirBuf, sems_t* pSems, edge_t* pResult);
error_t circular_buffer_write(shared_mem_circbuf_t* pCirBuf, sems_t* pSems, edge_t* pEd);
bool is_edge_delimiter(edge_t ed);
