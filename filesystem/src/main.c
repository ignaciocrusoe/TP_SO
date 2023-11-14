#include "main.h"


uint32_t abrir_archivo(char* path_fcb, char* nombre)
{
    t_fcb* fcb = malloc(sizeof(t_fcb));
    char* ruta = path_fcb;
	uint32_t tam_archivo;
    strcat(ruta, nombre);
    strcat(ruta, ".fcb");
	FILE* archivo_fcb = open(ruta, O_RDONLY);
	if(archivo_fcb == -1)
	{
		return -1;
	}
	fseek(archivo_fcb, 0, SEEK_END);
	tam_archivo = ftell(archivo_fcb);
	//El archivo no va estar realmente abierto (o podríamos dejarlo abierto...).
    fclose(archivo_fcb);
    return tam_archivo;
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
	char nombreArchivo[128]="";
	char modoApertura[10]="";

	//------------------------------------------------------------------------
    t_log* logger = iniciar_logger("log_filesystem.log","FILESYSTEM");
    t_config* config = iniciar_config("../filesystem/cfg/filesystem.config");
    
	char* puerto_escucha = config_get_string_value(config,"PUERTO_ESCUCHA");
    char* ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    char* puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
	char* path_fat = config_get_string_value(config,"PATH_FAT");
    char* path_bloques = config_get_string_value(config,"PATH_BLOQUES");
	char* path_fcb = config_get_string_value(config,"PATH_FCB");
	char* cant_bloques_total=config_get_string_value(config,"CANT_BLOQUES_TOTAL");
    char* cant_bloques_swap=config_get_string_value(config,"CANT_BLOQUES_SWAP");
    char* tam_bloque=config_get_string_value(config,"TAM_BLOQUE");
    //char* retardo_acceso_bloque=config_get_string_value(config,"RETARDO_ACCESO_BLOQUE");
    //char* retardo_acceso_fat=config_get_string_value(config,"RETARDO_ACCESO_FAT");

    ui32_cant_bloques_total=atoi(cant_bloques_total);
    ui32_cant_bloques_swap=atoi(cant_bloques_swap);
    ui32_max_entradas_fat=ui32_cant_bloques_total-ui32_cant_bloques_swap;
    ui32_tam_bloque=atoi(tam_bloque);

    printf("PUERTO_ESCUCHA=%s\n",puerto_escucha);

    int conexion_memoria = crear_conexion(logger, ip_memoria, puerto_memoria);

    int socket_servidor = iniciar_servidor(logger, puerto_escucha);
    int socket_kernel = esperar_cliente(logger, socket_servidor);
    if(socket_kernel){
        log_info(logger,"Se conectó kernel");
    }

	//Borra los archivos fcb para realizar las pruebas
	remove("../filesystem/fcbs/documento1.fcb");
	remove("../filesystem/fcbs/documento2.fcb");
	remove("../filesystem/fcbs/documento3.fcb");
    //Creación e inicialización de Filesystem y Fat
	/*-------------------------------------------------------*/
	printf("----------------------------------------------------------\n");
    filesystem=iniciarArchivoFilesystem(filesystem,path_bloques);
	printf ("---> Apertura filesystem: bloques.dat <ok>\n");
    filesystem=creacionFilesystem(filesystem,path_bloques); //(QUIRAR A FUTUTO)reset temporal de archivo bloques.dat
	printf ("---> Reset filesystem: bloques.dat <ok>\n");
	/*-------------------------------------------------------*/
	fat=iniciarArchivoFAT(fat,path_fat,ui32_max_entradas_fat);
	printf ("---> Apertura de archivo fat.dat: <ok>\n");
	fat=reiniciar_fat(fat,ui32_max_entradas_fat); //(QUIRAR A FUTUTO)Reset de fat.dat para probar la creación de la tabla
	printf ("---> Reset tabla FAT <ok>\n");
	/*-------------------------------------------------------*/
	//Documento para probar el funcionamiento del Filesystem y Fat
	printf ("---> Emulación de contenido en memoria de: documento1.txt <ok>\n");
    ui32_tam_de_archivo=abrirDocumento("../filesystem/archivo_datos/documento1.txt",documentoArchivo);
	printf("----------------------------------------------------------\n");
    op_code operacion;
	t_list* lista;
    while(1)  {
		printf("----------------------------------------------\n");
		operacion = recibir_operacion(socket_kernel);
		printf (">>Se recibe operacion:%d\n",operacion);

		if (operacion==F_OPEN) {
			printf(">>>Operacion recibida es F_OPEN\n");
			char *valor=recibir_mensaje(socket_kernel);
			printf("El valor recibido es:%s\n",valor);
			//-------------------------------------------------------
			//--- Nombre de archivo
			strcpy(nombreArchivo,buscaDatoEnMensaje(valor,valorParametro,0));
			printf ("Nombre del archivo recibido:%s\n",valorParametro);
			//-------------------------------------------------------
			//--- Modo de apertura
			strcpy(modoApertura,buscaDatoEnMensaje(valor,valorParametro,1));
			printf ("Modo de apertura:%s\n",modoApertura);
			//--- Crea el archivo si no existe y sino lo abre informado en tamaño
			char dataTamanio[10]="";
			itoa_(crear_abrir_archivo(nombreArchivo,path_fcb,logger),dataTamanio);
			enviar_mensaje(dataTamanio,socket_kernel);
		}

		else if (operacion==F_TRUNCATE) {
			printf(">>>Operacion recibida es F_TRUNCATE\n");
			char *valor=recibir_mensaje(socket_kernel);
			printf("El valor recibido es:%s\n",valor);
			//-------------------------------------------------------
			//--- Nombre de archivo
			strcpy(nombreArchivo,buscaDatoEnMensaje(valor,valorParametro,0));
			printf ("Nombre del archivo recibido:%s\n",valorParametro);
			//-------------------------------------------------------
			//--- Modo de apertura
			char cantBytesTruncar[10]="";
			strcpy(cantBytesTruncar,buscaDatoEnMensaje(valor,valorParametro,1));
			printf ("Cantidad de Bytes a <Truncar> archivo:%s\n",cantBytesTruncar);
			//--- Crea el archivo si no existe y sino lo abre informado en tamaño
			truncar_archivo(nombreArchivo,(uint32_t) atoi(cantBytesTruncar),logger,fat,ui32_tam_bloque,ui32_max_entradas_fat,path_fcb);
			//--- Envía mensaje de operacion finalizada
			enviar_mensaje("1",socket_kernel);
		}

		else if (operacion==F_READ) {
			printf(">>>Operacion recibida es F_READ\n");
			char *valor=recibir_mensaje(socket_kernel);
			printf("El valor recibido es:%s\n",valor);
			//-------------------------------------------------------
			//--- Nombre de archivo
			
			buscaDatoEnMensaje(valor,valorParametro,0);
			printf ("Nombre del archivo recibido:%s\n",valorParametro);
			//-------------------------------------------------------
			//--- Cantidad de bytes a leer
			uint32_t ui32_cantBytes=(uint32_t) atoi(buscaDatoEnMensaje(valor,valorParametro,1));
			printf ("Cantidad d Bytes a leer:%u\n",ui32_cantBytes);
			//-------------------------------------------------------
			//--- Posicion del puntero
			uint32_t ui32_posicionPuntero=(uint32_t) atoi(buscaDatoEnMensaje(valor,valorParametro,2));
			printf ("Posición del puntero en archivo:%u\n",ui32_posicionPuntero);		
			//-------------------------------------------------------
			//--- Direccion de memoria a volvar los datos
			uint32_t ui32_direccionDeMemoria=(uint32_t) atoi(buscaDatoEnMensaje(valor,valorParametro,3));
			printf ("Direccion de memoria:%u\n",ui32_direccionDeMemoria);	
			//-------------------------------------------------------
			//--- Direccion de memoria a volvar los datos
			printf("Datos del bloque:%s\n",leer_archivo_bloque_n("documento1",logger,fat,ui32_tam_bloque,filesystem,path_fcb,ui32_posicionPuntero,ui32_cantBytes,buffer_data));
			//-------------------------------------------------------
			//--- Envía mensaje de operarcion finalizada
			enviar_mensaje("1",socket_kernel);
		}

		else if (operacion==F_WRITE) {
			char valorParametro[128]="";

			printf(">>>Oeracion recibida es F_WRITE\n");
			char *valor=recibir_mensaje(socket_kernel);
			printf("Valor recibido es:%s\n",valor);
			//-------------------------------------------------------
			//--- Nombre de archivo
			buscaDatoEnMensaje(valor,valorParametro,0);
			printf ("Nombre del archivo recibido:%s\n",valorParametro);
			//-------------------------------------------------------
			//--- Cantidad de bytes a leer
			uint32_t ui32_cantBytes=(uint32_t) atoi(buscaDatoEnMensaje(valor,valorParametro,1));
			printf ("Cantidad de Bytes a escribir:%u\n",ui32_cantBytes);
			//-------------------------------------------------------
			//--- Posicion del puntero
			uint32_t ui32_posicionPuntero=(uint32_t) atoi(buscaDatoEnMensaje(valor,valorParametro,2));
			printf ("Posición del puntero en archivo:%u\n",ui32_posicionPuntero);			
			//-------------------------------------------------------
			//--- Direccion de memoria a volvar los datos
			uint32_t ui32_direccionDeMemoria=(uint32_t) atoi(buscaDatoEnMensaje(valor,valorParametro,3));
			printf ("Direccion de memoria:%u\n",ui32_direccionDeMemoria);	
			//-------------------------------------------------------
			//--- Direccion de memoria a volvar los datos
			char buffer_data[1024+1]="";
			//-- Emulación de pedidos de datos a la memoria para escribir en el archivo
			char paginaDeMemoria[1024+1]="";
			solicitarPaginaMemoria(paginaDeMemoria);
			printf ("El contenido de la página de memoria es:%s\n",paginaDeMemoria);
			determinaDatosEnPagina(paginaDeMemoria,ui32_direccionDeMemoria,ui32_cantBytes,buffer_data);
			printf ("Los datos a almacenar son:%s\n",buffer_data);
			escribir_archivo_n("documento1",logger,fat,ui32_tam_bloque,filesystem,path_fcb,ui32_posicionPuntero,ui32_cantBytes,buffer_data);
			//-------------------------------------------------------
			//--- Envía mensaje de operarcion finalizada
			enviar_mensaje("1",socket_kernel);
		}

		else if (operacion==RESET_FILE_SYSTEM){
			creacionFilesystem(filesystem,path_bloques);
		}

		else if (operacion==RESET_FAT){
			reiniciar_fat(fat,ui32_max_entradas_fat);
		}

		else if (operacion==MOSTRAR_TABLA_FAT){
			mostrar_tabla_FAT(fat,ui32_max_entradas_fat);
		}

		else if (operacion==FIN_DE_PROGRAMA){
			fclose(fat);
			fclose(filesystem);
			liberar_conexion(socket_kernel);
			return EXIT_SUCCESS;
		}
		else {
			//Completar con...
		}
    }
    return 0;
}