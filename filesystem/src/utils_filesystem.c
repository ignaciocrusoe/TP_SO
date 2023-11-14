#include "utils_filesystem.h"

#define LONG_BUFFER 50

/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/
/*----------------- OPERACIONES RELACIONADAS CON FILESYSTEM -----------------------*/
/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/

FILE *existeArchivoFilesystem(FILE *filesystem,char *c_path_bloques) {
    filesystem=fopen (c_path_bloques,"rb");
    if (filesystem==NULL) return (NULL); /*Retorna que el archivo NO existe */
    return (filesystem); /*Retorna que el archivo existe */
}

FILE *creacionFilesystem(FILE *filesystem,char *c_path_bloques) {
    uint32_t unBloque32Bits=0;
    filesystem=fopen (c_path_bloques,"w+b");
    if (filesystem==NULL) {
        return(NULL);
    }
    else {
        for (int i=0;i<262143;i++) fwrite(&unBloque32Bits,sizeof(unBloque32Bits),1,filesystem);
        return (filesystem);
    }
}

FILE *abrirFilesystem(FILE *filesystem,char *c_path_bloques) {
    filesystem=fopen (c_path_bloques,"r+b");
    if (filesystem==NULL) return (0);
    else return (filesystem);
}

FILE *iniciarArchivoFilesystem(FILE *filesystem,char *c_path_bloques) {
	filesystem=existeArchivoFilesystem(filesystem,c_path_bloques);
    if (filesystem) {
    	filesystem=abrirFilesystem(filesystem,c_path_bloques);
        if (filesystem) {
            return(filesystem);
        }
        else {
            printf("No se pudo abrir el archivo de Filesystem\n");
            return (NULL);
        }
    }
    else {
    	filesystem=creacionFilesystem(filesystem,c_path_bloques);
        if(filesystem) {
            return (filesystem);
        }
        else {
            printf("No se pudo crear el archivo de Filesystem\n");
            return (NULL);
        };
    }
}

char *obtenerLectura(char *buffer_bloque,char *buffer_data,uint32_t posicionPunteroRelativa,uint32_t cantBytes) {
    int i=0;
    int j=0;
    for (i=posicionPunteroRelativa;i<posicionPunteroRelativa+cantBytes;i++) {
        buffer_data[j]=buffer_bloque[i];
        j++;
    }
    j++;
    buffer_data[j]='\0';
    return(buffer_data);
}

/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/
/*---------------- OPERACIONES RELACIONADAS CON ARCHIVO FCB -----------------------*/
/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/

/*Obtiene del archivo fcb el tamanio del archivo del usuario*/
uint32_t tamanio_Archivo_fcb(char *nombreArchivo,char *c_directorio_fcb) {
    FILE *f_arch_fcb;
    char direccionArchivo[100]="";
    char cDatoLeido[LONG_BUFFER];
    char cTamanoArchivo[10]="";
    int i=0;
    int j=0;
    int tope=0;
    uint32_t numeroTamanioArchivo;

    strcpy(direccionArchivo,c_directorio_fcb);
    strcat(direccionArchivo,"/");
    strcat(direccionArchivo,nombreArchivo);
    strcat(direccionArchivo,".fcb");
    f_arch_fcb=fopen(direccionArchivo,"r");
    fgets(cDatoLeido,LONG_BUFFER,f_arch_fcb);
    fgets(cDatoLeido,LONG_BUFFER,f_arch_fcb);
    while (cDatoLeido[i++]!='=');
    tope=strlen(cDatoLeido)-i;
    for(j=0;j<tope;j++) cTamanoArchivo[j]=cDatoLeido[i+j];
    cTamanoArchivo[j]='\0';
    numeroTamanioArchivo = atoi(cTamanoArchivo);
    fclose(f_arch_fcb);
    return((uint32_t) numeroTamanioArchivo);
}

