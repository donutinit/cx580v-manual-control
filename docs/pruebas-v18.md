# Prueba completa de la v18

Preparada el 2026-11-27 de madrugada. Lo nuevo de la v18 pasa las pruebas en
QEMU (`preload/test/run.sh`), pero **nada de lo nuevo se ha probado en la
cámara**. Lo que ya estaba probado en la cámara (v16 y v17) no cambió.

Revisada el 2026-11-27 por la mañana contra
`reports/Barrido de fuentes para la CX580V.md`. Los binarios son los mismos
(mismo md5). Lo que cambió:

- Las pruebas de QEMU ahora emulan el procesador de la cámara (ARM11 MPCore);
  antes QEMU aceptaba instrucciones más nuevas que en la cámara habrían
  fallado. Las dos bibliotecas pasan igual.
- El `Backup.bin` de la unidad confirma por qué el menú oculta IRIS, Shutter
  Speed, AE Shift y WB Shift (`0x01070316 = 01`, `0x01070371 = 00`), que es
  justo lo que corrige `show`. Esas dos propiedades son de sólo lectura, así
  que no había atajo por Backup.
- Se agregó el clip 4, para saber de dónde salen los saltos de brillo.

## Qué trae

| Novedad | Cómo se hizo | Configuración |
| --- | --- | --- |
| Entradas IRIS, Shutter Speed, Gain y White Balance Shift en `MENU → Camera/Mic → Manual Settings` | el menú de fábrica ya las trae y el firmware las oculta en este modelo; la biblioteca las vuelve visibles (`show`) | v18, v18b, v18c |
| «AE Shift» se llama **Gain** y su descripción habla de ganancia | cambio de etiqueta por número (`label`) | todas |
| La pantalla de Gain muestra **0dB … 24dB** y «AGC OFF» en vez de −4 … +4 | iconos de fábrica del mismo tamaño (`image`) | todas |
| Menos saltos al cambiar entre iris y obturación | la corrección sigue vigilando 1.5 s y también corrige el control que queda en segundo plano | binario |
| Menú recortado según `docs/ui-spec.md` | `hide` | v18c |
| Bitácora de toques y botones, para el panel propio | variante aparte `cxg-v18-ux.so` | v18u |

Archivos, en `preload/release/`:

| Archivo | Para qué |
| --- | --- |
| `cxg-v18.so` | biblioteca de las etapas 1 a 3 |
| `cxmode-v18.txt` | etapa 1: entradas nuevas **y** las redirecciones de siempre, de respaldo |
| `cxmode-v18b.txt` | etapa 2: sólo las entradas nuevas; Exposure, Spot Meter, Spot Meter/Fcs y Tele Macro vuelven a ser los de fábrica |
| `cxmode-v18c.txt` | etapa 3: etapa 2 más el menú recortado |
| `cxg-v18-ux.so` + `cxmode-v18u.txt` | etapa 4: etapa 2 más la bitácora de toques y botones |
| `cxmode-v18r.txt` | respaldo si las entradas nuevas no aparecen: redirecciones con los nombres cambiados |
| `cxg-v16.so` + `cxmode-v16.txt` | última versión probada completa en la cámara |

Cada instalación son dos encendidos: uno entra al updater e instala, el
siguiente arranca normal. La cámara entra al updater un encendido sí y otro no.

```bash
# etapa 1
.venv/bin/python tools/cxdump.py guard-install --model 0x21010001 \
    --library preload/release/cxg-v18.so --config preload/release/cxmode-v18.txt --yes
# etapas 2 y 3: lo mismo con cxmode-v18b.txt o cxmode-v18c.txt
# etapa 4
.venv/bin/python tools/cxdump.py guard-install --model 0x21010001 \
    --library preload/release/cxg-v18-ux.so --config preload/release/cxmode-v18u.txt --yes
# bitácora de la sesión anterior, en palabras
.venv/bin/python tools/cxlogdecode.py backups/<carpeta>/cxreq.log --summary
```

## Etapa 1: v18

### 1. El menú

