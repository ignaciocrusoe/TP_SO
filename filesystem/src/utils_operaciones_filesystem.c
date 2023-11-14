#include "utils_operaciones_filesystem.h"
/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
/* Emulacion de peticiones del modulo Kernel*/
/*-----------------------------------------------------------------------------------*/
/* 1.1) CREAR O ABRE UN ARCHIVO ----------------------*/
uint32_t crear_abrir_archivo(char *nombreArchivo,char *c_directorio_fcb,t_log *logger) {
    FILE *f_arch_fcb;
    char direccionArchivo[100]="";
    strcpy(direccionArchivo,c_directorio_fcb);
    strcat(direccionArchivo,"/");
    strcat(direccionArchivo,nombreArchivo);
    strcat(direccionArchivo,".fcb");
    f_arch_fcb=fopen(direccionArchivo,"r");
    if (f_arch_fcb==NULL) {
        crear_archivo(nombreArchivo,c_directorio_fcb,logger);
        return(0);
    }
    return(tamanio_Archivo_fcb(nombreArchivo,c_directorio_fcb));
}

/* 1.2) CREAR_ARCHIVO-----------------------------*/
int crear_archivo(char *nombreArchivo,char *c_directorio_fcb,t_log *logger) {
    actualizar_Archivo_fcb(nombreArchivo,0,9999,c_directorio_fcb);
    committed_logger_CREAR_ARCHIVO(nombreArchivo,logger);
    return(1);
}
/* 3) TRUNCAR_ARCHIVO-----------------------------*/
int truncar_archivo(char *nombreArchivo,uint32_t ui32_longMen_datos,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,uint32_t ui32_max_entradas_fat,char *c_directorio_fcb) {
    uint32_t ui32_entrada_inicial;
    uint32_t ui32_entrada_FAT;
    uint32_t ui32_entrada_FAT_siguiente;
    uint32_t ui32_cant_bloques_necesitados;
    uint32_t ui32_cant_bloques_asignados;
    uint32_t ui32_cant_bloques_libres;
    uint32_t ui32_cant_bloques_adionales;
    uint32_t ui32_data_entrada;
    uint32_t indice;


    printf("OP:TRUNCAR_ARCHIVO\n");
    ui32_entrada_inicial=bloqueInicial_Archivo_fcb(nombreArchivo,c_directorio_fcb);

    if (ui32_entrada_inicial==9999) {
        printf("El archivo aún no tiene datos almacenados\n");
        /* Caso (1): Asignacion de entradas a la tabla FAT a un archivo recientemente creado*/
        /*El archivo fcb NO tiene una entrada inicial asignada en la tabla FAT*/
        ui32_cant_bloques_necesitados=cantBloques_FAT_necesitados(ui32_longMen_datos,ui32_tamBloque);
        ui32_cant_bloques_libres=cantidadBloques_FAT_libres(fat,ui32_max_entradas_fat);
        /*Evalua si la cantidad de entradas en la Tabla FAT son suficientes*/
        if(ui32_cant_bloques_necesitados<=ui32_cant_bloques_libres) {
            ui32_entrada_inicial=asignarBloquesFAT(fat,ui32_cant_bloques_necesitados,ui32_max_entradas_fat);
            actualizar_Archivo_fcb(nombreArchivo,ui32_longMen_datos,ui32_entrada_inicial,c_directorio_fcb);
        }
        else {
            /*El archivo fcb NO tiene una entrada inicial asignada en la tabla FAT, pero NO hay mas
            entradas disponibles en la tabla FAT*/
            printf ("No hay espacio sufienciente en Disco para guardar el archivo\n");
        }
    }
    else {
        printf("--- Caso (2) de TRUNCADO, el archivo ya existe -----\n");
        /* Caso (2): El archivo ya tiene entradas en la tabla FAT asignadas*/
        //- Calcula la cantidad de entradas que requeriran los datos de memoria a almacenar
        ui32_cant_bloques_necesitados=cantBloques_FAT_necesitados(ui32_longMen_datos,ui32_tamBloque);
        //- Calcula la cantidad de entradas que ya tiene el archivo asignados previamente
        ui32_cant_bloques_asignados=cantBloques_FAT_necesitados(tamanio_Archivo_fcb(nombreArchivo,c_directorio_fcb),ui32_tamBloque);

        if (ui32_cant_bloques_necesitados<=ui32_cant_bloques_asignados) {
            /*Caso 2A - La cantidad de entradas necesitadas es menor o igual a las que tiene
            ya asignadas el archivo*/
            ui32_entrada_FAT=bloqueInicial_Archivo_fcb(nombreArchivo,c_directorio_fcb); //Entrada inicial a la tabla FAT

            //Recorre la lista enlazada mientras que cuenta las entradas que necesita la version en memoria del archivo
            for (indice=0;indice<ui32_cant_bloques_necesitados;indice++) {
                if (indice==0) ui32_entrada_FAT=bloqueInicial_Archivo_fcb(nombreArchivo,c_directorio_fcb); //Entrada inicial a la tabla FAT
                else ui32_entrada_FAT=siguiente_entrada_tabla_FAT(fat,ui32_entrada_FAT);
            }
            //Si la ultima entrada que necesita la version actualizada es distinto a 9999 es poque debe liberar entradas
            if ((ui32_entrada_FAT!=9999) && (ui32_entrada_FAT!=2499)){
                ui32_entrada_FAT_siguiente=siguiente_entrada_tabla_FAT(fat,ui32_entrada_FAT);
                ui32_data_entrada=9999;
                actualizar_entrada_FAT(fat,ui32_entrada_FAT,ui32_data_entrada);
                ui32_entrada_FAT=ui32_entrada_FAT_siguiente;
                while ((ui32_entrada_FAT!=9999) && (ui32_entrada_FAT!=2499)) {
                    ui32_entrada_FAT_siguiente=siguiente_entrada_tabla_FAT(fat,ui32_entrada_FAT);
                    ui32_data_entrada=0;
                    actualizar_entrada_FAT(fat,ui32_entrada_FAT,ui32_data_entrada);
                    ui32_entrada_FAT=ui32_entrada_FAT_siguiente;
                }
                ui32_data_entrada=0;
                actualizar_entrada_FAT(fat,ui32_entrada_FAT,ui32_data_entrada);

            }
        }
        else {
            /*Caso 2B - La cantidad de entradas necesitadas es mayot a las que tiene
            ya asignadas el archivo almacendo*/
            ui32_cant_bloques_necesitados=cantBloques_FAT_necesitados(ui32_longMen_datos,ui32_tamBloque);
            ui32_cant_bloques_libres=cantidadBloques_FAT_libres(fat,ui32_max_entradas_fat);
            ui32_cant_bloques_asignados=cantBloques_FAT_necesitados(tamanio_Archivo_fcb(nombreArchivo,c_directorio_fcb),ui32_tamBloque);
            ui32_cant_bloques_adionales=ui32_cant_bloques_necesitados-ui32_cant_bloques_asignados;

            //Recorre la lista de entradas ya asignadas al archivo, en busqueda de la ultima entrada
            ui32_entrada_FAT=bloqueInicial_Archivo_fcb(nombreArchivo,c_directorio_fcb); //Entrada inicial a la tabla FAT
            ui32_entrada_FAT_siguiente=siguiente_entrada_tabla_FAT(fat,ui32_entrada_FAT);
            while ((ui32_entrada_FAT_siguiente!=9999) && (ui32_entrada_FAT_siguiente!=2499)) {
                ui32_entrada_FAT=ui32_entrada_FAT_siguiente;
                ui32_entrada_FAT_siguiente=siguiente_entrada_tabla_FAT(fat,ui32_entrada_FAT);
            }
            ui32_data_entrada=asignarBloquesFAT(fat,ui32_cant_bloques_adionales,ui32_max_entradas_fat)*4;
            actualizar_entrada_FAT(fat,ui32_entrada_FAT,ui32_data_entrada);
        }
    }
    committed_logger_TRUNCAR(nombreArchivo,logger,c_directorio_fcb);
    
    return(1);
}

