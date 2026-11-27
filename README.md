# cx580-pro

Control manual independiente de iris, obturación y ganancia en una Sony
HDR-CX580V (plataforma CXD4132, firmware 1.07), sin flashear el firmware. Una
biblioteca propia (`preload/cxg.so`) se carga desde `/setting` dentro de la app
de interfaz de la cámara (`im.elf`), habla con la capa de cámara con los mismos
mensajes que usa Sony (`MWF::ObjMsg`, categoría `0x3100`) y reacomoda las
pantallas de la PJ760V que ya vienen en el firmware.

> **Aviso.** Esto se probó en una sola unidad. Un error en `/setting` puede dejar
> una cámara sin arrancar: le pasó a otra persona con una NEX-7 en noviembre de
> 2026. Aquí el camino de vuelta es la bandera del updater que la biblioteca pone
> en cada arranque (`docs/research/firmware-analysis.md`). Úsalo bajo tu riesgo.

## Qué funciona (probado en la cámara)

- Iris, obturación y ganancia manuales al mismo tiempo, cada uno sin mover los
  otros: `cxdump.py guard-install --model 0x21010001 --config preload/cxmode-v16.txt`.
  - Exposure abre la pantalla de iris, Spot Meter la de obturación.
  - AE Shift elige la ganancia (−4 = 0 dB … +4 = 24 dB).
  - Cada control vuelve a su valor al regresar a su pantalla, y el automático
    es por control.
- Lo que no funcionó (ISO en video, exposición manual de fábrica) y por qué está
  en la tabla de versiones de `docs/research/firmware-analysis.md`.
- Estado al día y reglas de trabajo: `AGENTS.md`.

## Preparar el entorno

```bash
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
mkdir -p vendor && cd vendor
git clone https://github.com/ma1co/Sony-PMCA-RE.git && git -C Sony-PMCA-RE checkout a82f5ba
git clone https://github.com/ma1co/fwtool.py.git && git -C fwtool.py checkout cdba742
git clone https://github.com/ma1co/OpenMemories-Platform.git && git -C OpenMemories-Platform checkout 85bcb54
```

El firmware, los volcados de la cámara, los videos de prueba y las láminas de
manuales de servicio no están en el repositorio. `firmware/names/` tiene los
diccionarios de mensajes, parámetros y valores sacados de `libObj.so` 1.07, y
`firmware/ghidra/scripts/`, los scripts de análisis.

## Historia del proyecto