`MENU → Camera/Mic → Manual Settings`.

- [x] ¿Aparecen **IRIS**, **Shutter Speed**, **Gain** y **White Balance Shift**
      como entradas nuevas? ¿En qué página quedan?
- [x] ¿Las páginas se ven completas, sin huecos ni filas raras?
- [ ] ¿Qué valor muestra cada entrada a la derecha (Auto, un número, nada)?
Cuando me salgo de ellas aparece el valor en el qeu se dejaron, pero al moverle a otro aparece en auto, pero no están en auto se quedan en el valor previo
- [x] Las entradas viejas (Exposure, Spot Meter, Spot Meter/Fcs, Tele Macro) si, ahí y abren lo anterior
      siguen ahí con su nombre de fábrica. En esta etapa todavía abren iris,
      obturación, ganancia y WB Shift, por si las nuevas fallan.

Si las entradas nuevas **no** aparecen, sigue la prueba con las viejas y
avísame: la bitácora dirá por qué.

### 2. Cada control desde su entrada nueva

- [x] **IRIS**: abre, pasa a manual, se mueve de F1.8 a F10.
- [x] **Shutter Speed**: abre, pasa a manual, se mueve.
- [x] **Gain**: el valor se ve como **12dB** (o el que toque), no como un
      número de −4 a +4. Apagado dice **AGC OFF**. Cada paso son 3 dB, de 0 a
      24. ¿El icono se ve completo, sin recortes?
- [ ] La descripción de Gain habla de ganancia («Fixes the gain adjusted
Cuando estoy en la página de gain individual si se ve como DB, pero al darle para atras se ve como +0.7EV por ejempolo
      manually»), no de auto exposure.
- [ ] **White Balance Shift**: abre y cambia el color. si, y aparte tambien funciona bien el white balance normal

### 3. Independencia

Con valores intermedios, que es donde aparecían los saltos: **F3.7**,
**1/1000** y **12dB**.

- [x] Mover el iris no cambia obturación ni ganancia.
- [x] Mover la obturación no cambia iris ni ganancia.
- [x] Mover la ganancia no cambia iris ni obturación.
- [x] Cambia cinco veces entre la pantalla de IRIS y la de Shutter Speed sin
      mover nada. ¿Hay saltos de brillo? Si hay un saltito que se corrige solo
      en menos de un segundo, es la corrección trabajando: dímelo también.
- [x] Tapa la lente a medias: la imagen se oscurece y no se compensa.

### 4. Automático por control

- [x] Shutter Speed en automático: el iris sigue en su valor.
- [x] IRIS en automático: la obturación sigue en su valor.
- [x] Gain apagado (AGC OFF): la ganancia vuelve a ser automática.

### 5. Clips, para comprobarlo con números

Graba tres clips de 5 segundos, en interior con luz normal:

| Clip | Iris | Obturación | Gain |
| --- | --- | --- | --- |
| 1 | F2.8 | 1/60 | 0dB |
| 2 | F4.0 | 1/125 | 12dB |
| 3 | F1.8 | 1/60 | 24dB |

**Clip 4, para los saltos:** F2.8, 1/60 y **9dB**, 30 segundos, con la cámara
apoyada y quieta, apuntando a algo fijo con luz que no cambie (una pared o un
escritorio con foco, sin ventana ni pantallas en cuadro). No toques nada
mientras graba. Si ves un salto de brillo, dime más o menos en qué segundo.
El clip guarda iris, obturación y ganancia cuadro por cuadro, y la bitácora
guarda cada cambio de ganancia que avisa la cámara (en pasos de 3 dB). Si el
brillo salta y ninguno de los tres cambia, el salto viene del ajuste fino de
ganancia del automático, que es continuo por dentro; si cambia el F, es el
iris. Son las dos explicaciones que deja el informe.

Después conecta la cámara por USB (`USB Connect`), copio los clips a
`captures/prueba-v18/` y los leo con `tools/cxexposure.py`. Lo grabado tiene que
decir lo mismo que la pantalla. Si la escena es tan brillante que le sobra luz,
la ganancia puede quedar por debajo del tope: por eso en interior.

