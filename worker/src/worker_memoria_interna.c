#include "../include/worker_memoria_interna.h"

t_memoria_interna *memoria_crear(int tam_memoria, int tamanio_pagina)
{
    t_memoria_interna *mem = malloc(sizeof(t_memoria_interna));
    mem->tamanio_pagina = tamanio_pagina;
    mem->cant_marcos = tam_memoria / tamanio_pagina;
    mem->memoria = malloc(tam_memoria);
    mem->marcos = malloc(sizeof(t_pagina) * mem->cant_marcos);
    mem->puntero_clock = 0;

    for (int i = 0; i < mem->cant_marcos; i++)
    {
        mem->marcos[i].ocupada = false;
        mem->marcos[i].modificada = false;
        mem->marcos[i].frame = i;
        mem->marcos[i].uso = 0;
        mem->marcos[i].numero_pagina = -1;
        strcpy(mem->marcos[i].file, "");
        strcpy(mem->marcos[i].tag, "");
        mem->marcos[i].marco = (char *)mem->memoria + (i * tamanio_pagina);
    }

    mem->tablas_paginas = list_create();

    return mem;
}

int buscar_marco_libre(t_memoria_interna *mem)
{
    for (int i = 0; i < mem->cant_marcos; i++)
    {
        if (!mem->marcos[i].ocupada)
            return i;
    }
    return -1;
}

int buscar_pagina_en_memoria(t_memoria_interna *mem, const char *file, const char *tag, int numero_pagina)
{
    for (int i = 0; i < mem->cant_marcos; i++)
    {
        if (mem->marcos[i].ocupada &&
            mem->marcos[i].numero_pagina == numero_pagina &&
            strcmp(mem->marcos[i].file, file) == 0 &&
            strcmp(mem->marcos[i].tag, tag) == 0)
        {
            return i;
        }
    }
    return -1;
}

void asignar_pagina(t_memoria_interna *mem, int marco, int numero_pagina, char *file, char *tag, int query_id, t_log *logger)
{
    t_pagina *p = &mem->marcos[marco];
    p->numero_pagina = numero_pagina;
    strcpy(p->file, file);
    strcpy(p->tag, tag);
    p->ocupada = true;
    p->modificada = false;
    p->uso = 1;

    actualizar_tabla_pagina(mem, file, tag, numero_pagina, marco, true, false);

    log_info(logger, "Query %d: Se asigna el Marco: %d a la Página: %d perteneciente al - File: %s - Tag: %s",
           query_id, marco, numero_pagina, file, tag);
}

void liberar_marco(t_memoria_interna *mem, int marco, int query_id, t_log *logger)
{
    t_pagina *p = &mem->marcos[marco];
    log_info(logger, "Query %d: Se libera el Marco: %d perteneciente al - File: %s - Tag: %s",
           query_id, marco, p->file, p->tag);
    limpiar_tabla_pagina(mem, p->file, p->tag, p->numero_pagina);
    p->ocupada = false;
    p->modificada = false;
    p->numero_pagina = -1;
    strcpy(p->file, "");
    strcpy(p->tag, "");
    p->uso = 0;
}

int seleccionar_victima_LRU(t_memoria_interna *mem)
{
    int min_uso = __INT_MAX__;
    int victima = -1;
    for (int i = 0; i < mem->cant_marcos; i++)
    {
        if (mem->marcos[i].ocupada && mem->marcos[i].uso < min_uso)
        {
            min_uso = mem->marcos[i].uso;
            victima = i;
        }
    }
    return victima;
}

int seleccionar_victima_CLOCKM(t_memoria_interna *mem)
{
    int vueltas = 0;
    int victima = -1;

    while (vueltas < 2 && victima == -1)
    {
        for (int i = 0; i < mem->cant_marcos; i++)
        {
            int idx = (mem->puntero_clock + i) % mem->cant_marcos;
            t_pagina *p = &mem->marcos[idx];

            if (!p->ocupada)
                continue;

            if (vueltas == 0 && p->uso == 0 && !p->modificada)
            {
                victima = idx;
                break;
            }

            if (vueltas == 1 && p->uso == 0 && p->modificada)
            {
                victima = idx;
                break;
            }

            if (p->uso == 1)
                p->uso = 0;
        }

        if (victima == -1)
            vueltas++;
    }

    if (victima == -1)
        victima = mem->puntero_clock;

    mem->puntero_clock = (victima + 1) % mem->cant_marcos;

    return victima;
}

t_tabla_paginas_file *obtener_o_crear_tabla(t_memoria_interna *mem, const char *file, const char *tag)
{
    for (int i = 0; i < list_size(mem->tablas_paginas); i++)
    {
        t_tabla_paginas_file *tbl = list_get(mem->tablas_paginas, i);
        if (strcmp(tbl->file, file) == 0 && strcmp(tbl->tag, tag) == 0)
            return tbl;
    }

    t_tabla_paginas_file *nueva = malloc(sizeof(t_tabla_paginas_file));
    strcpy(nueva->file, file);
    strcpy(nueva->tag, tag);
    nueva->paginas = list_create();
    list_add(mem->tablas_paginas, nueva);

    return nueva;
}

t_pagina_tabla *buscar_pagina_tabla(t_tabla_paginas_file *tbl, int numero_pagina)
{
    for (int i = 0; i < list_size(tbl->paginas); i++)
    {
        t_pagina_tabla *p = list_get(tbl->paginas, i);
        if (p->numero_pagina == numero_pagina)
            return p;
    }
    return NULL;
}

void actualizar_tabla_pagina(t_memoria_interna *mem, const char *file, const char *tag, int numero_pagina, int marco, bool presente, bool modificada)
{
    t_tabla_paginas_file *tbl = obtener_o_crear_tabla(mem, file, tag);
    t_pagina_tabla *p = buscar_pagina_tabla(tbl, numero_pagina);

    if (!p)
    {
        p = malloc(sizeof(t_pagina_tabla));
        p->numero_pagina = numero_pagina;
        list_add(tbl->paginas, p);
    }

    p->marco = marco;
    p->presente = presente;
    p->modificada = modificada;
}

void limpiar_tabla_pagina(t_memoria_interna *mem, const char *file, const char *tag, int numero_pagina)
{
    t_tabla_paginas_file *tbl = obtener_o_crear_tabla(mem, file, tag);
    t_pagina_tabla *p = buscar_pagina_tabla(tbl, numero_pagina);
    if (p)
    {
        p->presente = false;
        p->marco = -1;
    }
}