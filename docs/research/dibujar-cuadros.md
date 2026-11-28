# Pintar cuadros completos en la LCD desde dentro de `im.elf`

Estudio estático del 2026-11-27 sobre `libObj.so` del firmware 1.07, hecho con
Ghidra sobre una copia privada del proyecto `app`. El fin es mostrar un búfer de
píxeles que cambia de 20 a 35 veces por segundo (los cuadros de doomgeneric).
**Nada de esto se probó en la cámara.** Cada afirmación lleva una marca:

- **[código]**: lo vi en el decompilado, el desensamblado o las tablas del binario.
- **[inferido]**: se deduce de lo que vi, sin comprobarlo.
- **[?]**: no lo pude determinar.

Las direcciones son de Ghidra (dirección en el archivo + `0x10000`), como en
`ux-panel-propio.md`. Los decompilados están en
`firmware/ghidra/out/agents/doom/cuadros/` (índice en su `LEEME.md`).

## 1. Resumen

La ruta recomendada es la que usa Sony para sus propias imágenes calculadas en
ejecución:

1. Crear un widget `GEN_Icon` propio (0x1f0 bytes, constructor exportado) y
   colgarlo de `A_PHYSICAL_LAYER`, igual que las `GEN_Label` del panel.
2. Pedirle al gestor de recursos una imagen vacía con
   `ResourceManagerImpl::createImage(CreateImageParam(320, 200, 0x41006), &img)`.
   La imagen es un bitmap GRM de 16 bits con alfa, el mismo formato que el OSD.
3. Poner esa imagen en el `Sprite` del icono (`Sprite::setImage(resID, 0x80)`)
   con un rectángulo escalado por hardware (`Sprite::setRectSize` de seis
   argumentos, por ejemplo de 320×200 a 640×400 o a 480×480).
4. En cada cuadro, desde el hilo de la interfaz: `GRM_bitmapWriteLockBits` sobre
   el bitmap de la imagen, escribir los píxeles, `GRM_bitmapWriteUnlockBits` y
   `Sprite::setImageDirty` para que el compositor vuelva a pintar esa zona.

Sony hace exactamente esa secuencia en `GEN_CacheInsImg`, `W_GEN_ClstrThumb` y
`GEN_PaintCanvas` **[código]** (sección 4). El compositor dibuja con el motor 2D
por hardware (`GRM_gpermRectblit`), sin GPU **[código]**. Lo que no se pudo cerrar
es el orden exacto de los cuatro canales dentro de los 16 bits y cuánta memoria
deja la arena de imágenes; la receta trae un cuadro de prueba que resuelve lo
primero en la cámara y un sondeo para lo segundo.

## 2. Cómo pinta la interfaz

### 2.1 El OSD

- **[código]** `uxengine_init` (`0x13a6cc`) arranca el motor con
  `FUN_0031f78c(…, 0x280, 0x1e0, 1, 0)`: 640×480 y formato de píxel GRM 1.
- **[código]** El formato GRM 1 ocupa 2 bytes por píxel (tabla de
  `pixelFormatToBytesPerPixel`, `0x34ab88`: formatos 1 a 8 = 2, 4, 2, 1, 2, 2, 2,
  4 bytes).
- **[código]** `share/grm/fbScreenMode.bin` trae, después de una cabecera de
  8 bytes, tres registros de dos planos de 640×480 con 640 píxeles por fila: uno
  con formato 2 (4 bytes) y dos con formato 1 (2 bytes). Qué significa el `4` de
  la cabecera es **[?]**. Dos planos por modo y el cambio de búfer de
  `drawFramebuffer` (2.2) dicen que el OSD tiene doble búfer **[inferido]**.
- **[código]** `GRM_fbrmSetScreenPreference(2)` se pide cuando algún widget
  necesita 32 bits (`LayoutableWidgetBase::setRGBA8888`, con contador; vuelve a
  `1` cuando nadie lo pide, `FUN_001a27c8`). Lo normal es el modo de 16 bits
  **[inferido]**; en la pantalla de grabación no debería haber nada que pida
  32 bits **[?]**.
- **[código]** La captura de pantalla de Sony (`FUN_0032f3c8`) lee el bitmap del
  OSD y sólo acepta formato 1 (2 bytes) o 2 (4 bytes).
- **[código]** `GraphicsEngine::getFramebufferWidth/Height` leen el ancho y alto
  del bitmap de pantalla que entrega GRM (`FUN_0037423c`). Conviene anotarlos una
  vez en la bitácora.

### 2.2 Una pasada de dibujo

**[código]** La fase 5 de `AppSystem::Main` es `FUN_008d3330`:
`updateFrame` (animaciones), `notifyDraw` y `drawFramebuffer` (`FUN_0013a128`).
`drawFramebuffer`:

1. junta los rectángulos sucios del cuadro con los del cuadro anterior (el
   búfer de atrás no tiene los cambios del otro);
2. redibuja esa región: cada nodo visible que la toca se pinta con blits de GRM
   sobre el bitmap de atrás (`FUN_0035489a`, el «off bitmap»);
3. cambia de búfer (`FUN_0032f30a`) y alterna el índice en
   `GraphicsEngine + 0x174`.

Un elemento se marca sucio con `FUN_0032a694(elemento, 0)`, que agrega sus
límites a la lista de rectángulos sucios (`FUN_0031de86`, hasta 16; con más,
redibuja la pantalla entera). Todos los `set*` de `Sprite` y `Element` lo llaman
sólo si el valor cambia **[código]**. Si nada está sucio, no se dibuja nada
**[código]**.

