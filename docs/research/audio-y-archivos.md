# Audio propio, archivos grandes y memoria desde `im.elf`

27 de noviembre de 2026. Análisis estático del firmware 1.07 desempacado
(`firmware/cx580v-1.07/unpacked/`), del volcado `backups/20261126-dump/`, del
listado del updater (`backups/20261126-recon/recon.txt`) y de las bitácoras
`backups/2026112*/cxreq.log`. **Nada de esto se probó en la cámara.**

Marcas: **[código]** lo vi en el binario, en el decompilado o en los datos del
volcado; **[inferido]** se deduce de lo anterior sin verlo directamente;
**[?]** no se pudo determinar.

Direcciones de `libObj.so` en la convención de Ghidra (archivo + `0x10000`),
salvo que diga «archivo». El material de trabajo (decompilados, tabla de
montaje, notas) está en `firmware/ghidra/out/agents/doom/audio/`.

## Resumen

1. **Audio.** No hay ALSA ni OSS: todo el audio lo hace el procesador AV de
   Sony (ADF, dentro de `av-cam.bin`), y el ARM sólo le manda órdenes. No
   encontré ninguna función que reciba un flujo continuo de PCM. Lo más
   cercano son los sonidos del sistema: en cada pitido `libObj.so` abre el
   archivo del sonido con `fopen`, lo copia con `fread` a un búfer físico
   que le da el procesador AV y le pide reproducirlo. Si se cambian esos
   bytes se reproduce lo que uno quiera, de a un disparo por vez. Sirve para
   efectos de Doom; para música continua no parece servir. La música de
   Highlight es AC-3 crudo en una partición propia.
2. **Archivos grandes.** La memoria interna y la SD no están montadas en
   Linux: las maneja dentro de `im.elf` un sistema de archivos de usuario
   (PrFILE de eSOL, `libInfraFsys.so`). Linux sí monta `/log`, que es la
   partición PMHOME (`nflasha12`), con 8.6 MB libres y lectura con `open()`
   normal. Para no escribir nada en la flash, se puede leer la SD cruda en
   sólo lectura.
3. **Memoria y carga.** En modo normal Linux administra unos 57 MiB, según
   la línea de órdenes de `kemco.txt`; cuánto queda libre no se sabe. No
   encontré límites para `mmap`. `dlopen` está disponible dentro de
   `im.elf`, y el propio firmware lo usa con rutas de `/tmp` y `/log`.

## 1. Audio

### No hay dispositivo de audio en Linux

- `/proc/devices` del updater no tiene `sound` (14) ni `alsa` (116), y en
  `/dev` no hay `snd/`, `dsp` ni `mixer` **[código]** (`recon.txt`). Ese
  listado es del kernel del updater.
- El kernel del sistema principal (`nflasha3/vmlinux.bin`, compilado el 24 de
  julio de 2012) no tiene ninguna cadena `snd_`, `ALSA`, `soundcore`,
  `/dev/dsp`, `audio` ni `pcm` **[código]**.
- Entre los módulos del initrd normal (`initrd.img_unpacked/kmod/*.ko`) no hay
  ningún controlador de sonido. El único que menciona audio es `hdmi.ko`, por
  los bloques de audio del EDID **[código]**.
- El ARM controla el audio mandando mensajes al procesador AV.
  `ObjAvbbcCmn::OsalSndMsg` los manda por `libosal_uipc.so` (`/dev/uipc`)
  **[código]**. En la placa el audio pasa por el NN12926A y el DSP CS48L10
  (según el L3, citado en el Barrido) **[inferido]**.

### Cómo suenan hoy los pitidos

Componentes (rutas de compilación dentro de `libObj.so`) **[código]**:
`App-Model/models/beep` (el modelo `model/BEEP`), `models/speaker`
(`model/SPEAKER`), `Avbb-SystemSound` (`ObjSystemSound`, con `systemsound`,
`InfraMemMgrSystemSound`, `Resource`, `Pin` e `InfraAdf`), `Avbb-Speaker`,
`Avbb-Mixer` y `Avbb-Avbbc/AudioDecoder`.

