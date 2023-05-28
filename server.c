//
// Este programa se encarga de la funcionalidad de recibir y responder las peticiones de los clientes.
//
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include "lista_server.h"
#include <stdbool.h>
#include <pthread.h>
#define MAXSIZE 256
struct Lista_clientes almacen_clientes;

//Variables globales relacionadas con concurrencia
pthread_mutex_t mutex_mensaje;
int mensaje_no_copiado = true;
pthread_cond_t cond_mensaje = PTHREAD_COND_INITIALIZER;
pthread_t thid;

//puerto en el que se escuchará
int portno;
int sd;


int tratar_mensaje(int sc){
    //Función que recoge la información de un mensaje y crea el hilo que decidirá que hacer con él
    pthread_attr_t t_attr;
    pthread_attr_init(&t_attr);
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

    char escritor[256];
    char receptor[256];
    char mensaje[256];
    int n;
    if ((n = recv(sc, escritor, 256, 0)) == -1) { //Recibe el nombre
        printf("Error en ReadLine");
        close(sc);
    }
    if ((n = recv(sc, receptor, 256, 0)) == -1) { //Recibe el nombre
        printf("Error en ReadLine");
        close(sc);
    }
    if ((n = recv(sc, mensaje, 256, 0)) == -1) { //Recibe el nombre
        printf("Error en ReadLine");
        close(sc);
    }
    //Creamos el argumento que hay que pasar al hilo
    struct argumentos_send args;
    args.sc = sc;
    strcpy(args.escritor, escritor);
    strcpy(args.mensaje, mensaje);
    strcpy(args.receptor, receptor);

    //Se crea el hilo
    if(pthread_create(&thid, &t_attr, (void*) tomar_decision_mensaje, (void*)&args)<0){
        perror("Error al crear el hilo");
    }
    else{
        //Da tiempo a que el mensaje se copie para evitar condiciones de carrera
        pthread_mutex_lock(&mutex_mensaje);
        while(mensaje_no_copiado){
            pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
        }
        mensaje_no_copiado = true;
        pthread_mutex_unlock(&mutex_mensaje);
    }

    //Ya se ha recibido
    return 0;
}

int conectar(int sc, char* ip) {
    //FUnción que cambia de 0 a 1 el estado de un cliente desconectad ->conectado
    pthread_attr_t t_attr;
    pthread_attr_init(&t_attr);
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);
    //Se lee la información
    struct cliente_servidor client_leido;
    struct leer_si lecturas;
    lecturas.leer_alias=1;
    lecturas.leer_username=0;
    lecturas.leer_fecha=0;
    lecturas.leer_port=1;
    lecturas.leer_estado=0;
    lecturas.leer_ip=0;
    client_leido = leer_cliente(sc, lecturas);
    strcpy(client_leido.IP, ip);
    struct argumentos args;
    args.cliente = client_leido;
    args.sc = sc;
    //Crea el hilo que conecta al usuario en cuestion
    if(pthread_create(&thid, &t_attr, (void*) conectar_user, (void*)&args)<0){
        perror("Error al crear el hilo");
    }
    else{
        //Da tiempo a que el mensaje se copie para evitar condiciones de carrera
        pthread_mutex_lock(&mutex_mensaje);
        while(mensaje_no_copiado){
            pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
        }
        mensaje_no_copiado = true;
        pthread_mutex_unlock(&mutex_mensaje);
    }
    //Imprimimos el estado de la base de datos por si se quiere consultar
    imprimir_clientes(&almacen_clientes);
    return 0;
}

