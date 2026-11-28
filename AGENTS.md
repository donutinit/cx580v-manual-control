# cx580-pro: instrucciones para agentes

Firmware y modificaciones de código abierto para una Sony HDR-CX580V que es del
dueño de este proyecto. El objetivo es control manual independiente y simultáneo
de iris, obturación, ganancia, foco y balance de blancos, con el menú
minimalista de `docs/ui-spec.md` (prototipo navegable en `ui/menu-prototype.html`).

La documentación del proyecto va en español, dentro de `docs/`.

## Reglas que no se rompen

1. Nunca se escriben `nflasha1` (el updater), el bootloader (`boot1`) ni
   `/setting/updater/comp`. Son el camino de rescate: con
   `/setting/updater/mode` presente al encender, el bootloader arranca el
   updater sin pasar por el firmware principal.
2. Antes de escribir en la flash de la cámara (`/dev/nflasha*`), en `Backup.bin`
   o en `/setting`, tiene que haber un respaldo verificado de lo que se cambia y
   un camino de recuperación para ese cambio. No hace falta pedir permiso para
   cada paso: el dueño autorizó el 2026-11-26 avanzar sin confirmación. Sí hay
   que decirle qué se va a escribir antes de hacerlo.
3. No modifiques `backups/` ni `/mnt/hera/cx580-pro-backups/`. Son los únicos
   respaldos del firmware y están protegidos contra escritura. Las carpetas
   nuevas que crea `cxdump.py` dentro de `backups/` sí están permitidas.
4. Orden de preferencia para cada cambio: ajuste de fábrica, luego propiedad de
   Backup reversible (leer y guardar antes el valor original), luego código que
   corre sin escribir el firmware (`preload/cxg.so` desde `/setting`), y al
   final parche de las particiones de firmware.
5. No digas que algo funciona sin probarlo en la cámara. Si no se probó, dilo.

## Estado verificado al 2026-11-26

- Unidad: HDR-CX580V, código de producto `0002408230`, firmware 1.07.
  USB: `054c:05fc` Mass Storage, `054c:05fd` MTP, `054c:03e2` modo updater,
  `054c:05b2` apagada (cargando).
- Updater: ID de modelo `0x21010001` (el de la HDR-PJ590V), región 0.
  Plataforma CXD4132, Linux 2.6.29.6-rt24, flash OneNAND de 256 MiB.
- `tools/cxdump.py` reinicia la cámara al modo updater con el shell de
  Sony-PMCA-RE. No flashea: la sección de firmware del payload va vacía. Desde
  modo normal la cámara pide confirmar en pantalla. Tarda unos 35 s en
  aparecer y vuelve sola al modo normal al salir.
- Respaldo completo: `backups/20261126-dump/`, con `manifest.json` (SHA-256 y
  md5 cotejado en la cámara), y copia en `/mnt/hera/cx580-pro-backups/`.
- Firmware extraído con fwtool en `firmware/cx580v-1.07/unpacked/`, analizado
  con Ghidra (`firmware/ghidra/`). Hallazgos en
  `docs/research/firmware-analysis.md`.
- **Código propio en modo normal**: `preload/cxg.so` se instala en `/setting`
  con `cxdump.py guard-install` y lo carga el `init` de fábrica vía
  `/setting/env.txt` (sólo si existe `/setting/mode/dmode`). En cada arranque
  pone la bandera del updater, así que un encendido sí y otro no la cámara entra
  al updater. Se quita con `guard-remove`. Probado en la cámara.
- **Iris, obturación y ganancia manuales e independientes funcionan** con
  `guard-install --config preload/cxmode-v9.txt`: Exposure abre `view/IRIS`,
  Spot Meter `view/SHUTTER_SPEED`, Spot Meter/Fcs (AE Shift) fija el límite de
  ganancia de 0 a 24 dB y Tele Macro abre WB Shift. Probado en la cámara por el
  dueño. Desde v11f (`cxg.so` md5 `dbf20855…`) cada control vuelve a su valor
  al regresar a su pantalla: `cxg.so` lee el F y la obturación reales con
  `ObjMsg::GetParam` y corrige con pasos. El automático es por control:
  ponerlo en una pantalla deja el otro fijo. Las dos cosas probadas en la
  cámara por el dueño. Detalle en `docs/research/firmware-analysis.md`.
- **Ganancia fija (v16)**: `guard-install --config preload/cxmode-v16.txt`
  (`cxg.so` md5 `88ad42cc…`, copia en `preload/release/`). El AE Shift de la
  pantalla elige el tope de ganancia (−4 = 0 dB … +4 = 24 dB) y a la cámara le
  llega siempre +4, así que con iris y obturación fijos el automático sube la
  ganancia hasta el tope; la pantalla sigue viendo su valor. Probado por el
  dueño: iris, obturación y ganancia se mueven cada uno sin tocar los otros.
  Pendiente: saltos esporádicos de 1/3 a 1/6 de paso.
