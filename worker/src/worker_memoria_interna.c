#include "../include/worker_memoria_interna.h"

t_memoria_interna* memoria_crear(int tam_memoria, int tamanio_pagina)
{
    t_memoria_interna* mem = malloc(sizeof(t_memoria_interna));
    mem->tamanio_pagina = tamanio_pagina;
    mem->cant_marcos = tam_memoria / tamanio_pagina;
    mem->memoria = malloc(tam_memoria);
    mem->marcos = malloc(sizeof(t_pagina) * mem->cant_marcos);

    for (int i = 0; i < mem->cant_marcos; i++)
    {
        mem->marcos[i].ocupada = false;
        mem->marcos[i].modificada = false;
        mem->marcos[i].frame = i;
        mem->marcos[i].uso = 0;
        mem->marcos[i].numero_pagina = -1;
        strcpy(mem->marcos[i].file, "");
        strcpy(mem->marcos[i].tag, "");
        mem->marcos[i].marco = (char*)mem->memoria + (i * tamanio_pagina);
    }

    return mem;
}

int buscar_marco_libre(t_memoria_interna* mem)
{
    for (int i = 0; i < mem->cant_marcos; i++)
    {
        if (!mem->marcos[i].ocupada)
            return i;
    }
    return -1;
}

void asignar_pagina(t_memoria_interna* mem, int marco, int numero_pagina, char* file, char* tag)
{
    t_pagina* p = &mem->marcos[marco];
    p->numero_pagina = numero_pagina;
    strcpy(p->file, file);
    strcpy(p->tag, tag);
    p->ocupada = true;
    p->modificada = false;
    p->uso = 1;

    printf("Se asigna el Marco: %d a la Página: %d del File: %s - Tag: %s\n",
           marco, numero_pagina, file, tag);
}

void liberar_marco(t_memoria_interna* mem, int marco)
{
    t_pagina* p = &mem->marcos[marco];
    printf("Se libera el Marco: %d del File: %s - Tag: %s\n",
           marco, p->file, p->tag);
    p->ocupada = false;
    p->modificada = false;
    p->numero_pagina = -1;
    strcpy(p->file, "");
    strcpy(p->tag, "");
    p->uso = 0;
}

int seleccionar_victima_LRU(t_memoria_interna* mem)
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