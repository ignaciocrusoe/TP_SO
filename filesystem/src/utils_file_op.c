#include "utils_file_op.h"

//mensaje: mensaje con parametros concatenados
//valor: es el string en donde escribirá el valor del parámetro buscado
//parametro: es la posicion del parametro deseado dentro del mensaje
char *buscaDatoEnMensaje(char *mensaje,char *valor,int parametro) {
    int i=0;
    int j=0;
    int w=0;

    for (w=0;w<parametro;w++) {
        //Busca los dos puntos luego de la palabra
        while (mensaje[i]!=' ') i++;
        i++;
    }
    //Busca los - luego del nombre del archivo
    while ((mensaje[i]!=' ') && (mensaje[i]!='\0')) {
        valor[j]=mensaje[i];
        i++;
        j++;
    }
    valor[j]='\0';
    return(valor);
}


/*---------------- OPERACIONES RELACIONADAS CON LISTA DE ARCHIVOS ABIERTOS --------------- */

struct nodo *new_nodo(char *nombreArchivo,uint32_t tamanioArchivo,uint32_t bloqueinicial) {
    lista aux =malloc(sizeof(nodoL));
    strcpy(aux ->nombreArchivo,nombreArchivo);
    aux -> tamanio_archivo = tamanioArchivo;
    aux -> bloque_inicial = bloqueinicial;
    aux -> sig = NULL;
    
    return(aux);
}

void add_opened_file(lista *L,char *nombreArchivo,uint32_t tamanioArchivo,uint32_t bloqueinicial) {
    struct nodo *ptr;
    
    if (*L==NULL) {
        *L=new_nodo(nombreArchivo,tamanioArchivo,bloqueinicial);
    }
    else {
        ptr=*L;
        while (ptr->sig!=NULL) ptr=ptr->sig;
        ptr->sig = new_nodo(nombreArchivo,tamanioArchivo,bloqueinicial);
    }
}

void del_opened_file(lista *L,char *nombreArchivo) {
    struct nodo *ptr;
    struct nodo *ptr2;

    ptr=*L;
    ptr2=*L;

    if (strcmp(ptr->nombreArchivo,nombreArchivo)!=0) {
        *L=ptr->sig;
        free(ptr);
    }
    else {
        while ((strcmp((ptr->sig)->nombreArchivo,nombreArchivo))!=0) ptr=ptr->sig;
        ptr2=ptr->sig;
        ptr->sig=(ptr->sig)->sig;
        free(ptr2);

    }
}

void update_opened_file(lista *L,char *nombreArchivo,uint32_t tamanioArchivo,uint32_t bloqueinicial) {
    struct nodo *ptr;

    ptr=*L;  
    while (strcmp(ptr->nombreArchivo,nombreArchivo)!=0) ptr=ptr->sig;
    strcpy(ptr ->nombreArchivo,nombreArchivo);
    ptr -> tamanio_archivo = tamanioArchivo;
    ptr -> bloque_inicial = bloqueinicial;
}

void mostrarLista (lista *L) {
    struct nodo *ptr;
    ptr=*L;
    while (ptr!=NULL) {
        printf ("-----------------------------------\n");
        printf ("El nombre del archivo es:%s\n",ptr->nombreArchivo);
        printf ("El tamaño del archivo es:%u\n",ptr->tamanio_archivo);
        printf ("La entrada al bloque inicial es:%u\n",ptr->bloque_inicial);
        ptr=ptr->sig;
    }
}
/*
int main (){
    
    lista l_opened_files=NULL;

    char parametro[128];    
    printf ("Archivo:%s\n",buscaDatoEnMensaje("archivo:documento1-posicion:20-cantBytes:10-direccion:12324",parametro,1));
    printf ("Posicion puntero:%u\n",atoi(buscaDatoEnMensaje("archivo:documento1-posicion:20-cantBytes:10-direccion:12324",parametro,2)));
    printf ("Cantidad de bytes a leer:%u\n",atoi(buscaDatoEnMensaje("archivo:documento1-posicion:20-cantBytes:10-direccion:12324",parametro,3)));
    printf ("Direccion de memoria:%u\n",atoi(buscaDatoEnMensaje("archivo:documento1-posicion:20-cantBytes:10-direccion:12324",parametro,4)));


    add_opened_file(&l_opened_files,"documento1",10,100);
    add_opened_file(&l_opened_files,"documento2",20,200);
    add_opened_file(&l_opened_files,"documento3",30,300);
    mostrarLista (&l_opened_files);
    printf("--------------------------\n\n");
    del_opened_file(&l_opened_files,"documento3");
    mostrarLista (&l_opened_files);
    update_opened_file(&l_opened_files,"documento3",90,900);
    printf("--------------------------\n\n");
    mostrarLista (&l_opened_files);
}
*/