`drawFramebuffer` corre en cada vuelta del lazo, sin tope propio de cuadros por
segundo **[inferido]**. El motor se inicializa con un `10` (`FUN_0031da4c`,
guarda `1000000 / 10`) que parece el paso de las animaciones (`updateFrame`
avanza en pasos fijos hasta alcanzar la hora actual), no un límite del dibujo
**[?]**. Las medidas del panel (80 llamadas por segundo a `getModelInfo`) dicen
que el lazo da vueltas de sobra.

### 2.3 Cómo se pinta un `Sprite`

**[código]** `Sprite::draw` (tabla virtual, posición 3, `FUN_003401ac`):

- Toma el bitmap de su imagen actual (`ImageData` en `Sprite + 0x58`; el bitmap
  queda en `+0x60`).
- Recorre los rectángulos activos (`setCurRectNum`, `Sprite + 0x6c`); cada uno
  mide 0x2c bytes y está en el arreglo de `Sprite + 0x68`. Salta los de ancho o
  alto 0.
- Por rectángulo: si no hay escala ni giro, blit directo (`FUN_00337d2e`); si el
  tamaño de destino difiere del de origen, blit con escala (`FUN_00337e58`); con
  giro, `FUN_00337f0e`. Los tres terminan en `GRM_gpermRectblit` hacia el bitmap
  de pantalla.
- Cuando la imagen es de formato GRM 4 (8 bits de alfa, las letras), usa una
  mezcla que tiñe con el color del rectángulo; con los demás formatos, copia o
  mezcla por alfa **[código para la rama del formato 4; inferido para el resto]**.

| Desplazamiento en el rectángulo | Contenido |
| --- | --- |
| `+0x00`, `+0x04` | posición (`Fixed32`), relativa al widget |
| `+0x08`, `+0x0c` | tamaño en pantalla |
| `+0x10`, `+0x14` | desplazamiento dentro de la imagen |
| `+0x18`, `+0x1c` | tamaño que se toma de la imagen |
| `+0x20` | banderas: bit 0 escalar, bit 1 repetir o parche, bit 2 el `bool` del `setRectSize` largo |
| `+0x22` | color del rectángulo (multiplica la imagen) |
| `+0x28` | ID de color de recurso |

**[código]** El escalado lo hace el motor 2D. El filtro bilineal se pide con
`Sprite::setImageFilter(true)` (bit 0 de `Sprite + 0x4c`); por omisión está
apagado, que para Doom da píxeles nítidos.

### 2.4 Una imagen es un bitmap GRM

- **[código]** `GraphicsImage::sys_getInternalObject(void *p)` devuelve `p`: el
  `GraphicsImage` de `ux` **es** el objeto bitmap de GRM.
- **[código]** `ImageData::sys_getImageData()` devuelve ese bitmap.
- **[código]** `ImageData::lock()` llama a `GRM_bitmapWriteLockBits(bmp, -1,
  &ptr)` y devuelve `ptr`; `ImageData::unlock(void*)` llama a
  `GRM_bitmapWriteUnlockBits(bmp)` e ignora su argumento.
- **[código]** El bitmap tiene un candado de lectores y escritor (`FUN_00361cdc`
  lee, `FUN_00361e10` escribe, `FUN_00361f14` suelta). `WriteLockBits` espera a
  que no haya lectores ni otro escritor: con `-1` espera sin límite, con `0` no
  espera y devuelve `0xfffffff8` si está ocupado, con otro valor espera ese
  tiempo y devuelve `0xfffffff5` si se vence. Al soltar, despierta a los que
  esperan y avisa al trabajador de GRM.
- **[inferido]** Los blits pendientes toman el bitmap como lectores; por eso
  escribir con el candado no rompe un cuadro a medio copiar. Sony escribe así
  sus propias imágenes, sin vaciar caché a mano (`FUN_003278ac`, `FUN_0084e260`),
  así que la coherencia de caché la resuelve GRM o la memoria no pasa por caché.
- **[código]** Las filas se alinean a 16 píxeles (`FUN_0034c356`; la memoria se
  calcula como `bytes × ((ancho + 15) & ~15) × alto`, `FUN_0037109c`).
  `GRM_bitmapGetRowPixelCount(bmp, u16 *)` escribe los píxeles por fila (un
  `strh`). 320 y 640 ya son múltiplos de 16.

## 3. Formato de píxel y alfa

Tabla de formatos de `ux` a GRM (`FUN_003742e2`, siete entradas en `0x1125040`)
y código del motor 2D (tabla en `0x111e620`, usada por `FUN_0036b0c6`)
**[código]**:

| Formato `ux` | GRM | Bytes | Motor 2D | Quién lo usa |
| --- | --- | --- | --- | --- |
| `0x41006` | 1 | 2 | 0 | el OSD (`uxengine_init`), las imágenes de 16 bits con alfa (`FUN_00330d32(alfa, 2)`) |
| `0x1006` | 7 | 2 | 0 | 16 bits sin alfa (`FUN_00330d32(0, 2)`) |
| `0x82000` | 2 | 4 | 1 | 32 bits con alfa: `GEN_CacheInsImg`; el modo de pantalla de 32 bits |
| `0x2000` | 8 | 4 | 1 | 32 bits sin alfa |
| `0x2000100f` | 3 | 2 | 2 | los cuadros YUV que llegan por dirección física **[inferido]** |
| `0x1008080b` | 4 | 1 | 3 | alfa de 8 bits: letras, el atlas de `FUN_003278ac` |
| `0x100a` | 6 | 2 | — | el motor 2D no lo acepta y `GRM_bitmapCreate` lo rechaza (`return -5`) |