La cadena de un pitido:

1. La interfaz llama `requestModelExecute("model/BEEP", 2, {0x100 = n})`. En
   las bitácoras aparecen `n` = `0xf`, `0x1d`, `8`, `9` y `0x1a`, a veces con
   `k101` = `k102` = 0 **[código]** (`cxreq.log`).
2. `ActionBeepPlayStart` (`FUN_0050fd64`) lee `0x100`, `0x101` y `0x102` del
   `ParamList`. Traduce `n` con `changeBeepId` (`FUN_00510f40`), cuya tabla
   es `n → n − 1` para `n` de 1 a `0x2f` **[código]**. El resultado es el
   `BEEP_ID` de `beeplist.txt`; `0xf`, por ejemplo, es `BEEP_ID_MENU_ON`.
3. Arma `ObjMsg(0x3410, 0x1120)`, que es `MSGID_START_SYSTEMSOUND_CMD`, con
   tres parámetros sin signo (tipo 8): `0x1000` = `BEEP_ID`, `0x1001` =
   `k101` y `0x1002` = `k102`. Lo manda con `MWF::ObjIf::IssueCommandAsync` al
   objeto que devuelve `FUN_004e9d6c(7)` (archivo `0x4d9d6c`) **[código]**.
   La panorámica usa el mismo mensaje con `0x1000` = 5 o 6 (inicio y fin de
   grabación) y `0x1001` = 1 (archivo `0x72d38c` y `0x72d50c`) **[código]**.
4. Otros mensajes de la misma categoría: `0x1160` es `setBeepType` (pitido
   encendido o apagado) y `0x1170` es `setBeepPlayForce` (forzar el sonido)
   **[código]**. `ObjSystemSound` tiene `ExeSetSysSndForced` con
   `PARAMID_SYSTEMSOUND_FORCED_ON` **[código]**.
5. `model/SPEAKER` aparece con la acción 5, clave `0x12008003` = 0 o 1, y con
   la acción 2, clave `0x12008002` = 1 u 8 **[código]**. Parece mudo y volumen
   **[inferido]**; el ADF acepta volumen de 0 a 15 según su ayuda de
   depuración (`av-cam.bin` archivo `0x52ec88`) **[código]**.

### De dónde salen las muestras

**Archivos en `/usr`, indexados por una lista de texto.**

- `/usr/share/avsystemsound/beeplist.txt` lista 46 IDs (`0x00`–`0x2d`), cada
  uno con columnas `BeepID_Name`, `BeepID_Number`, `MemoryType` (APP o PIN),
  `FileName`, `RsrcAcsType` (`AA_RT` o `AA_FIFO`), `Priority` y `KindID`.
  Después de `/* START */` va el directorio base, que hoy es
  `/usr/share/avsystemsound/`. Hay 32 archivos `Cam_*.bin`; 14 IDs dicen
  `None` **[código]**.
- El analizador es `FUN_0044bf8c`, con campos separados por tabuladores y
  números en hexadecimal. La tabla tiene `0x2e` entradas de `0x90` bytes, y
  el directorio y el nombre admiten hasta 128 bytes cada uno **[código]**.
- **Lista alternativa.** `FUN_00432680`, llamada desde
  `ObjSystemSound::onStarting` (`FUN_00430750`), lee `/tmp/beeplist.txt` en
  vez de la de `/usr` si el byte `+0x3b1` de `BackupControl` no es cero.
  `FUN_0042dab8` toma ese byte de la **propiedad de Backup `0x002b001c`**,
  que en esta unidad vale `00` **[código]** (lector `vendor/barrido-fuentes/r3/scripts2/bkread.py` sobre
  `backups/20261126-dump/Backup.bin`). Con esa propiedad en 1 y un
  `/tmp/beeplist.txt` propio, el directorio y los archivos de cada pitido
  podrían ser otros sin tocar código **[inferido]**. Pero `/tmp` es la
  partición `nflasha7` (ver §2), así que hay que escribir en la flash.
