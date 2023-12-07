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
    size_t limit;  /*!< number of generated solutions */
    uint16_t delayS; /*!< delay [s] before the starting to read the buffer */
} options_t;

/**
 * @brief Signal Interrupt
 * @details This global boolean flag is used to check if a signal interrupt happened
 */
static bool gSigInt = false; /*!< Signal Interrupt */
static const char* gAppName; /*!< Name of the application */

/**
 * @brief   Usage
 * @details This internal method is used to print the usage message and exit the application.
 * @param   msg     Message which will be printed
 */
static void usage(char* msg)
{
    // print the usage message
    fprintf(stderr, "%s\nUsage: %s [-n limit] [-w delay]\n", msg, gAppName);
    emit_error(msg, ERROR_PARAM);
}

/**
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
static void handle_opts(int argc, char* argv[], options_t* pOpts)
{
    int16_t ret = 0;

    // unlimited solutions per default
    pOpts->limit = 0U;

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
                if (0U != pOpts->limit)
                {
                    usage("Option was given more than once\n");
                }
                pOpts->limit = (size_t)strtol(optarg, NULL, 0);
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

/**
 * @brief   Initialize Semaphores
 * @details This internal method is used to initialize the semaphores.
 *          If there are semaphores which were not closed properly (due to error), they will be unlinked first.
 *          After that the semaphores will be created and stored in the given bundle.
 *
 * @param   pSems   Pointer to the semaphore bundle
 *
 * @return  error_t Error Code
 * @retval  ERROR_OK            Everything was successful
 * @retval  ERROR_SEMAPHORE     Something was wrong with opening the semaphores
 */
static error_t init_semaphores(sems_t* pSems)
{
    error_t retCode = ERROR_OK;

    // unlink if there are semaphores which were not closed properly (due to error)
    sem_unlink(SEM_NAME_MUTEX);
    sem_unlink(SEM_NAME_READ);
    sem_unlink(SEM_NAME_WRITE);

    // create the named semaphores
    pSems->mutex_write = sem_open(SEM_NAME_MUTEX, O_CREAT, 0666, 1);           // 1 = available for one to write
    pSems->reading = sem_open(SEM_NAME_READ, O_CREAT, 0666, 0);                // 0 = empty
    pSems->writing = sem_open(SEM_NAME_WRITE, O_CREAT, 0666, CIRBUF_BUFSIZE);  // CIRBUF_BUFSIZE = full

    // check if the opening was successful
    if ((SEM_FAILED == pSems->reading) || (SEM_FAILED == pSems->mutex_write) || (SEM_FAILED == pSems->writing))
    {
        debug("Semaphore Open error: %d\n", errno);
        retCode = ERROR_SEMAPHORE;
    }

    return retCode;
}

/**
 * @brief   Cleanup Semaphores
 * @details This internal method is used to cleanup the semaphores.
 *          If closing the semaphores fails, then they will not get unlinked, to avoid getting errors.
 *
 * @param   pSems   Pointer to the semaphore bundle
 *
 * @return  error_t Error Code
 * @retval  ERROR_OK            Everything was successful
 * @retval  ERROR_SEMAPHORE     Something was wrong with closing the semaphores
 */
static error_t cleanup_semaphores(sems_t* pSems)
{
    error_t retCode = ERROR_OK;         /*!< return code */

    if (sem_close(pSems->writing) == -1)
    {
        debug("Semaphore Close error: Buffer Full\n", NULL);
        retCode |= ERROR_SEMAPHORE;
    } else
    {
        // unlink the semaphore
        if (sem_unlink(SEM_NAME_WRITE) == -1)
        {
            debug("Semaphore Unlink error: Buffer Full\n", NULL);
            retCode |= ERROR_SEMAPHORE;
        }
        else
        {
            debug("Semaphore Unlink successful: Buffer Full\n", NULL);
        }
    }

    if (sem_close(pSems->reading) == -1)
    {
        debug("Semaphore Close error: Buffer Empty\n", NULL);
        retCode |= ERROR_SEMAPHORE;
    } else
    {
        // unlink the semaphore
        if (sem_unlink(SEM_NAME_READ) == -1)
        {
            debug("Semaphore Unlink error: Buffer Empty\n", NULL);
            retCode |= ERROR_SEMAPHORE;
        }
        else
        {
            debug("Semaphore Unlink successful: Buffer Empty\n", NULL);
        }
    }

    if (sem_close(pSems->mutex_write) == -1)
    {
        debug("Semaphore Close error: Mutex\n", NULL);
        retCode |= ERROR_SEMAPHORE;
    } else
    {
        debug("Semaphore Close successful: Mutex\n", NULL);
        // unlink the semaphore
        if (sem_unlink(SEM_NAME_MUTEX) == -1)
        {
            debug("Semaphore Unlink error: Mutex\n", NULL);
            retCode |= ERROR_SEMAPHORE;
        }
        else
        {
            debug("Semaphore Unlink successful: Mutex\n", NULL);
        }
    }

    return retCode;
}

