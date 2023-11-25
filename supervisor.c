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
 * @details This bundle is used to bundle all option of this moudle for easier access.
 */
typedef struct
{
    bool print;      /*!< boolean value of the graph should be printed */
    uint16_t limit;  /*!< number of generated solutions */
    uint16_t delayS; /*!< delay [s] before the starting to read the buffer */
} options_t;

/**
 * @brief Signal Interupt
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
                if (0 != pOpts->limit)
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
    sem_unlink(SEM_NAME_EMPTY);
    sem_unlink(SEM_NAME_FULL);

    // create the named semaphores
    pSems->mutex_write = sem_open(SEM_NAME_MUTEX, O_CREAT, 0666, 0);
    pSems->buffer_empty = sem_open(SEM_NAME_EMPTY, O_CREAT, 0666, 0);
    pSems->buffer_full = sem_open(SEM_NAME_FULL, O_CREAT, 0666, 0);

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

    // delete the semamorph files, if something fails here, there is now proper way to react, so ignore the retVal
    (void)sem_unlink(SEM_NAME_MUTEX);
    (void)sem_unlink(SEM_NAME_EMPTY);
    (void)sem_unlink(SEM_NAME_FULL);

    return retCode;
}

void handle_sigint(int32_t sig)
{
    // set the global variable to true
    gSigInt = true;
}

int main(int argc, char* argv[])
{
    error_t retCode = ERROR_OK; /*!< return code */
    options_t opts = {0U};      /*!< bundle of options */
    sems_t semaphores = {0};

    // register the signal handler
    signal(SIGINT, handle_sigint);

    /* get the options */
    handle_opts(argc, argv, &opts);

    retCode |= init_semaphores(&semaphores);

    if (ERROR_OK != retCode)
    {
        emit_error("Something was wrong with creating the semaphores\n", retCode);
    }

    // main operating loop
    while (false == gSigInt)
    {
    }

    retCode |= cleanup_semaphores(&semaphores);

    return retCode;
}