- **Cada pitido se lee del archivo en ese momento.** La secuencia de
  petición (`FUN_00437e0c`, `FUN_004395a0`, `FUN_0043a7d4` y `FUN_0043cf98`)
  hace esto **[código]**:
  1. reserva una de 9 ranuras (`FUN_00432b00`);
  2. pide un búfer al procesador AV (`FUN_0043e74c`: destino `0x61003c`,
     secuencia `0x8401`, con el valor `0x70817`);
  3. copia la respuesta (`FUN_0043e920`): `+0x10` es la dirección física y
     `+0x14` el tamaño;
  4. `FUN_0044d3e8` llama a `FUN_004338d4` (`ReadSoundFile`), que mapea esa
     dirección con `AvbbcPhycMem(dir, tamaño, 1)` y `GetLogAddr`, abre
     `directorio + archivo` con **`fopen`** (`FUN_0044bcf4`) y lo copia con
     **`fread`** en trozos de 128 bytes (`FUN_0044be90`);
  5. manda `RequestSound` al ADF (destino `0x58003f`) con el ID, la ranura y
     el número de bytes leídos.

  Que `0x70817` (460 823) sea el tamaño pedido es una suposición **[?]**.
- **Formato de los `.bin`.** No tienen cabecera de archivo y miden múltiplos
  de 32 bytes. Cada bloque de 32 bytes empieza con un byte de valor 0–8 o
  16–24, seguido de 31 bytes con signo que se comportan como muestras
  **[código]** (estadística en las notas). Parece PCM de 8 bits con escala por
  bloque, y con 48 kHz salen tonos y duraciones razonables (Cam_SelfTimer:
  ~2.35 kHz, 65 ms) **[inferido]**. La codificación exacta y la tasa no las
  pude confirmar **[?]**. El decodificador está en el ADF de `av-cam.bin`,
  que tiene las órdenes de depuración `SystemSound SetTable/Request/SetVol/
  RequestPin` y la ayuda `kind [1-10]`, `priority [1-10]` **[código]**.
- En `av-cam.bin` no encontré una tabla de pitidos incorporada; las muestras
  salen de los archivos **[inferido]**.

### La música de Highlight

- Está en la partición **`nflasha15`**: MBR con una FAT12 desde el sector 27,
  carpeta `MUSIC/` con `MUSIC000.IND` (48 bytes, empieza por `SS`) y
  `MUSIC001`–`MUSIC004` **[código]** (mtools sobre el volcado).
- Los cuatro son **AC-3 crudo, 48 kHz, estéreo, 128 kb/s**, de 4.4 a 4.6 MB
  y unos 4 min 48 s cada uno **[código]** (`file` y `ffprobe`). El índice no
  lo decodifiqué; el 4 del byte 4 sería la cantidad de pistas **[inferido]**.
- `libInfraFsys.so` la nombra «One Nand MUSIC» (`FSYS_DTYPE_ONAND_MUSIC` en
  `libObj.so`). `libmpr.so` la monta con su propio sistema de archivos y la
  lee con `BgmMMgr`, `LoadBgm`, `MUSIC000.IND` y «MUSIC Folder Not Found»
  **[código]**. `libObj.so` contempla `MUSIC001`…`MUSIC008` **[código]**, así
  que caben hasta 8 pistas **[inferido]**.
- **Download Music** es un modo USB: `viewUsbBgmDownload_L.so` y
  `MSGID_DISCONNECT_BGM_USB_MEDIA_STATUS` **[código]**. Expondría la
  partición al PC, como hacía «Music Transfer» de Sony **[inferido]**.
  **Empty Music** borra todo («Delete all slideshow music», «Deleting BGM
  data») **[código]**.