/*Obtiene del archivo fcb el bloque de inicio en la tabla FAT del archivo del usuario*/
uint32_t bloqueInicial_Archivo_fcb(char *nombreArchivo,char *c_directorio_fcb) {
    FILE *f_arch_fcb;
    char direccionArchivo[100]="";

    char cDatoLeido[LONG_BUFFER];
    char cBloqueInicio[4]="";
    int i=0;
    int j=0;
    int tope=0;
    int numeroBloqueIncial;


    strcpy(direccionArchivo,c_directorio_fcb);
    strcat(direccionArchivo,"/");
    strcat(direccionArchivo,nombreArchivo);
    strcat(direccionArchivo,".fcb");
    f_arch_fcb=fopen(direccionArchivo,"r");
    fgets(cDatoLeido,LONG_BUFFER,f_arch_fcb);
    fgets(cDatoLeido,LONG_BUFFER,f_arch_fcb);
    fgets(cDatoLeido,LONG_BUFFER,f_arch_fcb);
    while (cDatoLeido[i++]!='=');
    tope=strlen(cDatoLeido)-i;
    for(j=0;j<tope;j++) cBloqueInicio[j]=cDatoLeido[i+j];
    cBloqueInicio[j]='\0';
    numeroBloqueIncial = atoi(cBloqueInicio);
    fclose(f_arch_fcb);
    return((uint32_t) numeroBloqueIncial);
}

/*Calcula la cantidad de bloque FAT necesarios par almacenar datos de memoria*/
/*Retorna la cantidad de bloques necesitados contemplando la fragmentacion interna*/
uint32_t cantBloques_FAT_necesitados(uint32_t ui32_longMen_datos,uint32_t tamanioBloque) {
    uint32_t ui32_CantBloques=0;

    ui32_CantBloques=ui32_longMen_datos/tamanioBloque;
    if ((ui32_longMen_datos % tamanioBloque)!=0) ui32_CantBloques++;
    return(ui32_CantBloques);
}

/*Recorre la tabla FAT contando las entradas libres disponibles*/
/*Retorna la cantidad de entredas a la tabla FAT libres*/
uint32_t cantidadBloques_FAT_libres(FILE *fat,uint32_t MAX_ENTRADAS_FAT) {
    rewind(fat);
    uint32_t ui32_contBloques_FAT_libres=0;
    uint32_t ui32_indice=0;
    uint32_t data_tabla_FAT=0;

    for (ui32_indice=0;ui32_indice<MAX_ENTRADAS_FAT;ui32_indice++) {
        fread (&data_tabla_FAT,sizeof(data_tabla_FAT),1,fat);
        if (data_tabla_FAT==0) ui32_contBloques_FAT_libres++;
    }
    rewind(fat);
    return (ui32_contBloques_FAT_libres);
}

/*Busca un bloque libre en la tabla FAT*/
/*Retorna el primer bloque libre que encuentra*/
uint32_t buscarBloqueFatDisponible(FILE *fat,uint32_t MAX_ENTRADAS_FAT) {
    uint32_t ui32_data_bloque_tabla_FAT;
    uint32_t ui32_indiceBloque=0;

    ui32_indiceBloque=0;
    rewind(fat);
    fread (&ui32_data_bloque_tabla_FAT,sizeof(ui32_data_bloque_tabla_FAT),1,fat);
    while ((ui32_data_bloque_tabla_FAT!=0) && (ui32_indiceBloque<MAX_ENTRADAS_FAT)) {
        ui32_indiceBloque++;
        fread (&ui32_data_bloque_tabla_FAT,sizeof(ui32_data_bloque_tabla_FAT),1,fat);
    }
    fseek(fat,-4,SEEK_CUR);
    return (ui32_indiceBloque*4);
}

/*Actualiza datos almacenados en archivo fcb, cuando el archivo al que hace referencia tiene una modificación en relación a su tamaño*/
int actualizar_Archivo_fcb(char *nombreArchivo,uint32_t ui32_longMen_datos,uint32_t ui32_entrada_inicial,char *c_directorio_fcb) {
    FILE *f_arch_fcb;
    char direccionArchivo[100]="";
    char cInfo_a_Almacenar[100]="";
    char cNumero[10]="";
    strcpy(direccionArchivo,c_directorio_fcb);
    strcat(direccionArchivo,"/");
    /*------------Apertura de archivo fcb--------------------*/
    strcat(direccionArchivo,nombreArchivo);
    strcat(direccionArchivo,".fcb");
    f_arch_fcb=fopen(direccionArchivo,"w");
    /*-------------------------------------------------------*/
    strcpy(cInfo_a_Almacenar,"NOMBRE_ARCHIVO=");
    strcat(cInfo_a_Almacenar,nombreArchivo);
    strcat(cInfo_a_Almacenar,"\n");
    fwrite(cInfo_a_Almacenar,strlen(cInfo_a_Almacenar),1,f_arch_fcb);
        
    strcpy(cInfo_a_Almacenar,"TAMANIO_ARCHIVO=");
    strcat(cInfo_a_Almacenar,itoa_(ui32_longMen_datos,cNumero));
    strcat(cInfo_a_Almacenar,"\n");
    fwrite(cInfo_a_Almacenar,strlen(cInfo_a_Almacenar),1,f_arch_fcb);
    
    strcpy(cNumero,"");
    strcpy(cInfo_a_Almacenar,"BLOQUE_INICIAL=");
    strcat(cInfo_a_Almacenar,itoa_(ui32_entrada_inicial,cNumero));
    strcat(cInfo_a_Almacenar,"\n");
    fwrite(cInfo_a_Almacenar,strlen(cInfo_a_Almacenar),1,f_arch_fcb);
    /*-------------------------------------------------------*/
    fclose(f_arch_fcb);
    return(1);
}

