#include "common.h"

#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#include "errors.h"

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

static void circular_buffer_safeIncrease(ssize_t* pIndex) { *pIndex = (*pIndex + 1U) % CIRBUF_BUFSIZE; }

error_t circular_buffer_read(shared_mem_circbuf_t* pCirBuf, sems_t* pSems, edge_t* pResult)
{
    error_t retCode = ERROR_OK;

    // check if all pointer are valid
    if ((NULL == pCirBuf) || (NULL == pSems) || (NULL == pResult))
    {
        return ERROR_NULLPTR;
    }

    // check if the buffer is empty
    if (pCirBuf->head == pCirBuf->tail)
    {
        return ERROR_CIRBUF_EMPTY;
    }

    // cannot read if buffer is empty, so check if the buffer has elements
    if (sem_wait(pSems->reading) < 0) return ERROR_SEMAPHORE;

    // copy the element from the buffer to the result address
    memcpy(pResult, &pCirBuf->buf[pCirBuf->tail], sizeof(edge_t));
    circular_buffer_safeIncrease(&pCirBuf->tail);

    // something was read, so the fullness decreases
    if (sem_post(pSems->writing) < 0) return ERROR_SEMAPHORE;

    return retCode;
}

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
    if (sem_wait(pSems->writing) < 0) return ERROR_SEMAPHORE;

    // check if the buffer is full
    if (pCirBuf->head == pCirBuf->tail)
    {
        return ERROR_CIRBUF_FULL;
    }

    // set the element
    memcpy(&pCirBuf->buf[pCirBuf->head], pEd, sizeof(edge_t));
    circular_buffer_safeIncrease(&pCirBuf->head);

    // mutex: now another generator can write
    // something was written into the buffer, so the supervisor can read something now
    if (sem_post(pSems->reading) < 0) return ERROR_SEMAPHORE;

    return retCode;
}

bool is_edge_delimiter(edge_t ed)
{
    if (ed.start == DELIMITER_VERTEX && ed.end == DELIMITER_VERTEX)
    {
        return true;
    }
    return false;
}
