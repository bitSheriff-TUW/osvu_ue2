/**
 * @file generator.c
 * @author  Benjamin Mandl (12220853)
 * @date 2023-11-07
 */

#include <inttypes.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "errors.h"

static void readEdges(edge_t* pEdges[], char** argv, ssize_t argc)
{
    // check if edges were given, more than one is needed
    if (2 > argc)
    {
        emit_error("Not enough parameter given\n", ERROR_PARAM);
    }

    // step through all the given parameters and parse the endges
    for (ssize_t i = 1U; i < argc; i++)
    {
        // the verticies are sepertaed with a dash, and the edges with a space
        if (sscanf(argv[i], "%hu-%hu", &((*pEdges)[i - 1].start), &((*pEdges)[i - 1].end)) < 0)
        {
            emit_error("Something went wrong with reading edges\n", ERROR_PARAM);
        }

        // check for loop
        if ((*pEdges)[i - 1].start == (*pEdges)[i - 1].end)
        {
            emit_error("Loops are not allowed\n", ERROR_PARAM);
        }
    }
}

error_t init_semaphores(sems_t* pSems)
{
    error_t retCode = ERROR_OK;

    // open the named semaphores
    pSems->mutex_write = sem_open(SEM_NAME_MUTEX, 0);
    pSems->buffer_empty = sem_open(SEM_NAME_EMPTY, 0);
    pSems->buffer_full = sem_open(SEM_NAME_FULL, 0);

    // check if the opening was succesfull
    if ((SEM_FAILED == pSems->buffer_empty) || (SEM_FAILED == pSems->mutex_write) || (SEM_FAILED == pSems->buffer_full))
    {
        retCode = ERROR_SEMAPHORE;
    }

    return retCode;
}

error_t cleanup_semaphores(sems_t* pSems)
{
    error_t retCode = ERROR_OK;

    if (sem_close(pSems->buffer_full) == -1)
    {
        debug("Semaphore Close error: Buffer Full\n", NULL);
        retCode |= ERROR_SEMAPHORE;
    }

    if (sem_close(pSems->buffer_empty) == -1)
    {
        debug("Semaphore Close error: Buffer Empty\n", NULL);
        retCode |= ERROR_SEMAPHORE;
    }

    if (sem_close(pSems->mutex_write) == -1)
    {
        debug("Semaphore Close error: Mutex\n", NULL);
        retCode |= ERROR_SEMAPHORE;
    }

    return retCode;
}

error_t write_solution(sems_t* pSems)
{
    error_t retCode = ERROR_OK;
    if (sem_wait(pSems->mutex_write) < 0) return ERROR_SEMAPHORE;
    // TODO: do something

    if (sem_post(pSems->mutex_write) < 0) return ERROR_SEMAPHORE;
    return retCode;
}

int main(int argc, char* argv[])
{
    debug("This is the generator\n", NULL);
    error_t retCode = ERROR_OK;                      /*!< return code for error handling */
    ssize_t edgeCnt = argc - 1U;                     /*!< number of given edges */
    edge_t* edges = calloc(sizeof(edge_t), edgeCnt); /*!< memory to store all edges */
    sems_t semaphores = {0U};                        /*!< struct of all needed semaphores */

    debug("Number of Edges: %ld\n", edgeCnt);

    // read the edges from the parameters
    readEdges(&edges, argv, argc);

    retCode |= init_semaphores(&semaphores);

    if (ERROR_OK != retCode)
    {
        emit_error("Something was wrong with the semaphores\n", retCode);
    }

    cleanup_semaphores(&semaphores);

    return EXIT_SUCCESS;
}
