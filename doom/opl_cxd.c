/*
 * Backend de la biblioteca OPL de Chocolate Doom 2.2.1 para la cámara: el
 * emulador DBOPL (de DOSBox) genera las muestras cuando el juego las pide
 * (cxd_opl_render), en su propio hilo, y en ese mismo momento se llaman las
 * funciones que el secuenciador de música dejó programadas para ese instante.
 * Es opl_sdl.c sin SDL; se llama igual (opl_sdl_driver) para que opl.c lo
 * encuentre sin cambios.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dbopl.h"
#include "opl.h"
#include "opl_internal.h"
#include "opl_queue.h"

typedef struct { unsigned int rate, enabled, value; uint64_t expire_time; } opl_timer_t;

static opl_callback_queue_t *callback_queue;
static uint64_t current_time, pause_offset;
static int paused, ready;
static Chip opl_chip;
static int32_t mix_buffer[512];
static int register_num;
static opl_timer_t timer1 = { 12500, 0, 0, 0 };
static opl_timer_t timer2 = { 3125, 0, 0, 0 };
static unsigned int mixing_freq;

static void AdvanceTime(unsigned int nsamples)
{
    opl_callback_t callback;
    void *data;
    uint64_t us = ((uint64_t)nsamples * OPL_SECOND) / mixing_freq;
    current_time += us;
    if (paused) pause_offset += us;
    while (!OPL_Queue_IsEmpty(callback_queue) && current_time >= OPL_Queue_Peek(callback_queue) + pause_offset) {
        if (!OPL_Queue_Pop(callback_queue, &callback, &data)) break;
        callback(data);
    }
}

/* n muestras mono de 16 bits; out puede ser 0 (sólo avanzar). */
void cxd_opl_render(int16_t *out, unsigned int n)
{
    unsigned int filled = 0, i;
    if (!ready) { if (out) memset(out, 0, n * 2); return; }
    while (filled < n) {
        uint64_t k = n - filled;
        if (!paused && !OPL_Queue_IsEmpty(callback_queue)) {
            uint64_t next = OPL_Queue_Peek(callback_queue) + pause_offset;
            uint64_t until = next > current_time ? next - current_time : 0;
            uint64_t s = (until * mixing_freq + OPL_SECOND - 1) / OPL_SECOND;
            if (s < k) k = s;
        }
        if (k > 512) k = 512;
        if (k) {
            Chip__GenerateBlock2(&opl_chip, (Bitu)k, mix_buffer);
            if (out)
                for (i = 0; i < k; i++) {
                    int32_t v = mix_buffer[i];
                    out[filled + i] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
                }
        }
        filled += (unsigned int)k;
        AdvanceTime((unsigned int)k);
    }
}

/* Para la espera de OPL_Delay: un milisegundo de música, que nadie escucha. */
void cxd_opl_wait_step(void) { cxd_opl_render(0, mixing_freq / 1000 + 1); }

static int Init(unsigned int port_base)
{
    (void)port_base;
    paused = 0;
    pause_offset = 0;
    current_time = 0;
    if (!callback_queue) callback_queue = OPL_Queue_Create();
    mixing_freq = opl_sample_rate;
    DBOPL_InitTables();
    Chip__Chip(&opl_chip);
    Chip__Setup(&opl_chip, mixing_freq);
    ready = 1;
    return 1;
}

static void Shutdown(void) { ready = 0; }

static unsigned int PortRead(opl_port_t port)
{
    unsigned int r = 0;
    if (port == OPL_REGISTER_PORT_OPL3) return 0xff;
    if (timer1.enabled && current_time > timer1.expire_time) r |= 0x80 | 0x40;
    if (timer2.enabled && current_time > timer2.expire_time) r |= 0x80 | 0x20;
    return r;
}

static void TimerEnd(opl_timer_t *t)
{
    if (t->enabled) t->expire_time = current_time + ((uint64_t)(0x100 - t->value) * OPL_SECOND) / t->rate;
}

static void WriteRegister(unsigned int reg, unsigned int value)
{
    switch (reg) {
    case OPL_REG_TIMER1: timer1.value = value; TimerEnd(&timer1); break;
    case OPL_REG_TIMER2: timer2.value = value; TimerEnd(&timer2); break;
    case OPL_REG_TIMER_CTRL:
        if (value & 0x80) { timer1.enabled = timer2.enabled = 0; break; }
        if (!(value & 0x40)) { timer1.enabled = (value & 1) != 0; TimerEnd(&timer1); }
        if (!(value & 0x20)) { timer1.enabled = (value & 2) != 0; TimerEnd(&timer2); }   /* igual que opl_sdl.c */
        break;
    default: Chip__WriteReg(&opl_chip, reg, (Bit8u)value); break;
    }
}

static void PortWrite(opl_port_t port, unsigned int value)
{
    if (port == OPL_REGISTER_PORT) register_num = value;
    else if (port == OPL_REGISTER_PORT_OPL3) register_num = value | 0x100;
    else if (port == OPL_DATA_PORT) WriteRegister(register_num, value);
}

static void SetCallback(uint64_t us, opl_callback_t cb, void *data)
{
    OPL_Queue_Push(callback_queue, cb, data, current_time - pause_offset + us);
}

static void ClearCallbacks(void) { OPL_Queue_Clear(callback_queue); }
static void Lock(void) {}
static void Unlock(void) {}
static void SetPaused(int p) { paused = p; }
static void AdjustCallbacks(float factor) { OPL_Queue_AdjustCallbacks(callback_queue, current_time, factor); }

opl_driver_t opl_sdl_driver = {
    "cxd", Init, Shutdown, PortRead, PortWrite, SetCallback, ClearCallbacks, Lock, Unlock, SetPaused, AdjustCallbacks,
};
