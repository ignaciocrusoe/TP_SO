
#include "operaciones.h"
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

extern uint32_t cant_bloques_total;
extern uint32_t cant_bloques_swap;
extern uint32_t tam_bloque;
extern uint32_t retardo_acceso_bloque;
extern uint32_t retardo_acceso_fat;

extern t_list* archivos_abiertos;

extern t_fat* fat;

int32_t abrir_archivo(char* path_fcb, char* nombre)
{
	char* ruta = malloc(strlen(path_fcb) + 1 + strlen(nombre) + 4 + 1);
	strcpy(ruta, path_fcb);
	uint32_t tam_archivo;
	strcat(ruta, "/");
    strcat(ruta, nombre);
    strcat(ruta, ".fcb");
	int archivo_fcb = open(ruta, O_RDONLY);
	if(archivo_fcb == -1)
	{
		return -1;
	}
	//El archivo no va estar realmente abierto (o podríamos dejarlo abierto...).
    close(archivo_fcb);
    t_fcb* fcb = leer_fcb(path_fcb, nombre);
	list_add(archivos_abiertos, fcb);
	tam_archivo = fcb->tam_archivo;
	printf("aa_tam_archivo = %i\n", tam_archivo);
    return tam_archivo;
}

uint32_t crear_archivo(char* path_fcb, char* nombre)
{
    t_fcb* fcb = crear_fcb(nombre);
	char* ruta = malloc(strlen(path_fcb) + 1 + strlen(nombre) + 4 + 1);
	strcpy(ruta, path_fcb);
	uint32_t tam_archivo;
	strcat(ruta, "/");
    strcat(ruta, nombre);
    strcat(ruta, ".fcb");
	printf("PATH: %s", ruta);
	int archivo_fcb = open(ruta, O_CREAT | O_RDWR, 0664);
	t_config* config_fcb = iniciar_config(ruta);
	if(archivo_fcb == -1)
	{
		return -1;
	}
	list_add(archivos_abiertos, fcb);
	fcb->config = config_fcb;
	config_set_value(config_fcb, "NOMBRE_ARCHIVO", nombre);
	config_set_value(config_fcb, "TAMANIO_ARCHIVO", "0");
	config_set_value(config_fcb, "BLOQUE_INICIAL", int_to_string(UINT32_MAX));
	config_save(config_fcb);
	//El archivo no va estar realmente abierto (o podríamos dejarlo abierto...).
    close(archivo_fcb);
    return 1;
}

int32_t agrandar_archivo(t_fcb* archivo, uint32_t size)
{
	uint32_t por_asignar = size - archivo->tam_archivo;
	uint32_t tam_teorico = ceil(archivo->tam_archivo / tam_bloque) * tam_bloque;
	uint32_t bytes_libres = tam_teorico - archivo->tam_archivo;

	t_fcb* fcb;
	
	if(archivo->bloque_inicial == UINT32_MAX)
	{
		archivo->bloque_inicial = obtener_bloque_libre();
		fat->memory_map[archivo->bloque_inicial] = UINT32_MAX;
		//msync(fat->memory_map, (cant_bloques_total - cant_bloques_swap) * sizeof(uint32_t), MS_SYNC);
		//mem_hexdump(fat->memory_map, (cant_bloques_total - cant_bloques_swap) * sizeof(uint32_t));
		archivo->tam_archivo += MIN(tam_bloque, por_asignar);
		printf("pr_asignar = %i\n", por_asignar);
		por_asignar = MAX((int32_t)por_asignar - (int32_t)tam_bloque, 0);
		printf("Asigné el orimer bloque y me falta %i\n", por_asignar);
		config_set_value(archivo->config, "TAMANIO_ARCHIVO", int_to_string(archivo->tam_archivo));
		config_set_value(archivo->config, "BLOQUE_INICIAL", int_to_string(archivo->bloque_inicial));
		config_save(archivo->config);

		if(por_asignar == 0)
		{
			return;
		}
		
	}
	if(bytes_libres > 0)
	{
		bytes_libres -= MIN(bytes_libres, por_asignar);
		por_asignar -= MIN(bytes_libres, por_asignar);
		archivo->tam_archivo += MIN(bytes_libres, por_asignar);
		config_set_value(archivo->config, "TAMANIO_ARCHIVO", int_to_string(archivo->tam_archivo));
	}
	if(por_asignar > 0)
	{
		uint32_t nuevo_bloque = obtener_bloque_libre();
		fat->memory_map[ultimo_bloque(archivo->bloque_inicial)] = nuevo_bloque;
		fat->memory_map[nuevo_bloque] = UINT32_MAX;

		archivo->tam_archivo += MIN(tam_bloque, por_asignar);
		config_set_value(archivo->config, "TAMANIO_ARCHIVO", int_to_string(archivo->tam_archivo));
		bytes_libres = MAX((int32_t)tam_bloque - (int32_t)por_asignar, 0);
		por_asignar = MAX((int32_t)por_asignar - (int32_t)tam_bloque, 0);

		if(por_asignar > 0)
		{
			printf("Tengo que volver a agrandarlo por %i\n", por_asignar);
			agrandar_archivo(archivo, por_asignar);
			return;
		}
		else
		{
			return;
		}
	}
	else
	{
		return;
	}
}


