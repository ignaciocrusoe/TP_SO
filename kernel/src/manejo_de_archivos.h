#ifndef MANEJO_DE_ARCHIVOS_H
#define MANEJO_DE_ARCHIVOS_H

#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <sockets/sockets.h>
#include <sockets/client_utils.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pcb/pcb.h>
#include <semaphore.h>
#include <commons/collections/queue.h>
#include <log/log_utils.h>
#include <pcb/pcb.h>
#include <pthread.h>
#include <threads/thread_parameters.h>
#include <locks/lock.h>

#include "manejo_de_archivos.h"

typedef struct
{
    char* nombre;
    uint32_t tam_archivo;
    uint32_t puntero;
    t_lock lock;
    t_queue* cola_blocked;
    t_list* locks_lectura;
    sem_t mutex_cola_blocked;
    sem_t mutex_locks_lectura;
    uint32_t contador_aperturas;
}t_archivo;

t_archivo* crear_archivo(char* nombre_archivo, uint32_t tam_archivo, t_lock lock);
t_lock de_string_a_t_lock(char* str);
t_archivo* buscar_archivo(t_list* lista, char* nombre);
void file_open(void* arg);
void file_truncate(void* arg);
void file_write(void* arg);
void file_read(void* arg);
void file_close(void* arg);
void file_seek(void* arg);

#endif