- El ISO no sirve en video: con los bloqueos destrabados la cámara acepta
  `SET_ISO_SPEED_RATE` (`ret=0`) pero no lo aplica. La exposición manual de
  fábrica (`SET_MANUAL_EXPOSURE_MODE`) tampoco: reinicia iris y obturación.
- **v17** (`cxg.so` md5 `acb1d0dd…`) es lo instalado en la cámara al 2026-11-27:
  la v16 más la corrección del control en segundo plano. Probada por el dueño;
  la corrección cruzada no llegó a actuar en esa sesión.
- **v18** (`preload/release/cxg-v18.so`,
  md5 `0841f513…`): muestra las entradas de fábrica IRIS, Shutter Speed,
  AE Shift (rotulada Gain) y WB Shift en Manual Settings, pone iconos de dB en
  la pantalla de ganancia y trae configuraciones por etapas (`cxmode-v18*.txt`).
  La guía de prueba es `docs/pruebas-v18.md`. `cxg-v18-ux.so` agrega una
  bitácora de toques y botones.
- **v19** (`preload/release/cxg-v19.so`, md5 `e2c57318…`) es lo instalado al
  2026-11-27, con `cxmode-v18d.txt`. Probado en la cámara ese día con la
  v18: las entradas nuevas del menú aparecen y abren sus pantallas (dueño y
  bitácora), los iconos de dB se piden, y con `autounlock 1` la corrección del
  control en segundo plano pasa (`ret=0`) y devuelve iris y obturación a su
  valor en menos de un segundo. Ocultar ítems con el byte «válido» **no**
  funcionó en la cámara; la v19 lo hace con el estado en 0 y el dueño confirmó
  que las entradas ya no se ven. La variante `cxg-v19-ux.so` con
  `cxmode-v19u.txt` anotó toques (espacio de 640×480) y el número de evento de
  cada botón físico (tabla en `docs/research/ux-panel-propio.md`); desde las
  06:51 está otra vez la v19 normal con `cxmode-v18d.txt`. Los clips de `captures/prueba-v18/` coinciden con la
  bitácora; `tools/cxexposure.py` corrige la obturación de los AVCHD (ExifTool
  la da con divisor de 50 Hz).
- **Panel propio (v30)**: es lo instalado desde el 2026-11-27 09:33
  (`preload/release/cxg-v30.so`, md5 `3790c9d2…`, con `cxmode-v28p.txt`). En la
  pantalla de grabación de video muestra balance de blancos, iris, obturación y
  ganancia, cada uno con su automático (botón AUTO), y se ajustan tocando, sin
  entrar al menú. El balance pasa por AWB, sol, interior y One Push; con One
  Push el botón dice SET y toma el blanco de la carta gris. De lo nativo sólo
  deja MENU, el medidor de audio y el indicador rojo de REC. Confirmado por el
  dueño; sin órdenes del panel rechazadas en las bitácoras. Falta comprobar con
  un clip que lo grabado coincide con el panel, y si el One Push tomado desde
  el panel sobrevive al apagado. Si la interfaz reenvía sus ajustes (cambio de
  Audio Mode, quizá HD/STD), la exposición vuelve a automático; y si se elige
  una opción en un menú que cierra todo de golpe, el panel queda oculto hasta
  abrir y cerrar MENU. A `cxg.so` le quedan 1 084 bytes antes del tope de tres
  bloques. Detalle en `docs/panel-propio.md`.
- **DOOM, a medias**: el juego es un programa aparte (`doom/`, doomgeneric)
  que va en la partición de PlayMemories (`/log`) con el WAD shareware;
  `preload/cxdoom.c` lo carga, lo pinta sobre la pantalla de grabación y le
  pasa tacto y botones; se lanza desde Setup → Connection → DOOM (el ítem Disc
  Burn) o con un toque en la esquina de arriba a la derecha. Probado en la
  cámara el 2026-11-27: el cuadro de prueba se ve (colores R G B A confirmados),
  hay memoria, el menú lanza el juego, y con la v32 y la v33 el juego arrancó y
  corrió a unos 30 turnos por segundo tomando los botones, pero en pantalla no
  se vio. Causa probable: avanzaba dentro del hilo de la interfaz y doomgeneric
  duerme ahí hasta su siguiente tic. La v34 lo pasó a un hilo propio y tampoco
  se vio, ni las franjas de carga: el cuadro sólo se ve si se crea o se muestra
  desde un toque (como la sonda), no desde los turnos periódicos. La v35 lo creó
  desde el toque y tampoco se vio. Con la v36 **se vio**: el cuadro sólo se ve
  mientras nuestro panel está activo (a los 15 s el panel se ocultó y el juego
  desapareció). La v37 deja el panel activo todo el juego: el dueño confirmó
  que funciona perfecto. **Sonido:** los efectos salen por la ruta de los
  pitidos del sistema (`preload/cxsound.c`); con la v38 se oyeron pero todos
  iguales (la cámara guarda los pitidos cargados), y la v39 congeló la cámara
  (un pitido más largo que el búfer de la cámara deja su lector en un bucle
  sin fin). **Cuidado:** nada de lo que se entrega a un pitido puede medir el
  búfer o más; el tope es 36 480 bytes y `w_fread` lo vigila. La v40 trajo la
  guarda, la rotación de 11 pitidos y música OPL (E1M1 en bucle en el título);
  sonaba una octava más grave (tasa del emulador mal puesta, corregida) y le
  quitaba el procesador al juego. La v42 la genera en su propio hilo: fluye,
  pero seguía una octava abajo, así que la cámara reproduciría los pitidos a
  24 kHz. Con la v43 (24 kHz) el dueño confirmó que «suena precioso», pero
  tenue. Con la v44 (música ×12 y arranque rápido) «todo se escucha bien».
  Después: PHOTO a fondo ya sólo dispara (v45) y, por cortes breves entre
  trozos de música, la **v46** (`preload/release/cxg-v46.so`, md5 `cd211eb4…`,
  con `cxmode-v46p.txt` y `CXDOOM.BIN` md5 `436dbaed…`, que van juntos) encima
  los trozos 150 ms con fundido. **La v46 es lo instalado desde el 2026-11-27
  20:48**; pasa en QEMU y falta que el dueño confirme que se fueron los
  cortes. Detalle en `docs/doom.md`.
