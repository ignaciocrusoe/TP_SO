#include "manejo_de_archivos.h"

extern sem_t grado_de_multiprogramacion;
extern sem_t mutex_cola_new;
extern sem_t mutex_cola_ready;
extern sem_t mutex_cola_exit;
extern sem_t procesos_en_new;
extern sem_t procesos_en_ready;
extern sem_t procesos_en_exit;
extern sem_t planificacion_largo_plazo;
extern sem_t planificacion_corto_plazo;
extern sem_t mutex_file_management;
extern sem_t mutex_tabla_global_de_archivos;

extern t_queue *cola_ready;
extern t_queue *cola_exit;
extern t_queue *cola_new;

extern t_pcb *execute;
extern t_list *recursos_disponibles;
extern t_list *tabla_global_de_archivos;
extern t_log *logger;

extern char* ip_filesystem;
extern char* puerto_filesystem;
char* algoritmo_planificacion;

t_archivo *crear_archivo(char *nombre_archivo, uint32_t tam_archivo, t_lock lock)
{
    t_archivo *archivo = malloc(sizeof(t_archivo));
    archivo->cola_blocked = queue_create();
    archivo->tam_archivo = tam_archivo;
    archivo->nombre = malloc(strlen(nombre_archivo) + 1);
    strcpy(archivo->nombre, nombre_archivo);
    archivo->lock = lock;
    archivo->cola_blocked = queue_create();
    archivo->locks_lectura = list_create();
    sem_init(&(archivo->mutex_cola_blocked), 0, 1);
    sem_init(&(archivo->mutex_locks_lectura), 0, 1);
    archivo->contador_aperturas = 1;
    return archivo;
}

t_lock de_string_a_t_lock(char *str)
{
    t_lock lock = NONE;
    if (str[0] == 'R')
    {
        lock = READ;
    }
    else if (str[0] == 'W')
    {
        lock = WRITE;
    }
    return lock;
}

t_archivo *buscar_archivo(t_list *lista, char *nombre)
{
    bool tiene_el_mismo_nombre(void *arg)
    {
        t_archivo *archivo = (t_archivo *)arg;
        return (!strcmp(archivo->nombre, nombre));
    }
    t_archivo *archivo = NULL;
    archivo = list_find(lista, tiene_el_mismo_nombre);
    return archivo;
}

