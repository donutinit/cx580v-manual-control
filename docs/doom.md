# DOOM desde el menú de la cámara

Estado al 2026-11-27: **DOOM funciona en la cámara** con imagen, controles,
efectos de sonido y música OPL («At Doom's Gate» en el título). La imagen sólo
se ve mientras nuestro panel está activo. Los sonidos salen por la ruta de los
pitidos del sistema, que la cámara reproduce a 24 kHz. Resultados al final, en
«Pruebas en la cámara».

## Qué es

El motor es [doomgeneric](https://github.com/ozkl/doomgeneric) (GPL-2), fijado
en el commit `e257a095…`, con el WAD de la versión shareware 1.9
(`doom1.wad`, md5 `f0cefca4…`, de distribución libre). No hay sonido: la cámara
no tiene una salida de audio continua que se pueda usar desde Linux.

Son dos piezas:

- **`CXDOOM.BIN`** (`doom/`): el juego como programa aparte, 388 KiB. Es una
  imagen plana seguida de su tabla de reubicación (4 659 palabras que
  `doom/mkblob.py` saca de los registros de `ld -q`), para cargarla en
  cualquier dirección sin compilarla como código independiente de la posición.
  No usa la libc de la cámara: trae la suya
  (`doom/libc.c`, llamadas al kernel directas, montón propio, `printf`) y una
  pila propia de 256 KiB. Los archivos sólo se abren para leer, así que no
  guarda configuración ni partidas y no escribe en la flash. `exit()` no
  termina el proceso (que es la interfaz de la cámara): vuelve a quien lo llamó.
- **El anfitrión** (`preload/cxdoom.c`, dentro de `cxg.so`): carga la imagen,
  le da los cuadros, le pasa tacto y botones y lo conecta con el menú. Desde la
  v34 el juego corre en un hilo propio (prioridad normal, `nice` 10) que deja
  cada cuadro terminado en uno de tres búferes; el hilo de la interfaz sólo
  toma el último, lo copia al bitmap sin esperar y avisa al sprite. Las teclas
  van del hilo de la interfaz al del juego por una cola.

La interfaz entre los dos es `preload/cxd.h`: una cabecera de 64 bytes (marca,
versión, dirección de enlace, tamaños, suma, entradas de la tabla) y tres
entradas, `init`, `tick` y `key`.

## Dónde vive

En la partición de PlayMemories (`nflasha12`), que el `init` de la cámara monta
en `/log` en modo normal: `/log/CXDOOM.BIN` y `/log/DOOM1.WAD`, 4.6 MB de los
8.6 MB libres. En modo updater esa partición no está montada y el busybox no
trae `losetup`, así que `tools/cxpm.py` sube un ayudante de 2 KiB
(`tools/cxloop.c`) a la memoria, asocia `/dev/loop1` a partir del sector 3 y
monta con el `mount` de siempre. Antes de escribir saca una copia verificada de
la partición entera a `backups/<fecha>-doom-install/`, y después de escribir
vuelve a montar para comprobar el md5 de cada archivo.

```bash
doom/build.sh                                                  # compila doom/build/CXDOOM.BIN
.venv/bin/python tools/cxdump.py pm-check --model 0x21010001   # monta de sólo lectura y lista
.venv/bin/python tools/cxdump.py doom-install --model 0x21010001 --yes
.venv/bin/python tools/cxdump.py doom-remove  --model 0x21010001 --yes
# o todo en una sola entrada al updater:
.venv/bin/python tools/cxdump.py guard-install --model 0x21010001 --yes --doom \
    --library preload/release/cxg-v31.so --config preload/release/cxmode-v31p.txt
```

El WAD va en `vendor/doom/wad/doom1.wad` (no está en el repositorio).

## Cómo se lanza

`cxmode.txt` lleva `doom 1`, `doomview view/USB_OTDISC`, `show bb980 42` y
`text 250c DOOM`: el ítem «Disc Burn» de Setup → Connection se llama DOOM. Al
tocarlo, la pantalla de Disc Burn no se abre; el submenú se cierra a sí mismo
(la misma llamada que hace su botón de cerrar), medio segundo después se cierra
el menú principal y, cuando vuelve a verse la pantalla de grabación, arranca el
juego. Tiene que ser ahí porque es donde el hilo de la interfaz da turnos unas
80 veces por segundo. Si el menú no se cierra solo, el juego arranca cuando lo
cierra el usuario. Con `doom 2` también arranca con un toque en la esquina de
arriba a la derecha (si no está la sonda, que usa esa esquina).

Antes de cargar se comprueba que haya memoria libre de sobra (la zona del juego
más 8 MiB); si no, no arranca y lo anota. Mientras carga se ven las franjas de la sonda:
si aparecen y luego no cambian, el cuadro está pero el juego no le llega.

## Controles

| Control | En el juego |
| --- | --- |
| Palanca de zoom hacia T / W | avanzar / retroceder (en los menús, subir / bajar) |
| PHOTO a medias o a fondo | disparar |
| START/STOP | usar, abrir puertas |
| Ver imágenes | Esc: el menú del juego |
| MODE | salir a la cámara (el juego queda en pausa y sigue donde estaba) |
| Pantalla, tercio izquierdo / derecho | girar |
| Pantalla, centro arriba | Enter (y «y» cuando el juego pregunta) |
| Pantalla, centro en medio | disparar |
| Pantalla, centro abajo | arma siguiente |

Mientras el juego está en pantalla esos botones y el tacto no le llegan a la
cámara: START/STOP no graba. El botón de encendido y cerrar la pantalla sí
pasan. «Quit Game» en el menú del juego lo termina y devuelve la cámara; la
siguiente vez se carga de nuevo. Si se abre cualquier otra pantalla de la
cámara, el juego sale solo.

## Configuración

```
doom 1                 disponible desde el menú (necesita panel)
doom 2                 además, con un toque en la esquina de arriba a la derecha
doomview view/X        la pantalla de fábrica que lo lanza
doommb N               MiB para la memoria del juego (6; de 3 a 12)
doompix R G B A        dónde van los 4 bits de rojo, verde y azul, y los bits opacos (12 8 4 f)
doomwait 0             no esperar el bitmap del cuadro si está ocupado (se pierde ese turno)
probe 1                la sonda (probe 2: otra forma de avisar que el cuadro cambió;
                       probe 3: sólo avisa en los dos primeros cuadros)
```

## La sonda (v31)

Con `probe 1`, al arrancar se anotan `/proc/mounts`, `/proc/meminfo` y el
espacio de `/log`. Un toque en la esquina de arriba a la derecha de la pantalla
de grabación:

1. pide 8 MiB y los va usando mientras sobre memoria, y anota cuántos pudo;
2. lee 1 MiB de `/log/PMHOME.EXE` y anota cuánto tardó;
3. pide una página ejecutable, escribe dos instrucciones y las corre (en la
   v31 la pedía en `0x70000000` y por eso no llegó a correrlas);
4. muestra el cuadro de prueba: ocho franjas verticales (si el formato es el
   que se cree: rojo, verde, azul, blanco, negro, transparente, gris a medias y
   transparente) con una barra blanca que avanza. Otro toque lo quita.

Cada paso queda en la bitácora antes de hacerse (líneas `PR`), para saber dónde
se quedó si la interfaz se cae.

## Pruebas fuera de la cámara

- `doom/test/host.c`: hace de anfitrión en QEMU (`-cpu arm11mpcore`), corre 700
  cuadros con teclas y vuelca cuadros; `doom/test/raw2png.py` los pasa a PNG.
  Se vio la portada, la demo, el menú y una partida nueva en E1M1.
- `preload/test/run.sh`: con `CXDOOM.BIN` y el WAD presentes, `cxtest doom`
  recorre todo con el juego de verdad: lanzarlo desde el menú falso, cuadros,
  botones y tacto que no pasan a la cámara, salir con MODE, «Quit Game» y
  volver a cargar.
- `tools/test_cxpm.py`: el orden de la instalación en la partición (mirar,
  copia de respaldo, escribir, releer) con una cámara falsa. El ayudante
  `cxloop` no se pudo probar fuera de la cámara más allá de sus errores.

## Pruebas en la cámara

### v31, 2026-11-27 (bitácora en `backups/20261127-174852-dumps/`)

- **Instalación.** `guard-install --doom` escribió los dos archivos en la
  partición de PlayMemories y los releyó con el mismo md5 tras volver a
  montar. El ayudante `cxloop` funcionó a la primera. Copia previa de la
  partición en `backups/20261127-174319-doom-install/`.
- **Dibujo.** El cuadro de prueba se ve: el dueño vio las franjas y la barra
  avanzando «fluida con un poco de tearing». La imagen de 320×200 tiene 320
  píxeles por fila (sin relleno). Falta que el dueño diga los colores para
  confirmar el orden de los canales.
- **Ritmo.** Salieron unos 11 cuadros por segundo. El hilo de la interfaz da
  turnos en ráfagas, unas 16 veces por segundo (unas 65 llamadas a
  `getModelInfo` por segundo, en grupos), y uno de cada tres intentos de
  bloquear el bitmap sin esperar falló. Por eso la v32 espera el bitmap.
- **`/log`.** Montada como decía el análisis (`/dev/loop5`, vfat, lectura y
  escritura, `sync`). Leer 1 MiB de `PMHOME.EXE` tardó unos 140 ms.
- **Memoria.** Con la cámara en reposo hay 1.5 MB libres y 34 MB de caché; al
  pedir y usar 8 MiB la caché bajó 8 MB y no pasó nada más.
- **Dirección.** `mmap` con pista en `0x70000000` devolvió `0x40066000`: esa
  zona está ocupada. Fue lo único que impidió arrancar el juego («la direccion
  no esta libre» tres veces en la bitácora).
- **Menú.** Tocar el ítem DOOM hizo lo esperado dos veces: la pantalla de Disc
  Burn no se abrió, el submenú se cerró, un segundo después se cerró el menú
  principal (o sea que en el menú principal sí hay turnos) y volvió la pantalla
  de grabación con el panel.
- **Un cruce.** La pantalla de conexión USB tiene un botón Disc Burn que abre
  la misma vista; la v31 también lo interceptaba. La v32 sólo intercepta con
  un submenú abierto.

### v32, 2026-11-27 (bitácora en `backups/20261127-180411-dumps/`)

- **Avisar es necesario.** Con `probe 3` (sólo se avisa al sprite en los dos
  primeros cuadros) las franjas se ven pero la barra no se mueve: la pantalla
  no relee el bitmap por su cuenta.
- **Colores.** El dueño confirmó el orden de las franjas: el formato es R, G, B
  y alfa de 4 bits, de alto a bajo, con alfa 0 transparente.
- **El juego arrancó.** Programa reubicado en `0x46a00000`, `init` en 548 ms, y
  445 turnos en 15 s (unos 30 por segundo) hasta que se salió con MODE. Una
  segunda vez, 118 turnos hasta apagar. En pantalla no se vio nada.

### v33, 2026-11-27 (bitácora en `backups/20261127-182912-dumps/`)

- El juego pintaba en un búfer propio y sólo bloqueaba el bitmap para copiar.
  Corrió 1 024 turnos en 34 s, con 1 023 cuadros copiados. El bitmap **nunca**
  estuvo ocupado, el cuadro nunca quedó tapado por otro widget y su modo era
  el normal; aun así no se vio. Con la sonda de la v31, en cambio, uno de cada
  tres intentos encontraba el bitmap ocupado: la pantalla sí lo leía.
- Los botones sí le llegaron al juego y no a la cámara. El dueño cuenta que
  tuvo que salir con MODE para poder apagar.
- **Explicación más probable.** `doomgeneric_Tick()` llama a `TryRunTics()`,
  que duerme (`I_Sleep(1)`, d_loop.c:784) hasta que toca el siguiente tic, a
  35 por segundo. Como el juego avanzaba dentro de `getModelInfo`, en el hilo
  de la interfaz, ese hilo pasaba unos 28 ms de cada 33 dormido y la interfaz
  casi no corría: ni dibujaba ni atendía bien el apagado. La sonda pinta y
  vuelve al instante. La cámara tiene tres núcleos, así que no es falta de
  procesador. La v34 corre el juego en otro hilo.

### v34, 2026-11-27 (bitácora en `backups/20261127-184252-dumps/`)

- El juego en su propio hilo: el hilo que lo crea es `SCHED_RR` (tiempo real);
  el del juego pasó a `SCHED_OTHER` con `nice` 10. Corrió a unos 36 turnos y
  35 cuadros por segundo; la interfaz dio unos 95 turnos por segundo y copió
  unos 11 cuadros por segundo (sus turnos vienen en ráfagas). La medida del
  procesador del hilo dio 0: `CLOCK_THREAD_CPUTIME_ID` no sirve en este kernel.
- **No se vio nada, ni las franjas de carga**, y el bitmap otra vez nunca
  estuvo ocupado: la pantalla no lo leyó.
- Lo que separa lo que se vio de lo que no: la sonda creó y mostró el cuadro
  desde un toque, y después le cambió el contenido desde los turnos periódicos
  (la barra se movía). En la v32 a la v34 el cuadro se creó o se volvió a
  mostrar desde un turno periódico (dentro de `getModelInfo` o
  `TextSprite::setString`), y nunca se vio. Lo más probable es que el motor
  gráfico no registre un widget nuevo, o un cambio de modo, si llega a media
  actualización; en el firmware, la función que registra un sprite cambiado
  (`0x31a694`) vuelve sin hacer nada según lo que diga el motor
  (`0x30df8e`), pero no terminé de leer qué revisa.
- La v35 crea el cuadro al tocar (la esquina, o el ítem del menú antes de que
  se cierre), lo deja siempre visible y transparente, y prender o apagar el
  juego sólo cambia su contenido.

### v35, 2026-11-27 (bitácora en `backups/20261127-185346-dumps/`)

- El cuadro se creó desde el toque en la esquina, igual que la sonda, y el
  juego corrió como en la v34. **Tampoco se vio**, y el bitmap otra vez nunca
  estuvo ocupado. Lo del contexto no era.
- El dueño vio que, al tocar la esquina, el panel desaparecía y volvían los
  botones nativos (REC, W, T y los demás): ocultar el panel desde un toque sí
  funciona, y sin el panel nada vuelve a ocultar lo nativo.
- Lo único que queda en común: la sonda siempre corrió **con el panel activo**,
  y el juego lo ocultaba al arrancar. La v36 deja el panel activo, debajo del
  juego, los primeros 15 segundos (`doompanel 15`) y después lo oculta solo,
  para ver en una sola corrida si el juego se ve en una fase y en la otra no.

### v36, 2026-11-27: **se ve**

- Con el panel activo debajo del juego, el dueño vio DOOM en la pantalla de la
  cámara («LO VEO!»). A los 15 s el panel se ocultó solo y el juego
  desapareció. Queda confirmado: el cuadro sólo se ve mientras el panel está
  activo. Todavía no se sabe qué parte del panel hace falta (que sus textos se
  redibujen, o que mantenga ocultos los widgets nativos).
- La bitácora (`backups/20261127-191121-guard-install/cxreq.log`) lo confirma
  sin depender de la vista: en los 15 s con panel la interfaz copió 149
  cuadros (unos 10 por segundo) y 132 veces encontró el bitmap ocupado porque
  la pantalla lo estaba leyendo; al ocultarse el panel ese contador se quedó
  quieto (132 hasta el final). El juego avanzó a unos 35 turnos por segundo.
- La v37 deja el panel activo todo el tiempo que dure el juego (`doompanel N`
  sirve para volver a probar con el panel oculto a los N segundos).

## Sonido (v38, sin probar en la cámara)

Sólo efectos, por la ruta de los pitidos del sistema
(`docs/research/audio-y-archivos.md`, ruta A). Detalle en `preload/cxsound.c`.

- **Formato de los pitidos**, deducido de los 32 `Cam_*.bin` de la cámara:
  bloques de 32 bytes; el primero es la cabecera, con la escala en los 4 bits
  bajos (muestra × 2^escala) y, en el bit 4, si el bloque son diferencias con
  la muestra anterior; siguen 31 muestras de 8 bits con signo. Decodificados
  así, los bloques empalman sin saltos y el del autodisparador da un tono
  limpio de 2 354 Hz si la tasa es de 48 kHz. Que sean 48 kHz es una suposición.
- **El juego** (`doom/cxd.c`) convierte cada efecto del WAD (8 bits sin signo a
  11 025 Hz, formato DMX) a 48 kHz por vecino más cercano, con el volumen del
  canal, en bloques directos con escala 8: las muestras de 8 bits caben sin
  pérdida. Hasta 64 KB por efecto (1.3 s). Sin música.
- **La entrega:** mi biblioteca escribe la tabla de saltos de `libObj.so`
  (no tiene RELRO) para que sus `fopen`, `fread`, `feof` y `fclose` pasen por
  ella. Los efectos esperan en cuatro ranuras; en el turno siguiente de la
  interfaz se manda `ObjMsg(0x3410, 0x1120)` con uno de los tres pitidos
  «Jet», que la cámara no usa, al objeto que trajo el último pitido de
  fábrica. Cuando la cámara abre ese archivo, recibe el efecto. Todo lo demás
  pasa a la libc.
- La libc de la cámara (`libsonyc.so`) sólo tiene tabla de símbolos GNU;
  `cx_dlsym` ahora también la lee.
- Hace falta que antes haya sonado algún pitido de fábrica (tocar algo del
  menú) y que el pitido esté activado en el menú de la cámara.
- La biblioteca pasó de 64 KB: el tope del instalador es ahora de 5 bloques
  (80 KB); el espacio libre de `/setting` se sigue calculando aparte.

### Sonido en la cámara (v38 y v39, 2026-11-27)

- **v38:** los efectos se oyen y el juego va fluido, pero la bitácora
  (`backups/20261127-194053-dumps/`) muestra sólo tres lecturas en 20 pedidos,
  y el dueño confirmó que todos los efectos sonaban igual. La cámara guarda
  los pitidos ya cargados en una caché de 9 lugares y desaloja el menos usado
  (`FUN_00432934` busca si ya está; `FUN_00433110` escoge el más viejo): con
  tres números de pitido, los tres se quedaron guardados. Desde la v39 se rota
  entre 11 pitidos que la interfaz casi no usa (los tres «Jet», los cuatro
  «Event Anime», Media Change, Hold Enter, Self Timer y Start Up), así cada
  pedido obliga a leer otra vez. Hasta apagar la cámara, esos pitidos pueden
  sonar a DOOM.
- **v39: congeló la cámara.** La prueba de tonos mandó un pitido de 396 KB. El
  lector de la cámara (`FUN_0044be90`) copia de a 128 bytes hasta llenar el
  búfer que le da el procesador AV, y si el archivo mide lo mismo o más que el
  búfer su bucle pide 0 bytes, recibe 0 y vuelve a pedir para siempre, con el
  candado del sonido tomado (`FUN_004338d4`): la interfaz se trabó esperándolo
  y el dueño tuvo que desconectar la cámara (`backups/20261127-195624-dumps/`).
  El tamaño del búfer depende del tipo de memoria del pitido, no del archivo;
  alcanza al menos para el pitido de arranque de fábrica (36 480 bytes).
- **Corrección (v40):** todo lo que se entrega mide como mucho 36 480 bytes, y
  `w_fread` vigila el final del búfer: si el lector pide menos de 128 bytes y
  quedan más datos, se le da uno menos y `feof` dice que se acabó; si pide 0,
  se le devuelve 1 para que salga del bucle. En los dos casos se anota el
  tamaño real y se baja el tope. Probado en QEMU con un lector que repite el
  bucle de la cámara, con búferes de 20 000 y 8 192 bytes.

## Música (v40, sin probar en la cámara)

- El juego lleva el emulador OPL (DBOPL) y el secuenciador MIDI de Chocolate
  Doom 2.2.1 (`vendor/doom/chocolate-doom-2.2`, GPL-2), con un backend propio
  (`doom/opl_cxd.c`) que genera las muestras cuando el juego las pide. El MIDI
  temporal que arma el secuenciador va a un archivo en memoria de la libc
  mínima.
- En cada turno del juego se genera la música que toca según el reloj, a
  24 kHz, y cada 736 ms sale un trozo: pasado a 48 kHz, en bloques de pitido
  con la escala justa para cada bloque, 36 480 bytes, lo que cabe en un pitido.
  Cada trozo se manda a su hora (`doomlead`), por la misma rotación de pitidos
  que los efectos, en dos ranuras propias.
- La pantalla de título ya no pasa a las demos: se queda, con E1M1 («At Doom's
  Gate») en bucle, que es lo que el dueño pidió para el menú principal. Al
  empezar partida en E1M1 la canción sigue sin cortarse.
- No se sabe todavía si los trozos suenan pegados, con huecos o encimados, ni
  si un efecto corta la música.

### Música en la cámara (v40 y v41, 2026-11-27)

- **v40:** se oía «como el sintetizador» pero el dueño la notó una octava más
  grave. Causa: `i_oplmusic.c` fija la tasa del emulador con `snd_samplerate`
  (44 100 de fábrica, `OPL_SetSampleRate`) y el juego leía las muestras como
  si fueran de 24 000: la música salía al 54 % de velocidad, 10.5 semitonos
  más grave. En QEMU las notas caían todas unos 45 centésimos fuera de la
  escala; con la corrección (v41, sólo `CXDOOM.BIN`) caen en Mi, Si, Sol# y
  Fa#, afinadas.
- La bitácora de la v40 (`backups/20261127-200957-guard-install/`) mostró
  además que el juego no pintó nada en los primeros 8 s y que los trozos de
  música salían cada ~0.95 s en vez de cada 0.74: el sintetizador le quitaba
  casi todo el procesador al hilo del juego. Esta cámara no tiene VFP y sus
  núcleos son lentos; DBOPL es todo enteros al generar, pero aun así pesa.
- **v42:** la música va en su propio hilo (`nice` 5), en otro de los tres
  núcleos, por una cuarta entrada del programa del juego con su propia pila
  (interfaz 5). Todo lo que el juego le pide al módulo de música pasa por un
  candado que también toma el hilo de música mientras genera. Al recargar o
  terminar el juego, el hilo de música se detiene antes de que se libere la
  memoria. La bitácora anota cuánto tarda en calcularse cada trozo y cuánto
  duran los primeros turnos del juego. `preload/release/cxg-v42.so` (md5
  `0057851f…`) con `cxmode-v42p.txt` y `CXDOOM.BIN` md5 `4446800e…`, que van juntos.

- **v42 en la cámara:** tardó en arrancar pero funcionó; la música se oye
  más fluida, pero **sigue una octava más grave** aunque en QEMU sale afinada
  suponiendo 48 kHz. Lo más probable es que la cámara reproduzca los pitidos
  a 24 kHz, la mitad: eso baja todo una octava, efectos incluidos, y explica
  que antes cada trozo durara el doble y se encimara con el siguiente.
- **v43:** la tasa supuesta pasa a 24 kHz para efectos y música (la música ya
  no tiene una tasa fija de 48 kHz, sigue `sound_hz`), y PHOTO a fondo durante
  el juego la cambia entre 24 000, 22 050, 32 000, 48 000 y 16 000 para
  afinarla de oído (queda en la bitácora). `preload/release/cxg-v43.so` (md5
  `7b6976d0…`) con `cxmode-v43p.txt` y `CXDOOM.BIN` md5 `ed374bbf…`.

- **v43 en la cámara: «suena precioso»** (el dueño, 2026-11-27): con 24 kHz
  la música sale en su tono. Queda confirmado que la cámara reproduce los
  pitidos a 24 kHz. Pero sonaba muy tenue: el efecto de mover el menú, «mínimo
  al doble». Medido en QEMU, la música tenía RMS 287 (pico 2 144) contra 3 770
  del efecto del menú (DSPSTOP) y ~6 000 de los disparos: 22 dB menos.
- La bitácora de la v42 explicó también el arranque lento: el primer turno del
  juego tardó 37 s. El lector de MIDI de Chocolate Doom agranda su lista de
  eventos de a uno con `realloc`, y el `realloc` de la libc mínima copiaba todo
  cada vez: cuadrático, con miles de eventos.
- **v44** (sólo `CXDOOM.BIN`): ganancia ×12 a la música (RMS 3 436, pico
  25 856, sin recortes) y `realloc` que crece al doble (el primer turno pasa de
  214 a 22 ms en QEMU). La biblioteca sigue siendo la v43.

- **v44 en la cámara:** «todo se escucha bien» (el dueño). Pero disparar con
  PHOTO hasta el fondo cambiaba la tasa de los pitidos y con ella el tono.
- **v45** (sólo la biblioteca, `preload/release/cxg-v45.so`, md5 `bb1db527…`,
  con `cxmode-v45p.txt`; el `CXDOOM.BIN` es el de la v44): PHOTO a fondo ya no
  hace nada aparte de disparar, y la tasa queda fija en 24 kHz (`doomhz`).

- **Cortes breves cada pocos segundos** (el dueño, con la v44). Cada trozo se
  mandaba justo a su hora, pero lo manda el hilo de la interfaz, que despierta
  a ráfagas, y la cámara tarda en cargarlo: entre trozo y trozo quedaba un
  hueco de decenas de milisegundos. Como la cámara mezcla pitidos simultáneos
  (los disparos no cortan la música), la **v46** encima los trozos: cada uno
  lleva 1 s de música nueva más los últimos 150 ms del anterior, con fundido
  de entrada y de salida, y sale 150 ms antes de su hora
  (`doomchunk 1000`, `doomlead 1150`). En QEMU, mezclando los trozos como la
  cámara, la energía en las uniones queda igual que dentro de los trozos. La
  bitácora anota cada cuánto salen de verdad los trozos y cuánto esperan.
  `preload/release/cxg-v46.so` (md5 `cd211eb4…`) con `cxmode-v46p.txt` y
  `CXDOOM.BIN` md5 `436dbaed…`, que van juntos; también trae el cambio de la
  v45 (PHOTO a fondo sólo dispara).

## Lo que no se sabe todavía

- Si con el juego en su propio hilo (v34) se ve, y a cuántos cuadros por
  segundo.
- Cuánto procesador usa el juego (la v34 lo anota: «256 turnos en ms, de
  procesador»).
- Si la cámara se apaga sola por inactividad con el juego en pantalla.
- Si el formato y la tasa de los pitidos son los que se dedujeron, cuánto
  tarda un pitido en sonar, si dos se mezclan o uno corta al otro, y cuánto es
  lo máximo que la cámara lee de un archivo de pitido.
