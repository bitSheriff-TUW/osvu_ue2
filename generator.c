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
 * @brief   Read Edges
 * @details This internal method is used to read the edges from the argv array.
 *          The edges are stored in the pEdges array.
 *
 * @param   pEdges  Pointer to the array where the edges get written to
 * @param   argv    Array of parameters
 * @param   argc    Number of parameters
 */
static void readEdges(edge_t* pEdges[], char** argv, ssize_t argc)
{
    // check if edges were given, more than one is needed
    if (2 > argc)
    {
        usage("Not enough parameter given");
    }

    // step through all the given parameters and parse the edges
    for (ssize_t i = 1U; i < argc; i++)
    {
        // the vertices are separated with a dash, and the edges with a space
        if (sscanf(argv[i], "%hu-%hu", &((*pEdges)[i - 1].start), &((*pEdges)[i - 1].end)) < 0)
        {
            usage("Something went wrong with reading edges\n");
        }

        // check for loop
        if ((*pEdges)[i - 1].start == (*pEdges)[i - 1].end)
        {
            emit_error("Loops are not allowed\n", ERROR_PARAM);
        }
        debug("got Edge: %hu-%hu\n", (*pEdges)[i - 1].start, (*pEdges)[i - 1].end);
    }
}

/**
 * @brief   Init Semaphores
 * @details This internal method is used to initialize the semaphores.
 *        It is called when the application is starting.
 *
 * @param   pSems   Pointer to the struct of semaphores
 *
 * @return  retCode Error code
 * @retval  ERROR_OK            Everything went fine
 * @retval  ERROR_SEMAPHORE     Something went wrong with opening the semaphores
 */
static error_t init_semaphores(sems_t* pSems)
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

/**
 * @brief   Cleanup Semaphores
 * @details This internal method is used to close the semaphores.
 *         It is called when the application is terminating.
 *
 * @param   pSems   Pointer to the struct of semaphores
 *
 * @return  retCode Error code
 * @retval  ERROR_OK            Everything went fine
 * @retval  ERROR_SEMAPHORE     Something went wrong with closing the semaphores
 */
static error_t cleanup_semaphores(sems_t* pSems)
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

/**
 * @brief   Init Shared Memory
 * @details This internal method is used to initialize the shared memory.
 *          It is called when the application is starting.
 *          Firstly the shared memory is opened, then it is mapped.
 *          The pointer to the struct of shared memory is stored in the pSharedMem pointer.
 *          The file descriptor is stored in the pFd pointer.
 *
 * @param   pSharedMem  Pointer to the struct of shared memory
 * @param   pFd         Pointer to the file descriptor of the shared memory
 *
 * @returns retCode     Error code
 * @retval  ERROR_OK            Everything went fine
 * @retval  ERROR_SHMEM         Something went wrong with opening the shared memory
 */
static error_t init_shmem(shared_mem_t** pSharedMem, int16_t* pFd)
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
    *pSharedMem = (shared_mem_t*)mmap(NULL, sizeof(shared_mem_t), PROT_READ | PROT_WRITE, MAP_SHARED, *pFd, 0);
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

/**
 * @brief   Write Solution
 * @details This internal method is used to write a solution to the shared memory.
 *          It is called when a solution was found. It uses semaphores to synchronize the access to the shared memory.
 *          So that two different solutions get mixed up. Each solution is separated by a delimiter-edge.
 *
 * @param   pSharedMem  Pointer to the struct of shared memory
 * @param   pSems       Pointer to the struct of semaphores
 * @param   pEdges      Pointer to the array of edges
 * @param   edgeCnt     Number of edges
 * @param   pWritten    Pointer where written edges get written to
 *
 * @return  retCode     Error code
 * @retval  ERROR_OK            Everything went fine
 * @retval  ERROR_SEMAPHORE     Something went wrong with the semaphores
 */