/* 4) LEER_ARCHIVO-----------------------------*/
//FUNCION TEMPORAL PARA HACER PRUEBAS DE LECTURA COMPLETAS DE UN ARCHIVO
int leer_archivo(char *nombreArchivo,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,FILE *filesystem,char *c_directorio_fcb) {
    uint32_t ui32_cantBloques_a_leer=0;
    uint32_t ui32_entrada_FAT;
    uint32_t ui32_dataEntrada_FAT;
    uint32_t ui32_numero_bloque=0;
    uint32_t indice=0;
    char buffer_lectura[1024+1]="";
    char buffer_documento[1024*30]=""; //Tamanio del buffer es un punto a analizar porque no esta determinado

    printf("OP:LEER_ARCHIVO\n");
    /*Calcula la cantidad de bloques a escribir en el archivo bloques.dat*/
    /*Obtiene la direccion de la entrada inicial de la tabla FAT*/
    ui32_cantBloques_a_leer=cantBloques_FAT_necesitados(tamanio_Archivo_fcb(nombreArchivo,c_directorio_fcb),ui32_tamBloque);
    ui32_entrada_FAT=bloqueInicial_Archivo_fcb(nombreArchivo,c_directorio_fcb);
    for (indice=0;indice<ui32_cantBloques_a_leer;indice++) {
        strcat(buffer_documento,lectura_de_archivo_bloques(filesystem,ui32_entrada_FAT,buffer_lectura,ui32_tamBloque));
        ui32_dataEntrada_FAT=siguiente_entrada_tabla_FAT(fat,ui32_entrada_FAT);
        committed_logger_ACCESO_FAT(ui32_entrada_FAT,ui32_dataEntrada_FAT,logger);
        committed_logger_LECTURA_ARCHIVO(nombreArchivo,ui32_entrada_FAT,0,logger);//0 porque falta completar la direccion de memoria
        committed_logger_ACCESO_BLOQUE_ARCHIVO(nombreArchivo,ui32_numero_bloque,ui32_entrada_FAT,ui32_tamBloque,logger);
        ui32_entrada_FAT=ui32_dataEntrada_FAT;
        ui32_numero_bloque++;
    }
    printf("---------------------------------------------------------------------------------\n");
    printf("El documento leido desde el arhivo: %s es:...\n\n",nombreArchivo);
    printf("%s\n",buffer_documento);
    printf("---------------------------------------------------------------------------------\n");
    printf ("El tamanio del archivo es:%ld bytes\n",strlen(buffer_documento));
    printf("---------------------------------------------------------------------------------\n");
    return(1);
}