void file_open(void *arg)
{
    //sem_wait(&mutex_file_management);
    t_args_hilo_archivos *arg_h = (t_args_hilo_archivos *)arg;
    t_response respuesta;
    arg_h->socket_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);
    //printf("arg_h->socket_filesystem = %i\n", arg_h->socket_filesystem);
    t_archivo *archivo = buscar_archivo(tabla_global_de_archivos, arg_h->nombre_archivo);
    t_archivo_local *archivo_local = malloc(sizeof(t_archivo_local));
    archivo_local->archivo = archivo;
    archivo_local->puntero = 0;
    if (archivo != NULL)
    {
        //printf("archivo != NULL\n");

        if (archivo->lock == READ)
        {
            //printf("archivo->lock == READ");
            if (arg_h->lock == READ)
            {
                archivo->contador_aperturas++;
                //printf("arg_h->lock == READ");
                sem_wait(&(archivo->mutex_locks_lectura));
                list_add(archivo->locks_lectura, arg_h->execute); // no se bloquea el proceso
                list_add(arg_h->execute->tabla_de_archivos_abiertos, archivo_local);
                sem_post(&(archivo->mutex_locks_lectura));
                sem_wait(&mutex_cola_ready);
                //printf("execute = %i - arg_h->execute = %i\n", execute->pid, arg_h->execute->pid);
                arg_h->execute->estado = READY;
                queue_push(cola_ready, arg_h->execute);
                log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
                sem_post(&mutex_cola_ready);
                sem_post(&procesos_en_ready);
            }
            else // hay un lock de LECTURA y se pide ESCRITURA
            {
                //printf("arg_h->lock != READ");
                t_proceso_bloqueado_por_fs *bloqueado = malloc(sizeof(t_proceso_bloqueado_por_fs));
                bloqueado->pcb = arg_h->execute;
                bloqueado->lock = arg_h->lock;
                sem_wait(&(archivo->mutex_cola_blocked));
                queue_push(archivo->cola_blocked, bloqueado);
                log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
                sem_post(&(archivo->mutex_cola_blocked));
            }
        }
        else if (archivo->lock == WRITE)
        {
            int temp;
            sem_getvalue(&(archivo->mutex_cola_blocked), &temp);
            t_proceso_bloqueado_por_fs *bloqueado = malloc(sizeof(t_proceso_bloqueado_por_fs));
            bloqueado->pcb = arg_h->execute;
            bloqueado->lock = arg_h->lock;
            bloqueado->pcb->estado = BLOCKED;
            //printf("archivo->mutex_cola_blocked = %i\n", temp);
            sem_wait(&(archivo->mutex_cola_blocked));
            queue_push(archivo->cola_blocked, bloqueado);
            //printf("Agrego al proceso %i a bloqueado por lock (hay %i procesos) LOCK = %i\n", bloqueado->pcb->pid, archivo->cola_blocked->elements->elements_count, arg_h->lock);
            sem_post(&(archivo->mutex_cola_blocked));
            arg_h->execute->estado = BLOCKED;
        }
        else // Ningún archivo lo tiene abierto
        {
            archivo->contador_aperturas++;
            archivo->lock = READ;
            //printf("arg_h->lock == READ");
            sem_wait(&(archivo->mutex_locks_lectura));
            list_add(archivo->locks_lectura, arg_h->execute); // no se bloquea el proceso
            list_add(arg_h->execute->tabla_de_archivos_abiertos, archivo_local);
            sem_post(&(archivo->mutex_locks_lectura));
            sem_wait(&mutex_cola_ready);
            //printf("execute = %i - arg_h->execute = %i\n", execute->pid, arg_h->execute->pid);
            queue_push(cola_ready, arg_h->execute);
            log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
            //printf("1");
            sem_post(&mutex_cola_ready);
            //printf("2");
            sem_post(&procesos_en_ready);
        }
    }
    else
    {
        //printf("archivo == NULL\n");
        archivo = crear_archivo(arg_h->nombre_archivo, arg_h->tam_archivo, arg_h->lock);
        archivo_local->archivo = archivo;
        archivo_local->puntero = 0;
        //printf("archivo == %i\n", archivo);
        //printf("archivo->nombre == %s\n", archivo->nombre);
        sem_wait(&mutex_tabla_global_de_archivos);
        list_add(tabla_global_de_archivos, archivo);
        sem_post(&mutex_tabla_global_de_archivos);

        list_add(arg_h->execute->tabla_de_archivos_abiertos, archivo_local);
        log_info(logger, "Agregué el archivo %s en %i", archivo_local->archivo->nombre, tabla_global_de_archivos);

        enviar_operacion(arg_h->socket_filesystem, ABRIR_ARCHIVO);
        enviar_mensaje(arg_h->nombre_archivo, arg_h->socket_filesystem); // SERIALIZAR
        recv(arg_h->socket_filesystem, &(arg_h->tam_archivo), sizeof(int32_t), MSG_WAITALL);
        if (arg_h->tam_archivo != -1)
        {
            //printf("El archivo tiene un tamaño de %i bytes (lock = %i)\n", arg_h->tam_archivo, arg_h->lock);
            arg_h->execute->estado = READY;
            sem_wait(&mutex_cola_ready);
            queue_push(cola_ready, arg_h->execute);
            log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
            sem_post(&mutex_cola_ready);
            sem_post(&procesos_en_ready);
        }
        else
        {
            //printf("El archivo no existe\n");
            // Se le pide a Filesystem que cree el archivo
            arg_h->socket_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);
            enviar_operacion(arg_h->socket_filesystem, CREAR_ARCHIVO);
            enviar_mensaje(arg_h->nombre_archivo, arg_h->socket_filesystem); // SERIALIZAR
            //printf("Mandé el nombre del archivo\n");
            // Podríamos recibir un OK, de hecho creo que hay que recibirlo
            respuesta = recibir_respuesta(arg_h->socket_filesystem);
            switch (respuesta)
            {
            case OK:
                sem_wait(&mutex_cola_ready);
                queue_push(cola_ready, arg_h->execute); // Debería ser en la primera posición.
                log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
                sem_post(&mutex_cola_ready);
                sem_post(&procesos_en_ready);

                break;
            default:
                break;
            }
        }
    }

    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
    {
        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
    }

    liberar_conexion(arg_h->socket_filesystem);
    //sem_post(&mutex_file_management);
    //printf("TERMINA EL HILO\n");
    liberar_parametros(arg_h);
    return;
}