- Quién la reproduce: `MprIf::PlayHighlightBgm(MPR::EntryId,
  PlayHighlightBgmCtrl*)` en `libmpr.so` **[código]**, que pasa por
  `InfraAudioDecoder` (`SeqBgmOpen/Start/Stop/Close/ChgSpeed`, conexión de pin
  al mezclador) hasta el códec del procesador AV (`CID_CODEC_BGM_OPEN/START`
  con `IPCM CmdParamSMAddress`, una dirección de memoria compartida) **[código]**.
  El ARM entrega el AC-3 en memoria y el procesador AV lo decodifica
  **[inferido]**.

### Rutas para sonido propio

**A. Disparos por la ruta de los pitidos (la más viable).**

1. Disparo: lo más simple es hacer lo mismo que la interfaz,
   `ViewBase::requestModelExecute("model/BEEP", 2, {0x100 = BEEP_ID + 1})`,
   que `cxg.so` ya sabe llamar. La otra forma es mandar `ObjMsg(0x3410,
   0x1120)` con `0x1000` = `BEEP_ID` por `IssueCommandAsync`, usando el
   `ObjIf*` que se capture de la primera llamada de categoría `0x3410` que
   pase por la envoltura que ya existe **[inferido]**.
2. Contenido: `libObj.so` no tiene `GNU_RELRO` ni `BIND_NOW`, así que su GOT
   se puede escribir **[código]**. Las ranuras de `fopen` y `fread` están en
   el archivo `0x10e34a8` y `0x10e363c` (`R_ARM_JUMP_SLOT`) **[código]**.
   Cambiarlas desde `cxg.so`, sólo en `im.elf`, permite reconocer el nombre
   `Cam_*.bin` elegido y entregar nuestros bytes desde RAM **[inferido]**. No
   hace falta escribir en la flash ni tocar Backup.
3. Límites: el tamaño por disparo es el del búfer del procesador AV **[?]**;
   hay 9 ranuras y prioridades de 1 a 10; no sé si los sonidos se mezclan o
   uno corta al otro **[?]**. Cada disparo cuesta un viaje de ida y vuelta al
   procesador AV más la copia, una latencia que no conozco **[?]**. El
   formato hay que descubrirlo en la cámara.
4. Si el pitido está apagado en el menú, `0x1170` (forzar) debería
   saltárselo **[inferido]**.

**B. Música AC-3 por Highlight.** Se puede poner música propia por la vía de
fábrica (Empty Music y Download Music) **[inferido]**. Reproducirla desde un
juego obligaría a montar la cadena de `BgmMMgr`, `PlayHighlightBgm` y la
conexión al mezclador fuera de la reproducción de Highlight, y no sé si
funciona en el modo de grabación **[?]**. Sólo sirve para pistas codificadas
de antemano.

**C. Hablar directo con el ADF por UIPC.** El ADF tiene `SystemSound
RequestPin`, que en el texto dice «underconstruction», y ruta `Pin` para el
sonido del sistema **[código]**. No encontré ninguna orden que acepte un
búfer de PCM desde el ARM **[inferido]**. Lo descarto.

**Opinión franca.** Sonido propio, sí: disparos de efectos por la ruta A es
realista, a cambio de averiguar en la cámara el formato, la tasa y el tamaño
máximo. Audio de juego mezclado en continuo con baja latencia no lo veo: no
hay una API de flujo, y encadenar disparos dejaría huecos. Doom con efectos
sueltos y sin música es lo esperable. La música AC-3 de Highlight es un
experimento aparte, con pocas probabilidades.

### Qué probar en la cámara

1. Ampliar la bitácora de `IssueCommandAsync/Sync` a la categoría `0x3410`,
   tocar el menú y comprobar `0x1120` con `0x1000` = `BEEP_ID`.
2. Hacer el cambio de GOT de `fopen` sólo para `Cam_Tap.bin` y entregar el
   mismo archivo con los bytes cambiados: un tono puro con cabeceras de
   bloque iguales y con distintas tasas supuestas. Escuchar o grabar la
   bocina para fijar formato y tasa.