- **[código]** Los formatos 1 y 7 van al mismo código del motor 2D, y los 2 y 8
  también: `0x1006` es el mismo arreglo de bits que `0x41006`, con la marca de
  alfa apagada. Con los números de `ux`, `0x40000` parece «4 bits de alfa» y
  `0x80000` «8 bits» **[inferido]**. El formato de 16 bits es entonces
  **4-4-4-4 con alfa**, no 5-6-5.
- **[inferido]** El orden de los canales dentro de los 16 bits no está en el
  código que leí. `libmpr.so` nombra a los dos formatos de mapa de bits
  `RGBA4444` y `RGBA8888` («DrawPixels[BitMap] … srcColorMode»), y el proyecto
  público de la A7R3, con el mismo marco de interfaz, escribe el OSD como
  `uint16 = R << 12 | G << 8 | B << 4 | A`. Esa es la hipótesis de partida; el
  cuadro de prueba de la sección 7.4 la confirma o la corrige.
- **[código]** El motor 2D convierte entre formatos (tablas `cmA` y `cmB` en
  `0x111e644` y `0x111e744`); por ejemplo, de formato 1 o 2 a YUV. Una imagen de
  32 bits sobre un OSD de 16 bits se convierte sola.
- **Alfa y video [inferido]**: el OSD lleva alfa por píxel y la vista en vivo se
  ve detrás de las zonas transparentes, como se ve hoy en la pantalla de
  grabación. Con el alfa al máximo en cada píxel (nibble `0xF`) el cuadro tapa el
  video. No encontré color clave. Si el formato `0x1006` fuerza opacidad o copia
  los cuatro bits tal cual es **[?]**: la receta usa `0x41006` y escribe el alfa.
- **Proporción [inferido]**: la LCD de la CX580V es panorámica (16:9) y el OSD
  mide 640×480, así que cada píxel del OSD es un tercio más ancho que alto. Un
  cuadro 4:3 ocupa 480×480 píxeles del OSD (centrado en `x = 80`); 640×400 llena
  el ancho pero se ve estirado. Sale de la ficha de la cámara, no del firmware.

## 4. Lo que ya hace Sony con imágenes calculadas

Cuatro usos, todos con la misma API **[código]**:

| Dónde | Qué hace |
| --- | --- |
| `GEN_CacheInsImg` (`FUN_001682f8`, `FUN_00168604`) | `createImage(ancho, alto, 0x82000)` con la arena del widget, `GraphicsImage::clear` a transparente y `Sprite::setImage(resID, 0x80)`. Para cada cuadro nuevo arma un bitmap GRM sobre memoria física (`GRM_bitmapCreateFromPhys`), lo copia con `GRM_gpermRectblit` dentro de la imagen y llama a `Sprite::setImage` y `Sprite::setImageDirty`. Mensaje de error propio: «createImage NG. size=(%d, %d)» |
| `W_GEN_ClstrThumb` (`FUN_0016ffa8`) | `createImage` con formato elegido por quien llama, `setRectImageOffset`, `setRectPos`, `setRectSize` y `setImage(resID, 0x80)` |
| `GEN_PaintCanvas` (`FUN_00183728` y siguientes) | tres imágenes; dos son un doble búfer (`+0x144`, `+0x14c`, índice en `+0x156`). Para cada cambio copia la anterior en la nueva con `GraphicsImage::copy`, dibuja en la nueva y llama a `setImageDirty` y `setImage` con el ID nuevo (`FUN_001838a8`, `FUN_00183d80`). Es un lienzo de dibujo a mano, ninguna función suya está exportada |
| `FUN_001581e8`, `FUN_001590e8` (clase sin identificar) | dos imágenes `0x41006` del tamaño del widget, rellenas con rectángulos `GraphicsImage::clear(img, color, x, y, w, h)` |

Y `ux` mismo escribe píxeles a mano en una imagen propia: `FUN_003278ac` crea
una de 128×64 en formato `0x1008080b`, la bloquea (`FUN_00330c6c`), escribe un
atlas de letras de 8×8 byte a byte y la suelta.

## 5. Símbolos

Todos en `libObj.so`, resueltos con `cx_dlsym(cx_lib_base("/libObj.so"), …)`.
Los de `ux` y GRM son Thumb (el puntero trae el bit 0); `GEN_Icon` es ARM. Con
`-marm` el compilador llama por `blx rN`, que cambia de modo según el bit 0, como
ya pasa con `GEN_Label` **[código para el modo de cada símbolo]**.