int eliminar_registro(int sc){
    //Función que borra un usuario concreto de la BBDD
    pthread_attr_t t_attr;
    pthread_attr_init(&t_attr);
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

    struct cliente_servidor client_leido;
    struct leer_si lecturas;
    lecturas.leer_alias=1;
    lecturas.leer_username=0;
    lecturas.leer_fecha=0;
    lecturas.leer_port=0;
    lecturas.leer_estado=0;
    lecturas.leer_ip=0;
    client_leido = leer_cliente(sc, lecturas);
    struct argumentos args;
    args.cliente = client_leido;
    args.sc = sc;
    //Crea el hilo
    if(pthread_create(&thid, &t_attr, (void*)eliminar_user, (void*)&args)<0){
        perror("Error al crear el hilo");
    }
    else{
        //Da tiempo a que el mensaje se copie para evitar condiciones de carrera
        pthread_mutex_lock(&mutex_mensaje);
        while(mensaje_no_copiado){
            pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
        }
        mensaje_no_copiado = true;
        pthread_mutex_unlock(&mutex_mensaje);
    }
    //se imprime la BBDD
    imprimir_clientes(&almacen_clientes);
    return 0;
}

int registrar(int sc){
    //Crea el hilo que agrega a un usuario
    pthread_attr_t t_attr;
    pthread_attr_init(&t_attr);
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

    struct cliente_servidor client_leido;
    struct leer_si lecturas;
    lecturas.leer_fecha=1;
    lecturas.leer_username=1;
    lecturas.leer_alias=1;
    lecturas.leer_port=0;
    lecturas.leer_estado=0;
    lecturas.leer_ip=0;
    client_leido = leer_cliente(sc, lecturas);
    //La estructura que pasamos al hilo

    struct argumentos args;
    args.cliente = client_leido;
    args.sc = sc;
    //Crea el hilo
    if(pthread_create(&thid, &t_attr, (void*)agregar_cliente, (void*)&args)<0){
            perror("Error al crear el hilo");
        }
    else{
        //Da tiempo a que el mensaje se copie para evitar condiciones de carrera
        pthread_mutex_lock(&mutex_mensaje);
        while(mensaje_no_copiado){
            pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
        }
        mensaje_no_copiado = true;
        pthread_mutex_unlock(&mutex_mensaje);
    }

    imprimir_clientes(&almacen_clientes);
    return 0;
}

int desconectar(int sc){
    //Crea el hilo que desconecta un usuario estado: coenctado -> desconectado
    pthread_attr_t t_attr;
    pthread_attr_init(&t_attr);
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);
    //Leemos el cliente
    struct cliente_servidor client_leido;
    struct leer_si lecturas;
    lecturas.leer_alias=1;
    lecturas.leer_username=0;
    lecturas.leer_fecha=0;
    lecturas.leer_port=0;
    lecturas.leer_estado=0;
    lecturas.leer_ip=0;
    client_leido = leer_cliente(sc, lecturas);

    struct argumentos args;
    args.cliente = client_leido;
    args.sc = sc;

    //Se crea el hilo
    if(pthread_create(&thid, &t_attr, (void*) desconectar_user, (void*)&args)<0){
        perror("Error al crear el hilo");
    }
    else{
        //Da tiempo a que el mensaje se copie para evitar condiciones de carrera
        pthread_mutex_lock(&mutex_mensaje);
        while(mensaje_no_copiado){
            pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
        }
        mensaje_no_copiado = true;
        pthread_mutex_unlock(&mutex_mensaje);
    }
    //Imprime el cliente
    imprimir_clientes(&almacen_clientes);
    return 0;
}

int lista_users(int sc) {

    pthread_attr_t t_attr;
    pthread_attr_init(&t_attr);
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);
    //Leemos la información que queremos
    struct cliente_servidor client_leido;
    struct leer_si lecturas;
    lecturas.leer_alias = 1;
    lecturas.leer_username = 0;
    lecturas.leer_fecha = 0;
    lecturas.leer_port = 0;
    lecturas.leer_estado = 0;
    lecturas.leer_ip = 0;
    client_leido = leer_cliente(sc, lecturas);
    struct argumentos args;
    args.cliente = client_leido;
    args.sc = sc;
    //Creamos el hilo para enviar toda la información
    if(pthread_create(&thid, &t_attr, (void*) efectuar_connect_users, (void*)&args)<0){
        perror("Error al crear el hilo");
    }
    else{
        //Da tiempo a que el mensaje se copie para evitar condiciones de carrera
        pthread_mutex_lock(&mutex_mensaje);
        while(mensaje_no_copiado){
            pthread_cond_wait(&cond_mensaje, &mutex_mensaje);
        }
        mensaje_no_copiado = true;
        pthread_mutex_unlock(&mutex_mensaje);
    }
    return 0;

}

