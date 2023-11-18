#ifndef UTILS_MEMORIA_H
#define UTILS_MEMORIA_H

#include <stdint.h>
#include <commons/collections/list.h>
#include <sockets/sockets.h>
#include <sockets/client_utils.h>
#include <sockets/server_utils.h>

typedef struct 
{
    uint32_t frame;
    uint32_t pid;
    uint32_t orden;
}t_frame_info;

typedef struct
{
    uint32_t pagina;
    uint32_t frame;
    bool presencia;
    bool modificado;
    uint32_t posicion_en_swap;
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


void pedir_frame(int socket, uint32_t pid, uint32_t pagina);
uint32_t recibir_frame(int socket);
void enviar_frame (int socket, int32_t frame);
void enviar_direccion(int socket, t_direccion_fisica* direccion);
t_direccion_fisica* recibir_direccion(int socket);
t_pagina* crear_pagina(uint32_t pagina, uint32_t frame, uint32_t posicion_en_swap);
t_frame_info* crear_frame_info(uint32_t frame, uint32_t pid, uint32_t orden);
void* menor_que(void* frame1, void* frame2);

#endif