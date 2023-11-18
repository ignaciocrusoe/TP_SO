#ifndef MEMORIA_DE_USUARIO_H
#define MEMORIA_DE_USUARIO_H

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
#include <stdint.h>

uint8_t leer_de_memoria(t_direccion_fisica* direccion);
void* escribir_en_memoria(t_direccion_fisica* direccion, uint8_t a_escribir);
int32_t obtener_frame(uint32_t pagina_buscada,uint32_t pid);
void asignar_memoria(uint32_t pid, uint32_t size, t_list* bloques_reservados, void* arg);
t_pagina* obtener_pagina_de_frame(t_list* tabla_de_paginas, uint32_t frame);
t_pagina* pagina_en_proceso(uint32_t pagina_buscada, uint32_t pid);
t_proceso* buscar_proceso(uint32_t pid);
void quitar_de_memoria(uint32_t pid);
void escribir_en_swap(t_pagina* pagina, void* arg);
void eliminar_de_lista(u_int32_t frame);
uint32_t elegir_frame(void* arg);
t_frame_info* elegir_victima();
void actualizar_en_swap(t_pagina* pagina, void* arg);
void agregar_pagina(uint32_t pid, uint32_t nro_pagina, uint32_t nro_frame, uint32_t pos_swap);

void conexion_filesystem(void* arg);

#endif