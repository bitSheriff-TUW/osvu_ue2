#pragma once

#include <stdint.h>

typedef uint16_t error_t;

#define ERROR_OK 0U
#define ERROR_FORK_FAILED 0x01U
#define ERROR_PIPE_FAILED 0x02U
#define ERROR_PARAM 0x04U
#define ERROR_CIRBUF_EMPTY 0x08U
#define ERROR_CIRBUF_FULL 0x10U
#define ERROR_SEMAPHORE 0x20U
#define ERROR_NULLPTR 0x40U
#define ERROR_SHMEM 0x80U