#include "main.h"

uint32_t cant_bloques_total;
uint32_t cant_bloques_swap;
uint32_t tam_bloque;
uint32_t retardo_acceso_bloque;
uint32_t retardo_acceso_fat;
uint32_t tam_pagina;

t_list* archivos_abiertos; //Si bien la tabla global de archivos abiertos está en kernel, necesitamos guardar una lista de fcbs.
t_fat* fat;

FILE* bloques;
uint32_t puntero;
t_log* logger;
t_bitarray* swap_bitarray;

char* puerto_escucha;
char* ip_memoria;
char* puerto_memoria;
char* path_fat;
char* path_bloques;
char* path_fcb;

void conexion_kernel(void* arg)
{
	t_args_hilo* arg_h = (t_args_hilo*)arg;
	op_code operacion;
	char* nombre_archivo;
	uint32_t tam_archivo;
	printf("(%i)Socket = %i\n", arg, arg_h->socket_kernel);

	operacion = recibir_operacion(arg_h->socket_kernel);
	printf ("\nSe recibe operacion:%d\n",operacion);
	switch (operacion)
	{
	case ABRIR_ARCHIVO:
		nombre_archivo = recibir_mensaje(arg_h->socket_kernel);
		{ //Creo un bloque de código porque creo la variable tam_archivo (int32_t) con el mismo nombre que tam_archivo (uint32_t)
			int32_t tam_archivo = abrir_archivo(path_fcb, nombre_archivo);
			printf("m_tam_archivo = %i\n", tam_archivo);
			send(arg_h->socket_kernel, &tam_archivo, sizeof(int32_t), NULL);
		}
		break;
	case CREAR_ARCHIVO:
		nombre_archivo = recibir_mensaje(arg_h->socket_kernel);
		printf("Recibí el nombre del archivo\n");
		if(crear_archivo(path_fcb, nombre_archivo))
		{
			enviar_respuesta(arg_h->socket_kernel, OK);
			//log_info(logger_hilo, “Crear Archivo: %i, arg_h->socket_kernel”);
		}
		else
		{
			enviar_respuesta(arg_h->socket_kernel, ERROR);
		}
		break;
	case LEER_ARCHIVO:
		nombre_archivo = recibir_mensaje(arg_h->socket_kernel);
		{
			recv(arg_h->socket_kernel, &puntero, sizeof(uint32_t), MSG_WAITALL);
			printf("puntero = %i\n", puntero);
			t_direccion_fisica* direccion = recibir_direccion(arg_h->socket_kernel);
			printf("direccion = %i - %i\n", direccion->frame, direccion->offset);
			t_fcb* fcb = buscar_archivo(nombre_archivo, archivos_abiertos);
			uint32_t nro_bloque = fcb->bloque_inicial;
			printf("2\n");
			for(uint32_t i; i < ceil(puntero / tam_bloque); i++)
			{
				log_info(logger, "Acceso FAT - Entrada: %i - Valor: %i", nro_bloque, fat->memory_map[nro_bloque]);
				nro_bloque = fat->memory_map[nro_bloque]; //No validemos que se pida un dato mayor al mapeado;
			}
			printf("3\n");
			uint32_t a_enviar = leer_dato(nro_bloque, puntero - ceil(puntero / tam_bloque) * tam_bloque);
			printf("operacion: PEDIDO_ESCRITURA - direccion: %i:%i - datos: %i\n", direccion->frame, direccion->offset, a_enviar);
			enviar_operacion(arg_h->socket_memoria, PEDIDO_ESCRITURA);
			enviar_direccion(arg_h->socket_memoria, direccion);
			send(arg_h->socket_memoria, &a_enviar, sizeof(uint32_t), NULL);
			enviar_respuesta(arg_h->socket_kernel, OK);
		}
		break;
		case ESCRIBIR_ARCHIVO:
		nombre_archivo = recibir_mensaje(arg_h->socket_kernel);
		{
			recv(arg_h->socket_kernel, &puntero, sizeof(uint32_t), MSG_WAITALL);
			printf("puntero: %i\n", puntero);
			t_direccion_fisica* direccion = recibir_direccion(arg_h->socket_kernel);
			t_fcb* fcb = buscar_archivo(nombre_archivo, archivos_abiertos);
			printf("dir: %i\n", direccion->frame);
			uint32_t bloque = fcb->bloque_inicial;
			uint32_t a_escribir;
			for(uint32_t i = 0; i < ceil(puntero / tam_bloque); i++)
			{
				log_info(logger, "Acceso FAT - Entrada: %i - Valor: %i", bloque, fat->memory_map[bloque]);
				bloque = fat->memory_map[bloque]; //No validemos que se pida un dato mayor al mapeado;
			}
			printf("Pido el dato a memoria\n");
			enviar_operacion(arg_h->socket_memoria, PEDIDO_LECTURA);
			enviar_direccion(arg_h->socket_memoria, direccion);
			printf("Pedí el dato a memoria, ahora lo recibo\n");
			recv(arg_h->socket_memoria, &a_escribir, sizeof(uint32_t), MSG_WAITALL);
			//Escribir en el archivo
			printf("El dato que recibí es %i\n", a_escribir);
			escribir_dato(bloque, puntero - ceil(puntero / tam_bloque) * tam_bloque, a_escribir);
			enviar_respuesta(arg_h->socket_kernel, OK);
		}
		break;
	
	case TRUNCAR_ARCHIVO:
		nombre_archivo = recibir_mensaje(arg_h->socket_kernel);
		recv(arg_h->socket_kernel, &tam_archivo, sizeof(uint32_t), MSG_WAITALL);
		printf("Truncar %s a %u\n", nombre_archivo, tam_archivo);
		if(truncar_archivo(nombre_archivo, tam_archivo))
		{
			printf("ON\n");
			enviar_respuesta(arg_h->socket_kernel, OK);
		}
		else
		{
			enviar_respuesta(arg_h->socket_kernel, ERROR);
		}
		break;
	case RESET_FILE_SYSTEM: 
		//creacionFilesystem(filesystem,path_bloques);
		break;
	case RESET_FAT: 
		//reiniciar_fat(fat,ui32_max_entradas_fat);
		break;
	case MOSTRAR_TABLA_FAT: 
		mem_hexdump(fat->memory_map, (cant_bloques_total - cant_bloques_swap) * sizeof(uint32_t));
		break;
	case FIN_DE_PROGRAMA: 
		//fclose(fat);
		//fclose(filesystem);
		liberar_conexion(arg_h->socket_kernel);
		return EXIT_SUCCESS;
		break;
	default:
		liberar_conexion(arg_h->socket_kernel);
		return;
		break;
	}

}