static error_t write_solution(shared_mem_t* pSharedMem, sems_t* pSems, edge_t* pEdges, ssize_t edgeCnt, size_t* pWritten)
{
    error_t retCode = ERROR_OK;                        /*!< return code for error handling */
    edge_t del = {DELIMITER_VERTEX, DELIMITER_VERTEX}; /*!< delimiter edge, gets written at the end */

    *pWritten = 0U;  // reset the number of written edges

    if (sem_wait(pSems->mutex_write) < 0) return ERROR_SEMAPHORE;

    int semValWr, semValRd, semValMut;
    sem_getvalue(pSems->writing, &semValWr);
    sem_getvalue(pSems->reading, &semValRd);
    debug("Sem Write: %d, Sem Read: %d", semValWr, semValRd);
      

    for (ssize_t i = 0U; i < edgeCnt; i++)
    {
        // only write edges that are not the delimiter, or default
        if (!is_edge_delimiter(pEdges[i]))
        {
            retCode |= circular_buffer_write(&pSharedMem->circbuf, pSems, &pEdges[i]);
            debug("Writing edge %d with %d-%d\n", i, pEdges[i].start, pEdges[i].end);

            if (ERROR_OK != retCode)
            {
                debug("Error while writing\n", NULL);
                return retCode;
            }
            else
            {
                // actual size
                *pWritten += 1U;
            }
            
        }
    }

    // write the delimiter
    retCode |= circular_buffer_write(&pSharedMem->circbuf, pSems, &del);

    // increase the number of solutions
    pSharedMem->flags.numSols++;

    if (sem_post(pSems->mutex_write) < 0) return ERROR_SEMAPHORE;

    return retCode;
}

/**
 * @brief   Get Vertices
 * @details This internal method is used to get all vertices from the edges.
 *          The array for the vertices is allocated with the worstcase (2x edges).
 *
 * @param   pEdges      Pointer to the array of edges
 * @param   edgeCnt     Number of edges
 *
 * @return  vert        Pointer to the array of vertices
 */
static int16_t* get_vertices(edge_t* pEdges, ssize_t edgeCnt)
{
    int16_t* vert = malloc(sizeof(int16_t) * edgeCnt * 2);
    memset(vert, -1, sizeof(int16_t) * edgeCnt * 2);

    bool* isIncl = malloc(sizeof(bool) * edgeCnt * 2);
    memset(isIncl, false, sizeof(bool) * edgeCnt * 2);

    for (size_t i = 0; i < edgeCnt; i++)
    {
        edge_t currEdge = pEdges[i];

        if (!isIncl[currEdge.start])
        {
            vert[i] = currEdge.start;
            isIncl[currEdge.start] = true;
        }

        if (!isIncl[currEdge.end])
        {
            vert[i + edgeCnt / 2] = currEdge.end;
            isIncl[currEdge.end] = true;
        }
    }

    free(isIncl);
    return vert;
}

/**
 * @brief   Get Random Seed
 * @details This internal method is used to get a random seed from /dev/urandom.
 *
 * @return  seed        Random seed
 */
static uint16_t get_random_seed()
{
    uint16_t seed = 0U;                    /*!< random seed */
    FILE* fp = fopen("/dev/urandom", "r"); /*!< file pointer to /dev/urandom */

    if (NULL == fp)
    {
        emit_error("Could not open /dev/urandom\n", ERROR_PARAM);
    }

    fread(&seed, sizeof(uint16_t), 1U, fp);
    fclose(fp);

    return seed;
}

/**
 * @brief   Shuffle
 * @details This internal method is used to shuffle the vertices.
 *          It reads the edges and writes them to the same array, but in a random order.
 *
 * @param   pVert       Pointer to the array of vertices (read and write)
 * @param   edgeCnt     Number of edges
 */
static void shuffle(int16_t pVert[], ssize_t edgeCnt)
{
    // mix the vertices in the array
    for (ssize_t i = 0U; i < edgeCnt; i++)
    {
        ssize_t j = rand() % edgeCnt;
        int16_t temp = (pVert)[i];
        (pVert)[i] = (pVert)[j];
        (pVert)[j] = temp;
    }
}

/**
 * @brief   Sortout Solution
 * @details This internal method is used to sort out the solution.
 *         It reads the edges and writes them to the same array, but removes all edges which
 *         have a smaller index for the start vertex than the end vertex. The index of the vertices
 *         is generated by the shuffle method.
 *
 * @note   It uses a temporary array to store the edges, because it is easier. Later the edges get copied back.
 *
 * @param   pEdges      Pointer to the array of edges (read and write)
 * @param   edgeCnt     Number of edges
 * @param   pVert       Pointer to the array of vertices
 */