/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/
/*--------------------- OPERACIONES RELACIONADAS CON FAT --------------------------*/
/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/

/*Determina la existencia del archivo de tabla FAT*/
int existeArchivoFAT(char *c_path_fat) {
    FILE *fat;
    fat=fopen (c_path_fat,"rb");
    if (fat==NULL) {
        return (0); /*Retorn NULL si el archivo no exite*/
    }
    fclose(fat);
    return (1); /*Retorna el puntero de archivo si este exite*/
}

/*Crea el archivo de tabla FAT*/
FILE *creacionFAT(FILE *fat,char *c_path_fat,uint32_t ui32_tamanio_fat) {
    uint32_t unBloque32Bits=0;
    fat=fopen (c_path_fat,"w+b");
    if (fat==NULL) {
        printf("No fue posible crear el archivo fat.dat\n");
        return(0);
    }
    else {
        for (int i=0;i<ui32_tamanio_fat;i++) fwrite(&unBloque32Bits,sizeof(unBloque32Bits),1,fat);
        fclose(fat);
        return (fat);
    }
}

/*Abre archivo de tabla FAT*/
FILE *abrirFAT(FILE *fat,char *c_path_fat) {
    fat=fopen (c_path_fat,"r+b");
    if (fat==NULL) return (NULL);
    else return (fat);
}

/*Busca si hay un archivo fat.dat creado, si es así lo abre. De otro modo lo crea y lo abre*/
FILE *iniciarArchivoFAT(FILE *fat,char *c_path_fat,uint32_t ui32_tamanio_fat) {
    if (existeArchivoFAT(c_path_fat)) {
    	fat=abrirFAT(fat,c_path_fat);
        if (fat!=NULL) {
            return(fat);
        }
        else {
            printf("---> No se pudo abrir el archivo tabla FAT\n");
            return (NULL);
        }
    }
    else {
        if(creacionFAT(fat,c_path_fat,ui32_tamanio_fat)) {
            fat=abrirFAT(fat,c_path_fat);
            if (fat!=NULL) {
                return(fat);
            }
            return (NULL);
        }
        else {
            printf("--> No se pudo crear el archivo de tabla FAT\n");
            return (NULL);
        }
    }
}

/*Función auxiliar para establecer todas las entradas de la tabla FAT a 0*/
FILE *reiniciar_fat(FILE *fat,uint32_t ui32_max_entradas_fat) {
	uint32_t i;
    uint32_t unBloque32Bits=0;
    rewind(fat);
    for (i=0;i<ui32_max_entradas_fat;i++) fwrite(&unBloque32Bits,sizeof(unBloque32Bits),1,fat);
    rewind(fat);
    return(fat);
}

/*Retorna la siguiente entrada de la tabla FAT siguiendo la lista enlazada*/
uint32_t siguiente_entrada_tabla_FAT(FILE *fat,uint32_t ui32_entrada_FAT) {
    uint32_t ui32_data_bloque_tabla_FAT=0;
    rewind(fat);
    ui32_entrada_FAT=ui32_entrada_FAT*4;
    fseek(fat,ui32_entrada_FAT,SEEK_SET);
    fread (&ui32_data_bloque_tabla_FAT,sizeof(ui32_data_bloque_tabla_FAT),1,fat);
    return(ui32_data_bloque_tabla_FAT/4);

}

/*Actualiza una entrada de la tabla FAT con un valor específico*/
int actualizar_entrada_FAT(FILE *fat,uint32_t ui32_entrada,uint32_t ui32_data_entrada) {
    fseek(fat,ui32_entrada*4,SEEK_SET);
    fwrite (&ui32_data_entrada,sizeof(ui32_data_entrada),1,fat);
    return(1);
}

