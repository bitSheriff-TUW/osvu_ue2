/**
 * @file supervisor.c
 * @author  Benjamin Mandl (12220853)
 * @date 2023-11-07
 */

#include <getopt.h>
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

int main(int argc, char* argv[])
{
    error_t retCode = ERROR_OK; /*!< return code */
    options_t opts = {0U};      /*!< bundle of options */

    /* get the options */
    handle_opts(argc, argv, &opts);

    return retCode;
}
