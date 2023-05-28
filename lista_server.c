#include "lista_server.h"
#include <sys/socket.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void imprimir_clientes(struct Lista_clientes* lista) {
    //Función interesante para ver valores de la lista no tiene mas relevancia que eso
    struct Nodo* actual = lista->primero;
    //Inicio de lista
    printf("[");
    while (actual != NULL) {
        printf("(%s, %s, %s, %s, %d, %d, %u)",actual->valor->nombre,actual->valor->alias, actual->valor->fecha, actual->valor->IP, actual->valor->estado, actual->valor->puerto, actual->valor->contador_user);
        if (actual->siguiente != NULL) {
            printf(", ");
        }
        actual = actual->siguiente;
    }
    //Fin de lista
    printf("]\n");
}

int agregar_cliente(struct argumentos *args) {
    //Gracias a esta función se agregan clientes a la lista simplemente enlazada que actua de base de datos
    struct cliente_servidor cliente = args->cliente;
    struct Nodo* aux= almacen_clientes.primero;
    //Se recorre la lista hasta que se llegar al final
    while(aux !=NULL){
        //Si ya existe devolvemos el error correspondiente
        if(strcmp(cliente.alias, aux->valor->alias)==0){
            int res = 1;
            char result[2];
            sprintf(result,"%d", res);
            if (send(args->sc, result, 2, 0) == -1) {
                perror("Error al enviar datos");
                return -1;
            }
            return 1;
        }
        aux = aux->siguiente;
    }
    //Se crea una tupla con los datos a ingresar
    struct cliente_servidor* nueva_tupla = (struct cliente_servidor*) malloc(sizeof(struct cliente_servidor));
    //Copiamos el alias, nombre, fecha... en la estructura
    strncpy(nueva_tupla->alias, cliente.alias, 256);
    strncpy(nueva_tupla->nombre, cliente.nombre, 256);
    strncpy(nueva_tupla->fecha, cliente.fecha, 256);
    strncpy(nueva_tupla->IP, cliente.IP, 256);
    nueva_tupla->estado = cliente.estado;
    nueva_tupla->puerto = cliente.puerto;
    nueva_tupla->contador_user = 0;
    nueva_tupla->pendientes = 0;
    //Se crea un nuevo nodo de la lista y se añade
    struct Nodo* nuevo_nodo = (struct Nodo*) malloc(sizeof(struct Nodo));
    //Lo añadimos de forma segura
    pthread_mutex_lock(&mutex_mensaje);
    nuevo_nodo->valor = nueva_tupla;
    nuevo_nodo->siguiente = almacen_clientes.primero;
    almacen_clientes.primero = nuevo_nodo;
    pthread_mutex_unlock(&mutex_mensaje);
    //Fin de sección crítica
    int sc = args->sc;
    int res = 0;
    char result[2];
    //Se envia el resultado
    sprintf(result,"%d", res);
    if (send(sc, result, 2, 0) == -1) {
        perror("Error al enviar datos");
        return -1;
    }
    //Acabamos con el hilo
    mensaje_no_copiado = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_exit(0);
}

