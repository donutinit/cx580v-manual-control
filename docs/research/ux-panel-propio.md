# Panel propio dentro de la interfaz `ux`: texto en pantalla y entrada

Estudio estático del 2026-11-26 sobre `libObj.so` del firmware 1.07, hecho con
Ghidra sobre una copia privada del proyecto. **Nada de esto se probó en la
cámara.** Cada afirmación lleva una marca:

- **[código]**: lo vi en el decompilado o en la tabla de reubicaciones.
- **[inferido]**: se deduce de lo que vi, pero no lo comprobé.
- **[?]**: no lo pude determinar.

Las direcciones son de Ghidra (dirección en el archivo + `0x10000`, como en
`firmware-analysis.md`). Los símbolos se resuelven igual que en `cxview.c`:
`cx_dlsym(cx_lib_base("/libObj.so"), "<nombre>")`. Casi todo `ux` es Thumb; el
puntero que devuelve `cx_dlsym` ya trae el bit 0 y la llamada por puntero desde
ARM funciona igual que en `cxtext.c`.

## 1. Resumen

- **Texto (A).** Lo más corto es crear una `GEN_Label` propia (0x290 bytes, con
  constructor exportado), colgarla de la capa física `A_PHYSICAL_LAYER` con
  `Widget::setLayerID` y escribirle con `TextSprite::setString(char const*)`.
  Sony hace exactamente eso, con otro widget, en la pantalla de calibración del
  panel táctil (`FUN_005d2724`). Antes conviene un experimento sin reservar
  memoria: cambiar el texto de una etiqueta de fábrica dentro de la envoltura
  de `TextSprite::setString(unsigned int)` que ya existe en `cxtext.c`.
- **Tacto (B1).** `WidgetSystem::postMouseMove/Press/Release` se llaman por la
  PLT desde la máquina de estados táctil de `InputService`, en el hilo de la
  interfaz. Interponerlas permite ver cada toque (x, y, apoyar y soltar) y
  tragárselo con sólo no llamar a la original.
- **Teclas (B2).** Los botones físicos no pasan por `ux::wgtsys::KeyEvent`.
  `InputService` los convierte en un `Event` de la aplicación con un número
  propio para cada botón (PHOTO `0x15020040`, START/STOP `0x15020030`…) y los
  manda por `EventManager::push(Event*)`, que se llama por la PLT desde un hilo
  de Infra. Interponerla deja ver y descartar cualquier tecla.
- **Hilos.** Hay un solo hilo de interfaz, `AppSystem::Main()`, que despacha
  eventos, actualiza widgets y dibuja. La cola de eventos `ux` no tiene
  candado: todo lo que toque `ux` tiene que correr en ese hilo.

## 2. Modelo de hilos

**[código]** `FUN_008c9680` es `AppSystem::Main()` (imprime
`"AppSystem::Main() handle AFW::EVENT_APP_SYSTEM_END"`). Es un lazo con estas
fases, en orden:

1. `sleep` / `wake`: espera eventos.
2. `p update`: `FUN_008d34d0`, que llama a la actualización de `WidgetSystem`
   (`FUN_00345988`, gestos) y vacía la cola de eventos `ux`
   (`FUN_00345672` → `FUN_0034564c`, que despacha según el tipo). Aquí corren
   los `onMouseEvent`/`onKeyEvent` virtuales de los widgets.
3. `p in queue`: despacha la cola de eventos AFW (`FUN_008c8f68`). Aquí corren
   las vistas (`ViewBase::openView`, `getModelInfo`…) y los manejadores de
   `InputService`, que llaman a `postMouse*`.
4. `updateWidget` (`FUN_008d2f5c`, `FUN_008d2600`).
5. `draw in` / `draw out` (`FUN_008d3330`).

Consecuencias:

- **[código]** La cola de eventos `ux` (`FUN_00346d78` mete, `FUN_00346d9a`
  saca) es una lista enlazada sin candado. `postMouse*` sólo es seguro en el
  hilo de `AppSystem::Main`.
- **[código]** `EventManager::push` sí toma un mutex (`__libc_mutex_lock` en
  `this+0xc`, `FUN_008cadd0`). Es la puerta entre hilos: los hilos de Infra
  (teclas, tacto, dial) meten eventos ahí y `AppSystem::Main` los despacha.
- **[código]** Las devoluciones de llamada de `InputService` (por ejemplo
  `FUN_0019c454`, «callback_convert_cap») usan `osal_wai_sem_tmo` y crean el
  `Event` en el hilo de Infra.
- **[inferido]** Las envolturas que ya tiene `cxview.c` corren en
  `AppSystem::Main`, porque las vistas sólo se ejecutan en la fase 3. No hay
  que tomar ningún candado de `WidgetSystem` (no existe), pero sí comprobar el
  hilo. Recomendación: guardar `gettid()` (syscall 224 en ARM EABI) en la
  primera llamada a la envoltura de `ViewBase::openView` y no tocar `ux` si el
  hilo actual es otro.