void file_read(void *arg)
{
    //sem_wait(&mutex_file_management);
    t_args_hilo_archivos *arg_h = (t_args_hilo_archivos *)arg;
    t_response respuesta;
    arg_h->socket_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);

    bool es_el_archivo_local(void *arg)
    {
        return (!strcmp(((t_archivo_local *)arg)->archivo->nombre, arg_h->nombre_archivo));
    }
    //printf("arg_h->socket_filesystem = %i\n", arg_h->socket_filesystem);
    enviar_operacion(arg_h->socket_filesystem, LEER_ARCHIVO);
    enviar_mensaje(arg_h->nombre_archivo, arg_h->socket_filesystem); // SERIALIZAR (OP, NOMBRE_ARCHIVO, PUNTERO)
    t_archivo_local *archivo = list_find(arg_h->execute->tabla_de_archivos_abiertos, es_el_archivo_local);
    send(arg_h->socket_filesystem, &(archivo->puntero), sizeof(uint32_t), 0);

    //printf("Dirección = %i:%i\n", arg_h->direccion->frame, arg_h->direccion->frame);

    enviar_direccion(arg_h->socket_filesystem, arg_h->direccion); // SERIALIZAR NO HACE FALTA
    // recv(arg_h->socket_filesystem, &tam_archivo, sizeof(int32_t), MSG_WAITALL);

    respuesta = recibir_respuesta(arg_h->socket_filesystem);
    switch (respuesta)
    {
    case OK:
        //printf("OK lectura\n");
        sem_wait(&mutex_cola_ready);
        queue_push(cola_ready, arg_h->execute); // Debería ser en la primera posición.
        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
        sem_post(&mutex_cola_ready);
        sem_post(&procesos_en_ready);
        //printf("OK pasado a ready\n");

        break;
    default:
        break;
    }

    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
    {
        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
    }

    liberar_conexion(arg_h->socket_filesystem);
    //sem_post(&mutex_file_management);
    liberar_parametros(arg_h);
    return;
}

void file_write(void *arg)
{
    //sem_wait(&mutex_file_management);
    t_args_hilo_archivos *arg_h = (t_args_hilo_archivos *)arg;
    t_response respuesta;
    arg_h->socket_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);
    bool es_el_archivo_local(void *arg)
    {
        return (!strcmp(((t_archivo_local *)arg)->archivo->nombre, arg_h->nombre_archivo));
    }
    //printf("Direccion = %i:%i\n", arg_h->direccion->frame, arg_h->direccion->offset);
    t_archivo_local *archivo = list_find(arg_h->execute->tabla_de_archivos_abiertos, es_el_archivo_local);

    if(archivo->archivo->lock == READ)
    {
        sem_wait(&mutex_cola_exit);
        queue_push(cola_exit, arg_h->execute);
        log_info(logger, "Fin de proceso %i motivo INVALID WRITE", arg_h->execute->pid);
        sem_post(&mutex_cola_exit);
        sem_post(&procesos_en_exit);

        if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
        {
        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
        }

        liberar_conexion(arg_h->socket_filesystem);
        liberar_parametros(arg_h);
        return;
    }

    enviar_operacion(arg_h->socket_filesystem, ESCRIBIR_ARCHIVO);
    enviar_mensaje(arg_h->nombre_archivo, arg_h->socket_filesystem);
    send(arg_h->socket_filesystem, &(archivo->puntero), sizeof(uint32_t), 0);
    enviar_direccion(arg_h->socket_filesystem, arg_h->direccion); // SERIALIZAR (OPERACION, NOMBRE, PUNTERO, DIRECCION)
    respuesta = recibir_respuesta(arg_h->socket_filesystem);
    switch (respuesta)
    {
    case OK:
        //printf("OK escritura\n");
        sem_wait(&mutex_cola_ready);
        queue_push(cola_ready, arg_h->execute);
        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
        // enviar_operacion(arg_h->socket_interrupt, CLOCK_INTERRUPT); //Cambiar a I/O interrupt
        sem_post(&mutex_cola_ready);
        sem_post(&procesos_en_ready);
        break;
    default:
        break;
    }
    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
    {
        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
    }
    liberar_conexion(arg_h->socket_filesystem);
    //sem_post(&mutex_file_management);
    liberar_parametros(arg_h);
}