int eliminar_user(struct argumentos *args) {
    //Busca una clave en específico y con la referencia del nodo anterior libera la memoria del nodo y
    //gestiona la lista internamente
    int res;
    char alias[256];
    strcpy(alias,args->cliente.alias);
    struct Nodo* actual = almacen_clientes.primero;
    struct Nodo* anterior = NULL;
    //Se recorre la lista
    while (actual != NULL) {
        //Si se encuentra el nodo modificamos la base de datos de forma segura elimianndo el nodo
        if (strcmp(alias, actual->valor->alias)==0) {
            pthread_mutex_lock(&mutex_mensaje);
            if (anterior == NULL) {  // Si el nodo a eliminar es el primero de la lista
                almacen_clientes.primero = actual->siguiente;
            } else {
                anterior->siguiente = actual->siguiente;
            }
            //Libreamos la memoria
            free(actual->valor);
            free(actual);
            res = 0;
            //Se envia el error
            char result[11];
            sprintf(result,"%d", res);
            if (send(args->sc, result, 11, 0) == -1) {
                perror("Error al enviar datos");
                return -1;
            }
            mensaje_no_copiado = false;
            pthread_cond_signal(&cond_mensaje);
            pthread_mutex_unlock(&mutex_mensaje);
            pthread_exit(0);
        }
        //Se avanza en la lista para encontrar el nodo buscado
        anterior = actual;
        actual = actual->siguiente;
    }
    //Si nos encontramos aqui no existia
    res = 1;
    char result[11];
    sprintf(result,"%d", res);
    if (send(args->sc, result, 11, 0) == -1) {
        perror("Error al enviar datos");
        return -1;
    }
    //Fin del hilo
    mensaje_no_copiado = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_exit(0);
}

int conectar_user(struct argumentos* args){
    //Modifica un el estado a conectado
    //Copiamos los valores del parámetro
    char alias[256];
    char ip[256];
    int port;
    port = args->cliente.puerto;
    strcpy(ip,args->cliente.IP);
    strcpy(alias,args->cliente.alias);
    //Buscamos al usuario y modificamos su estado a conectado
    int res=1;
    struct Nodo* actual = almacen_clientes.primero;
    //Recorre la lista
    while (actual != NULL) {
        //Si coincide la clave, evaluamos:
        if (strcmp(actual->valor->alias,alias) ==0) {
            //Se modifica
            pthread_mutex_lock(&mutex_mensaje);
            //Si está desconectado lo conectamos
            if(actual->valor->estado==0){
                actual->valor->estado= 1; //Se conecta
                strcpy(actual->valor->IP, ip);
                actual->valor->puerto = port;
                res = 0;
            } //Está desconectado, asi que el error es 2
            else{
                res = 2;
            }
            pthread_mutex_unlock(&mutex_mensaje);
        }
        //Se avanza
        actual = actual->siguiente;
    }
    //Enviamos el error
    char result[11];
    sprintf(result, "%d", res);
    if (send(args->sc, result, 11, 0) == -1) {
        perror("Error al enviar datos");
        return -1;
    }
    //Se vacian los mensajes.
    vaciar_mensajes(&almacen_clientes,alias);
    mensaje_no_copiado = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_exit(0);
}

int desconectar_user(struct argumentos *args){
    //Buscamos un user y lo desconectamos
    struct Nodo* actual = almacen_clientes.primero;

    int res;
    int sc = args->sc;
    char alias[256];
    strcpy(alias, args->cliente.alias);
    //Recorre la lista
    while (actual != NULL) {
        //Si coincide la clave
        if (strcmp(actual->valor->alias,alias) ==0) {
            //Se modifica
            pthread_mutex_lock(&mutex_mensaje);
            if(actual->valor->estado==1){
                actual->valor->estado= 0; //Se desconecta
                strcpy(actual->valor->IP,"");
                actual->valor->puerto = 0;
                res = 0;

                char result[11];
                sprintf(result,"%d", res);
                if (send(sc, result, 11, 0) == -1) {
                    perror("Error al enviar datos");
                    return -1;
                }
                mensaje_no_copiado = false;
                pthread_cond_signal(&cond_mensaje);
                pthread_mutex_unlock(&mutex_mensaje);
                pthread_exit(0);

            } //Está desconectado
            else{
                res = 2;
                char result[11];
                sprintf(result,"%d", res);
                if (send(sc, result, 11, 0) == -1) {
                    perror("Error al enviar datos");
                    return -1;
                }

                mensaje_no_copiado = false;
                pthread_cond_signal(&cond_mensaje);
                pthread_mutex_unlock(&mutex_mensaje);
                pthread_exit(0);
            }

        }
        actual = actual->siguiente;
    }
    res = 1;
    char result[11];
    sprintf(result,"%d", res);
    if (send(sc, result, 11, 0) == -1) {
        perror("Error al enviar datos");
        return -1;
    }
    mensaje_no_copiado = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_exit(0);
}

