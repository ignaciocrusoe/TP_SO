#ifndef UTILS_MEMORIA_H
#define UTILS_MEMORIA_H

#include <stdint.h>
#include <commons/collections/list.h>
#include <sockets/sockets.h>
#include <sockets/client_utils.h>
#include <sockets/server_utils.h>
#include <time.h>

typedef struct
{
    uint32_t pagina;
    uint32_t frame;
    uint8_t presencia;
    uint8_t modificado;
    uint32_t posicion_en_swap;
    time_t timestamp;
}t_pagina;

typedef struct
{
    uint32_t pid;
    t_list* instrucciones;
	t_list* tabla_de_paginas;
}t_proceso;

typedef struct
{
    uint32_t frame;
    uint32_t offset;
}t_direccion_fisica;

/*
typedef struct
{
	op_code operacion;
	uint32_t pid;
	uint32_t registros;

} t_fetch_instruccion;
*/

void pedir_frame(int socket, uint32_t pid, uint32_t pagina);
uint32_t recibir_frame(int socket);
void enviar_frame (int socket, int32_t frame);
void enviar_direccion(int socket, t_direccion_fisica* direccion);
t_direccion_fisica* recibir_direccion(int socket);
t_pagina* crear_pagina(uint32_t pagina, uint32_t frame);

#endif