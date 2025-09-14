#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "ao_evt_mpool.h"
#include "ao_core.h"

/**
 * @brief Configuración del tamaño y cantidad de bloques en el pool de memoria.
 * @details Estos valores pueden ser ajustados según las necesidades del sistema.
 * @note Asegúrese de que MP_BLOCK_SIZE sea adecuado para los objetos que se almacenarán en el pool.
 * @note MP_BLOCK_COUNT debe ser suficiente para manejar la carga máxima esperada.
 */
#ifndef MP_BLOCK_SIZE
#define MP_BLOCK_SIZE   CONFIG_AO_MPOOL_BLOCK_SIZE     
#endif
#ifndef MP_BLOCK_COUNT
#define MP_BLOCK_COUNT  CONFIG_AO_MPOOL_BLOCK_COUNT
#endif

static_assert(MP_BLOCK_SIZE >= sizeof(ao_evt_t) + 1, "MP_BLOCK_SIZE demasiado pequeño");

// Memoria estática para el pool de memoria
static uint8_t s_mem[MP_BLOCK_COUNT][MP_BLOCK_SIZE];
static uint32_t s_bitmap = 0;   // 1 = ocupado, 0 = libre (hasta 32 bloques)

static bool s_inited = false;

/* Protección mínima: sección crítica (muy corta) */
static inline void mp_enter(void) { taskENTER_CRITICAL(NULL); }
static inline void mp_exit(void)  { taskEXIT_CRITICAL(NULL); }

bool mpool_start(void)
{
    if (s_inited)
        return false;   // Ya inicializado

    mp_enter();
    memset(s_mem, 0, sizeof(s_mem));
    s_bitmap = 0;
    s_inited = true;
    mp_exit();
    return true;
}

void* mpool_alloc(size_t size)
{
    if (!s_inited || size == 0 || size > MP_BLOCK_SIZE) 
        return NULL;    // No inicializado o tamaño inválido

    mp_enter();
    for (uint32_t i = 0; i < MP_BLOCK_COUNT; i++) {
        if ((s_bitmap & (1U << i)) == 0) { // Bloque libre
            s_bitmap |= (1U << i);         // Marcar como ocupado
            mp_exit();
            return s_mem[i];
        }
    }
    mp_exit();
    return NULL;    // No hay bloques libres
}

void mpool_free(void* ptr)
{
    if (!s_inited || ptr == NULL) 
        return; // No inicializado o puntero inválido

    mp_enter();
    for (uint32_t i = 0; i < MP_BLOCK_COUNT; i++) {
        if (ptr == s_mem[i]) {
            s_bitmap &= ~(1U << i); // Marcar como libre
            break;
        }
    }
    mp_exit();
}

size_t mpool_block_size(void) { return MP_BLOCK_SIZE; }

size_t mpool_capacity(void) { return MP_BLOCK_COUNT; }

size_t mpool_free_count(void) {
    mp_enter();
    uint32_t bm = s_bitmap;
    mp_exit();
    size_t used = 0;
    for (int i = 0; i < MP_BLOCK_COUNT; ++i) used += (bm >> i) & 1u;
    return MP_BLOCK_COUNT - used;
}