- Los cambios de estructura (crear, colgar, `moveLast`) conviene hacerlos en la
  fase 3: después de llamar a la `openView` original, o dentro de las
  envolturas de `postMouse*`. `setString` y `drawText` sobre la etiqueta propia
  se pueden hacer desde cualquier envoltura del hilo de la interfaz.

## 3. Receta A: texto propio sobre la vista en vivo

### 3.1 Lo que se verificó del framework

- **[código]** Toda llamada interna de `libObj.so` a una función exportada pasa
  por su PLT (la biblioteca no tiene `DT_SYMBOLIC` y cada exportada tiene su
  thunk; por ejemplo `postMouseMove` en `0x12fd1c`). Interponer una exportada
  atrapa también las llamadas internas.
- **[código]** `TextSprite::setString(unsigned int)` sólo la llama
  `GEN_Label::setText` (en `libObj.so`; ni `viewUnified2..7.so` ni
  `libWidgetMap.so` la importan). En la envoltura de `cxtext.c`, `self - 0x144`
  es siempre la `GEN_Label` dueña del sprite.
- **[código]** Capas: `FUN_008d3178` crea las capas físicas `A_PHYSICAL_LAYER`,
  `B_PHYSICAL_LAYER`… con `ID::genID(nombre)`, `WidgetSystem::createLayer`,
  `Layer::setActive` y `Layer::setClear`. Cada `Layer` (0x1c bytes) tiene su
  widget raíz (0x118 bytes) en `+0x10`, su pantalla gráfica en `+0x18` y su ID
  en `+0x14`. Las capas no se destruyen.
- **[código]** `Widget::setLayerID(id)` cuelga el widget como último hijo de la
  raíz de esa capa (`FUN_003439a0`) y su nodo gráfico de la pantalla de la capa
  (`FUN_0032a766`). El constructor de `Widget` ya lo cuelga de una capa por
  omisión, y Sony vuelve a llamar a `setLayerID` justo después (ver 3.3).
- **[código]** `Widget::~Widget` desengancha el widget del árbol y de
  `WidgetSystem` (`FUN_00346986`, `FUN_00343ab4`) y suelta a sus hijos uno por
  uno, sin destruirlos. Si una vista se cierra, un hijo ajeno queda huérfano,
  no liberado.
- **[código]** `Widget::moveLast()` vuelve a poner el widget al final de los
  hijos de su padre y su nodo al final de la lista gráfica.
- **[código]** `Widget::setMode(3)` lo oculta (`Element::setVisible(false)`);
  cualquier otro modo lo deja visible.
- **[código]** `ux::core::Fixed32` es un entero 16.16 que se pasa por valor en
  un registro (`640.0` = `0x2800000`). `Vector2` son dos `Fixed32`, `{x, y}`,
  8 bytes, y se pasa por referencia. `ux::core::ID` son 4 bytes;
  `ID::genID(char const*)` lo devuelve en `r0`.

### 3.2 `GEN_Label`: tamaño y campos

**[código]** `sizeof(GEN_Label) = 0x290`: `FUN_00859d64` arma un arreglo con
`__cxa_vec_ctor(…, 5, 0x290, GEN_Label::GEN_Label, ~GEN_Label)` y pone cuatro
`GEN_Label` seguidas cada `0x290`.

| Desplazamiento | Contenido |
| --- | --- |
| `+0x000` | `ux::wgtsys::Widget` (vtable, árbol en `+0x04`, nodo en `+0x1c`, nodo interno en `+0x90`, modo en `+0x19`) |
| `+0x138` | `ux::core::ID` |
| `+0x13c`, `+0x140` | ancho y alto (`Fixed32`) que guarda `GEN_Label::setSize` |
| `+0x144` | `MultiLineTextSprite` (un `TextSprite`): éste es el puntero para `setString`, `setFont`, `drawText` |
| `+0x258` | `LayoutAttributeWriter` |
| `+0x264` … `+0x278` | seis `u32` de fuente: id, tamaño, estilo, alineación horizontal, dirección y uno más. El constructor los pone en `0xffffffff` |
| `+0x284` | autolínea (byte) |
| `+0x285` | «sucio» (byte); lo ponen en 1 todos los `set*` |
| `+0x288` | `Dictionary*` de `setVariantInfo` (lo libera el destructor) |
| `+0x28c` | último número de texto de `setText` |

Jerarquía **[código]**: `GEN_Label` → `GeneralWidgetBase` → `FUN_00191fa8`
(base de aplicación) → `AbstractAppWidgetBase` → `ux::wgtsys::Widget`. Ninguno
de esos constructores toca una vista ni una lista global, salvo el de `Widget`,
que incrementa un contador de `WidgetSystem` y llama a `setLayerID` con la capa
por omisión.