static void sortout_solution(edge_t pEdges[], size_t edgeCnt, int16_t* pVert)
{
    edge_t* temp = calloc(sizeof(edge_t), edgeCnt);
    uint16_t idxV1 = 0U;
    uint16_t idxV2 = 0U;
    size_t tempIdx = 0U;

    for (ssize_t i = 0U; i < edgeCnt; i++)
    {
        edge_t currEdge = pEdges[i];

        debug("Edge: %d-%d\n", currEdge.start, currEdge.end);

        // search for the indexes of the vertices
        for (ssize_t j = 0U; j < edgeCnt * 2; j++)
        {
            if (pVert[j] == currEdge.start)
            {
                idxV1 = j;
                break;
            }
        }

        for (ssize_t j = 0U; j < edgeCnt * 2; j++)
        {
            if (pVert[j] == currEdge.end)
            {
                idxV2 = j;
                break;
            }
        }

        if (idxV1 > idxV2)
        {
            temp[tempIdx] = currEdge;
            tempIdx++;
            debug("Added Edge to deletion: %d-%d\n", currEdge.start, currEdge.end);
        }
    }

    memset(pEdges, 0, sizeof(edge_t) * edgeCnt);
    memcpy(pEdges, temp, sizeof(edge_t) * edgeCnt);
    free(temp);
}

/**
 * @brief   Generate Solution
 * @details This internal method is used to generate a solution.
 *
 * @param   pOrigEdges  Pointer to the array of edges (read only)
 * @param   pSolution   Pointer to the array of edges (read and write)
 * @param   edgeCnt     Number of edges
 *
 * @return  retCode     Error code
 * @retval  ERROR_OK            Everything went fine
 */
static error_t generate_solution(edge_t* pOrigEdges, edge_t* pSolution, ssize_t edgeCnt)
{
    error_t retCode = ERROR_OK; /*!< return code for error handling */

    // reset the solution
    memcpy(pSolution, pOrigEdges, sizeof(edge_t) * edgeCnt);

    // get the vertices
    int16_t* vert = get_vertices(pOrigEdges, edgeCnt);

    // shuffle the edges
    shuffle(vert, edgeCnt);

    // remove the edges which have a smaller index for the start vertex than the end vertex
    sortout_solution(pSolution, edgeCnt, vert);

    free(vert);

    return retCode;
}

/**
 * @brief   Main
 * @details This is the main method of the application.
 *          It is used to read the edges from the parameters, generate a solution and write it to the shared memory.
 *          These solutions are randomly generated. The supervisor will read them and check if they are better than the current best solution.
 *          If the given graph is acyclic, the generator will terminate. The supervisor will get this because a solution with 0 edges is written.
 *          Else only the supervisor can terminate the generators by a flag in the shared memory.
 *
 * @param   argc    Number of given parameters
 * @param   argv    Array of given parameters by user
 *
 * @return  retCode Error code
 * @retval  EXIT_SUCCESS    Everything went fine
 * @retval  EXIT_FAILURE    Something went wrong
 */
int main(int argc, char* argv[])
{
    debug("This is the generator\n", NULL);
    error_t retCode = ERROR_OK;                          /*!< return code for error handling */
    ssize_t edgeCnt = argc - 1U;                         /*!< number of given edges */
    edge_t* edges = malloc(sizeof(edge_t) * edgeCnt);    /*!< memory to store all edges */
    edge_t* solution = malloc(sizeof(edge_t) * edgeCnt); /*!< memory to store a solution */
    sems_t semaphores = {0U};                            /*!< struct of all needed semaphores */
    shared_mem_t* pSharedMem = NULL;
    int16_t fd = -1;
    size_t solSize = 0U;

    // set the application name
    gAppName = argv[0];

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

    // set the seed for the random number generator
    srand(get_random_seed());

    while (pSharedMem->flags.genActive)
    {
        debug_pid("Next Solution\n", NULL);

        // generate the solution
        retCode |= generate_solution(edges, solution, edgeCnt);
        
        debug_pid("Got Here", NULL);

        // write the edges to the shared memory
        retCode |= write_solution(pSharedMem, &semaphores, solution, edgeCnt, &solSize);

        debug_pid("Got Here", NULL);

        if (ERROR_OK != retCode)
        {
            debug_pid("Breaked because of error %d", retCode);
            break;
        }

        debug_pid("Got Here", NULL);

        // check if the solution is empty, this means termination
        if (0U == solSize)
        {
            debug_pid("Solution with 0 edges found, terminating now, supervise will terminate too\n", NULL);
            break;
        }
        debug_pid("Got Here", NULL);
    }

    debug_pid("While loop left", NULL);

    if ((retCode & ERROR_SIGINT) != 0)
    {
        // just a signal received, so everything is fine
        retCode = ERROR_OK;
    }
    

    // unmap memory
    munmap(pSharedMem, sizeof(shared_mem_t));
    cleanup_semaphores(&semaphores);
    free(edges);
    free(solution);

    return EXIT_SUCCESS;
}