//Lectura de archivo redefinido para leer en un lugar en particular
char *leer_archivo_bloque_n(char *nombreArchivo,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,FILE *filesystem,char *c_directorio_fcb,uint32_t posicionPuntero,uint32_t cantBytes,char *buffer_data) {
    uint32_t ui32_cantBloques_a_leer=0;
    uint32_t ui32_entrada_FAT;
    uint32_t ui32_dataEntrada_FAT;
    uint32_t ui32_numero_bloque=0;
    uint32_t indice=0;
    uint32_t ui32_bloque_a_leer;
    uint32_t posicionPunteroRelativa;
    char buffer_lectura[1024+1]=""; //Ver como incializar si el tamaño del bloque cambia
    char buffer_bloque[1024+1]=""; //Ver como incializar si el tamaño del bloque cambia
    
    printf("OP:LEER_ARCHIVO\n");
    /*Calcula la cantidad de bloques a escribir en el archivo bloques.dat*/
    /*Obtiene la direccion de la entrada inicial de la tabla FAT*/
    //ui32_cantBloques_a_leer=cantBloques_FAT_necesitados(tamanio_Archivo_fcb(nombreArchivo,c_directorio_fcb),ui32_tamBloque);
    ui32_bloque_a_leer = posicionPuntero/ui32_tamBloque;
    ui32_entrada_FAT=bloqueInicial_Archivo_fcb(nombreArchivo,c_directorio_fcb);
    for (indice=0;indice<ui32_bloque_a_leer;indice++) {
        ui32_dataEntrada_FAT=siguiente_entrada_tabla_FAT(fat,ui32_entrada_FAT);
        ui32_entrada_FAT=ui32_dataEntrada_FAT;
        ui32_numero_bloque++;
    }
    strcat(buffer_bloque,lectura_de_archivo_bloques(filesystem,ui32_entrada_FAT,buffer_lectura,ui32_tamBloque));
    buffer_bloque[ui32_tamBloque]='\0';
    if (posicionPuntero<ui32_tamBloque) posicionPunteroRelativa=posicionPuntero;
    else posicionPunteroRelativa=posicionPuntero-((posicionPuntero/ui32_tamBloque)*ui32_tamBloque);
    buffer_data=obtenerLectura(lectura_de_archivo_bloques(filesystem,ui32_entrada_FAT,buffer_lectura,ui32_tamBloque),buffer_data,posicionPunteroRelativa,cantBytes);
    return(buffer_data);
}