| Uso | Símbolo | Firma en C |
| --- | --- | --- |
| widget (ARM) | `_ZN8GEN_IconC1Ev` | `void *(*)(void *this)` |
| tamaño sin tocar banderas de `GEN_Icon` | `_ZN2ux6wgtsys6Widget7setSizeENS_4core7Fixed32ES3_` | `void (*)(void *w, int w16, int h16)` |
| ID de capa | `_ZN2ux4core2ID5genIDEPKc` | `u32 (*)(const char *)` |
| colgar de la capa | `_ZN2ux6wgtsys6Widget10setLayerIDERKNS_4core2IDE` | `void (*)(void *w, const u32 *id)` |
| posición | `_ZN2ux6wgtsys6Widget11setLocalPosERKNS_4core7Vector2E` | `void (*)(void *w, const int xy[2])` |
| no tapar toques | `_ZN2ux6wgtsys6Widget16setPassiveRegionEb` | `void (*)(void *w, int on)` |
| al frente | `_ZN2ux6wgtsys6Widget8moveLastEv` | `void (*)(void *w)` |
| ocultar o mostrar | `_ZN2ux6wgtsys6Widget7setModeENS1_10WidgetModeE` | `void (*)(void *w, int mode)` (3 = oculto) |
| gestor de recursos (**dato**) | `_ZN2ux6resmgr15ResourceManager10s_instanceE` | `u32 *`: su contenido es el `ResourceManagerImpl *` |
| parámetros de imagen | `_ZN2ux6resmgr16CreateImageParamC1Ettj` | `void (*)(void *p, u32 w, u32 h, u32 fmt)` (`w` y `h` son `u16`) |
| arena (opcional) | `_ZN2ux6resmgr16CreateImageParam10setArenaIDEh` | `void (*)(void *p, u32 arena)` |
| crear la imagen | `_ZN2ux6resmgr19ResourceManagerImpl11createImageERKNS0_16CreateImageParamEPNS0_9ImageDataE` | `int (*)(u32 rm, const void *p, void *img)`; distinto de 0 si salió bien |
| `ImageData` | `_ZN2ux6resmgr9ImageDataC1Ev` | `void (*)(void *img)` |
| ID de la imagen | `_ZNK2ux6resmgr9ImageData8getResIDEv` | `u32 (*)(void *img)`; `0xffffffff` si está vacía |
| bitmap GRM | `_ZN2ux6resmgr9ImageData16sys_getImageDataEv` | `u32 (*)(void *img)` |
| ancho y alto | `_ZNK2ux6resmgr9ImageData8getWidthEv`, `…9getHeightEv` | `int (*)(void *img)` (`short`) |
| bloquear | `GRM_bitmapWriteLockBits` | `int (*)(u32 bmp, int espera, void **ptr)`: 0 bien |
| soltar | `GRM_bitmapWriteUnlockBits` | `int (*)(u32 bmp)` |
| píxeles por fila | `GRM_bitmapGetRowPixelCount` | `int (*)(u32 bmp, u16 *out)` |
| imagen del sprite | `_ZN2ux6gfxeng6Sprite8setImageEjh` | `void (*)(void *spr, u32 resID, u32 arena)`; Sony pasa `0x80` |
| «cambió el contenido» | `_ZN2ux6gfxeng6Sprite13setImageDirtyEv` | `void (*)(void *spr)` |
| rectángulos activos | `_ZN2ux6gfxeng6Sprite13setCurRectNumEt` | `void (*)(void *spr, u32 n)` |
| posición del rectángulo | `_ZN2ux6gfxeng6Sprite10setRectPosEtRKNS_4core7Vector2E` | `void (*)(void *spr, u32 i, const int xy[2])` |
| origen en la imagen | `_ZN2ux6gfxeng6Sprite18setRectImageOffsetEtNS_4core7Fixed32ES3_` | `void (*)(void *spr, u32 i, int x16, int y16)` |
| tamaño sin escala | `_ZN2ux6gfxeng6Sprite11setRectSizeEtNS_4core7Fixed32ES3_` | `void (*)(void *spr, u32 i, int w16, int h16)` |
| tamaño con escala | `_ZN2ux6gfxeng6Sprite11setRectSizeEtNS_4core7Fixed32ES3_S3_S3_b` | `void (*)(void *spr, u32 i, int dw16, int dh16, int sw16, int sh16, int flag)`; los tres últimos van en la pila |
| filtro al escalar | `_ZN2ux6gfxeng6Sprite14setImageFilterEb` | `void (*)(void *spr, int on)` |
| tamaño del OSD | `_ZN2ux6gfxeng14GraphicsEngine15sys_getInstanceEv`, `_ZNK2ux6gfxeng14GraphicsEngine19getFramebufferWidthEv`, `…20getFramebufferHeightEv` | como en `ux-panel-propio.md` |

Notas de convención **[código]**:

- `Fixed32` es 16.16 en un registro; `Vector2` son dos `Fixed32` por referencia.
- Con `flag = 0` y origen distinto del destino, `setRectSize` de seis argumentos
  prende el bit 0 (escalar). Con `flag = 1` prende el bit 1, que lleva a otro
  camino de dibujo (`FUN_0033813a`, repetición o parche): no usarlo.
- `cx_dlsym` hoy sólo se usó con funciones; el `s_instance` es un `OBJECT`
  global de 4 bytes. Que `cx_dlsym` devuelva su dirección igual que con una
  función es **[inferido]** (la búsqueda es por nombre en la tabla dinámica).

### 5.1 Estructuras

| Qué | Tamaño | Campos | Marca |
| --- | --- | --- | --- |
| `GEN_Icon` | `0x1f0` | `Sprite` en `+0x134`; arena (byte) en `+0x130`, `0x80` al construir; banderas de «hay que rehacer» en `+0x1d5`, `+0x1d6`; ID de imagen de `GEN_Icon::setImage` en `+0x1e0` | [código]: `FUN_0084ec7c` arma cuatro seguidos cada `0x1f0` |
| `Sprite` | `0x8c` | imágenes en `+0x50` (pedida) y `+0x58` (actual), bitmap en `+0x60`, arena en `+0x64`, arreglo de rectángulos en `+0x68`, activos en `+0x6c`, máximo en `+0x6e` | [código]; el tamaño, [inferido] por dónde empiezan los campos de `GEN_Icon` |
| `CreateImageParam` | `0x18` | `+0` ancho, `+2` alto, `+4` formato, `+8`/`+a` ancho y alto otra vez, `+0x14` arena (`0x80`) | [código] |
| `ImageData` | 8 | `+0` registro del recurso, `+4` otro puntero; el constructor los pone en 0 | [código] |

