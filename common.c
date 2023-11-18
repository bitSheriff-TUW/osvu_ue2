#include "common.h"

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

error_t circular_buffer_read(shared_mem_circbuf_t* pCirBuf, edge_t* pResult)
{
    error_t retCode = ERROR_OK;

    // check if the buffer is empty
    if (pCirBuf->head == pCirBuf->tail)
    {
        return ERROR_CIRBUF_EMPTY;
    }

    // TODO: actual alogorithm
    return retCode;
}

error_t circular_buffer_write(shared_mem_circbuf_t* pCirBuf, edge_t ed)
{
    error_t retCode = ERROR_OK;

    // check if the buffer is full
    if (pCirBuf->head == pCirBuf->tail)
    {
        return ERROR_CIRBUF_FULL;
    }

    // TODO: actual alogorithm
    return retCode;
}
