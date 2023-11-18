#ifndef SOCKETS_H
#define SOCKETS_H

typedef enum
{   MENSAJE,
	PAQUETE,
	INICIAR_PROCESO,
	FETCH_INSTRUCCION,
	INTERRUPT,
	PEDIDO_DE_FRAME,
	FRAME,
	FINALIZAR_PROCESO,
	PEDIDO_SIZE_PAGINA,
	PEDIDO_LECTURA,
	PEDIDO_ESCRITURA,
	PAGE_FAULT,
	ABRIR_ARCHIVO,
	CREAR_ARCHIVO,
	LEER_ARCHIVO,
	ESCRIBIR_ARCHIVO,
	RESERVAR_SWAP,
	ESCRIBIR_SWAP,
	LEER_SWAP,
	ELIMINAR_EN_SWAP
}op_code;

typedef enum
{   
	OK,
	ERROR
}t_response;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef struct
{
	int size_mensaje;
	char* mensaje;
} t_mensaje;

#endif