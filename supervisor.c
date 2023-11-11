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

static void usage()
{
    // TODO: add usage prints
}

static void handle_opts(int argc, char** argv, options_t* pOpts)
{
    int16_t ret = 0;

    while ((ret = getopt(argc, argv, "pn:w:")) != -1)
    {
        switch (ret)
        {
            case 'p': {
                if (false != pOpts->print)
                {
                    /* option was given two times */
                    usage();
                } else
                {
                    pOpts->print = true;
                }
                break;
            }
            case 'n': {
                pOpts->limit = (uint16_t)strtol(optarg, NULL, 0);  // TODO: check if that works with str->int
                break;
            }

            case 'w': {
                pOpts->delayS = (uint16_t)strtol(optarg, NULL, 0);
                break;
            }

            default: {
                usage();
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