uint32_t ultimo_bloque(uint32_t puntero)
{
	uint32_t puntero_sig = puntero;
	while(puntero_sig != UINT32_MAX)
	{
		puntero_sig = fat->memory_map[puntero];
		if(puntero_sig != UINT32_MAX)
		{
			puntero = puntero_sig;
		}
	}
	return puntero;
}
/*
int32_t agrandar_archivo(t_fcb* archivo, uint32_t size)
{
	uint32_t bytes_por_asignar = size - archivo->tam_archivo;
	uint32_t tam_teorico = ceil(archivo->tam_archivo / tam_bloque) * tam_bloque;
	uint32_t bytes_libres = tam_teorico - archivo->tam_archivo;
	printf("size = %i\n",size);
	printf("archivo->tam_archivo = %i\n",archivo->tam_archivo);
	printf("tam_teorico = %i\n",tam_teorico);
	printf("bytes_libres = %i\n",bytes_libres);

	printf("BLOQUES POR ASIGNAR = %f\n", ( (float)bytes_por_asignar - (float)bytes_libres) / (float)tam_bloque);

	uint32_t bloques_por_asignar = ceil(((double)bytes_por_asignar - (double)bytes_libres) / (double)tam_bloque);

	uint32_t ult_bloque;

	//Acá podríamos validar si hay bloques libres suficientes

	printf("BLOQUES POR ASIGNAR = %i\n", bloques_por_asignar);
	if(archivo->bloque_inicial == UINT32_MAX)
	{
		archivo->bloque_inicial = obtener_bloque_libre();
		fat->memory_map[archivo->bloque_inicial] = UINT32_MAX;
		bytes_por_asignar -= tam_bloque;
		archivo->tam_archivo += tam_bloque;
		config_set_value(archivo->config, "BLOQUE_INICIAL", int_to_string(archivo->bloque_inicial));
		config_save(archivo->config);
		if(bytes_por_asignar <= 0)
		{
			config_set_value(archivo->config, "TAMANIO_ARCHIVO", int_to_string(archivo->tam_archivo));
			return;
		}
	}
	bloques_por_asignar -= 1;
	printf("BLOQUES POR ASIGNAR = %i\n", bloques_por_asignar);
	ult_bloque = ultimo_bloque(archivo->bloque_inicial);
	for(uint32_t i = 0; i < bloques_por_asignar; i++)
	{
		uint32_t nuevo_bloque = obtener_bloque_libre();
		fat->memory_map[nuevo_bloque] = UINT32_MAX;
		fat->memory_map[ult_bloque] = nuevo_bloque;
		ult_bloque = nuevo_bloque;
	}
	config_set_value(archivo->config, "TAMANIO_ARCHIVO", int_to_string(archivo->tam_archivo));
	return;
}
*/
int32_t truncar_archivo(char* nombre, uint32_t size)
{
	printf("truncar_archivo\n");
	t_fcb* archivo = buscar_archivo(nombre, archivos_abiertos);
	printf("Encontré al archivo\n");
	//archivo->tam_archivo = size; //Faltan validaciones

	if(size > archivo->tam_archivo) //Se amplía
	{
		agrandar_archivo(archivo, size);
		mem_hexdump(fat->memory_map, (cant_bloques_total - cant_bloques_swap) * sizeof(uint32_t));
		//msync(fat->memory_map, (cant_bloques_total - cant_bloques_swap) * sizeof(uint32_t), MS_SYNC);
		fsync(fat->file_descriptor);
	}
	else if (size < archivo->tam_archivo) // Valido que no sea igual porque en ese caso no se hace nada
	{

	}
	
archivo->tam_archivo = size;

}