### 6. Cierre

Apaga y prende: entra al updater y bajo la bitácora.

### Resultado de la etapa 1 (2026-11-27)

Bitácora en `backups/20261127-060635-dumps/`, clips en `captures/prueba-v18/`.
La bitácora se llena a los 48 KiB, que fueron los primeros 7 minutos; los clips
se grabaron después y no quedaron cubiertos.

- Las cuatro entradas nuevas se mostraron (`MN show … state 0->2`) y el dueño
  abrió las cuatro desde el menú: `view/IRIS_L`, `view/SHUTTER_SPEED_L`,
  `view/AE_SHIFT` y `view/WB_SHIFT`. Las redirecciones de respaldo también
  abrieron. Ninguna orden rechazada.
- Los pasos de iris y obturación, el WB Shift y el tope de ganancia llegaron a
  la cámara, y la cámara avisó la ganancia igual al tope en cada paso (0 a
  21 dB).
- La pantalla de ganancia pidió los iconos de dB (`TI icon … -> 19b`, etc.).
  Falta que el dueño diga si se ven completos.
- Al cambiar de pantalla la corrección no encontró desvíos (`FX … step=0`).
- Clips: el iris y la ganancia grabados coinciden con la tabla (F2.8/0 dB,
  F4/12 dB, F1.8/24 dB) y no cambian dentro de cada clip. La obturación grabada
  es 1/60 en el clip 1 y **1/100 en los clips 2 y 3**, donde la tabla pedía
  1/125 y 1/60: falta saber qué decía la pantalla. El brillo es estable cuadro
  a cuadro (variación máxima de 0.2 sobre 255).
- ExifTool muestra la obturación de los AVCHD de esta cámara con un divisor de
  50 Hz: «1/50» es 1/60 y «1/83» es 1/100 (valor crudo entre 33 750).
  `tools/cxexposure.py` ya la corrige; se cotejó con la bitácora de la etapa 2.
- No se grabó el clip 4.

En las etapas siguientes conviene grabar los clips al principio de la sesión
para que la bitácora los cubra.

## Etapa 2: v18b

Sólo las entradas nuevas.

- [ ] IRIS, Shutter Speed, Gain y White Balance Shift siguen funcionando como en
      la etapa 1.
- [ ] **Exposure** vuelve a ser el brillo único de fábrica.
- [ ] **Spot Meter**, **Spot Meter/Fcs** y **Spot Focus** vuelven a medir y
      enfocar en el punto que tocas.
- [ ] **Tele Macro** vuelve a ser Tele Macro.
- [ ] Ojo: la Exposure de fábrica y las prioridades se excluyen. Si activas
      Exposure manual, iris y obturación manuales se sueltan. Es el
      comportamiento del modelo de Sony, no un fallo nuevo.

### Resultado de la etapa 2 (2026-11-27)

Bitácora en `backups/20261127-061816-guard-install/`. El dueño reporta que todo
parece funcionar. Exposure y Spot Meter abrieron sus pantallas de fábrica.

**Hallazgo: la causa de los saltos de 1/3 de paso.** En dos de tres cambios de
la pantalla de IRIS a la de Shutter Speed el iris se cerró 1/3 de paso al
cambiar la prioridad (F2.6 a F2.4, F2.8 a F2.6). `cxg.so` lo detectó y mandó el
paso de corrección, pero la interfaz de fábrica lo bloqueó las dos veces
(`ID m=6502 step=1 ret=a000`): en prioridad de obturación no deja pasar
`SYNC_IRIS_DRIVE`. El iris quedó desviado hasta volver a su pantalla, donde la
corrección sí pasa (`ret=0`). El primer clip de esta etapa (60 s, 9 dB) se
grabó así: en pantalla F2.8, real F2.6 según la bitácora. Lo mismo, del lado de
la obturación, explicaría el 1/100 de los clips 2 y 3 de la etapa 1.