**Memoria.** `malloc`, `free`, `operator new` (`_Znwj`) y `operator delete`
(`_ZdlPv`) no están en `libObj.so`: los importa de `libsonyc.so` y
`libsonystdc++.so.6`. **[código]** Esas dos bibliotecas (en el initrd de
`nflasha3`) sólo traen `DT_GNU_HASH`, y `cx_dlsym` busca con la tabla SysV, así
que hoy no las encuentra. Lo más simple es un búfer estático en `cxg.so`:

```c
static u32 cx_label_mem[0x290 / 4] __attribute__((aligned(8)));
```

Nadie lo va a liberar: el padre sólo desengancha hijos (ver 3.1) y nosotros
nunca destruimos la etiqueta. La otra salida es enseñarle a `cxdl.c` la tabla
GNU.

### 3.3 La plantilla de Sony

**[código]** `FUN_005d2724` pertenece a la vista de calibración del panel
táctil (`[VIEWTP]`, `FUN_005d2b70`) y hace esto:

1. `id = ux::core::ID::genID("A_PHYSICAL_LAYER")`
2. `w = malloc(500)` y un constructor de widget con un `Sprite` de color
3. `Widget::setLayerID(w, &id)`
4. `Widget::setLocalPos(w, Vector2(0, 0))`
5. `w->setSize(0x2800000, 0x1e00000)`, es decir 640 × 480, y color negro
6. Luego cuatro cuadritos para la cruz, que `FUN_005d25c8` mueve con
   `setLocalPos` a la posición del toque

Esto confirma que colgar un widget creado a mano en `A_PHYSICAL_LAYER` lo dibuja
en pantalla, y sugiere que el OSD es de 640 × 480 **[inferido]**.

### 3.4 A0: experimento mínimo, sin memoria nueva

Cambia en pantalla una etiqueta de fábrica por un texto libre. Sirve para
comprobar que `setString(char const*)` dibuja ASCII con la fuente de la cámara.

1. Con `textlog 1` (ya existe en `cxtext.c`) anotar los números de texto que
   aparecen en la pantalla de grabación y elegir uno que se vea siempre y
   estorbe poco.
2. En `cxw_setstr` (envoltura de `_ZN2ux6gfxeng10TextSprite9setStringEj`): si
   `id` es el elegido, llamar a la original
   `_ZN2ux6gfxeng10TextSprite9setStringEPKc` con `(self, "F2.8 1/60 12dB")`
   en lugar de `setString(self, id)`.

El sprite sólo se toca durante una llamada de fábrica, así que no hay problema
de vida del objeto. El texto cambia únicamente cuando la interfaz vuelve a
poner esa etiqueta. **[inferido]** Si el texto es más ancho que la etiqueta, se
recorta.

### 3.5 A1: etiqueta propia y permanente

Símbolos (todos en `libObj.so`; los nombres salen de
`libObj-dynsyms-mangled.txt` o de las reubicaciones):

| Uso | Nombre | Firma en C |
| --- | --- | --- |
| constructor | `_ZN9GEN_LabelC1Ev` | `void *(*)(void *this)` |
| tamaño | `_ZN9GEN_Label7setSizeEN2ux4core7Fixed32ES2_` | `void (*)(void *, int w16, int h16)` |
| fuente | `_ZN9GEN_Label7setFontEj` | `void (*)(void *, u32 font)` |
| fuente (sprite) | `_ZN2ux6gfxeng10TextSprite7setFontEPjh` | `void (*)(void *spr, u32 *attr, u8 n)` |
| texto | `_ZN2ux6gfxeng10TextSprite9setStringEPKc` | `void (*)(void *spr, const char *utf8)` |
| render | `_ZN2ux6gfxeng10TextSprite8drawTextEv` | `int (*)(void *spr)` |
| ID de capa | `_ZN2ux4core2ID5genIDEPKc` | `u32 (*)(const char *)` |
| colgar de capa | `_ZN2ux6wgtsys6Widget10setLayerIDERKNS_4core2IDE` | `void (*)(void *w, const u32 *id)` |
| capa de otro | `_ZNK2ux6wgtsys6Widget10getLayerIDEv` | `const u32 *(*)(void *w)` |
| padre | `_ZNK2ux6wgtsys6Widget9getParentEv` | `void *(*)(void *w)` |
| posición | `_ZN2ux6wgtsys6Widget11setLocalPosERKNS_4core7Vector2E` | `void (*)(void *w, const int xy[2])` |
| al frente | `_ZN2ux6wgtsys6Widget8moveLastEv` | `void (*)(void *w)` |
| ¿hay algo encima? | `_ZNK2ux6wgtsys6Widget14getNextSiblingEv` | `void *(*)(void *w)` |
| ocultar o mostrar | `_ZN2ux6wgtsys6Widget7setModeENS1_10WidgetModeE` | `void (*)(void *w, int mode)` |
| no tapar toques | `_ZN2ux6wgtsys6Widget16setPassiveRegionEb` | `void (*)(void *w, int on)` |
| tamaño del OSD | `_ZN2ux6gfxeng14GraphicsEngine15sys_getInstanceEv`, `_ZNK2ux6gfxeng14GraphicsEngine19getFramebufferWidthEv`, `_ZNK2ux6gfxeng14GraphicsEngine20getFramebufferHeightEv` | `void *(*)(void)`, `int (*)(void *ge)` (devuelve `short`) |

