/*
 * Lo poco de SDL que usa la biblioteca OPL de Chocolate Doom (opl.c y
 * midifile.c). No hay hilos dentro del juego: el mutex no hace nada, y esperar
 * una condición es hacer avanzar al sintetizador (doom/opl_cxd.c) hasta que la
 * llamada pendiente la cumpla (OPL_Delay durante la detección del chip).
 */
#ifndef CXD_SDL_H
#define CXD_SDL_H
#include <stdint.h>
typedef struct cxd_sdl_obj SDL_mutex;
typedef struct cxd_sdl_obj SDL_cond;
void cxd_opl_wait_step(void);
static inline SDL_mutex *SDL_CreateMutex(void) { return (SDL_mutex *)1; }
static inline SDL_cond *SDL_CreateCond(void) { return (SDL_cond *)1; }
static inline void SDL_DestroyMutex(SDL_mutex *m) { (void)m; }
static inline void SDL_DestroyCond(SDL_cond *c) { (void)c; }
static inline int SDL_LockMutex(SDL_mutex *m) { (void)m; return 0; }
static inline int SDL_UnlockMutex(SDL_mutex *m) { (void)m; return 0; }
static inline int SDL_CondSignal(SDL_cond *c) { (void)c; return 0; }
static inline int SDL_CondWait(SDL_cond *c, SDL_mutex *m) { (void)c; (void)m; cxd_opl_wait_step(); return 0; }
static inline uint16_t SDL_SwapBE16(uint16_t x) { return (uint16_t)((x >> 8) | (x << 8)); }
static inline uint32_t SDL_SwapBE32(uint32_t x)
{
    return (x >> 24) | ((x >> 8) & 0xff00u) | ((x << 8) & 0xff0000u) | (x << 24);
}
#endif
