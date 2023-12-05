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
        // the vertices are separated with a dash, and the edges with a space
        if (sscanf(argv[i], "%hu-%hu", &((*pEdges)[i - 1].start), &((*pEdges)[i - 1].end)) < 0)
        {
            emit_error("Something went wrong with reading edges\n", ERROR_PARAM);
        }

        // check for loop
        if ((*pEdges)[i - 1].start == (*pEdges)[i - 1].end)
        {
            emit_error("Loops are not allowed\n", ERROR_PARAM);
        }
        debug("got Edge: %hu-%hu\n", (*pEdges)[i - 1].start, (*pEdges)[i - 1].end);
    }
}

error_t init_semaphores(sems_t* pSems)
{
    error_t retCode = ERROR_OK;

    // open the named semaphores
    pSems->mutex_write = sem_open(SEM_NAME_MUTEX, 1);
    pSems->reading = sem_open(SEM_NAME_READ, 0);
    pSems->writing = sem_open(SEM_NAME_WRITE, 0);

    // check if the opening was successful
    if ((SEM_FAILED == pSems->reading) || (SEM_FAILED == pSems->mutex_write) || (SEM_FAILED == pSems->writing))
    {
        debug("Semaphore Open error: %d\n", errno);
        retCode = ERROR_SEMAPHORE;
    }

    return retCode;
}