int check_existe(struct Lista_clientes* lista, char * alias){
    //Se comprueba si un usuario está conectado devuelve 0 si lo está y -1 en otro caso
    struct Nodo* actual = lista->primero;
    //Recorre la lista
    while (actual != NULL) {
        //Si coincide
        if (strcmp(actual->valor->alias,alias) ==0) {
            return 0;
        }
        actual = actual->siguiente;
    }
    return -1;
}

int check_conectado(struct Lista_clientes* lista, char * alias){
    //Se comprueba si un usuario está conectado devuelve 0 si lo está y -1 en otro caso
    struct Nodo* actual = lista->primero;
    //Recorre la lista
    while (actual != NULL) {
        //Si coincide
        if (strcmp(actual->valor->alias,alias) ==0) {
            if(actual->valor->estado == 1){
                //Está conectado
                return 0;
            }
            else{
                //NO lo está
                return -1;
            }
        }
        actual = actual->siguiente;
    }
    //No lo está porque ni está registrado
    return -1;
}

int agregar_mensaje_a_cliente(struct Lista_clientes *lista, struct Nodo_mensajes mensaje){
    struct Nodo* aux = lista->primero;
    //Se recorre la lista hasta que se llegar al final
    while(aux != NULL){
        //Si ya existe la clave seleccionada
        if (strcmp(aux->valor->alias, mensaje.receptor) == 0){
            pthread_mutex_lock(&mutex_mensaje);
            //Entramos en la lista de mensajes del cliente encontrado
            struct Nodo_mensajes* listaMensajes = aux->valor->mensajes.primero;
            if (listaMensajes == NULL) {
                //Si la lista está vacía, se crea el primer nodo
                listaMensajes = (struct Nodo_mensajes*) malloc(sizeof(struct Nodo_mensajes));
                strncpy(listaMensajes->mensaje, mensaje.mensaje, 256);
                strncpy(listaMensajes->escritor, mensaje.escritor, 256);
                strncpy(listaMensajes->receptor, mensaje.receptor, 256);
                listaMensajes->contador = mensaje.contador;
                listaMensajes->siguiente = NULL;
                aux->valor->mensajes.primero = listaMensajes;
            } else {
                //Si no está vacía, se recorre la lista hasta el final
                while (listaMensajes->siguiente != NULL) {
                    listaMensajes = listaMensajes->siguiente;
                }
                //Se crea un nuevo nodo con los datos a ingresar
                struct Nodo_mensajes* nueva_tupla = (struct Nodo_mensajes*) malloc(sizeof(struct Nodo_mensajes));
                strncpy(nueva_tupla->mensaje, mensaje.mensaje, 256);
                strncpy(nueva_tupla->escritor, mensaje.escritor, 256);
                strncpy(nueva_tupla->receptor, mensaje.receptor, 256);
                nueva_tupla->contador = mensaje.contador;
                nueva_tupla->siguiente = NULL;
                //Se agrega al final de la lista
                listaMensajes->siguiente = nueva_tupla;
            }
            pthread_mutex_unlock(&mutex_mensaje);
            return 0;
        }
        aux = aux->siguiente;
    }
    return 1; //Si no se encontró el cliente
}