`GEN_Icon` hace `initSprite(sprite, 9)`: su `Sprite` tiene nueve rectángulos y
los nueve activos **[código]**. Su actualización (posición 13 de la tabla
virtual, `FUN_0017b088`) sólo reacomoda los rectángulos si `+0x1d5` o `+0x1d6`
están en 1, y eso lo ponen `GEN_Icon::setSize`, `GEN_Icon::setImage` y otros
`set*` de la clase **[código]**. La receta no llama a ninguno, así que la clase no
toca el sprite. Con esas banderas en 1, `FUN_0017a710` pondría los nueve
rectángulos en 0 y luego los ajustaría al tamaño que el gestor de recursos
informe de la imagen, cosa que para una imagen creada en ejecución es **[?]**.

### 5.2 Memoria: las arenas

**[código]** `uxengine_init` crea tres arenas de imágenes, cada una sobre un
espacio de memoria GRM con nombre:

| Arena | Espacio | Acepta | Tamaño de imagen |
| --- | --- | --- | --- |
| 0 | `mpMemoryHeap` (unidad `0x40803`) | imágenes | desde `0xc800` (51 200 bytes), sin tope |
| 2 | `mpMemoryHeap2` (unidad `0x256`) | imágenes | hasta `0xc800` |
| 1 | `mpTextHeap` (unidad `0x4087b`) | texto | — |

Con arena `0x80` (la de omisión, y la que trae un `GEN_Icon` recién construido)
`createImage` elige la arena por tamaño y tipo (`FUN_0013f508`, `FUN_0037f370`)
**[código]**. Una imagen de 320×200 a 16 bits pide 128 000 bytes y cae en la
arena 0 **[inferido]**. Cuánto libre hay ahí con la pantalla de grabación
abierta es **[?]**: las unidades de memoria las reparte la infraestructura de
Sony. Si no alcanza, `createImage` devuelve 0. Hay un ajuste de depuración
(`FUN_0032193e(3, 0)`, «arena_overflow_exception») que, encendido, llamaría al
gancho de aserción; por omisión vale 0 **[inferido]**.

## 6. Dónde y cuándo llamar

- **Todo en el hilo de `AppSystem::Main`** **[código para el modelo de hilos,
  `ux-panel-propio.md` §2]**. El gestor de recursos y el árbol de widgets no
  tienen candado. Sólo el candado del bitmap es seguro entre hilos **[código]**,
  pero conviene igual escribir desde el hilo de la interfaz.
- **Crear** (icono, imagen, sprite) después de la `openView` original, en la fase
  3, como las etiquetas del panel.
- **Actualizar el cuadro** en cualquier envoltura del hilo de la interfaz:
  `getModelInfo` (unas 80 veces por segundo) sirve de reloj; se pinta un cuadro
  cada 28 ms o más. El cambio se dibuja en la fase 5 de esa misma vuelta
  (`drawFramebuffer`) **[inferido]**.
- **Invalidar** hace falta: escribir en el bitmap no avisa a nadie. La forma de
  Sony es `Sprite::setImageDirty` (más `setImage` con el mismo ID) **[código]**.
  `setImageDirty` suelta la imagen actual del sprite, lo marca sucio y la vuelve
  a pedir por ID al gestor de recursos (`FUN_0033e902`, `FUN_00140150`)
  **[código]**. Si esa nueva petición se resuelve en el acto o en el hilo de
  recursos es **[?]**; en el segundo caso podría verse un cuadro sin imagen. La
  alternativa (7.5) sólo marca sucio, sin volver a pedir la imagen.
- **No bloquear la interfaz**: bloquear con espera `0`. Si el bitmap sigue en uso
  por un blit, se salta ese cuadro.

## 7. Receta en C

Propuesta para un módulo nuevo (por ejemplo `cxframe.c`) con el estilo de
`cxpanel.c`. **No está compilada ni probada.** Quedan 1 084 bytes libres en
`cxg.so` (`docs/panel-propio.md`): esto no cabe sin hacer sitio, y el motor del
juego tendrá que cargarse aparte.

### 7.1 Símbolos y estado

