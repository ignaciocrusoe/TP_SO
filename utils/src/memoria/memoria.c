#include "memoria.h"

void pedir_frame(int socket, uint32_t pid, uint32_t pagina)
{
    enviar_operacion(socket, PEDIDO_DE_FRAME);
    send(socket, &pid, sizeof(uint32_t), NULL);
    send(socket, &pagina, sizeof(uint32_t), NULL);
}

void enviar_frame (int socket, int32_t frame)
{
    if(frame >= 0)
    {
        uint32_t frame_a_mandar = frame;
        enviar_operacion(socket, FRAME);
        send(socket, &frame_a_mandar, sizeof(uint32_t), NULL);
    }
    else
    {
        enviar_operacion(socket, PAGE_FAULT);
    }
}

uint32_t recibir_frame(int socket)
{
    int32_t frame;
    op_code operacion = recibir_operacion(socket);
    switch (operacion)
    {
    case FRAME:
        recv(socket, &frame, sizeof(uint32_t), MSG_WAITALL);
        break;
    
    default:
        break;
    }
    return frame;
}

void enviar_direccion(int socket, t_direccion_fisica* direccion)
{
    send(socket, &(direccion->frame), sizeof(uint32_t), NULL);
    send(socket, &(direccion->offset), sizeof(uint32_t), NULL);
}

t_direccion_fisica* recibir_direccion(int socket)
{
    t_direccion_fisica* direccion = malloc(sizeof(t_direccion_fisica));
    recv(socket, &(direccion->frame), sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &(direccion->offset), sizeof(uint32_t), MSG_WAITALL);
    return direccion;
}

t_pagina* crear_pagina(uint32_t nro_pagina, uint32_t frame, uint32_t posicion_en_swap)
{
    t_pagina* pagina = malloc(sizeof(t_pagina));
    pagina->pagina = nro_pagina;
    pagina->frame = frame;
    pagina->posicion_en_swap = posicion_en_swap;
    return pagina;
}

t_frame_info* crear_frame_info(uint32_t frame, uint32_t pid, uint32_t orden){
    t_frame_info* frame_info = malloc(sizeof(t_frame_info));
    frame_info->frame = frame;
    frame_info->pid = pid;
    frame_info->orden = orden;
    return frame_info;
}

void* menor_que(void* frame1, void* frame2){
    if(((t_frame_info*)frame1)->orden <= ((t_frame_info*)frame2)->orden)
        return frame1;
    else
        return frame2;
}