struct cliente_servidor leer_cliente(int sc, struct leer_si lecturas) {
    //Gracias a esta función se obtiene el nombre, alias ... (Lo que se pida leer
    //en la estructura leer si)
    struct cliente_servidor client;
    int n;
    //Leemos el nombre
    char nombre[256];
    if(lecturas.leer_username == 1){
        if ((n = recv(sc, nombre, 256, 0)) == -1) { //Recibe el nombre
            printf("Error en ReadLine");
            close(sc);
        }
        strcpy(client.nombre, nombre);
    }
    //Leemos el alias
    char alias[256];
    if(lecturas.leer_alias == 1){
        if ((n = recv(sc, alias, 256, 0)) == -1) { //Recibe el nombre
            printf("Error en ReadLine");
            close(sc);
        }
        strcpy(client.alias, alias);
    }
    char fecha[256];
    if(lecturas.leer_fecha == 1){
        if ((n = recv(sc, fecha, 256, 0)) == -1) { //Recibe el nombre
            printf("Error en ReadLine");
            close(sc);
        }
        strcpy(client.fecha, fecha);
    }
    //Leemos el estado
    char estado[256];
    if(lecturas.leer_estado == 1){
        if ((n = recv(sc, estado, 256, 0)) == -1) { //Recibe el nombre
            printf("Error en ReadLine");
            close(sc);
        }
        client.estado= atoi(estado);
    }
    else{
        client.estado= 0;
    }
    //Leemos la ip (realmente nunca se usa de cara a esta práctica)
    char ip[256];
    if(lecturas.leer_ip == 1){
        if ((n = recv(sc, ip, 256, 0)) == -1) { //Recibe el nombre
            printf("Error en ReadLine");
            close(sc);
        }
        strcpy(client.IP, ip);
    }
    //Leemos el puerto
    char port[256];
    if(lecturas.leer_port == 1){
        if ((n = recv(sc, port, 256, 0)) == -1) { //Recibe el nombre
            printf("Error en ReadLine");
            close(sc);
        }
        client.puerto= atoi(port);
    }
    else{
        client.puerto = 0;
    }
    //Devolvemos el cliente resultante
    return client;
}

