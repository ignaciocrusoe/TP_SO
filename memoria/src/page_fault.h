#ifndef PAGE_FAULT_H
#define PAGE_FAULT_H

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <log/log_utils.h>
#include <sockets/sockets.h>
#include <sockets/client_utils.h>
#include <sockets/server_utils.h>
#include <pcb/pcb.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/bitarray.h>
#include <threads/thread_parameters.h>
#include <memoria/memoria.h>

uint32_t buscar_victima_fifo(void);
int32_t buscar_frame_libre();
void* leer_pagina(uint32_t nro_frame);
void escribir_pagina(uint32_t nro_frame, void* a_escribir);

#endif