3. Registrar el tamaño de búfer que devuelve `FUN_0043e920` (`+0x14`) y
   entregar archivos cada vez más largos.
4. Disparar dos IDs seguidos para ver si se mezclan o se cortan, y medir la
   latencia.

## 2. Archivos grandes

### La memoria interna y la SD no están montadas en Linux

- `init` (`nflasha3`, `/sbin/init`) sólo monta, en modo normal: `/proc`,
  `/sys`, `/system` (`dnflasha3`, sólo lectura), `/setting` (`nflasha2`),
  **`/tmp` (`nflasha7`)**, `/usr` (`dnflasha8`, ext2 de sólo lectura, con
  `cramfs` de respaldo) y **`/log` (`nflasha12` por `/dev/loop5`)**. La
  tabla está en `.data`, en `0x12ba4`; el detalle va en las notas
  **[código]**. Ninguna entrada usa `mmc*`.
- En `libObj.so` sólo hay dos `mount()`, los dos del código del updater
  (`/dev/ms1` y `/dev/nflasha1`) **[código]**. `libmpr.so`,
  `libInfraFsys.so` y `libInfraMediaCommon.so` no importan `mount`; abren
  los dispositivos con `open64/read/lseek64/mmap64` **[código]**.
- Los medios los maneja **`libInfraFsys.so`**, una versión de PrFILE de eSOL
  (`Fsys_PrFILE_*`, `PFCACHE_*`, `PFDIR_EXFAT_*`; FAT16, FAT32 y exFAT)
  **[código]**. Se carga en `im.elf` porque `libObj.so` la necesita, y
  `libObj.so` y `libmpr.so` llaman `Fsys_Mount`, `Fsys_Open` y `Fsys_Read`
  **[código]**. Las rutas usan letra de unidad (`D:\@`, `\AVCHD\...`)
  **[código]**. Conoce `/dev/mmca*`, `/dev/mmcb*`, `/dev/mmcc*`,
  `/dev/nflasha12-15`, «One Nand MUSIC», «One Nand PMB» y «One Nand NORMAL»
  **[código]**.
- `mmc_drv.ko` registra discos del bloque genérico (`udif_bdev_add_disk`,
  `elv_next_request`), así que `/dev/mmca*` y `/dev/mmcb*` son dispositivos
  de bloque normales **[código]**. Un `open(O_RDONLY)` desde `im.elf`
  debería funcionar **[inferido]**. Que `mmca` sea la interna y `mmcb` la
  SD viene del encargo; no lo verifiqué **[?]**.
- `/proc/mounts` en modo normal no está en ningún respaldo **[?]**.

### Lo que sí está montado y tiene espacio

| Lugar | Partición | Tipo | Libre en el volcado | Comentario |
| --- | --- | --- | --- | --- |
| `/log` | `nflasha12` (PMHOME), loop desde el LBA 3 | FAT16, lectura y escritura | **8 617 984 B** | `PMHOME.EXE` (17 MB, instalador de PlayMemories) más licencias. También es la unidad PMHOME de solo lectura que ve el PC **[código]** |
| `/tmp` | `nflasha7` | FAT12, lectura y escritura | 1 736 704 B | Ahí están las imágenes de la interfaz (`/tmp/images/*.uxc`, según `global.xdb`) y `kikilog.dat` **[código]** |
| (no montada) | `nflasha15` (MUSIC) | FAT12 desde el sector 27 | 1 802 240 B, unos 19.7 MB tras Empty Music | La maneja Fsys **[código]**; en crudo se lee desde `/dev/nflasha15` **[inferido]** |
| `/setting` | `nflasha2` | FAT | ~448 KiB | Por debajo de 200 KiB libres, `init` corre `dosfsck -Y` **[código]**. No sirve |

Las opciones de montaje son `0x490` (`noatime`, `dirsync`, `sync`), sin
`noexec` **[código]**. Linux también escribe en `/log`: `libsencore.so` usa
`/log/escapelog.bin` y `/log/ulogio_*.txt` **[código]**.