int enviar_ack_mensaje(struct cliente_servidor cliente, unsigned contador){
    //Mediante esta función se envia un ack de confirmación al remitente de un mensaje

    //Abrimos el socket
    int sd;
    struct sockaddr_in serv_addr;
    sd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(cliente.IP);
    serv_addr.sin_port = htons(cliente.puerto);
    //Lo conectamos
    connect(sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    //Enviamos el mensaje del ack y el id del mensaje
    char ack[] = "SEND_MESS_ACK\0";
    if (send(sd, ack, 14, 0) == -1) {
        perror("Error al enviar datos");
        return -1;
    }
    char cont[12];
    snprintf(cont, sizeof(cont), "%011u", contador);
    if (send(sd, cont, 12, 0) == -1) {
        perror("Error al enviar datos");
        return -1;
    }
    close(sd);
    return 0;
}

int enviar_mensaje_conectado(struct Lista_clientes *almacen_clientes  ,struct Nodo_mensajes *mensaje){
    //Esta función permite enviar un mensaje a un receptor conectado

    //Esta función envía todos los mesnajes, ya sea llamada desde el vaciado del almacen o nada más se envia
    struct Nodo* actual = almacen_clientes->primero;
    char ip_enviar[256];
    char puerto_enviar[256];
    int puerto;
    int res = 0;
    //Recorre la lista para extraer los datos del receptor
    while (actual != NULL) {
        //Si coincide
        if (strcmp(actual->valor->alias,mensaje->receptor) ==0) {
            strcpy(ip_enviar, actual->valor->IP);
            puerto =  actual->valor->puerto;
        }
        actual = actual->siguiente;
    }
    struct cliente_servidor cliente_escritor;
    struct Nodo* actual2 = almacen_clientes->primero;
    //Se recorre la lista y cuando se encuntran los datos del escritor que se requieren o se modifican se hace lo que corresponda
    //Siempre de forma segura
    while (actual2 != NULL) {
        //Si coincide
        if (strcmp(actual2->valor->alias,mensaje->escritor) ==0) {
            pthread_mutex_lock(&mutex_mensaje);
            actual2->valor->pendientes = 0;
            cliente_escritor.puerto = actual2->valor->puerto;
            strcpy(cliente_escritor.IP, actual2->valor->IP);
            actual2->valor->contador_user ++;
            cliente_escritor.contador_user = actual2->valor->contador_user;
            pthread_mutex_unlock(&mutex_mensaje);
        }
        actual2 = actual2->siguiente;
    }
    //Creamos el socket
    sprintf(puerto_enviar, "%d", puerto);
    int sd;
    struct sockaddr_in serv_addr;
    sd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip_enviar);
    serv_addr.sin_port = htons(puerto);
    //LO conectamos
    connect(sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    //Enviamos los cuatro campos pedidos (menseje de 'SEND_MESSAGE', escritor, mesanje e id).
    char operacion[] = "SEND_MESSAGE\0\0";
    if (send(sd, operacion, 14, 0) == -1) {
        perror("Error al enviar datos");
        res = 2;
    }
    //Los campos se envian con padding para rellenar a la cantidad esperada. Siempre se reserva por exceso
    //Envio del escritor
    char escritor[256];
    memset(escritor, 0, sizeof(escritor)); // Inicializa el arreglo con ceros
    strcpy(escritor, mensaje->escritor);
    escritor[strlen(mensaje->escritor)] = '\0';

    if (strlen(escritor) < sizeof(escritor)) {
        memset(escritor + strlen(escritor), 0, sizeof(escritor) - strlen(escritor)); // Agrega ceros al final del mensaje
    }

    if (send(sd, escritor, sizeof(escritor), 0) == -1) {
        perror("Error al enviar datos");
        res = 2;
    }
    //Fin del envio del escritor

    //Envio de contador (id del mensaje)
    mensaje->contador = cliente_escritor.contador_user;

    char contador[11];
    memset(contador, 0, sizeof(contador)); // Inicializa el arreglo con ceros
    sprintf(contador, "%u", mensaje->contador);
    contador[strlen(contador)] = '\0';

    if (strlen(contador) < sizeof(contador)) {
        memset(contador + strlen(contador), 0, sizeof(contador) - strlen(contador)); // Agrega ceros al final del mensaje
    }

    if (send(sd, contador, sizeof(contador), 0) == -1) {
        perror("Error al enviar datos");
        res = 2;
    }

    //FIn de contador (id) del mensaje

    //Envío de mensaje
    char mensaje_enviar[256];
    memset(mensaje_enviar, 0, sizeof(mensaje_enviar)); // Inicializa el arreglo con ceros
    strcpy(mensaje_enviar, mensaje->mensaje);
    mensaje_enviar[strlen(mensaje->mensaje)] = '\0';

    if (strlen(mensaje_enviar) < sizeof(mensaje_enviar)) {
        memset(mensaje_enviar + strlen(mensaje_enviar), 0, sizeof(mensaje_enviar) - strlen(mensaje_enviar)); // Agrega ceros al final del mensaje
    }
    if (send(sd, mensaje_enviar, sizeof(mensaje_enviar), 0) == -1) {
        perror("Error al enviar datos");
        res = 2;
    }
    if(check_conectado(almacen_clientes, mensaje->escritor)==0){
        enviar_ack_mensaje(cliente_escritor, mensaje->contador);
    }
    close(sd);
    //FIn del envio del mensaje

    return res;
}


int vaciar_mensajes(struct Lista_clientes *almacen_clientes, char* alias) {
    // Con esta función se vacía el almacén de mensajes de un cliente y se envían al conectarse
    if (check_conectado(almacen_clientes, alias) == -1) {
        return 1;
    }
    //Recorremos la lista para enviar mensaje a mensaje y liberamos la memoria
    struct Nodo* actual = almacen_clientes->primero;
    while (actual != NULL) {
        //Hasta que hayamos acabado la lista comprobamos si se ha desconectado durante la recepción para detener el vaciado
        if (strcmp(actual->valor->alias, alias) == 0) {
            struct Lista_mensajes* lista = &(actual->valor->mensajes);
            while (lista->primero != NULL) {
                if (check_conectado(almacen_clientes, alias) == -1) {
                    return 1;
                }
                // Crea un nuevo nodo para enviar el mensaje
                struct Nodo_mensajes* nodoMensajes = malloc(sizeof(struct Nodo_mensajes));
                strcpy(nodoMensajes->mensaje, lista->primero->mensaje);
                strcpy(nodoMensajes->escritor, lista->primero->escritor);
                nodoMensajes->contador = lista->primero->contador;
                strcpy(nodoMensajes->receptor, lista->primero->receptor);
                nodoMensajes->siguiente = NULL;
                //Se envía el mensaje
                enviar_mensaje_conectado(almacen_clientes, nodoMensajes);

                // Borra el primer mensaje de la lista y libera su memoria
                struct Nodo_mensajes* temp = lista->primero;
                lista->primero = temp->siguiente;
                pthread_mutex_lock(&mutex_mensaje);
                free(temp);
                pthread_mutex_unlock(&mutex_mensaje);

            }
            // Libera la memoria de los nodos restantes en la lista (si los hay)
            struct Nodo_mensajes* nodo_actual = lista->primero;
            while (nodo_actual != NULL) {
                struct Nodo_mensajes* nodo_temp = nodo_actual;
                nodo_actual = nodo_actual->siguiente;
                free(nodo_temp);
            }
        }
        //Avanzamos en la lista
        actual = actual->siguiente;
    }
    // Devuelve 0 si ha ido bien
    return 0;
}


int tomar_decision_mensaje(struct argumentos_send* args){
    //Mediante esta función se elige si se debe guardar un mensaje o enviar en el momento
    int res;
    char escritor[256];
    char receptor[256];
    char mensaje[256];
    //Copiamos la información
    strcpy(escritor, args->escritor);
    strcpy(receptor, args->receptor);
    strcpy(mensaje, args->mensaje);
    int sc = args->sc;
    //Si alguno de los dos no existe, devolvemos un error instantaneamente
    int existe_escritor = check_existe(&almacen_clientes, escritor);
    int existe_repector = check_existe(&almacen_clientes, receptor);
    //Error
    if(existe_escritor == -1 || existe_repector == -1){
        res = 1;
    }
    //Si no está conectado el escritor no puede mandar mensajes y si no está coenctado el escritor se almacenan
    int conectado_escritor = check_conectado(&almacen_clientes, escritor);
    int conectado_receptor = check_conectado(&almacen_clientes, receptor);

    if(conectado_escritor== -1){
        //Si no estas conectado no puedes enviar mensajes
        res = 2;
    }
    struct Nodo_mensajes mensaje_enviar;
    strcpy(mensaje_enviar.escritor, escritor);
    strcpy(mensaje_enviar.receptor, receptor);
    strcpy(mensaje_enviar.mensaje, mensaje);
    mensaje_enviar.contador = 0;
    //Se envia al momento
    if(conectado_escritor == 0 && conectado_receptor ==0){
        //Ambos están conectados. Se envía en el momento
        res = enviar_mensaje_conectado(&almacen_clientes ,&mensaje_enviar);
    }
    //Guardar
    if(conectado_escritor == 0 && conectado_receptor ==-1){
        //El receptor no está conectado. Se debe guardar el mensaje
        res = agregar_mensaje_a_cliente(&almacen_clientes,mensaje_enviar);
    }
    //Bloqueamos con lock para agragar el mensaje pendiente
    pthread_mutex_lock(&mutex_mensaje);
    struct cliente_servidor cliente_escritor;
    struct Nodo* actual2 = almacen_clientes.primero;
    while (actual2 != NULL) {
        //Si coincide
        if (strcmp(actual2->valor->alias,escritor) ==0) {
            cliente_escritor.contador_user = actual2->valor->contador_user;
            if(conectado_escritor == 0 && conectado_receptor ==-1){
                //Si se ha agregado el mensaje
                actual2->valor->pendientes++;
            }
            cliente_escritor.pendientes = actual2->valor->pendientes;
        }
        actual2 = actual2->siguiente;
    }
    pthread_mutex_unlock(&mutex_mensaje);
    // Aumentamos el tamaño del buffer para incluir el carácter nulo
    char result[2];
    sprintf(result, "%d", res);
    // Establecer el último byte en '\0'
    result[sizeof(result) - 1] = '\0';
    if (send(sc, result, sizeof(result), 0) == -1) {
        perror("Error al enviar datos");
        return -1;
    }
    //Enviamos el id o contador
    char id[11];
    snprintf(id, sizeof(id), "%010u", cliente_escritor.contador_user+cliente_escritor.pendientes);
    if (send(sc, id, sizeof(id), 0) == -1) {
        perror("Error al enviar datos");
        return -1;
    }
    mensaje_no_copiado = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_exit(0);
}


int efectuar_connect_users(struct argumentos *args){
    //Esta función lleva a cabo la recopilación de usuarios conectados. No se bloquea ni nada porque solo lee
    struct cliente_servidor client_leido = args->cliente;
    int sc = args->sc;
    int res;
    //EL usuario tiene que existir
    if (check_existe(&almacen_clientes, client_leido.alias) == -1) {
        res = 2;
        char result[11];
        sprintf(result, "%d", res);
        if (send(sc, result, 11, 0) == -1) {
            perror("Error al enviar datos");
            return -1;
        }
    }

    //Si no está conectado error 1. Fin de la operacion
    if (check_conectado(&almacen_clientes, client_leido.alias) == -1) {
        res = 1;
        char result[11];
        sprintf(result, "%d", res);
        if (send(sc, result, 11, 0) == -1) {
            perror("Error al enviar datos");
            return -1;
        }
    }
    //Si está coenctado se procede
    if (check_conectado(&almacen_clientes, client_leido.alias) == 0) {
        res = 0;
        char result[11];
        sprintf(result, "%d", res);
        if (send(sc, result, 1, 0) == -1) {
            perror("Error al enviar datos");
            return -1;
        }
        int contador=0;
        //Contamos los usuarios conectados
        struct Nodo* actual = almacen_clientes.primero;
        //Recorre la lista
        while (actual != NULL) {
            //Si esta conectado
            if(actual->valor->estado == 1){
                contador++;
            }
            actual = actual->siguiente;
        }
        // 11 caracteres para el número, 1 para el terminador nulo y 1 para el padding
        char char_ct[13];
        // Agrega ceros a la izquierda
        snprintf(char_ct, sizeof(char_ct), "%012d", contador);
        char_ct[12] = '\0';

        if (send(sc, char_ct, sizeof(char_ct), 0) == -1) {
            perror("Error al enviar datos");
            return -1;
        }
        //Para cada conectado enviamos un mensaje, hay un bucle que sabe cuanto debe escuchar gracias al send de antes (En el cliente)
        struct Nodo* actual2 = almacen_clientes.primero;
        //Recorre la lista
        while (actual2 != NULL) {
            //Si esta conectado
            if(actual2->valor->estado == 1){
                char alias_padded[256];
                memset(alias_padded, '\0', 256);
                strncpy(alias_padded, actual2->valor->alias, strlen(actual2->valor->alias));
                if (send(sc, alias_padded, 256, 0) == -1) {
                    perror("Error al enviar datos");
                    return -1;
                }
            }
            //avanzamos
            actual2 = actual2->siguiente;
        }
    }
    //fin del hilo
    mensaje_no_copiado = false;
    pthread_cond_signal(&cond_mensaje);
    pthread_exit(0);
}