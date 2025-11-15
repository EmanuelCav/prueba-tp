#ifndef WORKER_MEMORIA_INTERNA_H
#define WORKER_MEMORIA_INTERNA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>

typedef struct
{
    int numero_pagina;
    char file[32];
    char tag[32];
    void *marco;
    bool modificada;
    bool ocupada;
    int frame;
    int uso;
} t_pagina;

typedef struct
{
    int cant_marcos;
    int tamanio_pagina;
    void *memoria;
    t_pagina *marcos;
    int puntero_clock;
    t_list *tablas_paginas;
} t_memoria_interna;

/**
 * @brief Crea e inicializa la memoria interna del Worker.
 *
 * @param tam_memoria Tamaño total de la memoria interna en bytes.
 * @param tamanio_pagina Tamaño de cada página en bytes.
 * @return t_memoria_interna* Puntero a la memoria interna creada.
 */
t_memoria_interna *memoria_crear(int tam_memoria, int tamanio_pagina);

typedef struct
{
    int numero_pagina;
    int marco;
    bool presente;
    bool modificada;
} t_pagina_tabla;

typedef struct
{
    char file[64];
    char tag[64];
    t_list *paginas;
} t_tabla_paginas_file;

/**
 * @brief Busca un marco libre dentro de la memoria interna.
 *
 * @param mem Puntero a la memoria interna.
 * @return int Índice del marco libre o -1 si no hay marcos disponibles.
 */
int buscar_marco_libre(t_memoria_interna *mem);

/**
 * @brief Busca si una página específica se encuentra cargada en memoria.
 *
 * @param mem Puntero a la memoria interna.
 * @param file Nombre del File.
 * @param tag Tag del File.
 * @param numero_pagina Número de la página a buscar.
 */
int buscar_pagina_en_memoria(t_memoria_interna *mem, const char *file, const char *tag, int numero_pagina);

/**
 * @brief Asigna una página a un marco de memoria.
 *
 * @param mem Puntero a la memoria interna.
 * @param marco Índice del marco a asignar.
 * @param numero_pagina Número de página a asignar.
 * @param file Nombre del File.
 * @param tag Tag del File.
 * @param query_id ID de la query para logging.
 * @param logger Path del logger.
 */
void asignar_pagina(t_memoria_interna *mem, int marco, int numero_pagina, char *file, char *tag, int query_id, t_log *logger);

/**
 * @brief Libera un marco de la memoria interna.
 *
 * @param mem Puntero a la memoria interna.
 * @param marco Índice del marco a liberar.
 * @param query_id ID de la query para logging.
 * @param logger Path del logger.
 */
void liberar_marco(t_memoria_interna *mem, int marco, int query_id, t_log *logger);

/**
 * @brief Selecciona una página víctima utilizando el algoritmo LRU.
 *
 * @param mem Puntero a la memoria interna.
 * @return int Índice del marco víctima.
 */
int seleccionar_victima_LRU(t_memoria_interna *mem);

/**
 * @brief Selecciona una página víctima utilizando el algoritmo CLOCK-M.
 *
 * @param mem Puntero a la memoria interna.
 */
int seleccionar_victima_CLOCKM(t_memoria_interna *mem);

/**
 * @brief Obtiene la tabla de páginas correspondiente a un File:Tag o la crea si no existe.
 *
 * @param mem Puntero a la memoria interna.
 * @param file Nombre del File.
 * @param tag Tag del File.
 */
t_tabla_paginas_file *obtener_o_crear_tabla(t_memoria_interna *mem, const char *file, const char *tag);

/**
 * @brief Busca una página específica dentro de la tabla de páginas de un File:Tag.
 *
 * @param tbl Puntero a la tabla de páginas del File:Tag.
 * @param numero_pagina Número de página a buscar.
 */
t_pagina_tabla *buscar_pagina_tabla(t_tabla_paginas_file *tbl, int numero_pagina);

/**
 * @brief Actualiza o inserta una entrada en la tabla de páginas.
 *
 * @param mem Puntero a la memoria interna.
 * @param file Nombre del File.
 * @param tag Tag del File.
 * @param numero_pagina Número de la página.
 * @param marco Número del marco donde está cargada.
 * @param presente Indica si la página está presente en memoria.
 * @param modificada Indica si la página fue modificada.
 */
void actualizar_tabla_pagina(t_memoria_interna *mem, const char *file, const char *tag, int numero_pagina, int marco, bool presente, bool modificada);

/**
 * @brief Limpia la entrada de una página en la tabla de páginas (marcándola como no presente).
 *
 * @param mem Puntero a la memoria interna.
 * @param file Nombre del File.
 * @param tag Tag del File.
 * @param numero_pagina Número de la página a limpiar.
 */
void limpiar_tabla_pagina(t_memoria_interna *mem, const char *file, const char *tag, int numero_pagina);

#endif