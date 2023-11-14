#ifndef KERNEL_H
#define KERNEL_H

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

#include "temp.h"
#include "planificador_largo_plazo.h"
#include "planificador_corto_plazo.h"


typedef struct
{
    char* nombre;
    uint16_t tam_archivo;
}t_archivo;


#endif 