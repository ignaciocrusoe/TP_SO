#ifndef UTILS_OPERACIONES_H
#define UTILS_OPERACIONES_H

#include <ctype.h>
#include <stdio.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <commons/log.h>
#include "utils_filesystem.h"
#include "utils_adicionales.h"
#include "utils_bloques.h"
#include "utils_committed_logger.h"


/* 1.1) ABRIR_ARCHIVO-----------------------------*/
uint32_t crear_abrir_archivo(char *nombreArchivo,char *c_directorio_fcb,t_log *logger);
/* 1.2) CREAR_ARCHIVO-----------------------------*/
int crear_archivo(char *nombreArchivo,char *c_directorio_fcb,t_log *logger);
/* 3) TRUNCAR_ARCHIVO-----------------------------*/
int truncar_archivo(char *nombreArchivo,uint32_t ui32_longMen_datos,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,uint32_t ui32_max_entradas_fat,char *c_directorio_fcb); 
/* 4) LEER_ARCHIVO-----------------------------*/
int leer_archivo(char *nombreArchivo,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,FILE *filesystem,char *c_directorio_fcb);
char *leer_archivo_bloque_n(char *nombreArchivo,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,FILE *filesystem,char *c_directorio_fcb,uint32_t posicionPuntero,uint32_t cantBytes,char *buffer_bloque);
/* 5) ESCRIBIR_ARCHIVO-----------------------------*/
int escribir_archivo(char *nombreArchivo,char *documentoArchivo,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,FILE *filesystem,char *c_directorio_fcb);
int escribir_archivo_n(char *nombreArchivo,t_log *logger,FILE *fat,uint32_t ui32_tamBloque,FILE *filesystem,char *c_directorio_fcb,uint32_t posicionPuntero,uint32_t cantBytes,char *buffer_bloque);
#endif