void file_truncate(void *arg)
{
    //printf("Empieza file_truncate()\n");
    //sem_wait(&mutex_file_management);
    t_args_hilo_archivos *arg_h = (t_args_hilo_archivos *)arg;
    t_response respuesta;
    arg_h->socket_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);
    //printf("PRINT");
    //printf("Pido truncar %s a %u", arg_h->nombre_archivo, arg_h->tam_archivo);
    enviar_operacion(arg_h->socket_filesystem, TRUNCAR_ARCHIVO);
    enviar_mensaje(arg_h->nombre_archivo, arg_h->socket_filesystem);
    send(arg_h->socket_filesystem, &(arg_h->tam_archivo), sizeof(uint32_t), NULL); // SERIALIZAR
    respuesta = recibir_respuesta(arg_h->socket_filesystem);
    switch (respuesta)
    {
    case OK:
        //printf("OK\n");
        int temp;
        sem_getvalue(&mutex_cola_ready, &temp);
        //printf("mutex_cola_ready = %i\n", temp);
        sem_wait(&mutex_cola_ready);
        queue_push(cola_ready, arg_h->execute);
        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
        sem_post(&mutex_cola_ready);
        sem_post(&procesos_en_ready);
        break;
    default:
        break;
    }
    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
    {
        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
    }
    liberar_conexion(arg_h->socket_filesystem);
    //sem_post(&mutex_file_management);
    //printf("TERMINA EL HILO\n");
    liberar_parametros(arg_h);
}