```c
#include "cx.h"

enum { F_ICON, F_WSIZE, F_GENID, F_LAYER, F_POS, F_PASSIVE, F_LAST, F_NEXT, F_MODE,
       F_CIP, F_CREATE, F_IMG, F_RESID, F_BMP, F_LOCK, F_UNLOCK, F_ROWPIX,
       F_SETIMG, F_DIRTY, F_CURRECT, F_RPOS, F_ROFF, F_RSIZE, F_RSIZE6, F_FILTER,
       F_RM, NFN };
static const char *const sym[NFN] = {
    "_ZN8GEN_IconC1Ev",                                                  /* ARM */
    "_ZN2ux6wgtsys6Widget7setSizeENS_4core7Fixed32ES3_",
    "_ZN2ux4core2ID5genIDEPKc",
    "_ZN2ux6wgtsys6Widget10setLayerIDERKNS_4core2IDE",
    "_ZN2ux6wgtsys6Widget11setLocalPosERKNS_4core7Vector2E",
    "_ZN2ux6wgtsys6Widget16setPassiveRegionEb",
    "_ZN2ux6wgtsys6Widget8moveLastEv",
    "_ZNK2ux6wgtsys6Widget14getNextSiblingEv",
    "_ZN2ux6wgtsys6Widget7setModeENS1_10WidgetModeE",
    "_ZN2ux6resmgr16CreateImageParamC1Ettj",
    "_ZN2ux6resmgr19ResourceManagerImpl11createImageERKNS0_16CreateImageParamEPNS0_9ImageDataE",
    "_ZN2ux6resmgr9ImageDataC1Ev",
    "_ZNK2ux6resmgr9ImageData8getResIDEv",
    "_ZN2ux6resmgr9ImageData16sys_getImageDataEv",
    "GRM_bitmapWriteLockBits",
    "GRM_bitmapWriteUnlockBits",
    "GRM_bitmapGetRowPixelCount",
    "_ZN2ux6gfxeng6Sprite8setImageEjh",
    "_ZN2ux6gfxeng6Sprite13setImageDirtyEv",
    "_ZN2ux6gfxeng6Sprite13setCurRectNumEt",
    "_ZN2ux6gfxeng6Sprite10setRectPosEtRKNS_4core7Vector2E",
    "_ZN2ux6gfxeng6Sprite18setRectImageOffsetEtNS_4core7Fixed32ES3_",
    "_ZN2ux6gfxeng6Sprite11setRectSizeEtNS_4core7Fixed32ES3_",
    "_ZN2ux6gfxeng6Sprite11setRectSizeEtNS_4core7Fixed32ES3_S3_S3_b",
    "_ZN2ux6gfxeng6Sprite14setImageFilterEb",
    "_ZN2ux6resmgr15ResourceManager10s_instanceE",                        /* dato */
};
static u32 fn[NFN];

#define ICON_BYTES 0x1f0
#define ICON_SPRITE 0x134
#define FMT_RGBA4444 0x41006
#define SRC_W 320
#define SRC_H 200

static u32 icon[ICON_BYTES / 4 + 4] __attribute__((aligned(8)));
static u32 img[2] __attribute__((aligned(8)));   /* ux::resmgr::ImageData */
static u32 bmp, res_id, pitch;                   /* bitmap GRM, ID, píxeles por fila */
static int ready;
```

`resolve()` igual que en `cxpanel.c`: si falta un símbolo, no se crea nada.

### 7.2 Crear

```c
/* dx, dy, dw, dh: dónde y de qué tamaño se ve, en píxeles del OSD.
 * Por ejemplo 80, 0, 480, 480 (4:3 en una LCD 16:9) o 0, 40, 640, 400. */
static int frame_create(int dx, int dy, int dw, int dh)
{
    u32 W = (u32)icon, S = W + ICON_SPRITE, id, rm, cip[6];
    u32 rp = 0;
    int xy[2], zero[2] = { 0, 0 };
    unsigned k;

    for (k = 0; k < sizeof(icon) / 4; k++) icon[k] = 0;
    ((u32 (*)(u32))fn[F_ICON])(W);                                  /* GEN_Icon::GEN_Icon */
    id = ((u32 (*)(const char *))fn[F_GENID])("A_PHYSICAL_LAYER");
    ((void (*)(u32, const u32 *))fn[F_LAYER])(W, &id);
    /* Widget::setSize de la base: GEN_Icon::setSize pondría +0x1d5 = 1 y la
     * clase rehacería los rectángulos a su manera. El tamaño del widget recorta. */
    ((void (*)(u32, int, int))fn[F_WSIZE])(W, dw << 16, dh << 16);
    xy[0] = dx << 16; xy[1] = dy << 16;
    ((void (*)(u32, const int *))fn[F_POS])(W, xy);
    ((void (*)(u32, int))fn[F_PASSIVE])(W, 1);

    /* La imagen: 320x200, 16 bits con alfa, arena 0x80 (la elige el gestor). */
    ((void (*)(u32 *))fn[F_IMG])(img);
    ((void (*)(u32 *, u32, u32, u32))fn[F_CIP])(cip, SRC_W, SRC_H, FMT_RGBA4444);
    rm = *(const u32 *)fn[F_RM];
    if (!rm || !((int (*)(u32, const u32 *, u32 *))fn[F_CREATE])(rm, cip, img))
        return 0;                                    /* anotar: arena sin sitio */
    res_id = ((u32 (*)(u32 *))fn[F_RESID])(img);
    bmp = ((u32 (*)(u32 *))fn[F_BMP])(img);
    ((int (*)(u32, u32 *))fn[F_ROWPIX])(bmp, &rp);  /* escribe 16 bits */
    pitch = rp & 0xffff;
    if (res_id == 0xffffffffu || !bmp || pitch < SRC_W) return 0;

    /* Un rectángulo: toda la imagen, escalada al tamaño del widget. */
    ((void (*)(u32, u32))fn[F_CURRECT])(S, 1);
    ((void (*)(u32, u32, const int *))fn[F_RPOS])(S, 0, zero);
    ((void (*)(u32, u32, int, int))fn[F_ROFF])(S, 0, 0, 0);
    ((void (*)(u32, u32, int, int, int, int, int))fn[F_RSIZE6])
        (S, 0, dw << 16, dh << 16, SRC_W << 16, SRC_H << 16, 0);
    ((void (*)(u32, u32, int, int))fn[F_RSIZE])(S, 1, 0, 0);       /* para 7.5 */
    ((void (*)(u32, int))fn[F_FILTER])(S, 0);                       /* píxeles nítidos */
    ((void (*)(u32, u32, u32))fn[F_SETIMG])(S, res_id, 0x80);
    ready = 1;
    return 1;
}
```

