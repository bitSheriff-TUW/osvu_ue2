/**
 * @file generator.c
 * @author  Benjamin Mandl (12220853)
 * @date 2023-11-07
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "debug.h"
#include "errors.h"

static void readEdges(edge_t* pEdges[], char** argv, ssize_t argc)
{
    // check if edges were given, more than one is needed
    if (2 > argc)
    {
        emit_error("Not enough parameter given\n", ERROR_PARAM);
    }

    // step through all the given parameters and parse the endges
    for (ssize_t i = 1U; i < argc; i++)
    {
        // the verticies are sepertaed with a dash, and the edges with a space
        if (sscanf(argv[i], "%hu-%hu", &((*pEdges)[i - 1].start), &((*pEdges)[i - 1].end)) < 0)
        {
            emit_error("Something went wrong with reading edges\n", ERROR_PARAM);
        }

        // check for loop
        if ((*pEdges)[i - 1].start == (*pEdges)[i - 1].end)
        {
            emit_error("Loops are not allowed\n", ERROR_PARAM);
        }
    }
}

int main(int argc, char* argv[])
{
    debug("This is the generator\n", NULL);

    ssize_t edgeCnt = argc - 1U;                     /*!< number of given edges */
    edge_t* edges = calloc(sizeof(edge_t), edgeCnt); /*!< memory to store all edges */

    debug("Number of Edges: %ld\n", edgeCnt);

    // read the edges from the parameters
    readEdges(&edges, argv, argc);

    return EXIT_SUCCESS;
}