Secuencia (todo en el hilo de `AppSystem::Main`):

1. **Conseguir la fuente** [código para los desplazamientos, inferido para el
   uso]. En `cxw_setstr`, con `lab = self - 0x144`, si
   `((u32 *)(lab + 0x264))[0] != 0xffffffff`, copiar los 24 bytes de
   `lab + 0x264` a un arreglo propio `font6[6]` (una sola vez, de una etiqueta
   que se vea en la pantalla de grabación). Anotar también los seis valores en
   la bitácora: hoy no se sabe ningún id de fuente.
2. **Crear** (la primera vez que se tenga `font6`, después de la `openView`
   original): `memset(cx_label_mem, 0, 0x290)` y
   `GEN_LabelC1(cx_label_mem)`. El constructor arma el sprite, lo engancha al
   nodo del widget (`Widget::initNode`), pone el recorte en 0 × 0 y el texto en
   `""`.
3. **Capa**: `u32 id = genID("A_PHYSICAL_LAYER"); setLayerID(L, &id);`. Si el
   nombre no existe, `getLayer` devuelve 0 y `setLayerID` lo desreferencia: la
   cadena tiene que ser exacta. Alternativa: subir con `getParent` desde la
   etiqueta de la fuente hasta el hijo directo de la raíz y usar su
   `getLayerID()` (sólo devuelve la capa real para un hijo directo de la raíz;
   para los demás devuelve la capa por omisión) **[código]**.
4. **Tamaño**: `GEN_Label::setSize(L, 240 << 16, 28 << 16)`. Sin esto el
   recorte queda en 0 × 0 y no se ve nada **[código]**.
5. **Fuente**: copiar `font6` a `L + 0x264` y llamar
   `GEN_Label::setFont(L, font6[0])`. Esa función guarda `[0x264]` y le pasa
   los seis valores al sprite con `TextSprite::setFont(L + 0x144, L + 0x264, 6)`
   **[código]**.
6. **Posición**: `int xy[2] = { x << 16, y << 16 }; setLocalPos(L, xy);`.
   Coordenadas en píxeles del OSD, relativas a la capa.
7. **Que no robe toques**: `setPassiveRegion(L, 1)`. Pone la bandera 2 y apaga
   algo en los dos nodos [código]. Que eso quite la etiqueta de la prueba de
   toque es [inferido].
8. **Texto**: `setString(L + 0x144, "F2.8 1/60 12dB")`,
   `((u8 *)L)[0x285] = 1` y `drawText(L + 0x144)`. `setString` convierte el
   UTF-8, marca el sprite como sucio y pide redibujar [código]. `GEN_Label`
   sólo llama a `drawText` en ciertos casos; que haga falta llamarlo a mano
   es [?], pero `drawText` sale enseguida si el sprite no está sucio, así que
   llamarlo no estorba [código].
9. **Mantenerla encima**: después de cada `openView` original, si
   `getNextSibling(L) != 0`, llamar `moveLast(L)`. Las vistas nuevas cuelgan
   sus widgets después de la etiqueta y la taparían. Que el orden de los hijos
   sea el orden de dibujo es [inferido].
10. **Actualizar**: cuando cambien iris, obturación o ganancia (ya pasan por
    las envolturas de `cxobj.c`/`cxview.c`), repetir el paso 8 sólo si el texto
    cambió.
11. **Ocultar o mostrar**: `setMode(L, 3)` y `setMode(L, 0)` [código para el 3;
    que 0 sea el modo normal es inferido: el constructor lo deja en 0].

Cierre de vistas: la etiqueta cuelga de la raíz de la capa, que vive mientras
corre la aplicación, así que no le pasa nada cuando se cierran las vistas
[código]. No hay que destruirla. Si alguna vez hiciera falta,
`_ZN9GEN_LabelD1Ev` (no libera la memoria) y nada más, porque el búfer es
estático.

## 4. Receta B: tacto y teclas

### 4.1 Cómo llega el tacto

**[código]** La cadena es:

