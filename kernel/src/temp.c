#include "temp.h"

// - Directivas de instrucciones ------
#define F_OPEN 6000
#define F_CLOSE 6001
#define F_SEEK 6002
#define F_READ 6003
#define F_WRITE 6004
#define F_TRUNCATE 6005

#define RESET_FILE_SYSTEM 6008
#define RESET_FAT 6009
#define MOSTRAR_TABLA_FAT 6010
#define FIN_DE_PROGRAMA 6011

char secuencia[50][100]={
	"RESET_FILE_SYSTEM",
	"RESET_FAT",
	"F_OPEN documento1 W",
	"F_TRUNCATE documento1 100",
	"F_SEEK documento1 20",
	"F_WRITE documento1 14",
	"F_CLOSE documento1",
	"MOSTRAR_TABLA_FAT",
	"F_OPEN documento1 R",
	"F_SEEK documento1 20",
	"F_READ documento1 14",
	"F_CLOSE documento1",
	"F_OPEN documento2 W",
	"F_TRUNCATE documento2 3000",
	"F_SEEK documento2 100",
	"F_WRITE documento2 14",
	"F_CLOSE documento2",
	"MOSTRAR_TABLA_FAT",
	"F_OPEN documento2 R",
	"F_SEEK documento2 100",
	"F_READ documento2 14",
	"F_CLOSE documento2",
	"F_OPEN documento3 W",
	"F_TRUNCATE documento3 5000",
	"F_SEEK documento3 2100",
	"F_WRITE documento3 14",
	"F_CLOSE documento3",
	"MOSTRAR_TABLA_FAT",
	"F_OPEN documento3 R",
	"F_SEEK documento3 2100",
	"F_READ documento3 14",
	"F_CLOSE documento3",
	"FIN_DE_PROGRAMA",
};

void ejecutarSecuencia(int conexion) {
	op_code operacion;
    int opc=0;
	char instruccion[128]="";
	char puntero[128]="";
	char parametro[128]="";
	char parametro_a_enviar[128]="";
	char *valor;
	int i;
	int index=0;
	int contEspacios=0;
	for (i=0;i<33;i++){
		printf("\n-----------------------------------------\n");
		printf(">>Ejecuta:%s\n",secuencia[i]);
		strcpy(instruccion,secuencia[i]);

		if (strncmp(instruccion,"F_OPEN",6)==0){
			//-------------------------------------------------
			//--Envía operacion
			operacion = F_OPEN;
			enviar_operacion(conexion,operacion);
			//-------------------------------------------------
			//--Prepara y envía parametros
			strcpy(parametro_a_enviar,"");
			strcpy(parametro_a_enviar,&instruccion[7]);
			printf(">>Parametro a enviar:%s\n",parametro_a_enviar);
			enviar_mensaje(parametro_a_enviar,conexion);
			//-------------------------------------------------
			//--Recibe verificación de operacion
			valor=recibir_mensaje(conexion);
			if (atoi(valor)==0) printf(">>Respuesta:El archivo no existe. Lo crea\n");
			else printf(">>Respusta:El archivo existe y su tamaño es:%u Bytes\n",atoi(valor));
		}
		if (strncmp(instruccion,"F_SEEK",6)==0){
			strcpy(puntero,"");
			//-- Obtiene el puntero que ubica la posición requerida dentro del archivo
			index=0;
			contEspacios=0;
			while (contEspacios<2) {
				if (instruccion[index]==' ') contEspacios++;
				index++;
			}
			strcpy(puntero,&instruccion[index]);
			printf(">>Puntero identificado:%s\n",puntero);
		}
		else if (strncmp(instruccion,"F_READ",6)==0){
			//-------------------------------------------------
			//--Envía operacion
			operacion = F_READ;
			enviar_operacion(conexion,operacion);
			//-------------------------------------------------
			//--Prepara y envía parametros
			strcpy(parametro_a_enviar,"");
			strcpy(parametro_a_enviar,&instruccion[7]);
			//-- Añade puntero de posicion requerida del archivo
			strcat(parametro_a_enviar," ");
			strcat(parametro_a_enviar,puntero);
			//-- Añade direccion de memoria
			strcat(parametro_a_enviar," 12345");	
			printf(">>Parametro a enviar:%s\n",parametro_a_enviar);
			enviar_mensaje(parametro_a_enviar,conexion);
			//-------------------------------------------------
			//--Recibe verificación de operacion
			valor=recibir_mensaje(conexion);
				if (atoi(valor)==1) printf("F_READ - Respuesta: Verificador de op CORRECTO\n");
				else printf("F_READ - Respuesta: Verificador de op INCORRECTO\n");
		}
		else if (strncmp(instruccion,"F_WRITE",7)==0){
			//-------------------------------------------------
			//--Envía operacion
			operacion = F_WRITE;
			enviar_operacion(conexion,operacion);
			//-------------------------------------------------
			//--Prepara y envía parametros
			strcpy(parametro_a_enviar,"");
			strcpy(parametro_a_enviar,&instruccion[8]);
			//-- Añade puntero de posicion requerida del archivo
			strcat(parametro_a_enviar," ");
			strcat(parametro_a_enviar,puntero);
			//-- Añade direccion de memoria
			strcat(parametro_a_enviar," 20");
			printf(">>Parametro a enviar:%s\n",parametro_a_enviar);
			enviar_mensaje(parametro_a_enviar,conexion);
			//-------------------------------------------------
			//--Recibe verificación de operacion
			valor=recibir_mensaje(conexion);
				if (atoi(valor)==1) printf("F_WRITE - Respuesta: Verificador de op CORRECTO\n");
				else printf("F_WRITE - Respuesta: Verificador de op INCORRECTO\n");
		}
		else if (strncmp(instruccion,"F_TRUNCATE",10)==0){
			//-------------------------------------------------
			//--Envía operacion
			operacion = F_TRUNCATE;
			enviar_operacion(conexion,operacion);
			//-------------------------------------------------
			//--Prepara y envía parametros
			strcpy(parametro_a_enviar,"");
			strcpy(parametro_a_enviar,&instruccion[11]);
			printf(">>Parametro a enviar:%s\n",parametro_a_enviar);
			enviar_mensaje(parametro_a_enviar,conexion);
			//-------------------------------------------------
			//--Recibe verificación de operacion
			valor=recibir_mensaje(conexion);
			if (atoi(valor)==1) printf("F_TRUNCATE - Respuesta: Verificador de op CORRECTO\n");
			else printf("F_TRUNCATE - Respuesta: Verificador de op INCORRECTO\n");
		}
		/*-----------------------------------------------------------------*/
		/*------ INSTRUCCIONES PARA OPERAR FILESYSTEM (TEMPORALES)---------*/
		else if (strncmp(instruccion,"RESET_FILE_SYSTEM",17)==0){				
            operacion = RESET_FILE_SYSTEM;
			enviar_operacion(conexion,operacion);
		}
		else if (strncmp(instruccion,"RESET_FAT",9)==0){				
            operacion = RESET_FAT;
			enviar_operacion(conexion,operacion);
		}
		else if (strncmp(instruccion,"MOSTRAR_TABLA_FAT",17)==0){				
            operacion = MOSTRAR_TABLA_FAT;
			enviar_operacion(conexion,operacion);
		}
		else if (strncmp(instruccion,"FIN_DE_PROGRAMA",17)==0){				
            operacion = FIN_DE_PROGRAMA;
			enviar_operacion(conexion,operacion);
		}
	}
	printf(">>> Fin ejecucion instrucciones FILESYSTEM\n\n");
}
