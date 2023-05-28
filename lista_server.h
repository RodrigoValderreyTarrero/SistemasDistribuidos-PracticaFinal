//
// Created by rodrigo on 9/05/23.
//

#ifndef PRACTICAFINAL_SISTEMASDISTRIBUIDOS_LISTA_SERVER_H
#define PRACTICAFINAL_SISTEMASDISTRIBUIDOS_LISTA_SERVER_H

#include <string.h>
#include "stdio.h"
#include <stdlib.h>
#include <stdbool.h>

extern int mensaje_no_copiado;
extern pthread_cond_t cond_mensaje;
extern struct Lista_clientes almacen_clientes;
extern pthread_mutex_t mutex_mensaje;

struct Lista_mensajes {
    struct Nodo_mensajes* primero;
};

struct Nodo_mensajes{
    char mensaje[256];
    unsigned contador;
    char escritor[256];
    char receptor[256];
    struct Nodo_mensajes* siguiente;
};

struct cliente_servidor{
    char nombre[256];
    char alias[256];
    char fecha[256];
    int estado;
    char IP[256];
    int puerto;
    unsigned contador_user;
    unsigned pendientes;
    struct Lista_mensajes mensajes;
};

struct Lista_clientes {
    struct Nodo* primero;
};

struct Nodo {
    struct cliente_servidor* valor;
    struct Nodo* siguiente;
};

struct argumentos{
    struct cliente_servidor cliente;
    int sc;
};

struct argumentos_send{
    char escritor[256];
    char receptor[256];
    char mensaje[256];
    int sc;
};

struct leer_si{
    int leer_username;
    int leer_alias;
    int leer_fecha;
    int leer_ip;
    int leer_port;
    int leer_estado;
};

int efectuar_connect_users(struct argumentos *args);

int tomar_decision_mensaje(struct argumentos_send* args);

int vaciar_mensajes(struct Lista_clientes *almacen_clientes, char* alias);

int enviar_mensaje_conectado(struct Lista_clientes *almacen_clientes  ,struct Nodo_mensajes *mensaje);

int enviar_ack_mensaje(struct cliente_servidor cliente, unsigned contador);

struct cliente_servidor leer_cliente(int sc, struct leer_si lecturas);

void imprimir_clientes(struct Lista_clientes* lista);

int agregar_cliente(struct argumentos* args);

int eliminar_user(struct argumentos* args);

int conectar_user(struct argumentos* args);

int desconectar_user(struct argumentos *args);

int check_existe(struct Lista_clientes* lista, char * alias);

int check_conectado(struct Lista_clientes* lista, char * alias);

int agregar_mensaje_a_cliente(struct Lista_clientes *lista, struct Nodo_mensajes mensaje);


#endif //PRACTICAFINAL_SISTEMASDISTRIBUIDOS_LISTA_SERVER_H