1. InfraTp (otro hilo) arma un `Event` AFW con los datos del toque en el
   `ParamList` (clave `0x17` o `0x1a`, tipo 6) y lo mete con
   `EventManager::push`.
2. En `AppSystem::Main`, `InputService` lo atiende en `FUN_0019e204`, que llama
   a `FUN_0018ae4c` y, según el modo, a `FUN_00155fe8` (toque simple) o a
   `FUN_00156438` (con proximidad o «hover»). Ambas registran
   `"[INPUTSERVICE] Off>Touch id=%d x=%d y=%d …"`. En el dato del toque, `x`
   es el `short` en `+6`, `y` el de `+4`, el id del dedo `+0x10 & 0xf` y la
   presión `(+0x10 >> 8) & 0xff`.
3. `FUN_0018a6f0`, `FUN_0018a79c` y `FUN_0018a80c` llaman a
   `WidgetSystem::postMouseMove/Press/Release` por la PLT, siempre con el
   ratón 0 y el botón 0. Antes, a `y` le aplica una corrección
   (`FUN_0018b970`).
4. Los `post*` meten el evento en la cola `ux`, y en la siguiente vuelta del
   lazo `WidgetSystem` lo reparte a los widgets.

Otros que llaman a `postMouse*` y que la envoltura también verá: el manejador
`[VIEWSYSTEM] WidgetSystem::postEvent()` (`FUN_005d6408`, que traduce eventos
AFW, puede usar otros números de ratón) y `FUN_0031f638`/`FUN_0031f760` de
`UXEngine` (sin llamadas directas conocidas).

Secuencia de un dedo **[código]** (`FUN_00155fe8`):

| Momento | Llamadas |
| --- | --- |
| apoyar | `postMouseMove(0, x, y)` y después `postMousePress(0, 0)` |
| arrastrar | `postMouseMove(0, x, y)` |
| soltar | `postMouseRelease(0, 0)` y después `postMouseMove(0, S, S)`, con `S` una constante fuera de pantalla (`DAT_0015638c`, valor [?]) |

### 4.2 B1: interponer `postMouse*`

Nombres exactos (están en las reubicaciones `R_ARM_JUMP_SLOT` de
`libObj.so`):

```c
#define SYM_MMOVE "_ZN2ux6wgtsys12WidgetSystem13postMouseMoveEhssNS_4core8MsecTimeE"
#define SYM_MPRESS "_ZN2ux6wgtsys12WidgetSystem14postMousePressEhhNS_4core8MsecTimeE"
#define SYM_MREL "_ZN2ux6wgtsys12WidgetSystem16postMouseReleaseEhhNS_4core8MsecTimeE"

void cxw_mmove(u32 ws, u32 mouse, int x, int y, const u32 *t) __asm__(SYM_MMOVE);
void cxw_mpress(u32 ws, u32 mouse, u32 button, const u32 *t) __asm__(SYM_MPRESS);
void cxw_mrel(u32 ws, u32 mouse, u32 button, const u32 *t) __asm__(SYM_MREL);
```

- `MsecTime` (8 bytes) llega por puntero: es el quinto argumento en la pila
  para `Move` y el cuarto en `r3` para `Press`/`Release`. `{0, 0}` significa
  «ahora» (`msec_now`) **[código]**. `x` e `y` son `short`: usar `(short)x`.
- Corren en `AppSystem::Main` **[código, por los que llaman]**. Desde ahí se
  puede tocar `ux`, incluida la etiqueta de la receta A.
- **Para tragar un toque**: no llamar a la original. Regla sugerida
  [inferido]:
  1. Dejar pasar siempre el `Move` previo (sin botón apretado la interfaz sólo
     ve el puntero moverse).
  2. En `Press`, si `(x, y)` del último `Move` cae dentro del panel propio,
     tragarlo y marcar «capturado».
  3. Mientras esté capturado, tragar los `Move` y el `Release`. Después dejar
     pasar el `Move(S, S)` que viene detrás.
- **Inyectar**: llamar a las originales desde el hilo de la interfaz con la
  misma secuencia de la tabla.
- **Coordenadas**: las mismas que usa `WidgetSystem` para la prueba de toque y
  que la vista de calibración usa para dibujar la cruz (`FUN_005d25c8` pone los
  cuadritos en `(x − 10, y − 10)`). Es decir, píxeles del OSD, con 640 × 480
  como tamaño probable [inferido]. Lo que se mete con `postMouseMove` se guarda
  en `WidgetSystem + 0x4ac`, y `getMousePosInFramebuffer` lee `+0x4a8` después
  de procesar; si hay una conversión entre los dos es [?]. Comprobarlo en la
  cámara anotando ambos y `getFramebufferWidth/Height`.

### 4.3 Cómo llegan las teclas

**[código]**

