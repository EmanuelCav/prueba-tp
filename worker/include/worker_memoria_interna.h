#ifndef WORKER_MEMORIA_INTERNA_H
#define WORKER_MEMORIA_INTERNA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

typedef struct {
    int numero_pagina;
    char file[32];
    char tag[32];
    void* marco;
    bool modificada;
    bool ocupada;
    int frame;
    int uso;
} t_pagina;

typedef struct {
    int cant_marcos;
    int tamanio_pagina;
    void* memoria;
    t_pagina* marcos;
} t_memoria_interna;

/**
 * @brief Crea e inicializa la memoria interna del Worker.
 *
 * @param tam_memoria Tamaño total de la memoria interna en bytes.
 * @param tamanio_pagina Tamaño de cada página en bytes.
 * @return t_memoria_interna* Puntero a la memoria interna creada.
 */
t_memoria_interna* memoria_crear(int tam_memoria, int tamanio_pagina);

/**
 * @brief Busca un marco libre dentro de la memoria interna.
 *
 * @param mem Puntero a la memoria interna.
 * @return int Índice del marco libre o -1 si no hay marcos disponibles.
 */
int buscar_marco_libre(t_memoria_interna* mem);

/**
 * @brief Asigna una página a un marco de memoria.
 *
 * @param mem Puntero a la memoria interna.
 * @param marco Índice del marco a asignar.
 * @param numero_pagina Número de página a asignar.
 * @param file Nombre del File.
 * @param tag Tag del File.
 */
void asignar_pagina(t_memoria_interna* mem, int marco, int numero_pagina, char* file, char* tag);

/**
 * @brief Libera un marco de la memoria interna.
 *
 * @param mem Puntero a la memoria interna.
 * @param marco Índice del marco a liberar.
 */
void liberar_marco(t_memoria_interna* mem, int marco);

/**
 * @brief Selecciona una página víctima utilizando el algoritmo LRU.
 *
 * @param mem Puntero a la memoria interna.
 * @return int Índice del marco víctima.
 */
int seleccionar_victima_LRU(t_memoria_interna* mem);

#endif