void file_close(void *arg)
{
    t_args_hilo_archivos *arg_h = (t_args_hilo_archivos *)arg;
    t_archivo *archivo;
    t_archivo_local *archivo_local = malloc(sizeof(t_archivo_local));
    //printf("Me piden F_CLOSE de %s\n", arg_h->nombre_archivo);
    t_proceso_bloqueado_por_fs *proceso_bloqueado;
    bool es_el_archivo(void *arg)
    {
        return (!strcmp(((t_archivo *)arg)->nombre, arg_h->nombre_archivo));
    }
    bool es_el_archivo_local(void *arg)
    {
        return (!strcmp(((t_archivo_local *)arg)->archivo->nombre, arg_h->nombre_archivo));
    }
    bool es_el_proceso(void *arg)
    {
        return (((t_pcb *)arg)->pid == arg_h->execute->pid);
    }
    bool tiene_lock_lectura(void *arg)
    {
        return (((t_proceso_bloqueado_por_fs *)arg)->lock == READ);
    }
    void iterator_agregar_a_ready(void *arg)
    {
        list_add(((t_pcb *)arg)->tabla_de_archivos_abiertos, archivo_local);
        //printf("AGREGUÉ EL ARCHIVO LOCAL %s\n\n", archivo_local->archivo->nombre);
        queue_push(cola_ready, (t_pcb *)arg);
        sem_post(&procesos_en_ready);
        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", ((t_pcb *)arg)->pid);
    }
    void iterator_agregar_a_locks_lectura(void *arg)
    {
        if ((t_proceso_bloqueado_por_fs *)arg != proceso_bloqueado)
        {
            //printf("Agrego PID: %i a los locks de lectura\n\n", ((t_proceso_bloqueado_por_fs *)arg)->pcb->pid);
            list_add(((t_proceso_bloqueado_por_fs *)arg)->pcb->tabla_de_archivos_abiertos, archivo_local);
            list_add(archivo->locks_lectura, ((t_proceso_bloqueado_por_fs *)arg)->pcb);
        }
    }
    void iterator_eliminar_procesos_con_lock_de_escritura(void *arg)
    {
        list_remove_by_condition(arg, tiene_lock_lectura);
    }
    //printf("file_close()\n");
    //sem_wait(&mutex_file_management);
    //printf("ejecuta file_close()\n");
    t_response respuesta;
    arg_h->socket_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);

    archivo = buscar_archivo(tabla_global_de_archivos, arg_h->nombre_archivo);
    if (archivo != NULL)
    {
        //printf("archivo != NULL (es %s)\n", archivo->nombre);
        //printf("Procesos bloqueados por lock = %i\n", archivo->cola_blocked->elements->elements_count);
        list_remove_by_condition(arg_h->execute->tabla_de_archivos_abiertos, es_el_archivo_local);

        arg_h->execute->estado = READY;
        sem_wait(&mutex_cola_ready);
        queue_push(cola_ready, arg_h->execute);
        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
        sem_post(&mutex_cola_ready);
        sem_post(&procesos_en_ready);
        //log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", arg_h->execute->pid);
        //printf("archivo->cola_blocked->elements->elements_count = %i\n", archivo->cola_blocked->elements->elements_count);
        //printf("archivo->locks_lectura->elements_count = %i\n", archivo->locks_lectura->elements_count);
        if (archivo->lock == READ)
        {
            //printf("archivo->lock == READ\n");
            list_remove_by_condition(archivo->locks_lectura, es_el_proceso);
            archivo->contador_aperturas--;
            if (archivo->locks_lectura->elements_count == 0) // El proceso es el único que lo está leyendo
            {
                //printf("archivo->locks_lectura->elements_count == 0\n");
                if (archivo->cola_blocked->elements->elements_count != 0) // Hay elememtos bloqueados
                {
                    proceso_bloqueado = queue_pop(archivo->cola_blocked);
                    if (proceso_bloqueado != NULL)
                    {
                        archivo->lock = proceso_bloqueado->lock;
                        proceso_bloqueado->pcb->estado = READY;
                        archivo_local->archivo = archivo;
                        archivo_local->puntero = 0;

                        list_add(proceso_bloqueado->pcb->tabla_de_archivos_abiertos, archivo_local);
                        sem_wait(&mutex_cola_ready);
                        // queue_push(cola_ready, arg_h->execute);
                        queue_push(cola_ready, proceso_bloqueado->pcb);
                        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_bloqueado->pcb->pid);
                        //printf("EL PROCESO QUE DESBLOQUEO QUE TENÍA LOCK DE ESCRITURA ES EL %i\n", proceso_bloqueado->pcb->pid);
                        sem_post(&mutex_cola_ready);
                        sem_post(&procesos_en_ready);
                        archivo->contador_aperturas++;
                    }

                    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
                    {
                        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
                    }

                    //printf("TERMINA EL HILO PORQUE TODOS LOS BLOQUEADOS SON DE ESCRITURA\n");
                    liberar_parametros(arg_h);
                    //sem_post(&mutex_file_management);
                    return;
                }
                else // Se elimina el archivo completamente
                {
                    log_warning(logger, "Se elimina el archivo");
                    list_remove_by_condition(tabla_global_de_archivos, es_el_archivo);


                    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
                    {
                        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
                    }

                    liberar_conexion(arg_h->socket_filesystem);
                    //printf("TERMINA EL HILO PORQUE NO HAY NADA\n");
                    liberar_parametros(arg_h);
                    //sem_post(&mutex_file_management);
                    return;
                }
            }
            else
            {
                liberar_conexion(arg_h->socket_filesystem);
                liberar_parametros(arg_h);
                //sem_post(&mutex_file_management);
                return;
            }
        }
        else // CUANDO EL LOCK DEL ARCHIVO ES WRITE
        {
            if (archivo->cola_blocked->elements->elements_count > 0) // Hay elementos bloqueados (no se sabe si R o W)
            {
                archivo_local->archivo = archivo;
                archivo_local->puntero = 0;
                // sem_wait(&(archivo->mutex_cola_blocked));
                proceso_bloqueado = queue_pop(archivo->cola_blocked);
                // sem_post(&(archivo->mutex_cola_blocked));
                if (proceso_bloqueado != NULL)
                {
                    archivo->lock = proceso_bloqueado->lock;
                    list_add(proceso_bloqueado->pcb->tabla_de_archivos_abiertos, archivo_local);
                    //printf("EL PROCESO BLOQUEADO ES EL QUE TIENE PID = %i\n", proceso_bloqueado->pcb->pid);

                    if (proceso_bloqueado->lock == READ && archivo->cola_blocked->elements->elements_count > 0) // Hay que desbloquear otros procesos con READ
                    {
                        archivo_local->archivo = archivo;
                        archivo_local->puntero = 0;
                        t_list *procesos_lectura = list_filter(archivo->cola_blocked->elements, tiene_lock_lectura);
                        list_iterate(procesos_lectura, iterator_agregar_a_locks_lectura);
                        sem_wait(&mutex_cola_ready);
                        queue_push(cola_ready, proceso_bloqueado->pcb);
                        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_bloqueado->pcb->pid);
                        list_iterate(archivo->locks_lectura, iterator_agregar_a_ready);
                        list_add(archivo->locks_lectura, proceso_bloqueado->pcb);
                        sem_post(&procesos_en_ready);
                        sem_post(&mutex_cola_ready);
                        void *element;
                        do
                        {
                            element = list_remove_by_condition(archivo->cola_blocked->elements, tiene_lock_lectura);
                        } while (element != NULL);
                        //printf("TERMINA EL HILO PORQUE NO HAY NADA\n");
                        if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
                        {
                            enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
                        }
                        liberar_parametros(arg_h);
                        liberar_conexion(arg_h->socket_filesystem);
                        //sem_post(&mutex_file_management);
                        //printf("Después del fclose, hay %i archivos con lock de lectura, y %i bloqueados\n", archivo->locks_lectura->elements_count, archivo->cola_blocked->elements->elements_count);
                        return;
                    }
                    else // No se desbloquea nada (El lock es WRITE)
                    {
                        archivo_local->archivo = archivo;
                        archivo_local->puntero = 0;
                        sem_wait(&mutex_cola_ready);
                        // queue_push(cola_ready, arg_h->execute);
                        queue_push(cola_ready, proceso_bloqueado->pcb);
                        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_bloqueado->pcb->pid);
                        sem_post(&procesos_en_ready);
                        sem_post(&mutex_cola_ready);
                        list_add(proceso_bloqueado->pcb->tabla_de_archivos_abiertos, archivo_local);
                        //printf("TERMINA EL HILO PORQUE NO HAY NADA\n");
                        if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
                        {
                            enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
                        }
                        liberar_parametros(arg_h);
                        liberar_conexion(arg_h->socket_filesystem);
                        //sem_post(&mutex_file_management);
                        return;
                    }
                }
            }
            else
            {
                // Se elimina el archivo
                list_remove_element(tabla_global_de_archivos, archivo);
                log_warning(logger, "Se elimina el archivo");
            }
        }
        if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
        {
            enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
        }
        liberar_parametros(arg_h);
        liberar_conexion(arg_h->socket_filesystem);
        //sem_post(&mutex_file_management);
        return;
    }

    // sem_wait(&mutex_cola_ready);
    // queue_push(cola_ready, arg_h->execute);
    // sem_post(&mutex_cola_ready);
    // sem_post(&procesos_en_ready);
    // //printf("TERMINA EL HILO\n");
    // //sem_post(&mutex_file_management);
}