1. InfraInput (`libObj.so`, hilo propio) llama a las devoluciones de llamada que
   `InputService` registró con `InfraInput::SetCallback` y
   `InfraInput::SetEventKeyList` (`FUN_00198ed8`, «toActive»).
2. La tabla estática de `InputService` empieza en `0x111b0b0`. Son entradas de
   0x14 bytes, `{lista_de_teclas, función, n, enviar_estado_inicial,
   condición}`. Cada lista es de `n` tríos
   `{código_de_tecla_Infra, número_de_evento_base, 0xfffffffe}`.
3. La función (por ejemplo `FUN_0019c454`, «callback_convert_cap») recibe
   `(int, _InfraInputEvent *ev)`, con `ev[0]` = evento base, `ev[2]` = estado
   y `ev[4]` = valor. Crea `new Event(base + d, 4, 1)`, con `d = 1` al apretar
   y `d = 2` (o 4 si `ev[5] != 0`) al soltar, le agrega el valor con la clave
   `0x1f` y lo manda por `AbstractUtilityManager::pushEvent` →
   `EventManager::push`. Que las demás funciones usen los mismos `d` es
   [inferido].
4. `AppSystem::Main` lo despacha a las vistas, que reaccionan según sus tablas
   de disparo. No pasa por `ux::wgtsys::KeyEvent`.

`Event` **[código]** (`Event::Event(unsigned long, uchar, uchar)` en
`0x8cac54`): 0x10 bytes, `+0` puntero a contador de referencias, `+4` número
de evento, `+8` el primer `uchar` (4 en las teclas), `+9` el segundo (1 =
encolar y despertar, 0 = sólo encolar) y `+0xc` `ParamList*`.

Teclas de la tabla de `InputService`. El nombre sale de una tabla
`{nombre, código}` de `libObj.so` (`0x1183cf8`–`0x11849f8`) que mezcla espacios
de nombres; algunos códigos tienen dos nombres o uno que no cuadra, así que la
columna de nombre es **[inferido]** hasta verla en la bitácora:

| Código Infra | Evento base | Nombre en la tabla | Probable botón |
| --- | --- | --- | --- |
| `0xa7` | `0x15020040` | `KEY_TACT_CAP` / `KEY_TACT_CUSTOM` | PHOTO (lo atiende «callback_convert_cap») |
| `0x1f1` | `0x15020030` | `KEY_TACT_STARTSTOP` / `KEY_TACT_LOCK` | START/STOP |
| `0x7028` | `0x150200e0` | `KEY_TACT_MODE` | MODE |
| `0x1d1` | `0x150b0060` | `SLIKEY_ZOOM` | palanca de zoom (analógica) |
| `0x1f7` | `0x15020020` | `KEY_TACT_TELE` | zoom T (botón del panel) |
| `0x2` | `0x15020010` | sin nombre | [?] (¿zoom W?) |
| `0x1ff` | `0x150b0010`, `0x150b0020` | `KEY_TACT_CONTSHOT` (no cuadra) | abrir y cerrar el panel («callback_panel») |
| `0x7034` | `0x15020140` | `KEY_TACT_IAUTO` | [?] |
| `0x7030` | `0x15020100` | `KEY_TACT_NIGHTSHOT` | [?] |
| `0x07000001` | `0x15040010` | `KEY_TP_RAW` | tacto crudo |
| `0x7026` | `0x15030030` | `KEY_POS_MODE_DIAL` | [?] |

My Button no es un botón físico en la CX580V: es un botón en la pantalla
táctil, así que llega por `postMouse*` (4.2), no por aquí.

### 4.4 B2: interponer `EventManager::push`

```c
#define SYM_EVPUSH "_ZN12EventManager4pushEP5Event"
int cxw_evpush(u32 self, u32 ev) __asm__(SYM_EVPUSH);   /* la original devuelve 0 */
```

- Pasan por aquí todos los eventos de la aplicación: teclas, tacto, modelos y
  temporizadores. Se llama por la PLT (`R_ARM_JUMP_SLOT`) desde
  `AbstractUtilityManager::pushEvent`, `ModelManager::pushEvent`,
  `AppWidgetBase::pushEvent` y otros **[código]**.
- **Hilo**: el de quien llama. Para las teclas es un hilo de Infra
  **[código]**. Ahí no se puede tocar `ux` ni la etiqueta. Sólo leer
  `*(u32 *)(ev + 4)` y pasarlo al hilo de la interfaz con un búfer circular
  propio. Escribir la bitácora desde varios hilos a la vez puede mezclar
  líneas: mejor anotar desde el hilo de la interfaz.
- **Para tragar una tecla**: no llamar a la original y devolver 0. El `Event`
  queda sin dueño. Liberarlo bien es `_ZN5EventD1Ev(ev)` y después
  `operator delete(ev)`, pero `_ZdlPv` está en `libsonystdc++.so.6`, que hoy
  `cx_dlsym` no puede leer (3.2). Para el experimento basta con perder unos
  40 bytes por tecla tragada. Hay que tragar el par completo, el de apretar y
  el de soltar, para que ninguna vista quede con la tecla «apretada».