### Opciones, de menos a más riesgo para los datos del usuario

1. **Leer la SD en crudo, en sólo lectura (no escribe nada en la cámara).**
   `open("/dev/mmcb1", O_RDONLY)` más una de dos cosas: un lector mínimo de
   FAT32 y exFAT, o un archivo contiguo con su LBA y su suma calculados en
   el PC y anotados en `cxmode.txt` **[inferido]**. Riesgos: Fsys usa el
   mismo dispositivo y la capa de bloques ordena las peticiones, así que leer
   no daña nada **[inferido]**. Pero puede haber datos viejos si la cámara
   escribió el archivo hace poco, y no sé qué hace `mmc_drv` con la energía
   de la tarjeta en reposo (`mcmnd_api_setdrvpwr`) **[?]**. Hay que leer
   sólo sin grabar y cargar el archivo de una vez en RAM.
2. **Montar la SD con Linux en sólo lectura** (`mount("/dev/mmcb1", "/mnt",
   "vfat", MS_RDONLY)`; `/mnt` existe en el initrd) **[código]**. Sólo sirve
   con FAT32, porque este kernel no tiene exFAT y las SDXC traen exFAT
   **[inferido]**. `mount` pide el dispositivo en exclusiva; si Fsys lo abre
   con `O_EXCL` alguno de los dos falla **[?]**. Lo veo peor que la opción 1.
3. **API de Sony (`Fsys_*`).** Exportadas por `libInfraFsys.so`, con
   `DT_HASH`, así que `cx_dlsym` las encuentra **[código]**. Firmas, sacadas
   de sus mensajes de traza: `int Fsys_Open(const char *ruta, void **handle,
   uint modo)`, `int Fsys_Read(void *handle, void *buf, int n)`,
   `Fsys_Seek(handle, long long desplazamiento, int origen, long long
   *posición)` y `int Fsys_Close(void *handle)`; los errores son negativos
   `0x8000xxxx` **[código]**
   (`fsys-api.c`). Falta saber qué letra tiene cada medio
   (`Fsys_GetMountDrives`, `Fsys_GetDriveLetter`) **[?]**. Comparte
   cerrojos y caché con la grabación: usarla sólo para leer y sin grabar.
4. **`/log` (PMHOME).** `open()` normal desde `im.elf`, sin lector propio.
   Caben el binario de 600 KB y una WAD de hasta unos 8 MB; `doom1.wad`
   ocupa unos 4.2 MB. Con `PMHOME.EXE` fuera habría unos 25 MB
   **[inferido]**. Escribirla es escribir `nflasha12`: hay respaldo
   verificado (`backups/20261126-dump/nflasha12.bin`, sha256 `ace24b29…`). El
   camino más limpio sería armar la imagen en el PC sobre una copia del
   volcado (mtools), escribir la partición entera desde el updater y
   comprobar el md5 en la cámara; para volver, se escribe el volcado
   **[inferido]**. El `busybox` del updater no trae `losetup` **[código]**, y
   no sé si su `mount` acepta `offset=` **[?]**. No toca los datos del
   usuario.
5. **Partición MUSIC.** Con Empty Music y Download Music (rutas de fábrica)
   quedarían unos 19.7 MB en una FAT12 que se lee en crudo **[inferido]**.
   No sé si el modo USB acepta archivos que no sean AC-3, ni qué hace
   Highlight con ellos **[?]**. Respaldo: `nflasha15.bin` del volcado y los
   cuatro AC-3 originales.
6. `/tmp` (`nflasha7`): el binario cabe, pero ahí están las imágenes de la
   interfaz. No lo recomiendo.

Queda descartado cualquier camino que escriba en `mmca` o `mmcb`.

### Qué probar en la cámara

1. Desde `cxg.so`, copiar a `/setting` el contenido de `/proc/mounts` y el
   resultado de `statfs("/log")` y `statfs("/tmp")`.
