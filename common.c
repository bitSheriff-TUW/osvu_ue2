#include "common.h"

#include <assert.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#include "errors.h"

/**
 * @file common.c
 * @author  Benjamin Mandl (12220853)
 * @date 2023-11-07
 */


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
void emit_error(char* msg, error_t retCode)
{
    // only print the error code if some was attached
    if (ERROR_OK != retCode)
    {
        fprintf(stderr, "%s\nCode: %d\n", msg, retCode);
    } else
    {
        fprintf(stderr, "%s\n", msg);
    }
    exit(EXIT_FAILURE);
}

/**
 * @brief       Safe Increase
 *              This method is used to increase the index of the circular buffer but without the risk of an overflow.
 * @param       pIndex      Pointer to the index which should be increased
*/
static void circular_buffer_safeIncrease(ssize_t* pIndex) { *pIndex = (*pIndex + 1U) % CIRBUF_BUFSIZE; }

/**
 * @brief       Circular Buffer Read
 * @details     This method is used to read an element from the circular buffer. It will wait until an element is
 *             available to read and then copy it to the result address.
 * 
 * @param       pCirBuf     Pointer to the circular buffer
 * @param       pSems       Pointer to the semaphores
 * @param       pResult     Pointer to the result address
 * 
 * @return      Error code
 * @retval      ERROR_OK        Everything went fine
 * @retval      ERROR_NULLPTR   One of the pointers was NULL
 * @retval      ERROR_SIGINT    The process was interrupted by a signal
 * @retval      ERROR_SEMAPHORE The semaphore could not be accessed
*/
error_t circular_buffer_read(shared_mem_circbuf_t* pCirBuf, sems_t* pSems, edge_t* pResult)
{
    error_t retCode = ERROR_OK;

    // check if all pointer are valid
    if ((NULL == pCirBuf) || (NULL == pSems) || (NULL == pResult))
    {
        return ERROR_NULLPTR;
    }

    // cannot read if buffer is empty, so check if the buffer has elements
    if (sem_wait(pSems->reading) < 0)
    {
        if (errno == EINTR)
        {
            return ERROR_SIGINT;
        }
        else
        {
            return ERROR_SEMAPHORE;
        }
    } 

    // copy the element from the buffer to the result address
    memcpy(pResult, &pCirBuf->buf[pCirBuf->tail], sizeof(edge_t));
    circular_buffer_safeIncrease(&pCirBuf->tail);

    // something was read, so the fullness decreases
    if (sem_post(pSems->writing) < 0)
    {
        if (errno == EINTR)
        {
            return ERROR_SIGINT;
        }
        else
        {
            return ERROR_SEMAPHORE;
        }
    } 

    return retCode;
}

/**
 * @brief       Circular Buffer Write
 * @details     This method is used to write an element to the circular buffer. It will wait until an element can be
 *              written and then copy it to the buffer.
 * 
 * @param       pCirBuf     Pointer to the circular buffer
 * @param       pSems       Pointer to the semaphores
 * @param       pEd         Pointer to the element which should be written
 * 
 * @return      Error code
 * @retval      ERROR_OK        Everything went fine
 * @retval      ERROR_NULLPTR   One of the pointers was NULL
 * @retval      ERROR_SIGINT    The process was interrupted by a signal
 * @retval      ERROR_SEMAPHORE The semaphore could not be accessed
*/
error_t circular_buffer_write(shared_mem_circbuf_t* pCirBuf, sems_t* pSems, edge_t* pEd)
{
    error_t retCode = ERROR_OK;

    // check if all pointer are valid
    if ((NULL == pCirBuf) || (NULL == pSems) || (NULL == pEd))
    {
        return ERROR_NULLPTR;
    }

    // mutex: only one generator is allowed to write at the same time
    // if the buffer is full, you have to wait until it gets read
    if (sem_wait(pSems->writing) < 0)
    {
        if (errno == EINTR)
        {
            return ERROR_SIGINT;
        }
        else
        {
            return ERROR_SEMAPHORE;
        }
    } 

    // set the element
    memcpy(&pCirBuf->buf[pCirBuf->head], pEd, sizeof(edge_t));
    circular_buffer_safeIncrease(&pCirBuf->head);

    // something was written into the buffer, so the supervisor can read something now
    if (sem_post(pSems->reading) < 0)
    {
        if (errno == EINTR)
        {
            return ERROR_SIGINT;
        }
        else
        {
            return ERROR_SEMAPHORE;
        }
    } 

    assert(pCirBuf->head < CIRBUF_BUFSIZE && pCirBuf->tail < CIRBUF_BUFSIZE);

    return retCode;
}

/**
 * @brief       Is Edge Delimiter
 * @details     This method is used to check if an edge is a delimiter.
 * @param       ed      Edge which should be checked
 * @return      True if the edge is a delimiter, false otherwise
 * @retval      true    The edge is a delimiter
 * @retval      false   The edge is not a delimiter
*/
bool is_edge_delimiter(edge_t ed)
{
    return (ed.start == DELIMITER_VERTEX) && (ed.end == DELIMITER_VERTEX);
}