/*Asigna bloques de la tabla FAT a un archivo*/
/*Retorna la direccion del bloque inicial de lista enlazada*/
uint32_t asignarBloquesFAT(FILE *fat,uint32_t cant_entradas_FAT_requeridas,uint32_t MAX_ENTRADAS_FAT) {
    uint32_t ui32_entrada_inicial=0;
    uint32_t ui32_entrada_actual=0;
    uint32_t ui32_entrada_anterior=0;
    uint32_t ui32_data_entrada=9999;
    uint32_t i;
    rewind(fat);
    for (i=0;i<cant_entradas_FAT_requeridas;i++) {
        ui32_entrada_actual=buscarBloqueFatDisponible(fat,MAX_ENTRADAS_FAT);
        if (i==0) {
            ui32_entrada_inicial=ui32_entrada_actual;
        }
        else {
            fseek(fat,ui32_entrada_anterior,SEEK_SET);
            ui32_data_entrada=ui32_entrada_actual;
            fwrite (&ui32_data_entrada,sizeof(ui32_data_entrada),1,fat);
            fseek(fat,ui32_entrada_actual,SEEK_SET);
        }
        ui32_data_entrada=9999;
        ui32_entrada_anterior=ui32_entrada_actual;
        fwrite (&ui32_data_entrada,sizeof(ui32_data_entrada),1,fat);
    }
    rewind(fat);
    return (ui32_entrada_inicial/4);
}

/*Muestrar el contenido del archivo de la tabla FAT - Limitado a primeros 40 bloques*/
void mostrar_tabla_FAT(FILE *fat,uint32_t MAX_ENTRADAS_FAT) {
    uint32_t ui32_bloqueFAT=0;
    uint32_t ui32_indice=0;
    
    rewind(fat);
    printf ("---------------------------------------\n");
    printf ("---------Contenido de la FAT-----------\n");
    printf ("---------------------------------------\n");
    for (ui32_indice=0;ui32_indice<10;ui32_indice++) {
        fread (&ui32_bloqueFAT,sizeof(ui32_bloqueFAT),1,fat);
        if (ui32_bloqueFAT!=9999) printf ("Entrada:(%u) se enlaza con:(%u)\n",ui32_indice,ui32_bloqueFAT/4);
        else printf ("Entrada:(%u) Fin de archivo con: (%u)\n",ui32_indice,ui32_bloqueFAT);
    }
    printf ("---------------------------------------\n");
    rewind(fat);
}

/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/
/*-------------------- OPERACIONES AUXILIARES CON ARCHIVOS ------------------------*/
/*---------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------*/
/*Función axiliar: Obtiene el tamanio del archivo*/
uint32_t tamanioArchivo (char *nombreArchivo) {
    FILE *f;
    uint32_t tamArchivo;

    f=fopen(nombreArchivo,"r");;
    fseek(f, 0L, SEEK_END);
    tamArchivo = ftell(f);
    fclose(f);
    return((uint32_t) tamArchivo);
}

/*Función axiliar: Obtiene el tamanio del archivo*/
uint32_t abrirDocumento(char *nombreArchivo,char *documentoArchivo) {
    FILE *f;
    int i=0;
    char caracter;
    uint32_t tamArchivo = tamanioArchivo(nombreArchivo);
    
    f=fopen(nombreArchivo,"r");
    for (i=0;i<tamArchivo;i++) {
        fread(&caracter,sizeof(caracter),1,f);
        documentoArchivo[i]=caracter;
    }
    documentoArchivo[tamArchivo]='\0';
    fclose(f);
    return(tamArchivo);
}

char *solicitarPaginaMemoria(char *paginaDeMemoria) {
    //La emulación de la página tiene 20 espacios y luego tiene la info a guardar
    char contenidoPagina[1024+1]="                    LOS-OPERATIVOS";
    strcpy(paginaDeMemoria,contenidoPagina);
    return(paginaDeMemoria);
}

char *determinaDatosEnPagina(char *paginaDeMemoria,uint32_t ui32_direccionDeMemoria,uint32_t ui32_cantBytes,char *buffer_data) {
    uint32_t i=0;
    uint32_t j=0;
    for (i=ui32_direccionDeMemoria;i<ui32_direccionDeMemoria+ui32_cantBytes;i++) {
        buffer_data[j]=paginaDeMemoria[i];
        j++;
    }
    j++;
    buffer_data[j]='\0';
    return(buffer_data);
}
int escribir_en_archivo_fisico(FILE *filesystem,uint32_t ui32_entrada_FAT,uint32_t ui32_tamBloque,char *buffer_data,uint32_t posicionPunteroRelativa,uint32_t cantBytes);
