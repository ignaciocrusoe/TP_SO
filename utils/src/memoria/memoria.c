#include "memoria.h"
/*
void pedir_instruccion()
{
    t_paquete* serializar_pedido_cpu(t_pedCPU *valor){

        t_paquete* paquete = malloc(sizeof(t_paquete));
        paquete->buffer = malloc(sizeof(t_buffer));

        int bytes = sizeof(valor->operacion) + strlen(valor->pid) + 1 + sizeof(uint32_t) * 2;

        paquete->buffer->size = bytes;

        void* stream = malloc(paquete->buffer->size);
        int offset = 0;

        memcpy(stream + offset, (&valor->operacion), sizeof(valor->operacion));
        offset += sizeof(valor->operacion);
        memcpy(stream + offset, (&valor->pid), sizeof(uint32_t));
        offset += sizeod(uint32_t);
        memcpy(stream + offset, (&valor->puntero), sizeof(uint32_t));

        paquete->buffer->stream = stream;

        return paquete;
    }

    t_fetch_instruccion* deserializar_pedido_cpu(t_buffer* buffer){

        t_fetch_instruccion* valor = malloc(sizeof(t_fetch_instruccion));

        stream += sizeof(uint32_t);

        memcpy(&(valor->operacion), stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);
        memcpy(&(valor->pid), stream, sizeof(uint32_t));
        stream += sizeof(uint32_t);
        memcpy(&(valor->puntero, stream, sizeof(uint32_t)));

        return valor;
        
    }
}
*/
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

t_pagina* crear_pagina(uint32_t nro_pagina, uint32_t frame)
{
    t_pagina* pagina = malloc(sizeof(t_pagina));
    pagina->pagina = nro_pagina;
    pagina->frame = frame;
    pagina->timestamp = time(NULL);
    return pagina;
}