void file_seek(void *arg)
{
    t_args_hilo_archivos *arg_h = (t_args_hilo_archivos *)arg;
    t_response respuesta;

    bool es_el_archivo_local(void *arg)
    {
        return (!strcmp(((t_archivo_local *)arg)->archivo->nombre, arg_h->nombre_archivo));
    }

    //sem_wait(&mutex_file_management);
    arg_h->socket_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);
    log_info(logger, "Busco el archivo %s en %i", arg_h->nombre_archivo, tabla_global_de_archivos);
    // sem_wait(&mutex_tabla_global_de_archivos);
    t_archivo_local *archivo = list_find(arg_h->execute->tabla_de_archivos_abiertos, es_el_archivo_local);
    // sem_post(&mutex_tabla_global_de_archivos);
    if (archivo != NULL)
    {
        archivo->puntero = arg_h->puntero;
        log_info(logger, "Archivo: %s - Puntero: %i", arg_h->nombre_archivo, arg_h->puntero);
        sem_wait(&mutex_cola_ready);
        agregar_primero_en_cola(cola_ready, arg_h->execute);
        sem_post(&mutex_cola_ready);
        sem_post(&procesos_en_ready);
    }
    else
    {
        log_error(logger, "El archivo no existe");
    }
    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
    {
        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
    }
    liberar_conexion(arg_h->socket_filesystem);
    //sem_post(&mutex_file_management);
}