- Es la función más caliente de la aplicación: la envoltura tiene que ser
  mínima y no fallar nunca.

Otra opción, más cruda: interponer
`_ZN10InfraInput11SetCallbackEPiPFviP16_InfraInputEventE`, llamar a la original
con una función propia y guardar la original según el identificador que
devuelve en `*int`. Así se ve el `_InfraInputEvent` antes que `InputService`,
también en el hilo de Infra. Se registra al activar `InputService`; si es antes
o después de que cargue `cxg.so` da igual, porque la PLT se resuelve hacia la
biblioteca precargada.

### 4.5 Lo que no conviene interponer

- `ux::wgtsys::Widget::onKeyEvent` y `onMouseEvent`: **[código]** son
  funciones virtuales; las tablas virtuales de `libObj.so` las referencian con
  407 y 390 reubicaciones `R_ARM_ABS32` contra el símbolo. Si `cxg.so` las
  exportara, el cargador metería la versión propia en todas las clases que no
  las redefinen, y en ninguna de las que sí. Es una vista parcial y muy
  invasiva.
- La ruta de teclas `ux` (`FUN_0031f638`, que mete eventos de tipo 7 y 8 con
  `FUN_003454ec`/`FUN_0034553c`, internas) no tiene llamadas directas
  conocidas; no parece la que usan los botones de esta cámara.

## 5. Riesgos

- **Que `im.elf` se caiga al arrancar.** Cualquier fallo en una envoltura de
  `EventManager::push` o `postMouse*` tumba la interfaz. Hay que encenderlo
  con una opción de `cxmode.txt` apagada por omisión, no tocar `ux` antes de la
  primera `openView` y crear la etiqueta de forma perezosa. Si algo sale mal,
  el camino de rescate es el de siempre: la guarda pone la bandera del updater
  en cada arranque, así que un encendido sí y otro no la cámara entra al
  updater, y `guard-remove` deja `/setting` de fábrica (ver `AGENTS.md`).
- **Comprobaciones de `ux`.** Los «`<UX_LOG_ERROR>`» llaman a `FUN_003219c0`,
  que salta a un gancho registrado. Si ese gancho aborta es [?]. Varias
  comprobaciones desreferencian de todas formas, por ejemplo `setLayerID` con
  una capa que no existe.
- **Hilo equivocado.** Llamar a `postMouse*`, `setLayerID` o `moveLast` fuera
  de `AppSystem::Main` corrompe listas sin candado. Por eso la comprobación con
  `gettid()`.
- **Cambiar el árbol durante un recorrido.** La creación y `moveLast` van en la
  fase 3, nunca en envolturas que puedan correr mientras se actualizan widgets
  (`getModelInfo` las llaman también los widgets enlazados) [inferido].
- **Hijos de la raíz que no son vistas.** Las raíces de capa suelen tener por
  hijos las raíces de las vistas. Si algún código recorre esos hijos
  suponiendo que lo son, puede tropezar con la etiqueta. Sony cuelga widgets
  sueltos de `A_PHYSICAL_LAYER` en la vista de calibración, lo que hace pensar
  que está permitido [inferido].
- **Que tape cosas o quede tapada.** Con `moveLast` la etiqueta queda encima de
  los menús. Habría que ocultarla con `setMode(3)` mientras haya un menú
  abierto, usando los nombres de `openView`/`closeView`.
- **Glifos.** Que la fuente de la cámara tenga «/», «.» y las letras de
  «F» y «dB» es [inferido] (las Sony de esa época las muestran).

## 6. Lo que no se pudo determinar

- El tamaño real del OSD. `getFramebufferWidth/Height` lo leen del GRM
  (`FUN_003742ce`/`FUN_003742d8`). El 640 × 480 sale de la vista de
  calibración.
- Si las coordenadas de `postMouseMove` y las de `getMousePosInFramebuffer` son
  las mismas, y el valor fuera de pantalla `S`.
- Ningún id de fuente. Hay que tomarlo de una etiqueta de fábrica en la cámara.
- Quién consume la bandera `+0x285` de `GEN_Label`, y por tanto si `drawText` a
  mano es necesario.
- En qué capa física está el OSD de grabación y cuántas capas hay (el número
  sale de una llamada virtual en `FUN_008d3178`).
- Los `d` (apretar y soltar) de la función genérica `FUN_0019c3e8`, y qué botón
  físico es cada código: la tabla de nombres es ambigua.
- Qué hace el gancho de `FUN_003219c0`.
- Si `Widget::setPassiveRegion(true)` saca de verdad el widget de la prueba de
  toque.

