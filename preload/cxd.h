/*
 * Interfaz entre cxg.so (el anfitrión, preload/cxdoom.c) y el programa aparte
 * de DOOM (CXDOOM.BIN). El programa es una imagen plana enlazada en CXD_BASE y
 * seguida de su tabla de reubicación: el anfitrión la carga donde el kernel dé
 * sitio, suma la diferencia a cada palabra de la tabla, comprueba la cabecera y
 * llama a sus tres entradas, siempre desde el hilo de la interfaz. (La primera
 * versión se cargaba en una dirección fija, 0x70000000; en la cámara está
 * ocupada: prueba del 2026-11-27.)
 */
#ifndef CXD_H
#define CXD_H

#define CXD_MAGIC   0x4d445843u      /* "CXDM" */
#define CXD_VERSION 5u
#define CXD_BASE    0x70000000u      /* dirección de enlace */

struct cxd_header {                  /* los primeros 64 bytes del archivo */
    unsigned magic, version;
    unsigned base;                   /* dirección de enlace; ya cargada, la real */
    unsigned file_size;              /* bytes de la imagen (código y datos) */
    unsigned mem_size;               /* más lo que hay que dejar en cero detrás */
    unsigned sum;                    /* suma de las palabras de 32 bits desde el byte 64 hasta el final del archivo */
    unsigned init, tick, key;        /* direcciones de las entradas (reubicadas como todo lo demás) */
    unsigned nreloc;                 /* palabras de la tabla que sigue a la imagen: desplazamientos a corregir */
    unsigned music;                  /* desde la versión 5: la entrada del hilo de música */
    unsigned pad[5];
};

struct cxd_host {
    void (*log)(const char *line);   /* una línea para la bitácora, sin salto */
    const char *wad;                 /* ruta del IWAD */
    unsigned zone_mb;                /* MiB para la memoria del juego (6 de fábrica) */
    unsigned char shift[3];          /* dónde van los 4 bits de rojo, verde y azul */
    unsigned char pad;
    unsigned opaque;                 /* bits que hacen opaco el píxel */
    /* desde la versión 3 */
    void (*sound)(const unsigned char *data, unsigned len);   /* un efecto en el formato de los
                                                                 pitidos; 0: sin sonido */
    unsigned sound_hz;               /* muestras por segundo de los pitidos (48000) */
    unsigned sound_shift;            /* escala de cada bloque: 8 es el volumen completo */
    /* desde la versión 4: música (OPL emulado, en trozos por la misma ruta) */
    void (*music)(const unsigned char *data, unsigned len);   /* un trozo; 0: sin música */
    unsigned music_rate;             /* muestras por segundo del emulador OPL (24000) */
    unsigned music_chunk_ms;         /* largo de cada trozo (2000) */
    unsigned music_lead_ms;          /* cuánto antes de su hora se manda cada trozo (2000: a su hora) */
    unsigned music_title;            /* 1: la pantalla de título se queda, con E1M1 en bucle */
};

/* int  music(void)                              desde otro hilo, cada ~10 ms: genera la música que
 *                                                toca y manda los trozos; < 0 si el juego terminó
 * int  init(const struct cxd_host *)             0, o < 0 si no arrancó
 * int  tick(unsigned short *dst, unsigned pitch) 1 si pintó un cuadro de 320x200 en dst
 *                                                (pitch en píxeles), 0 si no, < 0 si terminó
 * void key(int pressed, int key)                 una tecla, con los códigos de doomkeys.h */
typedef int  (*cxd_init_fn)(const struct cxd_host *);
typedef int  (*cxd_tick_fn)(unsigned short *, unsigned);
typedef void (*cxd_key_fn)(int, int);
typedef int  (*cxd_music_fn)(void);

/* Teclas que manda el anfitrión (doomkeys.h). */
#define CXK_RIGHT 0xae
#define CXK_LEFT  0xac
#define CXK_UP    0xad
#define CXK_DOWN  0xaf
#define CXK_USE   0xa2
#define CXK_FIRE  0xa3
#define CXK_ESC   27
#define CXK_ENTER 13
#define CXK_YES   'y'
#define CXK_NEXTW ']'      /* arma siguiente (se asigna al arrancar) */
#define CXK_MAP   9

#endif
