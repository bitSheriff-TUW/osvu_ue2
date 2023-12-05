/**
 * @file supervisor.c
 * @author  Benjamin Mandl (12220853)
 * @date 2023-11-07
 */

#include <getopt.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "errors.h"

/**
 * @brief Bundle of options
 * @details This bundle is used to bundle all option of this module for easier access.
 */
typedef struct
{
    bool print;      /*!< boolean value of the graph should be printed */
    uint16_t limit;  /*!< number of generated solutions */
    uint16_t delayS; /*!< delay [s] before the starting to read the buffer */
} options_t;

/**
 * @brief Signal Interrupt
 * @details This global boolean flag is used to check if a signal interrupt happend
 */
static bool gSigInt = false;

static void usage(char* msg)
{
    // TODO: add usage prints
    emit_error(msg, ERROR_PARAM);
}

/*!
 * @brief   Handle Options
 *
 * @details This internal method is used to read the option given by the user.
 *
 * @warning For the limit and the delay a maximum of 65535 can be used, which would be around 1000h of delay....
 *
 * @param   argc    Argument Counter
 * @param   argv    Argument Variables
 * @param   pOpts   Pointer to the option bundle
 **/
static void handle_opts(int argc, char** argv, options_t* pOpts)
{
    int16_t ret = 0;

    // init limit with max
    pOpts->limit = UINT16_MAX;

    while ((ret = getopt(argc, argv, "pn:w:")) != -1)
    {
        switch (ret)
        {
            // Print
            case 'p': {
                if (false != pOpts->print)
                {
                    /* option was given two times */
                    usage("Option was given more than once\n");
                }
                pOpts->print = true;
                break;
            }

            // Limit
            case 'n': {
                // check if option was given more than once
                if (UINT16_MAX != pOpts->limit)
                {
                    usage("Option was given more than once\n");
                }
                pOpts->limit = (uint16_t)strtol(optarg, NULL, 0);
                break;
            }

            // Delay
            case 'w': {
                // check if option was given more than once
                if (0 != pOpts->delayS)
                {
                    usage("Option was given more than once\n");
                }

                pOpts->delayS = (uint16_t)strtol(optarg, NULL, 0);
                break;
            }

            // Unknown option
            default: {
                usage("Unknown option\n");
                break;
            }
        }
    }
}

error_t init_semaphores(sems_t* pSems)
{
    error_t retCode = ERROR_OK;

    // unlink if there are semaphores which were not closed properly (due to error)
    sem_unlink(SEM_NAME_MUTEX);
    sem_unlink(SEM_NAME_READ);
    sem_unlink(SEM_NAME_WRITE);

    // create the named semaphores
    pSems->mutex_write = sem_open(SEM_NAME_MUTEX, O_CREAT | O_EXCL, 0666, 1);
    pSems->reading = sem_open(SEM_NAME_READ, O_CREAT | O_EXCL, 0666, 0);
    pSems->writing = sem_open(SEM_NAME_WRITE, O_CREAT | O_EXCL, 0666, CIRBUF_BUFSIZE);

    // check if the opening was succesfull
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

    // delete the semaphore files, if something fails here, there is now proper way to react, so ignore the retVal
    (void)sem_unlink(SEM_NAME_MUTEX);
    (void)sem_unlink(SEM_NAME_READ);
    (void)sem_unlink(SEM_NAME_WRITE);

    return retCode;
}

void handle_sigint(int32_t sig)
{
    // set the global variable to true
    gSigInt = true;
    debug("SIGINT received\n", NULL);
}

error_t init_shmem(shared_mem_t** pSharedMem, int16_t* pFd)
{
    error_t retCode = ERROR_OK;

    // unlink the shared memory if a file already exists
    shm_unlink(SHAREDMEM_FILE);

    // open the shared memory
    *pFd = shm_open(SHAREDMEM_FILE, O_RDWR | O_CREAT | O_EXCL, 0600);

    // check if the opening was successful
    if (*pFd < 0)
    {
        retCode = ERROR_SHMEM;
        debug("Opening failed %d\n", errno);

        debug("Already exists, try to unlink\n", NULL);
        shm_unlink(SHAREDMEM_FILE);

        *pFd = shm_open(SHAREDMEM_FILE, O_RDWR | O_CREAT | O_EXCL, 0600);

        if (*pFd < 0)
        {
            debug("Opening failed again, exit %d\n", errno);
          return retCode;
        }
        else
        {
            // everything was successful (on the second try)
            retCode = ERROR_OK;
        }
    }

    if(ftruncate(*pFd, sizeof(shared_mem_t)) < 0)
	{
        debug("ftruncate failed\n", NULL);
        retCode = ERROR_SHMEM;
        return retCode;
	}

    // map the shared memory
    *pSharedMem = (shared_mem_t*) mmap(NULL, sizeof(shared_mem_t), PROT_READ | PROT_WRITE, MAP_SHARED, *pFd, 0);

    // check if the mapping was successful
    if (MAP_FAILED == *pSharedMem)
    {
        retCode = ERROR_SHMEM;
        debug("Mapping failed\n", NULL);
        return retCode;
    }

    // if everything was successful, reset the memory
    memset(*pSharedMem, 0, sizeof(shared_mem_t));
    close(*pFd);

    return retCode;
}

