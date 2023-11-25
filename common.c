#include "common.h"

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

    // TODO: semaphores wait?

    // copy the element from the buffer to the result address
    memcpy(pResult, &pCirBuf->buf[pCirBuf->tail], sizeof(edge_t));
    circular_buffer_safeIncrease(&pCirBuf->tail);

    // TODO: semaphores post?

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

    // TODO: semaphores wait

    // check if the buffer is full
    if (pCirBuf->head == pCirBuf->tail)
    {
        return ERROR_CIRBUF_FULL;
    }

    // set the element
    memcpy(&pCirBuf->buf[pCirBuf->head], pEd, sizeof(edge_t));
    circular_buffer_safeIncrease(&pCirBuf->head);

    // TODO: semaphores post

    return retCode;
}