| Paso | Estado |
|---|---|
| Entorno (venv, Sony-PMCA-RE, fwtool.py, OpenMemories-Platform) | hecho |
| Generador de payload del updater para cualquier ID de modelo | hecho, probado |
| Sesión automatizada: recon + volcado verificado de la flash | hecho, probado contra un backend falso |
| Cámara visible por USB | comprobado en Mass Storage (`054c:05fc`) y MTP (`054c:05fd`) |
| Controles de captura por MTP | la unidad anuncia transferencia y carece de propiedades de exposición |
| Primera prueba de exposición | iris, obturación, ganancia y modo verificados con Data Code en cuatro clips STD. [Resultados](docs/research/web-research.md#primera-prueba-en-la-hdr-cx580v) |
| Balance de blancos y foco en STD | el lector reconoce Auto, Indoor y One Push, y distingue foco Auto y Manual. [Resultados](docs/research/web-research.md#resultados-de-las-cinco-tomas-std) |
| Prueba de Low Lux en HD | en esta escena subió la ganancia de 6 a 9 dB, con iris f/1.8 y obturación 1/50 s en las dos tomas. [Resultados](docs/research/web-research.md#resultados-de-las-dos-tomas-hd) |
| Identificar unidad | HDR-CX580V, código de producto `0002408230`, firmware `1.07`. ID de modelo del updater **`0x21010001`**, región **0**, confirmados con la comprobación de guarda |
| Entrada al modo updater | **funciona**. La cámara reinicia a `054c:03e2`, carga el shell de PMCA y vuelve sola al modo normal. Tarda unos 35 s en reaparecer |
| Alimentación externa | fuente de banco a 8.4 V soldada a los tres cables de la toma DC IN. [Detalle](#alimentación-por-dc-in) |
| Recon del sistema | hecho, en `backups/20261126-recon/recon.txt`. [Resumen](#sistema) |
| Camino de rescate | **probado en la cámara**: con `/setting/updater/mode` el bootloader arranca el updater de `nflasha1` sin pasar por el firmware principal. [Detalle](docs/research/firmware-analysis.md#arranque-y-camino-de-rescate) |
| Código propio en modo normal | **probado en la cámara**: `preload/cxguard.so` se carga desde `/setting` vía `env.txt` y pone la bandera de rescate en cada arranque. No se escribe ninguna partición de firmware. Se quita con `cxdump.py guard-remove` |
| Investigación sobre GPL, firmware y controles de fábrica | [informe con fuentes](docs/research/web-research.md) |

La comprobación del equipo real se hizo por USB en modo Mass Storage. Se
leyeron las licencias en `PMHOME` y se consultaron el modelo y la versión.
El respaldo de la flash está en `backups/20261126-dump/`, protegido contra
escritura: las 8 particiones y la imagen completa de 268 304 384 bytes
coinciden con el md5 calculado en la cámara, más `Backup.bin`, un bloque de
bootloader y el boot ROM. `cxdump.py verify` lo volvió a comprobar en disco.
Segunda copia en `/mnt/hera/cx580-pro-backups/` (otro disco físico), verificada
contra el mismo manifiesto y protegida contra escritura.

## Hallazgos

- La CX580V no está en la lista de Sony-PMCA-RE, pero sus hermanas de 2012 sí:
  HDR-PJ590V (`0x21010001`), HDR-PJ760V (`0x21010031`), HDR-TD20 (`0x21010052`),
  todas arquitectura **CXD4132**. El kernel **Linux 2.6.29.6-rt24** que Sony
  publica para la CX580V también incluye una configuración para CXD4132.
  La unidad lo confirma: su updater acepta el ID `0x21010001` (el de la PJ590V)
  con región 0 y rechaza con «Invalid model» los de PJ760V, TD20, SLT-A99 y un
  vecino inventado (`0x21010002`); las regiones 1 y 2 dan «Invalid region».
  La comprobación se hizo por MTP, sin cambiar de modo ni escribir nada.
- El firmware CXD4132 va cifrado con AES-ECB (clave conocida) en bloques de 1024
  bytes con checksum de 16 bits; **no va firmado**.
- El payload del "updater shell" es idéntico entre modelos salvo los primeros
  48 bytes (ID de modelo, región, CRC de cabecera, checksum del primer bloque).
  `tools/cxlib.py` reconstruye byte a byte las 42 cabeceras que trae PMCA, así
  que puede generar la de cualquier ID.
- La PJ760V ofrece prioridad de iris y de obturación. Cada una cancela la otra
  y la ganancia se controla mediante un límite automático. Los modelos también
  difieren en sensor y lente. Compartir plataforma sigue siendo una hipótesis.
  [Comparación de manuales](docs/research/web-research.md#4-controles-de-fábrica)
- Una CX580V desmontada muestra la placa **VC-653**. Los manuales de la
  PJ260/XR260 documentan esa familia de placa. La HXR-NX70 sirve de referencia
  para el control independiente y sus clips permiten medir la exposición con
  ExifTool. [Cámaras emparentadas](docs/research/web-research.md#6-cámaras-emparentadas)

## Sistema

Leído en modo updater el 2026-11-26, sin escribir en la cámara.

- Linux `2.6.29.6-rt24-alp_nl`, compilado el 29 de septiembre de 2011, `ARM-CXD4132`, tres núcleos ARM11 MPCore.
- Flash Samsung OneNAND de 256 MiB, bloques de 128 KiB. Consola serie `ttyAM0` a 115200 en la línea de arranque.
- El kernel nombra las particiones `nflasha0pN`; los nodos son `/dev/nflashaN`.

| Nodo | Tamaño | Contenido conocido |
| --- | ---: | --- |
| `/dev/nflasha1` | 4 MiB | por identificar |
| `/dev/nflasha2` | 1.5 MiB | FAT de ajustes: `Backup.bin` y `Backup.bak` (437 176 bytes), `DmmConfig.bin`, `.hasp` |
| `/dev/nflasha3` | 10 MiB | partición comprimida LZP |
| `/dev/nflasha4` | 10 MiB | por identificar |
| `/dev/nflasha7` | 9.75 MiB | por identificar |
| `/dev/nflasha8` | 46 MiB | partición comprimida LZP |
| `/dev/nflasha12` | 25 MiB | coincide en tamaño con el volumen `PMHOME` |
| `/dev/nflasha15` | 19 MiB | por identificar |

BusyBox 1.13.4 del updater incluye `md5sum`, `sha1sum`, `dd` y `tar`, así que cada volcado se coteja con un hash calculado en la cámara.

## Alimentación por DC IN

El updater exige adaptador de corriente y la unidad no tiene el AC-L200. Se alimentó con una fuente de banco soldada a los cables que van de la toma a la placa.

| Cable | Función | Conexión |
| --- | --- | --- |
| Rojo (pin central) | Entrada positiva | Fuente `+`, 8.4 V |
| Negro | Tierra | Fuente `−` |
| Azul | Detección de clavija | A tierra a través de 10 kΩ |

Con rojo y negro la cámara enciende sin batería, pero la guarda del updater sigue respondiendo «AC adapter required». Al llevar el azul a tierra con 10 kΩ la guarda pasa. Resistencias medidas sin alimentación: tierra–central 146 kΩ, central–azul 92 kΩ, tierra–azul 238 kΩ. [Más detalle y esquema de referencia](docs/research/web-research.md#alimentación-externa-y-toma-dc-in)

## Conexión USB

Linux registró errores de enumeración `-71` en dos puertos mientras el LCD
mostraba «Preparing…» en «USB Connect». Al volver a probar, apareció
`054c:05fc` como dispositivo Mass Storage. Linux montó `PMHOME`, un volumen de
25 MiB, en solo lectura. La otra unidad USB informó medio ausente.

1. Encender la cámara (abrir el LCD), con batería cargada.
2. Tocar "Conexión USB" si aparece la pantalla de selección.
3. `MENU → Configuración → Conexión`: conexión USB = Mass Storage, USB LUN = Multi.
4. Si se repite el fallo, probar otro puerto directo o la toma mini-USB de la cámara.
5. Comprobar la enumeración con `lsusb -d 054c:`.

## Uso (en este orden)

```bash
.venv/bin/python -m unittest discover -s tools      # pruebas sin cámara
.venv/bin/python tools/cxprobe.py usb               # ¿se ve la cámara?
.venv/bin/python tools/cxprobe.py info --unmount    # modelo, serie, versión (solo lectura)
.venv/bin/python tools/cxprobe.py guess-model --unmount   # ID de modelo/región (solo lectura)
.venv/bin/python tools/cxdump.py recon --model 0x...      # reinicia a modo updater, solo lee
.venv/bin/python tools/cxdump.py dump  --model 0x...      # respaldo verificado en backups/
.venv/bin/python tools/cxdump.py verify backups/<dir>
```

`recon` y `dump` reinician la cámara al modo actualizador de Sony y cargan el
shell de PMCA en lugar de flashear; la sección de firmware del payload va vacía.
Probado en esta unidad: entra, responde y regresa sola al modo normal.

## Seguridad

- Ningún flasheo sin: volcado completo verificado, copia fuera de este disco y
  un camino de recuperación comprobado. Eso requiere confirmación explícita.
- `cxdump.py verify` comprueba los tamaños y SHA-256 en disco. La integridad
  local de los archivos por sí sola no acredita un respaldo completo ni un
  mecanismo de recuperación.
- Si una sesión falla a medias, `cxdump.py` igualmente cierra el shell para que
  la cámara reinicie en vez de quedarse en el updater.

## Cambios hechos en este equipo

- `/etc/udev/rules.d/70-cx580-sony-camera.rules`: da acceso libusb al usuario
  para dispositivos Sony (`054c`). Para revertir: borrar el archivo y
  `sudo udevadm control --reload-rules`.

## Estructura

- `tools/cxlib.py` construcción de payloads; `tools/cxprobe.py` reconocimiento;
  `tools/cxsession.py` sesión y volcado verificado; `tools/cxdump.py` CLI.
- `tools/cxexposure.py` lee exposición, balance de blancos y foco en MPG
  originales de la HDR-CX580V y en AVCHD mediante ExifTool.
- `captures/exposure-test/` cuatro originales MPG y fotos de Data Code.
- `captures/color-focus-low-lux/` segunda serie de originales.
- `vendor/` clones de terceros sin modificar.
- `backups/20261126-recon/` recon del sistema. `backups/20261126-dump/` respaldo
  verificado con `manifest.json`. `docs/research/` investigación.
- `firmware/cx580v-1.07/unpacked/` particiones extraídas del respaldo con fwtool.
- `ui/menu-prototype.html` prototipo interactivo del menú propuesto (`docs/ui-spec.md`).

## Siguientes pasos

1. Revisar `docs/ui-spec.md`: reparto de los ~90 ítems del menú de fábrica en
   las pestañas Exposición, AF, Formatos, Audio y Media view, más lo que se
   oculta. Decisiones abiertas: pestaña «Ajustes», ubicación del balance de
   blancos, Smth Slw REC y Cinematone.
2. Confirmar en la cámara que ese inventario coincide con su menú real.
3. Analizar el firmware extraído en `firmware/cx580v-1.07/unpacked/` para
   ubicar menús y control de exposición. Primera pasada por cadenas en
   [docs/research/firmware-analysis.md](docs/research/firmware-analysis.md):
   las pantallas de iris, obturación y AGC Limit ya vienen en el firmware.
   Falta el desensamblado.
4. Probar primero cambios reversibles (propiedades de Backup) antes de
   plantear cualquier parche de firmware, y sólo con un mecanismo de
   recuperación comprobado.
5. Toolchain ARM (Linaro 4.9 `arm-linux-gnueabi`) para compilar código propio.