/**
 * @brief   Handle Signal Interrupt
 * @details This internal method is used to handle the signal interrupt.
 *          It will set the global variable to true, so the main loop will be exited.
 *
 * @param   sig     Signal which was received
 */
static void handle_sigint(int32_t sig)
{
    // set the global variable to true
    gSigInt = true;
    debug("SIGINT received\n", NULL);
}

/**
 * @brief   Initialize Shared Memory
 * @details This internal method is used to initialize the shared memory.
 *          If there is already a shared memory, it will be unlinked first.
 *          After that the shared memory will be created (file descriptor) and mapped to an address.
 *          The opening as a file descriptor is allowed to fail once. Further the memory is truncated to the size of
 *          the shared memory struct.
 *          After that the memory is reset to 0 and the file descriptor is closed.
 *
 * @param   pSharedMem  Pointer to the shared memory
 * @param   pFd         Pointer to the file descriptor
 *
 * @return  error_t Error Code
 * @retval  ERROR_OK            Everything was successful
 * @retval  ERROR_SHMEM         Something was wrong with the shared memory
 */
static error_t init_shmem(shared_mem_t** pSharedMem, int16_t* pFd)
{
    error_t retCode = ERROR_OK;

    // unlink the shared memory if a file already exists
    shm_unlink(SHAREDMEM_FILE);

    // open the shared memory
    *pFd = shm_open(SHAREDMEM_FILE, O_RDWR | O_CREAT, 0600);

    // check if the opening was successful
    if (*pFd < 0)
    {
        retCode = ERROR_SHMEM;
        debug("Opening failed %d\n", errno);

        debug("Already exists, try to unlink\n", NULL);
        shm_unlink(SHAREDMEM_FILE);

        *pFd = shm_open(SHAREDMEM_FILE, O_RDWR | O_CREAT, 0600);

        if (*pFd < 0)
        {
            debug("Opening failed again, exit %d\n", errno);
            return retCode;
        } else
        {
            // everything was successful (on the second try)
            retCode = ERROR_OK;
        }
    }

    if (ftruncate(*pFd, sizeof(shared_mem_t)) < 0)
    {
        debug("ftruncate failed\n", NULL);
        retCode = ERROR_SHMEM;
        return retCode;
    }

    // map the shared memory
    *pSharedMem = (shared_mem_t*)mmap(NULL, sizeof(shared_mem_t), PROT_READ | PROT_WRITE, MAP_SHARED, *pFd, 0);

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

/**
 * @brief   Get Solution
 * @details This internal method is used to get a solution from the shared memory.
 *          It will read the edges from the shared memory until it finds the delimiter.
 *          The edges will be stored in the given array.
 *
 * @param   pSharedMem  Pointer to the shared memory
 * @param   pSems       Pointer to the semaphores
 * @param   pEdges      Pointer to the array of edges
 * @param   pEdgeCnt    Pointer to the number of edges
 *
 * @return  error_t Error Code
 * @retval  ERROR_OK            Everything was successful
 * @retval  ERROR_CIRBUF_EMPTY  The circular buffer is empty
 */
static error_t get_solution(shared_mem_t* pSharedMem, sems_t* pSems, edge_t* pEdges[], size_t* pEdgeCnt)
{
    error_t retCode = ERROR_OK;
    edge_t currEdge = {0U};
    size_t iter = 0;
    *pEdgeCnt = SIZE_MAX;  // set max value, due to interrupt

    while (true)
    {
        retCode |= circular_buffer_read(&pSharedMem->circbuf, pSems, &currEdge);

        if (ERROR_SEMAPHORE == retCode)
        {
            return retCode;
        }
        
        if (is_edge_delimiter(currEdge))
        {
            break;
        }

        if (retCode != ERROR_OK)
        {
            debug("Error while reading\n", NULL);
            return retCode;
        } else
        {
            memcpy(&(*pEdges)[iter], &currEdge, sizeof(edge_t));
            iter++;
        }
    }

    *pEdgeCnt = iter;

    return retCode;
}

/**
 * @brief   Print Solution
 * @details This internal method is used to print a solution.
 *          It will print the edges of the given array.
 *          If the solution is empty, nothing will be printed, because there is
 *          a special message for that in the main function.
 *
 * @param   pEdges      Pointer to the array of edges
 * @param   edgeCnt     Number of edges
 */
static void print_solution(edge_t* pEdges, size_t edgeCnt)
{
    // do not print if there are no edges
    if (edgeCnt == 0U)
    {
        return;
    }

    fprintf(stderr, "Solution with %zu edges:", edgeCnt);
    for (size_t i = 0; i < edgeCnt; i++)
    {
        fprintf(stderr, " %d-%d", pEdges[i].start, pEdges[i].end);
    }
    fprintf(stderr, "%s", "\n");
}

/**
 * @brief   Main Function
 * @details This is the main function of the supervisor.
 *          The supervisor is responsible for reading the shared memory and determining the best solution.
 *          It will read the edges from the shared memory until it finds the delimiter. These read edges form a "solution".
 *          The supervisor will compare the size of the current solution with the best solution and store the smaller one.
 *          If a solution with 0 edges is found, the graph is acyclic and therefore the program can terminate.
 *
 * @note    The supervisor will terminate if the optional limit is reached or if a signal interrupt is received.
 *
 * @param   argc    Argument Counter
 * @param   argv    Argument Variables
 *
 * @return  int         Return Code
 * @retval  ERROR_OK    Everything was successful
 * @retval  else        Something was wrong
 */
int main(int argc, char* argv[])
{
    error_t retCode = ERROR_OK; /*!< return code */
    options_t opts = {0U};      /*!< bundle of options */
    sems_t semaphores = {0};
    shared_mem_t* pSharedMem = NULL;
    edge_t* bestSol = {0U};        /* best found solution */
    edge_t* currSol = {0U};        /* current solution */
    size_t bestSolSize = SIZE_MAX; /* size of the best solution */
    size_t currSolSize = SIZE_MAX; /* size of the current solution */
    int16_t fd = -1;               /* file descriptor of the shared memory */

    // set the application name
    gAppName = argv[0];

    // allocate the memory for the solutions
    bestSol = calloc(sizeof(edge_t), BEST_SOL_ARRAY_SIZE);
    currSol = calloc(sizeof(edge_t), BEST_SOL_ARRAY_SIZE);

    if ((bestSol == NULL) || (currSol == NULL))
    {
        emit_error("Something was wrong with allocating memory\n", retCode);
    }

    // register the signal handler
    struct sigaction sa = {.sa_handler = handle_sigint};
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

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

    // set the flag that the generators should be active
    pSharedMem->flags.genActive = true;

    if (opts.delayS > 0U)
    {
        // sleep for the given time
        sleep(opts.delayS);
        debug("Delay done\n", NULL);
    }

    // main operating loop
    debug("Starting main loop\n", NULL);

    // main loop
    // SIZE_MAX is the indicator for unlimited solutions
    while ((false == gSigInt) && ((pSharedMem->flags.numSols < opts.limit) || (opts.limit == 0U)))
    {
        // reset the memory
        memset(currSol, 0, sizeof(edge_t) * BEST_SOL_ARRAY_SIZE);
        currSolSize = SIZE_MAX;

        // check if there is something to read, and further if semaphores are successful
        retCode |= get_solution(pSharedMem, &semaphores, &currSol, &currSolSize);

        if (ERROR_OK != retCode)
        {
            debug("Error while reading: %d\n", retCode);
            break;
        }
        
        if (currSolSize < bestSolSize)
        {
            print_solution(currSol, currSolSize);
            memcpy(bestSol, currSol, sizeof(edge_t) * currSolSize);
            bestSolSize = currSolSize;
        }

        // no edges needed to be removed, so finish because acyclic
        if (bestSolSize == 0U)
        {
            break;
        }
    }

    if ( ((retCode & ERROR_SIGINT) != 0) || ((retCode & ERROR_SEMAPHORE) != 0 ))
    {
        // just a signal received, so everything is fine
        retCode = ERROR_OK;
    }
    

    pSharedMem->flags.genActive = false;

    // print the best solution
    switch (bestSolSize)
    {
        case 0U:
            fprintf(stdout, "The graph is acyclic!\n");
            break;
        case SIZE_MAX:
            fprintf(stdout, "The graph might not be acyclic, no solution found.\n");
            break;
        default:
            fprintf(stdout, "The graph might not be acyclic, best solution removes %zu edges.\n", bestSolSize);
            break;
    }

    // free the memory
    free(bestSol);
    free(currSol);
    bestSol = NULL;
    currSol = NULL;

    // unmap memory
    if (munmap(pSharedMem, sizeof(shared_mem_t)) == 0)
    {
        debug("Unmapping successful\n", NULL);
        if (0U != shm_unlink(SHAREDMEM_FILE))
        {
            debug("Unlinking failed errno: %d\n", errno);
        } else
        {
            debug("Unlinking successful\n", NULL);
        }
    } else
    {
        debug("Unmapping failed\n", NULL);
    }

    retCode |= cleanup_semaphores(&semaphores);

    // all error should be handled before
    retCode = ERROR_OK;

    return retCode;
}