int main(int argc, char* argv[]) {

	uint32_t ui32_tam_de_archivo=0;
    uint32_t ui32_max_entradas_fat=0;
    uint32_t ui32_cant_bloques_total=0;
	uint32_t ui32_cant_bloques_swap=0;
	uint32_t ui32_tam_bloque=0;

	//Usados para operaciones de lectura y escritura
	char buffer_data[1024+1]="";   
	char valorParametro[128]="";

	//------------------------------------------------------------------------
    logger = iniciar_logger("log_filesystem.log","FILESYSTEM");
    t_config* config = iniciar_config("./cfg/filesystem.config");
    
	puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	path_fat = config_get_string_value(config,"PATH_FAT");
    path_bloques = config_get_string_value(config,"PATH_BLOQUES");
	path_fcb = config_get_string_value(config,"PATH_FCB");
	cant_bloques_total=config_get_int_value(config,"CANT_BLOQUES_TOTAL");
    cant_bloques_swap=config_get_int_value(config,"CANT_BLOQUES_SWAP");
    tam_bloque=config_get_int_value(config,"TAM_BLOQUE");
    retardo_acceso_bloque=config_get_int_value(config,"RETARDO_ACCESO_BLOQUE");
    retardo_acceso_fat=config_get_int_value(config,"RETARDO_ACCESO_FAT");



	//bloques = fopen(path_bloques, "rb+");
	int archivo_de_bloques = abrir_archivo_de_bloques(path_bloques);

	char* s_bitarray = malloc((tam_bloque * cant_bloques_swap) / 8);
    swap_bitarray = bitarray_create_with_mode(s_bitarray, sizeof(s_bitarray), LSB_FIRST);

    printf("PUERTO_ESCUCHA=%s\n",puerto_escucha);
	fat = crear_fat_mapeada(path_fat);
	mem_hexdump(fat->memory_map, (cant_bloques_total - cant_bloques_swap) * sizeof(uint32_t));

    int socket_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);
    int socket_servidor = iniciar_servidor(logger, puerto_escucha);

	int socket_swap = esperar_cliente(logger, socket_servidor);
	recv(socket_swap, &tam_pagina, sizeof(uint32_t), MSG_WAITALL);

	t_args_hilo* args_hilo = malloc(sizeof(t_args_hilo));
	args_hilo->socket_memoria = socket_memoria;
	args_hilo->socket_swap = socket_swap;
    pthread_t h_conexion_memoria;
	pthread_create(&h_conexion_memoria, NULL, &conexion_memoria, (void*)args_hilo);
    pthread_detach(h_conexion_memoria);

	archivos_abiertos = list_create();

	//---------------------------------------------------------------
	remove("../filesystem/fcbs/ARCHITEST.fcb");
	remove("../filesystem/fcbs/ARCHITEST2.fcb");
	//---------------------------------------------------------------
	
	
	while(1)
	{
		t_args_hilo* args_hilo = malloc(sizeof(t_args_hilo));
		args_hilo->socket_memoria = socket_memoria;
		args_hilo->socket_kernel = esperar_cliente(logger, socket_servidor);
		pthread_t h_conexion_kernel;
		printf("(%i)Socket = %i\n",args_hilo, args_hilo->socket_kernel);
		pthread_create(&h_conexion_kernel, NULL, &conexion_kernel, (void*)args_hilo);
    	pthread_detach(h_conexion_kernel);
	}
	close(archivo_de_bloques);
	return 0;
}
