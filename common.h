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

#include "debug.h"
#include "errors.h"

/* **** SHARED MEMORY **** */
#include <errno.h>
#include <fcntl.h> /* For O_* constants */
#include <fcntl.h>
#include <semaphore.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>

#define SHAREDMEM_FILE "12220853_sharedMem" /*!< Name of the shared memory file */
#define CIRBUF_BUFSIZE 256U                 /*!< Size of the circular buffer */
#define DELIMITER_VERTEX 0                  /*!< Vertex for the delimiter, delimiter edge is defined by a loop to this vertex */

#define SEM_NAME_MUTEX "12220853_sem_mutex"
#define SEM_NAME_READ "12220853_sem_read"
#define SEM_NAME_WRITE "12220853_sem_write"

#define BEST_SOL_MAX_EDGES 32U /*!< Maximum number of edges for the best solution */

/*!
 * @struct edge_t
 * @brief  Struct to store edges (unidirected)
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
    bool genActive;  /*!< Flag that the generators should be active */
    ssize_t numSols; /*!< Number of solutions found */
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

/*!
 * @struct sems_t
 * @brief  Structure of needed semaphores
 *
 * @details Bundle of semaphores
 *
 **/
typedef struct
{
    sem_t* mutex_write; /*!< Mutex for the generator (writing to circular buffer) */
    sem_t* writing;     /*!< semaphore to handle emptiness */
    sem_t* reading;     /*!< semaphore to handle fullness */
} sems_t;

/* **** FUNCTIONS **** */
void emit_error(char* msg, error_t retCode);
error_t circular_buffer_read(shared_mem_circbuf_t* pCirBuf, sems_t* pSems, edge_t* pResult);
error_t circular_buffer_write(shared_mem_circbuf_t* pCirBuf, sems_t* pSems, edge_t* pEd);
bool is_edge_delimiter(edge_t ed);