Arreglo a probar: `autounlock 1`, que ya trae el binario de la v18 y se probó
en la cámara con el ISO. Cuando una orden propia vuelve bloqueada, destraba esa
función y la reintenta. Configuración `cxmode-v18d.txt` (la v18c más esa
línea). Falta ver si la cámara obedece el paso de iris en prioridad de
obturación; la bitácora lo dirá (`UL aprendida función …` y el `ret` del `ID`).

## Etapa 3: v18c, menú recortado

Se ocultan, además: Scene Selection, Fader, Self-Timer, Digital Zoom,
Conversion Lens, Auto Back Light, Smile Shutter, Smile Sensitivity, Closer
Voice, Audio Mode y Light Bright (Camera/Mic); Download Music, Empty Music, Disc
Burn, Your Location, GPS Setting, Demo Mode, Auto Clock ADJ y Auto Area ADJ
(Setup); Image Size (Image Quality/Size).

- [ ] Recorre las seis pestañas de Camera/Mic, Setup e Image Quality/Size: ¿las
      páginas se ven completas?
- [ ] ¿El cursor y el desplazamiento se comportan bien?
- [ ] Abre tres o cuatro entradas que sigan visibles: ¿abre la correcta, o la de
      al lado?
- [ ] No oculté la pestaña Flash ni My Button. Flash quedaría vacía y hay que
      ocultar la pestaña entera, que es otro paso.

### Resultado de la etapa 3 con `autounlock` (2026-11-27)

Configuración `cxmode-v18d.txt` (v18c más `autounlock 1`), bitácora en
`backups/20261127-062956-guard-install/`.

- **El ocultado no funcionó.** Las reglas `hide` corrieron (`MN hide … valid
  1->0`, 24 líneas) y el dueño no vio ningún cambio: en la cámara, poner
  «válido» en 0 después de `initMenuData` no oculta el ítem. Los ítems que
  oculta el firmware llegan con estado 0 y válido 1, así que la **v19**
  (`preload/release/cxg-v19.so`, md5 `e2c57318…`) oculta poniendo el estado en
  0. Pasa las pruebas de QEMU; instalada a las 06:29, falta verla en la cámara.
- **La corrección en segundo plano ya funciona.** Con `autounlock 1`, `cxg.so`
  aprendió las dos funciones que bloqueaban sus pasos (`0xdb` para la
  obturación en prioridad de iris, `0xda` para el iris en prioridad de
  obturación) y la cámara obedeció (`ret=0`). En seis cambios de pantalla con
  F2.8 y 1/125: la obturación cayó una vez a 1/60 y otra a 1/100, el iris tres
  veces a F2.6, y todas volvieron a su valor en menos de un segundo. Lo que
  queda es ese parpadeo breve al cambiar de pantalla.

### Clips de las etapas 2 y 3, y menú recortado con la v19 (2026-11-27)

Clips en `captures/prueba-v18/`, leídos con `tools/cxexposure.py`. Cada uno
tiene un solo valor de iris, obturación y ganancia de principio a fin.

| Clip | Duración | En pantalla | Grabado |
| --- | --- | --- | --- |
| `00004.MTS` | 60 s | F2.8, 1/60, 9dB | **F2.6**, 1/60, 9 dB |
| `00005.MTS` | 40 s | F4.0, 1/125, 12dB | F4.0, 1/125, 12 dB |
| `00006.MTS` | 12 s | F2.8, 1/60, 24dB | F2.8, 1/60, 24 dB |
| `00007.MTS` | 14 s | F2.8, 1/125, sin tope de ganancia | F2.8, 1/125, 27 dB |

- El `00004` confirma el desvío sin corregir de la etapa 2 (sin `autounlock`):
  la bitácora decía F2.6 y eso se grabó. El `00007`, ya con `autounlock`,
  quedó en los valores pedidos después de seis cambios de pantalla.
- En el `00004`, con los tres controles fijos y la escena quieta, el brillo
  medio se mueve menos de ±1 sobre 255 en 60 segundos: grabando no hay saltos.
  Los saltos eran el desvío al cambiar de prioridad.
