/* Lo que i_oplmusic.c (Chocolate Doom 2.2.1) espera de su i_sound.h y doomgeneric no trae. */
#ifndef CXD_OPL_COMPAT_H
#define CXD_OPL_COMPAT_H
typedef enum { opl_v_old, opl_v_new } opl_driver_ver_t;
void I_SetOPLDriverVer(opl_driver_ver_t ver);
#endif