/* 5) ESCRIBIR_ARCHIVO-----------------------------*/
//FUNCION TEMPORAL PARA HACER PRUEBAS DE ESCRITURA COMPLETAS DE UN ARCHIVO
int escribir_archivo(char *nombreArchivo,char *documentoArchivo,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,FILE *filesystem,char *c_directorio_fcb) {
    uint32_t ui32_cantBloques_a_escribir=0;
    uint32_t ui32_entrada_FAT=0;
    uint32_t ui32_dataEntrada_FAT;
    uint32_t ui32_numero_bloque=0;
    char buffer_escritura[1024];
    uint32_t indice=0;

    printf("OP:ESCRIBIR_ARCHIVO\n");
    /*Calcula la cantidad de bloques a escribir en el archivo bloques.dat*/
    /*Obtiene la direccion de la entrada inicial de la tabla FAT*/
    ui32_cantBloques_a_escribir=cantBloques_FAT_necesitados(tamanio_Archivo_fcb(nombreArchivo,c_directorio_fcb),ui32_tamBloque);
    ui32_entrada_FAT=bloqueInicial_Archivo_fcb(nombreArchivo,c_directorio_fcb);
    for (indice=0;indice<ui32_cantBloques_a_escribir;indice++) {
        strcpy(buffer_escritura,cargar_buffer_escritura(buffer_escritura,ui32_numero_bloque,documentoArchivo,ui32_tamBloque));
        escribir_en_archivo_bloques(filesystem,ui32_entrada_FAT,buffer_escritura,ui32_tamBloque);
        ui32_dataEntrada_FAT=siguiente_entrada_tabla_FAT(fat,ui32_entrada_FAT);
        committed_logger_ACCESO_FAT(ui32_entrada_FAT,ui32_dataEntrada_FAT,logger);
        committed_logger_ESCRITURA_ARCHIVO(nombreArchivo,ui32_entrada_FAT,0,logger);//0 porque falta completar la direccion de memoria
        committed_logger_ACCESO_BLOQUE_ARCHIVO(nombreArchivo,ui32_numero_bloque,ui32_entrada_FAT,ui32_tamBloque,logger);
        ui32_entrada_FAT=ui32_dataEntrada_FAT;
        ui32_numero_bloque++;
    }
    return(1);
}

/* 5) ESCRIBIR_ARCHIVO-----------------------------*/
int escribir_archivo_n(char *nombreArchivo,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,FILE *filesystem,char *c_directorio_fcb,uint32_t posicionPuntero,uint32_t cantBytes,char *buffer_data) {
    uint32_t ui32_bloque_a_escribir=0;
    uint32_t ui32_entrada_FAT=0;
    uint32_t ui32_dataEntrada_FAT;
    uint32_t ui32_numero_bloque=0;
    uint32_t posicionPunteroRelativa;
    char buffer_escritura[1024];
    uint32_t indice=0;

    printf("OP:ESCRIBIR_ARCHIVO\n");
    /*Calcula la cantidad de bloques a escribir en el archivo bloques.dat*/
    /*Obtiene la direccion de la entrada inicial de la tabla FAT*/
    ui32_bloque_a_escribir = posicionPuntero/ui32_tamBloque;
    ui32_entrada_FAT=bloqueInicial_Archivo_fcb(nombreArchivo,c_directorio_fcb);
    for (indice=0;indice<ui32_bloque_a_escribir;indice++) {
        ui32_dataEntrada_FAT=siguiente_entrada_tabla_FAT(fat,ui32_entrada_FAT);
        ui32_entrada_FAT=ui32_dataEntrada_FAT;
        ui32_numero_bloque++;
    }
    if (posicionPuntero<ui32_tamBloque) posicionPunteroRelativa=posicionPuntero;
    else posicionPunteroRelativa=posicionPuntero-((posicionPuntero/ui32_tamBloque)*ui32_tamBloque);
    escribir_en_archivo_fisico(filesystem,ui32_entrada_FAT,ui32_tamBloque,buffer_data,posicionPunteroRelativa,cantBytes);

    return(1);
}