2. Leer el sector 0 de `/dev/mmca`, `/dev/mmcb` y `/dev/mmcb1`, sólo
   lectura, para saber cuál es la SD y con qué sistema de archivos.
3. Leer de `/dev/mmcb1` un archivo contiguo de prueba en distintos estados
   (espera, menú, reproducción) y medir la velocidad.

## 3. Memoria y carga de código

### RAM

- Las cifras de `recon.txt` (`MemTotal` 39 416 kB, `MemFree` 23 076 kB)
  son del **updater**, con otra línea de órdenes (`mem=512M`,
  `memrsv=448M@0x84000000`) **[código]**. No sirven para el modo normal.
- La línea de órdenes del modo normal está en `/setting/kemco.txt`
  **[código]**:
  - `mem=80M@0x80000000`;
  - reservas de 32 KiB, `0xE80000` en `0x82D90000` (14.5 MiB),
    `0x480000` en `0x84B80000` (4.5 MiB) y `0x340000` en `0x80408000`
    (3.25 MiB, la raíz AXFS);
  - 44 secciones `secgrp` de 4 MiB, de `0x85000000` a `0x8FFFFFFF`
    (176 MiB), que arrancan `offline`.
- Así, Linux arranca con unos **57.7 MiB** (80 MiB menos 22.3 de reservas),
  y algo menos tras restar el kernel y `mem_map` **[inferido]**.
  `dmm.ko` (`DmmConfig.bin`, `dmm_ctrl_HotplugHdlr`, «online», «offline»)
  puede poner en línea parte de esos 176 MiB según el modo **[inferido]**;
  si lo hace, y cuándo, no se sabe **[?]**.
- **La memoria libre en modo normal no está medida [?].** En `im.elf`
  residen `libObj.so` (18.8 MB de texto), `libmpr.so` (9 MB), las vistas y
  el montón de la aplicación. Las páginas de texto vienen de `/usr` (ext2)
  y se pueden desalojar.

### `mmap` de 8 a 16 MB

- No hay swap **[código]**: `SwapTotal: 0` en el updater, y ni `init`, ni
  `bootin.elf`, ni las bibliotecas de `/usr` mencionan `swapon`.
- Ni `im.elf`, ni `bootin.elf`, ni `libObj.so`, `libMWF.so`, `libosal_*`
  o `libmpr.so` importan `setrlimit`, `mlock` ni `mlockall` **[código]**.
  El único ajuste de `/proc/sys` en `init` es `max_map_count`, y sólo en el
  modo de depuración DUMA **[código]**. `overcommit_memory` queda en su
  valor por omisión, 0 **[inferido]**.
- `init` exporta `MALLOC_OPTIONS=gfffffffff` (el `malloc` de NetBSD de
  `libsonyc`) **[código]**; un `mmap` anónimo directo no pasa por ahí.
- Conclusión **[inferido]**: un `mmap` anónimo de 8 MB no debería fallar. El
  riesgo está al tocar las páginas: si falta RAM, el OOM killer mataría el
  proceso más grande, que es `im.elf`, y la guarda llevaría al updater en el
  siguiente encendido. 16 MB es arriesgado mientras no se mida. Conviene
  leer `/proc/meminfo` antes de dimensionar la zona de Doom.

### `dlopen` de una biblioteca propia

- `im.elf` necesita `libsonydl.so` **[código]**, que exporta `dlopen`,
  `dlsym`, `dlerror`, `dlclose` y `dladdr`, envolturas de `__dlopen` y demás
  en `sonyld.so` **[código]**. `sonyld.so` deriva de `ld.elf_so` de NetBSD
  (`$NetBSD: rtld.c,v 1.123`), pide rutas absolutas y no tiene ninguna
  verificación de firma **[código]**.
- El firmware ya lo hace en marcha: `libObj.so` llama
  `dlopen("/tmp/gpslogdiag.so")` y `dlsym("get_instance")` (`FUN_008b1c9c`),
  y también `dlopen("/log/libInfraMapEngine.so")` según un bit de
  configuración (`FUN_00bd9594`) **[código]**. `im.elf`, `libObj.so` y
  `libMWF.so` importan `dlopen` **[código]**.
