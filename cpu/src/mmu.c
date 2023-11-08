#include "mmu.h"

t_direccion_fisica* traducir_direccion(char* direccion_logica, uint32_t tam_pagina, int socket_memoria)
{
    t_direccion_fisica* direccion_fisica = malloc(sizeof(t_direccion_fisica));
    uint32_t pagina = floor(atof(direccion_logica));
    direccion_fisica->offset = atoi(direccion_logica) - pagina * tam_pagina;
    pedir_frame(socket_memoria);
    direccion_fisica->frame = recibir_frame(socket_memoria);
    return direccion_fisica;
}