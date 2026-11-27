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
- Pruebas de la biblioteca en QEMU con el cargador de la cámara:
  `preload/test/run.sh`. Compilador: contenedor `cx580-toolchain`
  (`toolchain/Containerfile`).
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
preload/test/run.sh                                                       # pruebas en QEMU
```

## Documentos

- `README.md`: estado general y estructura.
- `docs/ui-spec.md`: el menú objetivo ítem por ítem y qué existe de fábrica.
- `ui/menu-prototype.html`: prototipo del menú en el navegador.
- `docs/research/web-research.md`: fuentes, hardware, pruebas con clips y DC IN.
- `docs/research/firmware-analysis.md`: arranque, rescate, tabla de bloqueos,
  protocolo de la interfaz con la cámara y pruebas en la cámara.