error_t cleanup_semaphores(sems_t* pSems)
{
    error_t retCode = ERROR_OK;

    if (sem_close(pSems->writing) == -1)
    {
        debug("Semaphore Close error: Buffer Full\n", NULL);
        retCode |= ERROR_SEMAPHORE;
    }

    if (sem_close(pSems->reading) == -1)
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


error_t init_shmem(shared_mem_t** pSharedMem, int16_t* pFd)
{
    error_t retCode = ERROR_OK;

    // open the shared memory
    *pFd = shm_open(SHAREDMEM_FILE, O_RDWR, 0600);

    // check if the opening was successful
    if (*pFd < 0)
    {
        retCode = ERROR_SHMEM;
        debug("Opening failed %d\n", errno);
        return retCode;
    }
    debug("Shared fd: %d\n", *pFd);

    // map the shared memory
    *pSharedMem = (shared_mem_t*) mmap(NULL, sizeof(shared_mem_t), PROT_READ | PROT_WRITE, MAP_SHARED, *pFd, 0);
    debug("Shared mem: %p\n", *pSharedMem);

    // check if the mapping was successful
    if (MAP_FAILED == *pSharedMem)
    {
        retCode = ERROR_SHMEM;
        debug("Mapping failed\n", NULL);
        return retCode;
    }

    close(*pFd);

    return retCode;
}

error_t write_solution(shared_mem_t* pSharedMem, sems_t* pSems, edge_t* pEdges, ssize_t edgeCnt)
{
    error_t retCode = ERROR_OK;
    edge_t del = { DELIMITER_VERTEX, DELIMITER_VERTEX};

    if (sem_wait(pSems->mutex_write) < 0) return ERROR_SEMAPHORE;

    for(ssize_t i = 0U; i < edgeCnt; i++)
    {
        // only write edges that are not the delimiter, or default
        if (pEdges[i].start != 0U)
        {
            retCode |= circular_buffer_write(&pSharedMem->circbuf, pSems, &pEdges[i]);
            debug("Writing edge %d with %d-%d\n", i, pEdges[i].start, pEdges[i].end);
        }
    }

    // write the delimiter
    retCode |= circular_buffer_write(&pSharedMem->circbuf, pSems, &del);

    // increase the number of solutions
    pSharedMem->flags.numSols++;

    if (sem_post(pSems->mutex_write) < 0) return ERROR_SEMAPHORE;

    return retCode;
}

int16_t* get_vertices(edge_t* pEdges, ssize_t edgeCnt)
{
    int16_t* vert = calloc(sizeof(int16_t), edgeCnt * 2);
    memset(vert, -1, sizeof(int16_t) * edgeCnt * 2);

    bool* isIncl = calloc(sizeof(bool), edgeCnt * 2);
    memset(isIncl, false, sizeof(bool) * edgeCnt * 2);

    for (size_t i = 0; i < edgeCnt; i++)
    {
        edge_t currEdge = pEdges[i];

        if (!isIncl[currEdge.start])
        {
            vert[i] = currEdge.start;
            isIncl[currEdge.start] = true;
            debug("Vert: %d\n", currEdge.start);
        }

        if (!isIncl[currEdge.end])
        {
            vert[i + edgeCnt/2] = currEdge.end;
            isIncl[currEdge.end] = true;
            debug("Vert: %d\n", currEdge.end);
        }
    }
    
    free(isIncl);
    return vert;
}

void shuffle(int16_t** pVert, ssize_t edgeCnt)
{
    // set the seed for the random number generator
    srand(getpid());

    // mix the vertices in the array
    for (ssize_t i = 0U; i < edgeCnt; i++)
    {
        ssize_t j = rand() % edgeCnt;
        int16_t temp = (*pVert)[i];
        (*pVert)[i] = (*pVert)[j];
        (*pVert)[j] = temp;
    }
}

void sortout_solution(edge_t** pEdges, ssize_t edgeCnt, int16_t* pVert)
{

    edge_t* temp = calloc(sizeof(edge_t), edgeCnt);
    uint16_t idxV1 = 0U;
    uint16_t idxV2 = 0U;
    size_t tempIdx = 0U;

    for (ssize_t i = 0U; i < edgeCnt; i++)
    {
        edge_t currEdge = (*pEdges)[i];

        // search for the indexes of the vertices
        for (ssize_t j = 0U; j < edgeCnt*2; j++)
        {
            if (pVert[j] == currEdge.start)
            {
                idxV1 = j;
            }

            if (pVert[j] == currEdge.end)
            {
                idxV2 = j;
            }
        }

        if (idxV1 < idxV2)
        {
            temp[tempIdx] = currEdge;
            tempIdx++;
        }
    }

    memcpy(*pEdges, temp, sizeof(edge_t) * edgeCnt);
    free(temp);
}


error_t generate_solution(edge_t* pOrigEdges, edge_t*pSolution, ssize_t edgeCnt)
{
    error_t retCode = ERROR_OK;

    // reset the solution
    memcpy(pSolution, pOrigEdges, sizeof(edge_t) * edgeCnt);

    int16_t* vert = get_vertices(pOrigEdges, edgeCnt);

    // shuffle the edges
    shuffle(&vert, edgeCnt);

    sortout_solution(&pSolution, edgeCnt, vert);


    free(vert);


    return retCode;
}



int main(int argc, char* argv[])
{
    debug("This is the generator\n", NULL);
    error_t retCode = ERROR_OK;                      /*!< return code for error handling */
    ssize_t edgeCnt = argc - 1U;                     /*!< number of given edges */
    edge_t* edges = calloc(sizeof(edge_t), edgeCnt); /*!< memory to store all edges */
    edge_t* solution = calloc(sizeof(edge_t), edgeCnt); /*!< memory to store a solution */
    sems_t semaphores = {0U};                        /*!< struct of all needed semaphores */
    shared_mem_t* pSharedMem = NULL;
    int16_t fd = -1;

    debug("Number of Edges: %ld\n", edgeCnt);

    // read the edges from the parameters
    readEdges(&edges, argv, argc);

    retCode |= init_semaphores(&semaphores);

    if (ERROR_OK != retCode)
    {
        cleanup_semaphores(&semaphores);
        emit_error("Something was wrong with the semaphores\n", retCode);
    }

    retCode |= init_shmem(&pSharedMem, &fd);

    if (ERROR_OK != retCode)
    {
        cleanup_semaphores(&semaphores);
        emit_error("Something was wrong with the shared memory\n", retCode);
    }


    int debugInt = 0; 

    while ( (pSharedMem->flags.genActive) && debugInt < 10)
    {
        int semValWr, semValRd, semValMut;
        sem_getvalue(semaphores.writing, &semValWr);
        sem_getvalue(semaphores.reading, &semValRd);
        sem_getvalue(semaphores.mutex_write, &semValMut);
        debug("Sem Write: %d, Sem Read: %d, Mut: %d Sols: %d\n", semValWr, semValRd, semValMut, pSharedMem->flags.numSols);

        // generate the solution
        retCode |= generate_solution(edges, solution, edgeCnt);

        // write the edges to the shared memory
        retCode |= write_solution(pSharedMem, &semaphores, solution, edgeCnt);

        sem_post(semaphores.reading);
        debugInt++;
    }


    // unmap memory
    munmap(pSharedMem, sizeof(shared_mem_t));
    cleanup_semaphores(&semaphores);
    free(edges);
    free(solution);

    return EXIT_SUCCESS;
}