- **Menú recortado:** con la v19 el dueño confirma que ya no ve las entradas
  ocultas. La bitácora (`backups/20261127-063914-guard-install/`) muestra
  `MN hide … state 2->0` en Camera/Mic y Setup. Image Quality/Size no se abrió
  y no se probó abrir entradas vecinas a las ocultas.

## Etapa 4: bitácora de toques y botones

No cambia nada de lo que se ve. Sirve para saber dónde cae cada toque y qué
número manda cada botón, que es lo que falta para dibujar un panel propio
(`docs/research/ux-panel-propio.md`).

Instalada el 2026-11-27 06:39: `cxg-v19-ux.so` con `cxmode-v19u.txt` (la
configuración de la etapa 3 con `autounlock` más `uxlog 1` y `textlog 1`).

En la pantalla de grabación, sin abrir menús:

1. Toca las cuatro esquinas de la pantalla y el centro, en ese orden: arriba
   izquierda, arriba derecha, abajo izquierda, abajo derecha, centro.
2. Aprieta, de uno en uno y con un par de segundos entre cada uno: PHOTO,
   START/STOP (dos veces, para no dejarla grabando), MODE, la palanca de zoom
   hacia T y hacia W.
3. Abre el menú y entra a una pantalla cualquiera.
4. Apaga y prende para bajar la bitácora.

### Resultado de la etapa 4 (2026-11-27)

Bitácora en `backups/20261127-064406-guard-install/` (32 KiB, sesión completa).

- **Tacto:** 26 toques con su par apretar y soltar (`MP`/`MR`), todos en el
  hilo de la interfaz. Las coordenadas van de (27, 36) a (614, 437): el tacto
  usa un espacio de **640×480** con el origen arriba a la izquierda.
- **Botones:** sólo quedaron dos. `0x15020022` y `0x15020021` al salir de la
  reproducción hacia el modo de grabación (se cierra `view/EVENTVIEW`, cambian
  los LED de modo), y `0x15020012` y `0x15020011` justo antes del apagado
  (`view/ROOT`). En los dos llega primero el `+2` y después el `+1`. Al
  arrancar llegan `0x150b0013` y `0x150b0023` (estado inicial de dos palancas).
  Eran el botón de ver imágenes y el de encendido. Los demás se midieron en
  una segunda sesión (`backups/20261127-065106-guard-install/`): encendido
  `0x15020010`, ver imágenes `0x15020020`, PHOTO a medias `0x15020030` y a
  fondo `0x15020040`, START/STOP `0x15020050`, MODE `0x15020090`, palanca de
  zoom `0x150b0030`. En las teclas `+2` es apretar y `+1` soltar. La tabla
  completa está en `docs/research/ux-panel-propio.md`.
- **Etiquetas:** 12 juegos de fuente (`LB … font …`), con ids `0x0203xxxx` y
  `0x0100xxxx`, para crear etiquetas propias.

Después de la sesión de botones quedó instalada la versión de uso diario:
`cxg-v19.so` con `cxmode-v18d.txt` (06:51).

## Si algo sale mal

- **La cámara no arranca o se queda en negro:** apágala y préndela. El siguiente
  arranque entra al updater sin pasar por el firmware principal, y desde ahí
  reinstalo la v16.
- **Volver a la última versión probada:**
  ```bash
  .venv/bin/python tools/cxdump.py guard-install --model 0x21010001 \
      --library preload/release/cxg-v16.so --config preload/release/cxmode-v16.txt --yes
  ```
- **Dejar la cámara de fábrica:** `cxdump.py guard-remove --model 0x21010001`.

## Lo que no entra todavía

- Los ajustes manuales no se guardan al apagar: en cada encendido la cámara
  arranca en automático.
- AGC Limit (la pantalla con la lista Off, 24dB … 0dB) sigue oculta: abre pero
  no manda nada. La causa probable está en `docs/research/vistas-agc-aeshift.md`.
- El panel propio de `docs/ui-spec.md`. La etapa 4 junta los datos que faltan.