size_t size_of_graph(edge_t* pEds)
{
    size_t size = 0U;
    while ( (is_edge_delimiter(pEds[size])) && (size < BEST_SOL_MAX_EDGES))
    {
        size++;
    }

    return size;
}

error_t get_solution(shared_mem_t* pSharedMem, sems_t* pSems,  edge_t** pEdges, size_t* pEdgeCnt)
{
    error_t retCode = ERROR_OK;
    edge_t currEdge = {0U};
    size_t iter = 0;
    *pEdgeCnt = SIZE_MAX; // set max value, due to interrupt

    while(true)
    {
        retCode |= circular_buffer_read(&pSharedMem->circbuf, pSems, &currEdge);

        debug("Read edge %d with %d-%d\n", iter, currEdge.start, currEdge.end);

        if (is_edge_delimiter(currEdge))
        {
            break;
        }
       

        if(retCode != ERROR_OK)
        {
            debug("Error while reading\n", NULL);
            return retCode;
        }
        else
        {
            memcpy(&(*pEdges)[iter], &currEdge, sizeof(edge_t));
            iter++;
        }
    }

    *pEdgeCnt = iter;
    
    return retCode;
}

int main(int argc, char* argv[])
{
    error_t retCode = ERROR_OK; /*!< return code */
    options_t opts = {0U};      /*!< bundle of options */
    sems_t semaphores = {0};
    shared_mem_t* pSharedMem = NULL;
    edge_t* bestSol = {0U}; /* best found solution */
    edge_t* currSol = {0U}; /* current solution */
    size_t bestSolSize = SIZE_MAX;                   /* size of the best solution */
    size_t currSolSize = SIZE_MAX;                   /* size of the current solution */
    int16_t fd = -1;                           /* file descriptor of the shared memory */

    // allocate the memory for the solutions
    bestSol = calloc(sizeof(edge_t), BEST_SOL_MAX_EDGES);
    currSol = calloc(sizeof(edge_t), BEST_SOL_MAX_EDGES);

    // register the signal handler
    struct sigaction sa = { 
    .sa_handler = handle_sigint 
    };
    sigaction(SIGINT, &sa, NULL);

    /* get the options */
    handle_opts(argc, argv, &opts);
    debug("Options: Print: %d, Limit: %d, Delay: %d\n", opts.print, opts.limit, opts.delayS);

    retCode |= init_semaphores(&semaphores);
    debug("Semaphores initialized\n", NULL);

    if (ERROR_OK != retCode)
    {
        emit_error("Something was wrong with creating the semaphores\n", retCode);
    }

    retCode |= init_shmem(&pSharedMem, &fd);
    debug("Shared Memory initialized: fd: %d, addr: %d\n", fd, pSharedMem);

    // main operating loop
    debug("Starting main loop\n", NULL);

    // set the flag that the generators should be active
    pSharedMem->flags.genActive = true;

    while ((false == gSigInt) && (pSharedMem->flags.numSols < opts.limit))
    {
        currSolSize = SIZE_MAX;
        int semValWr, semValRd, semValMut;
        sem_getvalue(semaphores.writing, &semValWr);
        sem_getvalue(semaphores.reading, &semValRd);
        sem_getvalue(semaphores.mutex_write, &semValMut);
        debug("Sem Write: %d, Sem Read: %d, Mut: %d Sols: %d\n", semValWr, semValRd, semValMut, pSharedMem->flags.numSols);

        sem_wait(semaphores.reading);

        get_solution(pSharedMem, &semaphores, &currSol, &currSolSize);

        debug("Best: %d, Curr: %d\n", bestSolSize, currSolSize);

        if((currSolSize < bestSolSize) && (currSolSize != 0U))
        {
            memcpy(bestSol, currSol, sizeof(edge_t) * currSolSize);
            bestSolSize = currSolSize;
            debug("New best solution found with %d edges removed\n", bestSolSize);
        }

        // if (bestSolSize == 0U)
        // {
        //     fprintf(stdout, "The graph is acyclic!\n");
        //     break;
        // }
        
        sem_post(semaphores.writing);
    }

    pSharedMem->flags.genActive = false;

    // print the best solution
    fprintf(stdout, "Best Solution removes %ld edges\n", bestSolSize);

    free(bestSol);
    free(currSol);

    // unmap memory
    munmap(pSharedMem, sizeof(shared_mem_t));
    shm_unlink(SHAREDMEM_FILE);
    retCode |= cleanup_semaphores(&semaphores);

    return retCode;
}