int chechkargumentos(int argc,char* argv[]){
    // Comprobar los argumentos necesarios
    if (argc < 3) {
        printf("Uso: %s -p <port>\n", argv[0]);
        exit(1);
    }
    return 0;
}

void controlador_senal() {
    //Para cerrar el socket cuando se acabe con la ejecución
    printf("\nCerrando servidor...\n");
    close(sd);
    exit(0);
}

int main(int argc, char *argv[]) {
    //Checkeamos que los argumentos son corrcetos
    chechkargumentos(argc,argv);
    //Se crean variables para el socket
    struct sockaddr_in server_addr, client_addr;
    int val;
    struct hostent *hp;
    // Registrar el manejador de la señal para control + C
    signal(SIGINT, controlador_senal);
    //Creamos el socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(1);
    }
    // Obtener el puerto de la línea de comandos
    portno = atoi(argv[2]);
    bzero((char *)&server_addr, sizeof(server_addr));
    hp = gethostbyname ("localhost");
    if (hp == NULL) {
        perror("Error en gethostbyname");
        exit(1);
    }
    // Configurar opciones del socket
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(int));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portno);
    //Imprimimos iformación referente a la conexión, por si fuese de interés
    printf("s> init server %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    // Enlazar socket a dirección y puerto
    if (bind(sd, (const struct sockaddr *) &server_addr, sizeof(server_addr))<0) {
        perror("bind failed");
        exit(0);
    }
    //Se pone a escuchar
    if (listen(sd, SOMAXCONN) != 0) {
        perror("Error in listen");
        exit(0);
    }
    //Inicializamos variables necesarias y relacionadas con hilos
    int size;
    int sc;
    size = sizeof(client_addr);
    pthread_mutex_init(&mutex_mensaje, NULL);
    pthread_cond_init(&cond_mensaje, NULL);

    //Se pone a escuchar
    while(1) {
        // Imprimir mensaje de espera de comandos
        printf("s> ");
        fflush(0);
        sc = accept(sd, (struct sockaddr *) &client_addr, (socklen_t *) &size);
        if (sc == -1) {
            printf("Error en accept\n");
            return -1;
        }
        //Imprimimos la infromación
        printf("conexión aceptada de IP: %s   Puerto: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        char operacion[256];
        //Recibimosla operación
        int recv_error = recv(sc, operacion, 256, 0);
        if (recv_error == -1) { //Recibe la operación
            printf("Error en ReadLine\n");
            close(sc);
            continue;
        }
        //En función de lo recibidido se lanza una función u otra que creará el hilo correspondiente.
        if(strcmp(operacion,"REGISTER")==0){
            registrar(sc);
        }
        else if(strcmp(operacion,"UNREGISTER")==0){
            eliminar_registro(sc);
        }
        else if(strcmp(operacion,"CONNECT")==0){
            char *client_ip = inet_ntoa(client_addr.sin_addr);
            conectar(sc, client_ip);
        }
        else if(strcmp(operacion,"DISCONNECT")==0){
            desconectar(sc);
        }
        else if(strcmp(operacion,"SEND")==0){
            tratar_mensaje(sc);
        }
        else if(strcmp(operacion,"CONNECTEDUSERS")==0){
            lista_users(sc);
        }
        //Cerramos el socket
        close(sc);
    }
    return 0;
}
