#ifndef MEMORIA_DE_INSTRUCCIONES_H
#define MEMORIA_DE_INSTRUCCIONES_H

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

#include "memoria_de_usuario.h"
#include "page_fault.h"

void conexion_cpu(void* arg);
void parsear_instrucciones(t_log* logger,t_proceso* proceso, char* str);
char* leer_pseudocodigo(t_log* logger, char* nombre_archivo);
t_proceso* crear_proceso(uint32_t pid);
void conexion_kernel(void* arg);
void finalizar_proceso(uint32_t pid, int socket_swap);

#endif