- Pruebas de la biblioteca en QEMU con el cargador de la cámara y emulando su
  procesador (`-cpu arm11mpcore`): `preload/test/run.sh`. Compilador:
  contenedor `cx580-toolchain` (`toolchain/Containerfile`).
- Lectura de exposición, balance y foco en clips grabados: `tools/cxexposure.py`.

## Requisito físico

El updater exige adaptador de corriente. La unidad usa una fuente de banco a
8.4 V soldada a la toma DC IN: cable rojo al positivo, negro a tierra y azul a
tierra por 10 kΩ. Sin el azul a tierra, la guarda responde «AC adapter
required». Conectar la fuente y la cámara le toca al dueño.

## Comandos

```bash
.venv/bin/python -m unittest discover -s tools          # pruebas, sin cámara
.venv/bin/python tools/cxprobe.py usb                   # ¿se ve la cámara?
.venv/bin/python tools/cxprobe.py info                  # modelo y versión, solo lectura
.venv/bin/python tools/cxprobe.py guess-model           # guarda del updater, solo lectura
.venv/bin/python tools/cxdump.py recon --model 0x21010001 --extra 'CMD'   # reinicia al updater, solo lee
.venv/bin/python tools/cxdump.py verify backups/20261126-dump
.venv/bin/python tools/cxdump.py guard-status  --model 0x21010001         # qué hay en /setting
.venv/bin/python tools/cxdump.py guard-install --model 0x21010001 [--mode iris]
.venv/bin/python tools/cxdump.py guard-fetch   --model 0x21010001 --delete # baja bitácora y volcado
.venv/bin/python tools/cxdump.py guard-remove  --model 0x21010001         # deja /setting de fábrica
.venv/bin/python tools/cxlogdecode.py backups/<dir>/cxreq.log --summary   # la bitácora en palabras
doom/build.sh                                                             # compila doom/build/CXDOOM.BIN
.venv/bin/python tools/cxdump.py pm-check     --model 0x21010001          # PlayMemories, sólo lectura
.venv/bin/python tools/cxdump.py doom-install --model 0x21010001          # copia el juego a PlayMemories
preload/test/run.sh                                                       # pruebas en QEMU
```

## Documentos

- `README.md`: estado general y estructura.
- `docs/ui-spec.md`: el menú objetivo ítem por ítem y qué existe de fábrica.
- `ui/menu-prototype.html`: prototipo del menú en el navegador.
- `docs/research/web-research.md`: fuentes, hardware, pruebas con clips y DC IN.
- `docs/research/firmware-analysis.md`: arranque, rescate, tabla de bloqueos,
  protocolo de la interfaz con la cámara, textos y pruebas en la cámara.
- `docs/pruebas-v18.md`: guía de la prueba completa por etapas.
- `docs/panel-propio.md`: el panel propio, su configuración y el mapa de la
  pantalla de grabación.
- `docs/doom.md`: DOOM desde el menú: piezas, instalación, controles, sonda y
  lo que falta probar. `docs/research/dibujar-cuadros.md` y
  `docs/research/audio-y-archivos.md`: el análisis estático en que se apoya.
- `reports/Barrido de fuentes para la CX580V.md`: firmwares hermanos (NX70,
  EA50, AX100 en `firmware/`), valores del `Backup.bin` de la unidad, riesgos y
  lista priorizada de siguientes pasos. Lectura estática y fuentes, sin probar.
- `docs/research/menu-fabrica.md`: estructura del menú de fábrica y cómo
  mostrar u ocultar ítems. `docs/research/vistas-agc-aeshift.md`: pantallas de
  ganancia. `docs/research/ux-panel-propio.md`: dibujar y leer tacto y botones
  con el framework de Sony. Los tres son análisis estático, sin probar.