- **Símbolos.** `libsonydl.so`, `libsonyc.so`, `libsonypthread.so` y
  `sonyld.so` sólo traen `DT_GNU_HASH` **[código]**, y `cx_dlsym`
  (`preload/cxdl.c`) sólo entiende `DT_HASH`. Tres salidas, sin probar
  **[inferido]**:
  1. agregar a `cx_dlsym` la búsqueda por GNU hash, unas 30 líneas;
  2. declarar en `cxg.so` `dlopen` y `dlsym` como externos débiles para que
     `sonyld` los resuelva al cargar; en procesos sin `libsonydl.so` quedan
     en 0;
  3. llamar por la PLT de `libObj.so`. La ranura de GOT de `dlopen` está en
     el archivo `0x10e2ef8` y la de `dlsym` en `0x10e2ef0`, pero con enlace
     perezoso pueden no estar resueltas todavía **[?]**.
- **La biblioteca cargada.** `libsonyc.so` es una libc de NetBSD adaptada
  (`$NetBSD: ...`, `__errno`, `__sF` como `stdin/stdout/stderr`) **[código]**.
  Exporta `malloc`, `free`, `realloc`, `calloc`, `mmap`, `munmap`, `open`,
  `read`, `pread`, `lseek`, `fopen`, `fread`, `printf`, `snprintf`,
  `vsnprintf`, `sscanf`, `qsort`, `strtol`, `atoi`, `toupper`,
  `strcasecmp`, `getenv` y `exit` **[código]**. `libsonypthread.so` exporta
  `pthread_create`, `pthread_attr_setstacksize` y `pthread_setschedparam`
  **[código]**. Compilar con cabeceras de glibc y enlazar contra
  `libsonyc` sólo es seguro si no se usan macros ni estructuras de stdio,
  `errno` como variable ni `struct stat` **[inferido]**. Tampoco sirve una
  libc estática como musl: su arranque cambia el registro de TLS del hilo y
  su `malloc` usa `brk`, igual que el de `libsonyc` **[inferido]**.
- **Alternativa.** Un binario plano propio cargado con `mmap`
  (`PROT_READ|PROT_EXEC` sobre `/log` o `/setting`, que no son `noexec`)
  **[código]**, con una libc mínima de llamadas al sistema como la de
  `cxg.so` y un asignador sobre un `mmap` anónimo. Es más trabajo pero no
  depende de la ABI de `libsonyc`. Si hace falta `printf` o `qsort`, se
  piden a `libsonyc` por puntero.
- **Hilos.** Si Doom corre en un hilo propio, conviene crearlo con
  `pthread_create` de `libsonypthread`, que prepara el TLS; un hilo de
  `clone()` no tiene TLS (ver `firmware-analysis.md`). La otra opción es
  correr cada tic en el hilo de la interfaz, que `getModelInfo` ya visita
  unas 80 veces por segundo **[inferido]**.

### Qué probar en la cámara

1. Copiar a `/setting` `/proc/meminfo` y `/proc/self/status` (`VmRSS`,
   `VmSize`) de `im.elf` en espera, en el menú y en reproducción.
2. Hacer `mmap` anónimo de 8 MB, tocar todas las páginas y volver a medir.
3. `dlopen` de una biblioteca de prueba en `/setting` que sólo escriba un
   archivo, resuelto por la salida 1 o la 2.

## Material

- `firmware/ghidra/out/agents/doom/audio/notas-trabajo.md`: tabla de montaje
  de `init`, contenido de las particiones y estadística de los `.bin`.
- `*.c` del mismo directorio: decompilados citados arriba. Los de
  `libObj.so` salen de una copia del proyecto `firmware/ghidra/proj`; los de
  `libInfraFsys.so` (`fsys-api.c`), de un proyecto aparte en `/tmp`.
