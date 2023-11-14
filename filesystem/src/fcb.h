#ifndef FCB_H
#define FCB_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <commons/config.h>
#include <log/log_utils.h>

typedef struct
{
	char* nombre;
	uint32_t tam_archivo;
    uint32_t bloque_inicial;
}t_fcb;

#endif