Anotar en la bitácora, antes de cada paso, como hace `make()` en `cxpanel.c`; y
una vez: `res_id`, `bmp`, `pitch`, `getFramebufferWidth/Height`.

### 7.3 Cada cuadro

```c
static u16 pal16[256];                    /* RGBA4444 hipotético, ver 7.4 */

void frame_palette(const u8 *rgb)        /* 256 tríos de 8 bits (PLAYPAL) */
{
    int i;
    for (i = 0; i < 256; i++)
        pal16[i] = (u16)(((rgb[3 * i] >> 4) << 12) | ((rgb[3 * i + 1] >> 4) << 8) |
                         ((rgb[3 * i + 2] >> 4) << 4) | 0xf);
}

/* Desde el hilo de la interfaz. 1: se pintó; 0: el bitmap estaba ocupado. */
int frame_push(const u8 *src)            /* SRC_W x SRC_H índices de paleta */
{
    u32 S = (u32)icon + ICON_SPRITE;
    void *p = 0;
    u16 *d;
    int x, y;
    if (!ready) return 0;
    if (((int (*)(u32, int, void **))fn[F_LOCK])(bmp, 0, &p) != 0 || !p) return 0;
    d = p;
    for (y = 0; y < SRC_H; y++, d += pitch, src += SRC_W)
        for (x = 0; x < SRC_W; x++)
            d[x] = pal16[src[x]];
    ((int (*)(u32))fn[F_UNLOCK])(bmp);
    ((void (*)(u32))fn[F_DIRTY])(S);                       /* como GEN_PaintCanvas */
    ((void (*)(u32, u32, u32))fn[F_SETIMG])(S, res_id, 0x80);
    return 1;
}
```

Y en la envoltura de `getModelInfo`, con `cx_now`:

```c
    u32 frac, ms = cx_now(&frac) * 1000 + frac;     /* frac ~ 1,05 ms */
    if (ready && ms - last_ms >= 28) { last_ms = ms; doom_tick(); frame_push(doom_buf); }
```

Después de cada `openView` original, si `getNextSibling(icon) != 0`, llamar
`moveLast(icon)` para seguir encima, como con las etiquetas. Para salir,
`setMode(icon, 3)`; la imagen y el widget quedan en memoria estática y no se
destruyen.

### 7.4 Primer cuadro: prueba de formato

Antes de Doom, llenar la imagen con ocho franjas verticales de 40 píxeles con
estos valores de 16 bits y fotografiar la pantalla:

| Franja | Valor | Si es `R G B A` (alto a bajo) | Si es `A R G B` |
| --- | --- | --- | --- |
| 1 | `0xF00F` | rojo | azul |
| 2 | `0x0F0F` | verde | transparente |
| 3 | `0x00FF` | azul | transparente |
| 4 | `0xFFFF` | blanco | blanco |
| 5 | `0x000F` | negro | transparente |
| 6 | `0xF000` | transparente (se ve el video) | negro |
| 7 | `0x8888` | gris a medias, mitad transparente | gris a medias, mitad transparente |
| 8 | `0xF0F0` | transparente | verde |

Con la foto se arma `pal16` con el orden real (si rojo y azul salen cambiados,
es `B G R A`). Si las franjas salen corridas o en diagonal, `pitch` no es el que
se cree.

### 7.5 Invalidar sin volver a pedir la imagen

Si con `setImageDirty` se ve un parpadeo (un cuadro sin imagen), cambiar las dos
últimas líneas de `frame_push` por:

```c
    static int flip;
    flip ^= 1;
    ((void (*)(u32, u32))fn[F_CURRECT])(S, flip ? 2 : 1);
```

`setCurRectNum` marca sucio el sprite sólo cuando el número cambia, y el dibujo
salta los rectángulos de tamaño 0 **[código]**; el rectángulo 1 se puso en 0 al
crear. Que esto baste para que se vuelva a dibujar la imagen ya cargada es
**[inferido]**: no pasa por el gestor de recursos.

## 8. Planes B

1. **Si `createImage` devuelve 0** (arena sin sitio): probar tamaños crecientes
   (64×64, 160×100, 320×200) y anotar cuál falla. Luego, una imagen de 320×100
   escalada a 640×400 con media resolución vertical, o dos imágenes de 320×100 en
   dos `GEN_Icon`. `0x82000` (32 bits, el formato de `GEN_CacheInsImg`) pide el
   doble de memoria y sólo conviene si el orden de 16 bits resulta imposible.
2. **Si hay desgarro** (medio cuadro viejo, medio nuevo): doble búfer como
   `GEN_PaintCanvas`, dos imágenes y dos IDs, escribir en la que no se ve y
   pasar el sprite a ella con `Sprite::setImage(otro_id, 0x80)`. Un ID distinto
   obliga a una petición nueva al gestor (`FUN_00140150`), así que el cambio
   puede llegar un cuadro tarde **[inferido]**.