## 7. Primer experimento en la cámara

Una sola instalación de `cxg.so`, sin tragar nada, sólo bitácora:

1. En `cxw_setstr`: anotar `id`, `self - 0x144` y los seis `u32` de fuente de
   la etiqueta (las primeras N distintas).
2. Envolturas de `postMouse*`: anotar `mouse`, `button`, `x`, `y`, `gettid()`,
   y una vez `getFramebufferWidth/Height`.
3. Envoltura de `EventManager::push`: anotar en un búfer circular
   `*(u32 *)(ev + 4)` cuando `(id >> 16) == 0x1502` o `0x150b`, y volcarlo
   desde el hilo de la interfaz. Apretar PHOTO, START/STOP, MODE, la palanca de
   zoom y tocar My Button.
4. Con esos datos, probar A0, y después A1 con la fuente anotada.

## 7b. Lo medido en la cámara, 27 de noviembre

Con `cxg-v19-ux.so` y `uxlog 1` (`backups/20261127-064406-guard-install/`):

- `postMousePress`/`postMouseRelease` se llaman una vez por toque, siempre con
  `mouse = 0`, `button = 0` y en el hilo de la interfaz. Las coordenadas del
  último `postMouseMove` cubren **640×480**, origen arriba a la izquierda
  (toques en las esquinas: de (27, 36) a (614, 437)).
- `EventManager::push` recibió `0x15020022` y luego `0x15020021` al volver de
  la reproducción al modo de grabación, y `0x15020012` y luego `0x15020011`
  antes del apagado. Llega primero el `+2` y después el `+1`, al revés de lo
  que se había inferido en 4.3 (`d = 1` al apretar). Eran el botón de ver
  imágenes y el de encendido; ver la tabla de abajo.
- Al arrancar llegan `0x150b0013` y `0x150b0023`.
- Las seis palabras de fuente de `GEN_Label + 0x264` salieron en 12 etiquetas,
  por ejemplo `2038000 203e010 203e003 2036029 2038002 1000037`.

Segunda sesión, sólo botones (`backups/20261127-065106-guard-install/`). El
dueño apretó MODE, START/STOP, PHOTO, ver imágenes, zoom T, zoom W, ver
imágenes y encendido; cada número se cotejó con lo que hizo la cámara en ese
instante (cambio de vista, inicio de grabación, apagado):

| Botón | Evento base | Al apretar | Al soltar | Qué se vio en la bitácora |
| --- | --- | --- | --- | --- |
| Encendido | `0x15020010` | `…12` | `…11` | `model/ROOT a=2`, `view/ROOT` |
| Ver imágenes | `0x15020020` | `…22` | `…21` | abre o cierra `view/EVENTVIEW` |
| PHOTO a medias | `0x15020030` | `…32` | `…31` | llega antes que el disparo |
| PHOTO a fondo | `0x15020040` | `…42` | `…41` | |
| START/STOP | `0x15020050` | `…52` | `…51` | `model/MOVIE_REC_DUAL a=3` y `a=6` |
| MODE | `0x15020090` | `…92` | `…91` | `view/STL_REC_CLASSIC_L` y `view/MOV_REC_L` |
| Palanca de zoom | `0x150b0030` | `…32` y `…34` hacia T, `…33` y `…35` hacia W | `…31` | se repite cada 0.1 s mientras se sostiene |

En las teclas el `+2` es apretar y el `+1` soltar, al revés de lo inferido en
4.3. La tabla de 4.3 queda corregida por ésta: `0x15020030` no es START/STOP
sino PHOTO a medias, `0x15020020` no es el zoom T sino ver imágenes, MODE no es
`0x150200e0` y la palanca de zoom no es `0x150b0060`. Coincide con la tabla de
nombres que citó `reports/Barrido de fuentes para la CX580V.md` (`LOCK` en
`0x1f1`, `PLAY` en `0x1f7`). El filtro de `cxux.c` (grupos `0x1502`, `0x150b` y
`0x1503`) alcanzó para todos los botones de la cámara.

## 8. Material de trabajo

Decompilados crudos en la carpeta temporal de la sesión (no son parte del
proyecto y pueden desaparecer):
`firmware/ghidra/out/agents/ux/`
(`d1.c` a `d16.c`, `xr*.txt`, `keynames.txt` con la tabla de nombres de
teclas). Funciones clave por dirección de Ghidra: `GEN_Label::GEN_Label`
`0x18288c`, `Widget::setLayerID` `0x342ab0`, `Widget::~Widget` `0x342cb4`,
`postMouseMove` `0x34536c`, `EventManager::push` `0x8caed0`, `AppSystem::Main`
`0x8c9680`, máquina táctil `0x155fe8`, plantilla de Sony `0x5d2724`, tabla de
teclas `0x111b0b0`.