/*FUNCIONES PARA FINALIZAR_PROCESO*/
void cerrar_archivo(void* arg)
{
    t_args_hilo_archivos *arg_h = (t_args_hilo_archivos *)arg;
    t_archivo *archivo;
    t_archivo_local *archivo_local = malloc(sizeof(t_archivo_local));
    //printf("Me piden F_CLOSE de %s\n", arg_h->nombre_archivo);
    t_proceso_bloqueado_por_fs *proceso_bloqueado;
    bool es_el_archivo(void *arg)
    {
        return (!strcmp(((t_archivo *)arg)->nombre, arg_h->nombre_archivo));
    }
    bool es_el_archivo_local(void *arg)
    {
        return (!strcmp(((t_archivo_local *)arg)->archivo->nombre, arg_h->nombre_archivo));
    }
    bool es_el_proceso(void *arg)
    {
        return (((t_pcb *)arg)->pid == arg_h->execute->pid);
    }
    bool tiene_lock_lectura(void *arg)
    {
        return (((t_proceso_bloqueado_por_fs *)arg)->lock == READ);
    }
    void iterator_agregar_a_ready(void *arg)
    {
        list_add(((t_pcb *)arg)->tabla_de_archivos_abiertos, archivo_local);
        //printf("AGREGUÉ EL ARCHIVO LOCAL %s\n\n", archivo_local->archivo->nombre);
        queue_push(cola_ready, (t_pcb *)arg);
        sem_post(&procesos_en_ready);
        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", ((t_pcb *)arg)->pid);
    }
    void iterator_agregar_a_locks_lectura(void *arg)
    {
        if ((t_proceso_bloqueado_por_fs *)arg != proceso_bloqueado)
        {
            //printf("Agrego PID: %i a los locks de lectura\n\n", ((t_proceso_bloqueado_por_fs *)arg)->pcb->pid);
            list_add(((t_proceso_bloqueado_por_fs *)arg)->pcb->tabla_de_archivos_abiertos, archivo_local);
            list_add(archivo->locks_lectura, ((t_proceso_bloqueado_por_fs *)arg)->pcb);
        }
    }
    void iterator_eliminar_procesos_con_lock_de_escritura(void *arg)
    {
        list_remove_by_condition(arg, tiene_lock_lectura);
    }
    //printf("file_close()\n");
    //sem_wait(&mutex_file_management);
    //printf("ejecuta file_close()\n");
    t_response respuesta;
    arg_h->socket_filesystem = crear_conexion(logger, ip_filesystem, puerto_filesystem);

    archivo = buscar_archivo(tabla_global_de_archivos, arg_h->nombre_archivo);
    if (archivo != NULL)
    {
        //printf("archivo != NULL (es %s)\n", archivo->nombre);
        //printf("Procesos bloqueados por lock = %i\n", archivo->cola_blocked->elements->elements_count);
        list_remove_by_condition(arg_h->execute->tabla_de_archivos_abiertos, es_el_archivo_local);

        arg_h->execute->estado = READY;
        
        //printf("archivo->cola_blocked->elements->elements_count = %i\n", archivo->cola_blocked->elements->elements_count);
        //printf("archivo->locks_lectura->elements_count = %i\n", archivo->locks_lectura->elements_count);
        if (archivo->lock == READ)
        {
            //printf("archivo->lock == READ\n");
            list_remove_by_condition(archivo->locks_lectura, es_el_proceso);
            archivo->contador_aperturas--;
            if (archivo->locks_lectura->elements_count == 0) // El proceso es el único que lo está leyendo
            {
                //printf("archivo->locks_lectura->elements_count == 0\n");
                if (archivo->cola_blocked->elements->elements_count != 0) // Hay elememtos bloqueados
                {
                    proceso_bloqueado = queue_pop(archivo->cola_blocked);
                    if (proceso_bloqueado != NULL)
                    {
                        archivo->lock = proceso_bloqueado->lock;
                        proceso_bloqueado->pcb->estado = READY;
                        archivo_local->archivo = archivo;
                        archivo_local->puntero = 0;

                        list_add(proceso_bloqueado->pcb->tabla_de_archivos_abiertos, archivo_local);
                        sem_wait(&mutex_cola_ready);
                        // queue_push(cola_ready, arg_h->execute);
                        queue_push(cola_ready, proceso_bloqueado->pcb);
                        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_bloqueado->pcb->pid);
                        //printf("EL PROCESO QUE DESBLOQUEO QUE TENÍA LOCK DE ESCRITURA ES EL %i\n", proceso_bloqueado->pcb->pid);
                        sem_post(&mutex_cola_ready);
                        sem_post(&procesos_en_ready);
                        archivo->contador_aperturas++;
                    }
                    //printf("TERMINA EL HILO PORQUE TODOS LOS BLOQUEADOS SON DE ESCRITURA\n");
                    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
                    {
                        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
                    }
                    liberar_parametros(arg_h);
                    //sem_post(&mutex_file_management);
                    return;
                }
                else // Se elimina el archivo completamente
                {
                    log_warning(logger, "Se elimina el archivo");
                    list_remove_by_condition(tabla_global_de_archivos, es_el_archivo);

                    liberar_conexion(arg_h->socket_filesystem);
                    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
                    {
                        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
                    }
                    //printf("TERMINA EL HILO PORQUE NO HAY NADA\n");
                    liberar_parametros(arg_h);
                    //sem_post(&mutex_file_management);
                    return;
                }
            }
            else
            {
                liberar_conexion(arg_h->socket_filesystem);
                liberar_parametros(arg_h);
                //sem_post(&mutex_file_management);
                return;
            }
        }
        else // CUANDO EL LOCK DEL ARCHIVO ES WRITE
        {
            if (archivo->cola_blocked->elements->elements_count > 0) // Hay elementos bloqueados (no se sabe si R o W)
            {
                archivo_local->archivo = archivo;
                archivo_local->puntero = 0;
                // sem_wait(&(archivo->mutex_cola_blocked));
                proceso_bloqueado = queue_pop(archivo->cola_blocked);
                // sem_post(&(archivo->mutex_cola_blocked));
                if (proceso_bloqueado != NULL)
                {
                    archivo->lock = proceso_bloqueado->lock;
                    list_add(proceso_bloqueado->pcb->tabla_de_archivos_abiertos, archivo_local);
                    //printf("EL PROCESO BLOQUEADO ES EL QUE TIENE PID = %i\n", proceso_bloqueado->pcb->pid);

                    if (proceso_bloqueado->lock == READ && archivo->cola_blocked->elements->elements_count > 0) // Hay que desbloquear otros procesos con READ
                    {
                        archivo_local->archivo = archivo;
                        archivo_local->puntero = 0;
                        t_list *procesos_lectura = list_filter(archivo->cola_blocked->elements, tiene_lock_lectura);
                        list_iterate(procesos_lectura, iterator_agregar_a_locks_lectura);
                        sem_wait(&mutex_cola_ready);
                        queue_push(cola_ready, proceso_bloqueado->pcb);
                        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_bloqueado->pcb->pid);
                        list_iterate(archivo->locks_lectura, iterator_agregar_a_ready);
                        list_add(archivo->locks_lectura, proceso_bloqueado->pcb);
                        sem_post(&procesos_en_ready);
                        sem_post(&mutex_cola_ready);
                        void *element;
                        do
                        {
                            element = list_remove_by_condition(archivo->cola_blocked->elements, tiene_lock_lectura);
                        } while (element != NULL);
                        //printf("TERMINA EL HILO PORQUE NO HAY NADA\n");
                        if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
                        {
                            enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
                        }
                        liberar_parametros(arg_h);
                        liberar_conexion(arg_h->socket_filesystem);
                        //sem_post(&mutex_file_management);
                        //printf("Después del fclose, hay %i archivos con lock de lectura, y %i bloqueados\n", archivo->locks_lectura->elements_count, archivo->cola_blocked->elements->elements_count);
                        return;
                    }
                    else // No se desbloquea nada (El lock es WRITE)
                    {
                        archivo_local->archivo = archivo;
                        archivo_local->puntero = 0;
                        sem_wait(&mutex_cola_ready);
                        // queue_push(cola_ready, arg_h->execute);
                        queue_push(cola_ready, proceso_bloqueado->pcb);
                        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", proceso_bloqueado->pcb->pid);
                        sem_post(&procesos_en_ready);
                        sem_post(&mutex_cola_ready);
                        list_add(proceso_bloqueado->pcb->tabla_de_archivos_abiertos, archivo_local);
                        //printf("TERMINA EL HILO PORQUE NO HAY NADA\n");
                        if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
                        {
                            enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
                        }
                        liberar_parametros(arg_h);
                        liberar_conexion(arg_h->socket_filesystem);
                        //sem_post(&mutex_file_management);
                        return;
                    }
                }
            }
            else
            {
                // Se elimina el archivo
                list_remove_element(tabla_global_de_archivos, archivo);
                log_warning(logger, "Se elimina el archivo");
            }
        }
        liberar_parametros(arg_h);
        liberar_conexion(arg_h->socket_filesystem);
        //sem_post(&mutex_file_management);
        return;
    }
    if(!strcmp(algoritmo_planificacion, "PRIORIDADES"))
    {
        enviar_operacion(arg_h->socket_interrupt, INTERRUPT);
    }
    liberar_parametros(arg_h);
    liberar_conexion(arg_h->socket_filesystem);
    // sem_wait(&mutex_cola_ready);
    // queue_push(cola_ready, arg_h->execute);
    // sem_post(&mutex_cola_ready);
    // sem_post(&procesos_en_ready);
    // //printf("TERMINA EL HILO\n");
    // //sem_post(&mutex_file_management);
}