3. **Diagnóstico sin `ux`**: `GRM_screenGetOnBitmap(u32 *out)` (0 si sale bien)
   da el bitmap que está en pantalla **[código]**. Su formato y medidas se leen
   por su tabla virtual, como hace GRM por dentro: `+0x28` formato
   (`int (*)(u32, int *)`), `+0x18` ancho y alto (`int (*)(u32, u16 *, u16 *)`),
   `+0x20` bytes por fila **[código]**. Bloquear, escribir las franjas de 7.4 en
   una esquina y soltar muestra el formato real del OSD de inmediato, pero el
   compositor lo pisa en la siguiente pasada que toque esa zona, y con dos
   búferes parpadea. Sirve para ver el formato; no para Doom.
4. **Plano YUV** (`GRM_fbrm*YUV`), con menos confianza. GRM trae una prueba
   interna, `CmdTestYuvDraw` (`FUN_003528de`), que hace `GRM_fbrmFillrectYUV`
   cinco veces, `GRM_fbrmPrepareUpdateYUV()` y `GRM_fbrmUpdateYUV(0, …, 0)`
   **[código]**; la reproducción de Sony usa `GRM_fbrmRectblitYUV(bmp, &src,
   &dst, …, 0)` con bitmaps YUV compuestos (`FUN_001a6a94`) **[código]**. El
   motor 2D acepta copiar de formato 1 a YUV (`cmA`, fila del formato 3)
   **[código]**, así que se podría copiar la imagen de 7.2 a ese plano. Quién es
   dueño del plano en modo grabación, y si iniciarlo corta la vista en vivo, es
   **[?]**.

## 9. Riesgos

- **Que la interfaz se caiga.** Un símbolo mal resuelto, un `GEN_Icon` con
  memoria corta o un argumento de más en `setRectSize` (tres van en la pila)
  tumban `im.elf`. Mismo resguardo que el panel: opción de `cxmode.txt` apagada
  por omisión, bitácora antes de cada paso y el camino de rescate de siempre
  (`AGENTS.md`).
- **Bloqueos.** `ImageData::lock()` espera sin límite; la receta usa espera 0.
  Nunca llamar a nada de `ux` con el bitmap bloqueado: el dibujo toma el mismo
  candado como lector **[inferido]** y se trabaría.
- **Memoria de la arena compartida.** La imagen sale de la misma arena que los
  fondos y los iconos grandes de los menús. Ocupar 128 KB de forma permanente
  puede hacer que una pantalla de Sony no consiga sus imágenes **[inferido]**.
  Lo de Sony tiene su propio mensaje de error («createImage NG»); que alguna
  pantalla falle de forma visible es **[?]**.
- **Costo del dibujo.** Cada cuadro marca sucia la zona del icono en los dos
  búferes; el compositor repinta todo lo que haya debajo antes del icono. Ocultar
  lo nativo de esa zona (el recorrido de `cxpanel.c`) baja el trabajo
  **[inferido]**.
- **Capas encima.** Otras capas físicas (`B_PHYSICAL_LAYER`…) y los avisos de
  Sony pueden quedar por encima del icono; su orden es **[?]**. `Layer::moveLast`
  está exportada, pero reordenar capas de Sony no es reversible a ciegas.
- **El tacto sigue yendo a lo nativo** que hay debajo, salvo que `cxtouch.c` se
  lo trague. `setPassiveRegion(1)` evita que el icono mismo robe toques.
- **Bits por píxel de la pantalla.** Si algo pide el modo de 32 bits mientras se
  juega, el motor 2D convierte solo **[código]**; el costo extra es **[?]**.
- **Tamaño de `cxg.so`.** No hay lugar para esta receta sin quitar código o subir
  el tope del instalador.

## 10. Lo que no se pudo determinar

- El orden de los canales en `0x41006` (la prueba de 7.4 lo decide).
- Cuánta memoria libre tiene la arena 0 (`mpMemoryHeap`) con la pantalla de
  grabación abierta.
- Si la petición que dispara `setImageDirty` se resuelve en el acto o en el hilo
  de recursos (`GraphicsEngine::updateResource`), y por tanto si hay parpadeo.
- Si el motor 2D toma el bitmap como lector mientras el blit está pendiente (de
  eso depende que no haya desgarro) y si GRM vacía la caché al soltar.
- Si `0x1006` fuerza opacidad.
- Qué significa el `10` con que se inicializa el motor gráfico.
- El orden de las capas físicas y si hay alguna por encima de `A_PHYSICAL_LAYER`.
- El dueño del plano YUV en modo grabación.

## 11. Direcciones clave

| Qué | Dirección |
| --- | --- |
| `uxengine_init` (OSD 640×480 formato 1, arenas) | `0x13a6cc` |
| `drawFramebuffer` | `0x13a128` |
| fase de dibujo de `AppSystem::Main` | `0x8d3330` |
| `Sprite::draw` / un rectángulo | `0x3401ac` / `0x33f804` |
| blit sin escala / con escala | `0x337d2e` / `0x337e58` |
| invalidar un elemento | `0x32a694` |
| `GEN_Icon::GEN_Icon` / actualización | `0x179d18` / `0x17b088` |
| `GEN_CacheInsImg` crea y actualiza | `0x1682f8`, `0x168604` |
| `GEN_PaintCanvas` (doble búfer) | `0x183728`, `0x1838e0`, `0x183d80`, `0x1838a8` |
| tabla de formatos `ux` → GRM | `0x3742e2` (datos en `0x1125040`) |
| bytes por píxel | `0x34ab88` |
| candado del bitmap (leer, escribir, soltar) | `0x361cdc`, `0x361e10`, `0x361f14` |
| motor 2D (registros y tablas de formato) | `0x36b0c6` |
| prueba YUV interna de GRM | `0x3528de` |
