#pragma once

#include <stdint.h>

typedef uint16_t error_t;

#define ERROR_OK 0U
#define ERROR_FORK_FAILED 0x01U
#define ERROR_PIPE_FAILED 0x02U
#define ERROR_PARAM 0x04U
