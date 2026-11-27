# Menú de fábrica: cómo se arma y cómo tocarlo desde `cxg.so`

Análisis estático del firmware 1.07 (Ghidra 12.1.4 y `objdump`), 26 de
noviembre de 2026. **Nada de esto se probó en la cámara.** Las marcas:

- **[código]**: leído en el código desensamblado o descompilado.
- **[inferido]**: deducción razonable que falta confirmar en la cámara.

Direcciones: las de `objdump`/`nm` son direcciones virtuales del archivo
(vaddr). En Ghidra, tanto `libObj.so` como `viewUnified7.so` están cargadas
con `+0x10000` (un `FUN_0007af6c` de Ghidra es `0x6af6c` en el archivo). Aquí
se usa vaddr salvo cuando dice «Ghidra».

## Resumen

1. Cada lista de menú es un árbol estático de `CmnViewMenuItemStruct` de
   0x28 bytes en `.data` de `viewUnified7.so` (escribible), más un arreglo
   `CmnViewMenuItemInconstPart` de 3 bytes por id de ítem, también en `.data`.
   La clase `CmnViewMenuData` de `libObj.so` recorre el árbol. [código]
2. Un ítem se ve si `state > 1 && valid == 1` en su entrada del arreglo de 3
   bytes. Todas las cuentas, conversiones de índice y movimientos de cursor
   que usan los widgets saltan los ítems que no se ven, así que poner
   `valid = 0` oculta el ítem de forma consistente para scroll y cursor.
   [código]
3. Los widgets (`PAS_ListBtnTitleValue`, `PAS_ListBtnTitleDTI`,
   `PAS_MenuText`) llaman a `CmnViewMenuData` sólo por su vtable, y la vtable
   apunta a funciones internas (reubicaciones `R_ARM_RELATIVE`). **Esas
   llamadas no se pueden interceptar por símbolo.** Lo que sí pasa por la PLT
   son las funciones exportadas que llama el código de cada vista en
   `viewUnified7.so`: `initMenuData`, `setItemValid`, `setGreyOut`, `getId`,
   etc. [código]
4. La etiqueta de cada fila la pone `GEN_Label::setText(unsigned int)` (PLT),
   que llama a `ux::gfxeng::TextSprite::setString(unsigned int)` (PLT). La
   interceptación que ya tiene `cxtext.c` sobre `TextSprite::setString(unsigned
   int)` sí alcanza los rótulos del menú. `ResourceManagerImpl::getText` no
   está en ese camino. [código]
5. Receta para ocultar: interceptar `CmnViewMenuData::initMenuData`, llamar a
   la original y, antes de volver, poner `valid = 0` a los ítems elegidos (con
   la `setItemValid` original o escribiendo el byte). Así el relleno de
   páginas que la vista calcula justo después queda bien. [código, sin probar]
6. IRIS, Shutter Speed, AGC Limit, AE Shift y White Balance Shift **ya están**
   en la tabla de Manual Settings de `view/CAMERA_SETTING_MENU` (ids 12 a 16).
   Los oculta el «exclusivo» de la vista según la propiedad de Backup
   `0x1070371` y una tabla por modelo. [código; el valor de la propiedad en
   esta unidad no se leyó]

## Estructuras

### `CmnViewMenuItemStruct` (0x28 bytes) [código]

| Offset | Tipo | Campo | Quién lo lee |
| --- | --- | --- | --- |
| `+0x00` | `CmnViewMenuItemStruct*` | arreglo de hijos (contiguo, paso 0x28) | todas |
| `+0x04` | `uint8_t` | número de hijos; `+0x05..0x07` siempre 0 | `getCountAll` |
| `+0x08` | `int32_t` | id del ítem, único dentro de su menú; indexa el arreglo de 3 bytes | `getId` |
| `+0x0c` | `uint32_t` | texto: id de cadena `0x0204xxxx` | vtable `+0x38` (texto) |
| `+0x10` | `uint32_t` | ícono (`0x0220xxxx` en opciones, `0x0204631c..` en pestañas) | vtable `+0x40` |
| `+0x14` | `uint32_t` | texto de ayuda («Adjusts color balance…») | vtable `+0x9c` |
| `+0x18` | `uint32_t` | ícono del valor; `-1` = usar el del hijo elegido | vtable `+0x30` |
| `+0x1c` | `uint8_t` | tipo de fila (ver abajo) | vtable `+0x4c` |
| `+0x1d` | `uint8_t` | tipo de ejecución | `getExecType` |
| `+0x1e` | 2 bytes | relleno, siempre 0 | |
| `+0x20` | `int32_t` | `opt_num`, segundo argumento de `openView` | `getOptionNum` |
| `+0x24` | `const char*` | `opt_str`, nombre de la vista a abrir | `getOptionString` |

En las 7 tablas de `viewUnified7.so` (1358 estructuras) aparecen estas
combinaciones [código]:

| Nivel | Tipo de fila `+0x1c` | Exec `+0x1d` | Qué es |
| --- | --- | --- | --- |
| 0 | 0 | 0 | raíz (un solo ítem; su id es el último, N−1) |
| 1 | 0 | 0 | pestaña o categoría (Manual Settings…) |
| 2 | 8 | 0 o 5 | fila con título y valor en texto |
| 2 | 10 | 0 o 5 | fila con título, valor en texto e ícono de valor |
| 2 | 0 | 5 u 8 | fila que sólo abre una vista (Format, Media Info…) |
| 2 | 11 | 7 | fila vacía de relleno (siempre al final de cada pestaña) |
| 3 | 0 o 9 | 2 | opción dentro de una fila; 9 muestra además `opt_num` |

Exec: 0 = la fila tiene opciones propias (lista de hijos), 2 = opción
elegible, 5 = abre `opt_str` con `ViewBase::openView(opt_str, opt_num)`
(`FUN_00078528` de Ghidra en CAMERA_SETTING_MENU), 7 = relleno, 8 = abre vista
(sólo «Download Music» del SETTING_MENU viejo). El tipo de fila lo usa el widget
(`FUN_0087f16c` de Ghidra): 11 apaga la fila, 8 y 10 piden el texto del valor,
10 pide también el ícono, 9 pide `opt_num`. [código]

### `CmnViewMenuItemInconstPart` (3 bytes por id) [código]

`inconst[id*3 + k]`, con el puntero que recibe `initMenuData` (lo guarda en
`this+0x08`):

| Byte | Campo | Valores |
| --- | --- | --- |
| 0 | cursor | índice absoluto (0..n−1) del hijo elegido de este ítem |
| 1 | estado | 0 y 1 = no se muestra; 2 = normal; 3 = normal y elegido; 4 = gris; 5 = gris y elegido |
| 2 | válido | 1 = válido; cualquier otro valor lo oculta |

`isItemDisplayed` (`0x186ea8` → `0x185a00`) es `estado > 1 && válido == 1`.
`isItemGreyout` es `estado ∈ {4, 5}`. `setGreyOut(item, b)` pasa 2↔4 y 3↔5;
no toca 0 ni 1. El bit «elegido» lo mueven `cursorMoveTo` (`0x185ed0`): quita
el bit al hijo anterior, guarda el índice en el byte 0 del padre y se lo pone
al nuevo, y no hace nada si el hijo destino no se ve.

Los arreglos estáticos vienen de fábrica con `{0, 2, 1}` en casi todos los ids;
unos pocos traen `válido = 0` (marcados en las tablas de abajo). Como son
estáticos, **el estado se conserva entre aperturas del menú** (cursor incluido).

Si `initMenuData` recibe `inconst == NULL`, `CmnViewMenuData` usa en su lugar
un `std::map<id, uint8_t[3]>` en `this+0x10`. Las 7 vistas de
`viewUnified7.so` siempre pasan el arreglo.

### Objeto `CmnViewMenuData` (0x4c bytes, `operator new(0x4c)`) [código]

| Offset | Contenido |
| --- | --- |
| `+0x00` | vtable (`0x11b7b0c`, 42 entradas, todas `R_ARM_RELATIVE`) |
| `+0x04` | raíz (`CmnViewMenuItemStruct*`) |
| `+0x08` | arreglo de 3 bytes (`CmnViewMenuItemInconstPart*`) |
| `+0x0c` | profundidad máxima vista |
| `+0x10..0x24` | `std::map` id → 3 bytes (sólo si `+0x08` es NULL) |
| `+0x28..0x3c` | `std::map` id → `{ítem*, padre*}`; lo arma `initMenuData` recorriendo el árbol |
| `+0x40..0x44` | `std::list<int>` de ids ocultos extra (lo agrega `0x187340`, sin llamadores encontrados) |
| `+0x48` | objeto «modificador» opcional: sus métodos `+8/+0xc/+0x10/+0x14` pueden reemplazar texto, ícono, ayuda e ícono de valor por id (lo pone `0x187338`, sin llamadores encontrados) |

Vtable (`CmnViewMenuDataIF`, la usan los widgets). «Visible» = índice entre
los hijos que se ven; «absoluto» = índice en el arreglo:

| Slot | vaddr | Qué hace |
| --- | --- | --- |
| `+0x08` | `0x186564` | `getRoot(&out)` |
| `+0x1c` | `0x1865b8` | índice visible del hijo elegido de `item` |
| `+0x20` | `0x1865d8` | cantidad de hijos visibles |
| `+0x24` | `0x186614` | visible → absoluto (−1 si se pasa) |
| `+0x28` | `0x186720` | absoluto → visible |
| `+0x2c`/`+0x30` | `0x186820`/`0x186780` | ícono de valor (por visible / del ítem) |
| `+0x34`/`+0x38` | `0x186954`/`0x1868bc` | **texto** (por visible / del ítem: `item+0x0c`, o el modificador) |
| `+0x3c`/`+0x40` | `0x186a48`/`0x1869b0` | ícono |
| `+0x44`/`+0x48`/`+0x9c` | | ayuda |
| `+0x4c`/`+0x50` | | tipo de fila |
| `+0x54`/`+0x58` | | estado (byte 1) |
| `+0x64` | `0x18711c` | hijo por índice visible |
| `+0x6c`/`+0x70` | | `opt_num` |
| `+0x78` | `0x186884` | id del hijo por índice absoluto |
| `+0xa0` | `0x186df4` | pone el byte «válido» de un ítem |

Las cuentas e índices visibles salen de `0x185d24` y `0x186614`: cuentan un
hijo sólo si se ve (`estado > 1 && válido == 1`) y su id no está en la lista de
`+0x40`. Es la única lógica, así que ocultar por «válido» o por «estado» deja
consistentes la cantidad de filas, el scroll, el índice que devuelve el widget
y el ítem que se ejecuta. [código]

## Funciones exportadas de `CmnViewMenuData`

Todas en `libObj.so`. `this` va en `r0`. Ninguna está en la vtable. [código]

| Símbolo | Firma y efecto | Llamadas desde `viewUnified7.so` |
| --- | --- | --- |
| `_ZN15CmnViewMenuData12initMenuDataEP21CmnViewMenuItemStructP26CmnViewMenuItemInconstPartP22CmnViewMenuExclusiveIF` | `(this, raíz, inconst, exclusivo)`. Guarda inconst, arma el mapa id → ítem y pide a `exclusivo->vtable[+8](id)` el estado (0 a 5) de cada ítem; luego deja el cursor de cada padre en un hijo visible. Devuelve en `r0` lo de `0x18643c` (1 si terminó bien); quien llama lo ignora | 8 (7 tablas) |
| `_ZN15CmnViewMenuData12setItemValidEP21CmnViewMenuItemStructib` | `(this, padre, índice absoluto, bool)`. Si `0 ≤ i < n` pone el byte «válido» del hijo `i` | 60 (todas para el relleno, con `false`) |
| `_ZN15CmnViewMenuData10setGreyOutEP21CmnViewMenuItemStructb` | `(this, ítem, bool)`. Pasa estado 2↔4, 3↔5 | 12 |
| `_ZN15CmnViewMenuData5getIdEPK21CmnViewMenuItemStruct` | `item->id` o −1 | 76 |
| `_ZN15CmnViewMenuData11getCountAllEPK21CmnViewMenuItemStruct` | `item->n` en bruto, **incluye ocultos** | 8 |
| `_ZN15CmnViewMenuData7getTextEPK21CmnViewMenuItemStructi` | `(this, padre, i absoluto)` → id de cadena `0x0204xxxx` del hijo (vía vtable `+0x38`) | 1 (SETTING_MENU) |
| `_ZN15CmnViewMenuData15isItemDisplayedEPK21CmnViewMenuItemStruct` | `estado > 1 && válido == 1` | 1 (SETTING_MENU) |
| `_ZN15CmnViewMenuData13isItemGreyoutEPK21CmnViewMenuItemStruct` / `…StructEi` | estado 4 o 5 | 12 / 25 |
| `_ZN15CmnViewMenuData10getSubItemEPK21CmnViewMenuItemStructiPPS0_` | `*out = &padre->child[i]` (absoluto) | 2 (SETTING_MENU) |
| `_ZN15CmnViewMenuData11getExecTypeEPK21CmnViewMenuItemStruct` | byte `+0x1d` | 18 |
| `_ZN15CmnViewMenuData12getOptionNumEPK21CmnViewMenuItemStruct` | `+0x20` | 5 |
| `_ZN15CmnViewMenuData15getOptionStringEPK21CmnViewMenuItemStructPPKc` | `*out = +0x24` | 5 |
| `_ZN15CmnViewMenuData12cursorMoveToEP21CmnViewMenuItemStructi` | `(this, padre, i absoluto)`; nada si el hijo no se ve | 1 |
| `_ZN15CmnViewMenuData16cursorMoveToByIdEi` | busca el id en el mapa y llama a `cursorMoveTo` | 11 |

`libObj.so` también las llama por su PLT: `initMenuData` desde `0x788cfc`,
`0x788f78`, `0x7892c8` (listas de Flash, Macro y escenas de otro modelo) y
`0x8079e0` (`ACT_DtiShooting`, la lista de My Button). [código]

## Dónde están las tablas

`viewUnified7.so`, todas en `.data`. La raíz es un ítem suelto y el arreglo de
3 bytes empieza justo después (raíz `+0x28`). En código ARM sólo se encontraron
referencias a la raíz y al arreglo, nunca a ítems sueltos (búsqueda de pares
`ldr rX,[pc]` + `add rX,pc,rX`). [código]

| Vista | Código (vaddr) | Raíz | Inconst | Ids | Widget | ¿Se abre en esta unidad? |
| --- | --- | --- | --- | --- | --- | --- |
| `view/CAMERA_SETTING_MENU` (Camera/Mic) | `0x643dc` | `0xc347c` | `0xc34a4` | 256 | `PAS_ListBtnTitleValue` | sí (bitácoras) |
| `view/IMAGE_QUALITY_SIZE_MENU` | `0x29024` | `0xb7220` | `0xb7248` | 79 | `PAS_ListBtnTitleValue` | [inferido] |
| `view/SETUP_MENU` | `0x3e314` | `0xbb980` | `0xbb9a8` | 207 | `PAS_ListBtnTitleValue` | [inferido] |
| `view/TCUB_MENU` | `0x5255c` | `0xc0380` | `0xc03a8` | 29 | `PAS_ListBtnTitleValue` | [inferido] |
| `view/CUSTOMIZE_DTI_L` (My Button) | `0x1a8c8` | `0xb589c` | `0xb58c4` | 320 | `PAS_ListBtnTitleDTI` | [inferido] |
| `view/SIMPLE_SETTING` | `0x465bc` | `0xbf3ac` | `0xbf3d4` | 320 | ninguno propio | [inferido] |
| `view/SETTING_MENU` | `0x37e30` | `0xb8ac8` | `0xb8af0` | 147 | `PAS_MenuText` | no aparece en las bitácoras; es el menú de otra generación |

`view/GLOBAL_MENU` y `view/CAMERA_MODE_MENU` (Shooting Mode) **no usan
`CmnViewMenuData`**: son pantallas de botones `PAS_BtnBalloon` que llaman a
`ViewBase::openView` con nombres fijos. GLOBAL_MENU abre `CAMERA_MODE_MENU`,
`CAMERA_SETTING_MENU`, `IMAGE_QUALITY_SIZE_MENU`, `PB_FUNC_MENU`,
`EDIT_COPY_MENU`, `SETUP_MENU`, `TCUB_MENU` y `EVENTVIEW`. CAMERA_MODE_MENU
abre `MOV_REC`, `STL_REC_CLASSIC`, `HS_REC` (Smth Slw REC), `GOLF_SHOT` y
`MOV_REC_3D`. `PB_FUNC_MENU` y `EDIT_COPY_MENU` tampoco llaman a
`initMenuData`. [código]

Cómo reconocer el menú dentro de un `initMenuData` interceptado: `dladdr(raíz)`
da la base de `viewUnified7.so` y `raíz − base` es la columna «Raíz». Sin
`dladdr`, sirve `raíz->id` (255, 78, 206, 28, 319, 146) más el texto de la
primera pestaña, salvo DTI_L y SIMPLE_SETTING, que son copias iguales. [código]

El id de un ítem es único dentro de su menú; el texto no (hay tres «Frame
Rate», tres «SteadyShot», siete «Media Info», cinco «Component»: variantes por
modelo o modo). Para ocultar hay que usar `(menú, id)`. [código]

## El exclusivo de CAMERA_SETTING_MENU

`menuExclusiveCameraSetting` (vtable Ghidra `0xd7188`, objeto en
`vista+0xf8`) responde `getState(id)` (slot `+8`, Ghidra `FUN_00073f98`).
`initMenuData` lo llama una vez por ítem [código]:

1. Columna por modelo (Ghidra `FUN_00073f58`, tabla vaddr `0xa2474`, 28 ids):
   si vale 2 para la columna `this+0x5c`, estado 0 (oculto). La columna sale
   de la propiedad de Backup `0x1070316` (por defecto 2), mapeo
   `v → {0:1, 1:0, 2:1, 3:2, 4:3, 5:4, 6:5, 7:6}`. Con la columna 1 la
   función no consulta la tabla y no oculta nada.
2. Reglas por capacidad (vaddr `0x9cb70`, 29 reglas `{id, condición, tipo}`):
   tipo 1 oculta si la condición vale 0; tipo 2 oculta si vale 1.
3. Gris por modo de grabación (Ghidra `FUN_00073e4c`, tabla vaddr `0x9cccc`,
   51 ids × 10 columnas; las columnas activas dependen de `this+0x58`, que pone
   la vista con el slot `+0x1c`): 1 = gris (estado 4), 2 = oculto.

Condiciones (`exclusivo+0x14+4·c`), puestas en Ghidra `FUN_0007af6c` desde
propiedades de Backup. Valor 1 = la función existe. Si la lectura falla vale
1, y si la propiedad `0x1070408` vale 1 todas valen 1 [código]:

| Cond. | Propiedad | Ítems que oculta si vale 0 |
| --- | --- | --- |
| 0 | `0x1070371` | 12 IRIS, 13 Shutter Speed, 15 AE Shift, 16 White Balance Shift, 35 y 36 Dial Setting |
| 1 | `0x107036e` | 17 Low Lux |
| 2 | `0x107036f` | 33 Conversion Lens |
| 3 | `0x1070379` | 37 NIGHTSHOT Light |
| 4 | `0x10703fd` | 47 Flash, 48 Flash Level, 49 Red Eye Reduction (y el id 3, que en esta tabla es la pestaña Flash) |
| 5 | `0x107037f` | 69 Light Bright |
| 6 | `0x10703d3` | 57 Audio Mode |
| 7 | `0x1070372` | 64 Expanded Focus |
| 8 | `0x1070381` | 67 Zebra |
| 9 | `0x107037b` | 68 Peaking |
| 10 | `0x10703d1` | 24 Cinematone |
| 11 | `0x10703d0` | 70 Audio Level Display |
| 12 | `0x1070400` (sólo si cond. 7 = 1) | 236/237, opciones «On» de Expanded Focus |
| 13 | `0x10704d6` | 22 Auto 3D Lens Adjust |
| 14 | `0x107095e` | 53 Closer Voice, 55 Auto Wind NR; 56 Wind Noise Reduct. se oculta si vale 1. Si la lectura falla vale −1 y oculta los tres |
| 15 | calculada (Ghidra `FUN_0007af40`) | 7 Spot Meter/Fcs, 8 Spot Meter, 9 Spot Focus |

Columna por modelo (ids con algún 2; columnas 0 a 6; la 1 nunca se aplica):

| id | Ítem | Columnas que lo ocultan |
| --- | --- | --- |
| 7, 8, 9 | Spot Meter/Fcs, Spot Meter, Spot Focus | 4, 5 |
| 12, 13, 14 | IRIS, Shutter Speed, AGC Limit | 0, 6 |
| 21 | 3D Depth Adjustment | 0, 6 |
| 22 | Auto 3D Lens Adjust | 0 |
| 28 | SteadyShot (sin vista) | 0, 1, 6 |
| 29 | SteadyShot (`view/MOVIE_STEADY_SHOT`) | 2, 3, 4, 5 |
| 30 | SteadyShot On/Off | 6 |
| 31 | Digital Zoom `${v1}×` | 4, 5 |
| 32 | Digital Zoom On/Off | 0, 1, 2, 3, 5, 6 |
| 33 | Conversion Lens | 2, 6 |
| 35 | Dial Setting (lista) | 2, 3, 4, 5 |
| 36 | Dial Setting (`view/CAMERA_CTRL_DIAL_SETTING`) | 0, 6 |
| 42, 43 | Smile Shutter, Smile Sensitivity | 4, 5 |
| 47, 48, 49 | Flash, Flash Level, Red Eye | 6 |
| 53 | Closer Voice | 6 |
| 54 | Blt-in Zoom Mic | 4, 5 |
| 57 | Audio Mode | 6 |
| 59 | Audio Output Timing | 0, 2, 6 |
| 68 | Peaking | 6 |
| 69 | Light Bright | 6 |
| 71 | Audio Format | 0, 2, 4, 5, 6 |

La columna 0 (propiedad `0x1070316` = 1) es la que mejor cuadra con el menú
de la CX580V de la guía de ayuda: oculta IRIS, Shutter Speed, AGC Limit, 3D y
Audio Format y deja Flash, Peaking y Light Bright. AE Shift y WB Shift tampoco
están en el menú de la CX580V, lo que encaja con la condición 0 = 0
(propiedad `0x1070371`). **Falta leer las dos propiedades en el Backup.bin de
la unidad** para confirmarlo. [inferido]

Consecuencia: los ítems IRIS (12, `view/IRIS_L`), Shutter Speed (13,
`view/SHUTTER_SPEED_L`) y AGC Limit (14, `view/AGC_LIMIT`), más AE Shift (15)
y WB Shift (16), ya existen en Manual Settings con sus pantallas. Basta
que el exclusivo no los oculte (ver «Mostrar» abajo). Las tres pantallas `_L`
ya se abrieron en esta unidad con la redirección de `cxmode` (bitácoras). Que
abran bien desde el menú no se probó.

## Inventario de ítems

Sacado de las tablas con los textos de `share/app/string_english.uxc`. «fila» y
«exec» son los bytes `+0x1c` y `+0x1d`. No se listan las filas de relleno
(fila 11, exec 7) ni las opciones de nivel 3 una por una. Que un ítem esté en
la tabla no quiere decir que se vea en esta unidad: lo decide el exclusivo
(sección anterior; la tabla de reglas sólo se leyó para CAMERA_SETTING_MENU).
[código]

Conteo de ítems reales (sin relleno):

| Menú | Pestañas e ítems |
| --- | --- |
| Shooting Mode (`CAMERA_MODE_MENU`) | sin tabla: 5 botones (`MOV_REC`, `STL_REC_CLASSIC`, `HS_REC`, `GOLF_SHOT`, `MOV_REC_3D`) |
| Camera/Mic (`CAMERA_SETTING_MENU`) | Manual Settings 12, Camera Settings 17, Face 3, Flash 3, Microphone 7, Shooting Assist 9: **51** (+18 de relleno) |
| Image Quality/Size (`IMAGE_QUALITY_SIZE_MENU`) | Main Settings 11 (+4) |
| Setup (`SETUP_MENU`) | Media Settings 11, Playback Settings 4, Connection 17, General Settings 23, Clock Settings 4: **59** (+15) |
| TC/UB (`TCUB_MENU`) | 8 (+4) |
| My Button (`CUSTOMIZE_DTI_L`, igual a `SIMPLE_SETTING`) | 61 (+3) |
| `SETTING_MENU` (otra generación) | 41 en 5 pestañas |

### view/CAMERA_SETTING_MENU (Camera/Mic) (raíz `0xc347c`, id raíz 255, inconst `0xc34a4`)

**Manual Settings** (`0x02042da0`, id 0): 12 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 6 | White Balance (`2d2e`) | 10 | 5 | `view/WHITE_BALANCE` | Auto, Outdoor, Indoor, One Push |
| 7 | Spot Meter/Fcs (`2bf1`) | 8 | 5 | `view/SPOT_METER_FOCUS` | Auto, Manual |
| 8 | Spot Meter (`2bef`) | 8 | 5 | `view/SPOT_METER` | Auto, Manual |
| 9 | Spot Focus (`2beb`) | 8 | 5 | `view/SPOT_FOCUS` | Auto, Manual |
| 10 | Exposure (`2614`) | 10 | 5 | `view/EXPOSURE` | Auto, Manual |
| 11 | Focus (`2690`) | 10 | 5 | `view/FOCUS` | Auto, ${v1}.${v2}m, ${v1}m,  |
| 12 | IRIS (`28b6`) | 10 | 5 | `view/IRIS_L` | Auto, F${v1}.${v2}, F${v1:2}, ---- |
| 13 | Shutter Speed (`2b9c`) | 10 | 5 | `view/SHUTTER_SPEED_L` | Auto, ${v1}.${v2}, ${v1}, ${v1}.${v2}", ${v1} |
| 14 | AGC Limit (`2322`) | 10 | 5 | `view/AGC_LIMIT` | 0dB, 3dB, 6dB, 9dB, 12dB, 15dB, 18dB, 21dB, 24dB, 27dB, OFF |
| 15 | AE Shift (`2311`) | 10 | 5 | `view/AE_SHIFT` | Off, -1.0EV, -0.7EV, -0.5EV, -0.2EV, 0EV, +0.2EV, +0.5EV, +0.7EV, +… |
| 16 | White Balance Shift (`2d23`) | 10 | 5 | `view/WB_SHIFT` | Off, , , , , , , , ,  |
| 17 | Low Lux (`293a`) | 10 | 0 |  | Off, On |

**Camera Settings** (`0x02042d9a`, id 1): 17 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 21 | 3D Depth Adjustment (`22e3`) | 10 | 5 | `view/3D_DEPTH_ADJUSTMENT_SHOOTING` | , , , , , , , , , , , , , , , , , Auto |
| 22 | Auto 3D Lens Adjust (`2374`) | 8 | 5 | `view/AUTO_3D_LENS_ADJUST` |  |
| 23 | Scene Selection (`2e18`) | 10 | 5 | `view/SCENE_SELECTION` | Auto, Landscape, Night Portrait, Night Scene, Beach, Snow, Firework… |
| 24 | Cinematone (`241f`) | 10 | 0 |  | Off, On |
| 25 | Fader (`264b`) | 10 | 0 |  | Off, White Fader, Black Fader |
| 26 | Self-Timer (`2b51`) | 10 | 0 |  | Off, On |
| 27 | Tele Macro (`2c65`) | 10 | 5 | `view/TELEMACRO` | Off, On |
| 28 | SteadyShot (`2c06`) | 10 | 0 |  | Active, Standard, Off |
| 29 | SteadyShot (`2c06`) | 10 | 5 | `view/MOVIE_STEADY_SHOT` | Active, Standard, Off |
| 30 | SteadyShot (`2c0c`) | 10 | 0 |  | On, Off |
| 31 | Digital Zoom (`2504`) | 10 | 0 |  | Off, ${v1}× |
| 32 | Digital Zoom (`2504`) | 8 | 0 |  | Off, On |
| 33 | Conversion Lens (`2449`) | 10 | 0 |  | Off, Wide Conversion, Tele Conversion |
| 34 | Auto Back Light (`2381`) | 10 | 0 |  | On, Off |
| 35 | Dial Setting (`24fa`) | 10 | 0 |  | Focus, Exposure, IRIS, Shutter Speed, AE Shift, White Balance Shift |
| 36 | Dial Setting (`24fa`) | 10 | 5 | `view/CAMERA_CTRL_DIAL_SETTING` | Focus, Exposure, IRIS, Shutter Speed, AE Shift, White Balance Shift… |
| 37 | NIGHTSHOT Light (`29ec`) | 10 | 0 |  | On, Off |

**Face** (`0x02042d9d`, id 2): 3 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 41 | Face Detection (`2632`) | 10 | 0 |  | Auto, Child Priority, Adult Priority, Off |
| 42 | Smile Shutter (`2bc1`) | 10 | 0 |  | Dual Capture, Always On, Off |
| 43 | Smile Sensitivity (`2bad`) | 10 | 0 |  | Big Smile, Normal Smile, Slight Smile |

**Flash** (`0x02042d9e`, id 3): 3 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 47 | Flash (`2665`) | 10 | 0 |  | Auto, On, Off |
| 48 | Flash Level (`2667`) | 10 | 0 |  | High, Normal, Low |
| 49 | Red Eye Reduction (`2ad2`) | 10 | 0 |  | Off, On |

**Microphone** (`0x02042da2`, id 4): 7 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 53 | Closer Voice (`305b`) | 10 | 0 |  | Off, On |
| 54 | Blt-in Zoom Mic (`23c3`) | 10 | 0 |  | Off, On |
| 55 | Auto Wind NR (`3023`) | 10 | 0 |  | Off, On |
| 56 | Wind Noise Reduct. (`2d62`) | 10 | 0 |  | Off, On |
| 57 | Audio Mode (`2369`) | 10 | 0 |  | 5.1ch Surround, 2ch Stereo |
| 58 | Micref Level (`2984`) | 10 | 0 |  | Normal, Low |
| 59 | Audio Output Timing (`2371`) | 10 | 0 |  | Live, Lip Sync |

**Shooting Assist** (`0x02042da4`, id 5): 9 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 63 | My Button (`29df`) | 0 | 5 | `view/CUSTOMIZE_DTI_L` |  |
| 64 | Expanded Focus (`260b`) | 8 | 0 |  | Off, On, On |
| 65 | Guide Frame (`271e`) | 10 | 0 |  | Off, On |
| 66 | Display Setting (`253f`) | 10 | 0 |  | Auto, On |
| 67 | Zebra (`2d74`) | 10 | 5 | `view/ZEBRA` | 100, 70, Off |
| 68 | Peaking (`2a54`) | 10 | 5 | `view/PEAKING` | White, Red, Yellow, Off |
| 69 | Light Bright (`2cec`) | 10 | 0 |  | Standard, Bright |
| 70 | Audio Level Display (`2366`) | 10 | 0 |  | On, Off |
| 71 | Audio Format (`2725`) | 10 | 0 |  | Linear PCM, Dolby Digital |

### view/IMAGE_QUALITY_SIZE_MENU (Image Quality/Size) (raíz `0xb7220`, id raíz 78, inconst `0xb7248`)

**Main Settings** (`0x020427db`, id 0): 11 ítems + 4 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 1 | Movie format (`2991`) | 0 | 5 | `view/_KMOVFORMAT` |  |
| 2 | REC Mode (`2ab4`) | 10 | 5 | `view/REC_MODE` | 60p Quality, Highest Quality, High Quality, Standard, Long Time |
| 3 | Frame Rate (`26ba`) | 10 | 5 | `view/FRAME_RATE` | 60p, 50p, 60i, 50i, 24p, 25p |
| 4 | Frame Rate (`22ec`) (válido = 0 de fábrica) | 10 | 5 | `view/FRAME_RATE` opt=9 | 60p, 50p, 60i, 50i, 24p, 25p |
| 5 | Frame Rate (`22e0`) (válido = 0 de fábrica) | 10 | 5 | `view/FRAME_RATE` opt=1 | 60p, 50p, 60i, 50i, 24p, 25p |
| 6 | /Setting (`275e`) | 10 | 5 | `view/SET_HD_STD` | HD Quality, STD Quality |
| 7 | Wide Mode (`2d58`) | 10 | 0 |  | 16:9 WIDE, 4:3 |
| 8 | 2D/3D Mode Select (`22dc`) | 10 | 5 | `view/CHANGE_MODE_2D3D` | 2D, 3D |
| 9 | x.v.Color (`2d6c`) | 10 | 0 |  | Off, On |
| 10 | Image Size (`27ff`) (válido = 0 de fábrica) | 10 | 0 |  | 12.3M, 9.2M, 7.1M, 5.3M, 3.3M, 3.3M, 2.5M, 1.9M, VGA(0.3M) |
| 11 | Image Size (`27ff`) | 10 | 5 | `view/IMAGE_SIZE` | L (24.1M), M (12.3M), S (2.0M), L (18.1M), M (5.0M), S (VGA), L (20… |

### view/SETUP_MENU (Setup) (raíz `0xbb980`, id raíz 206, inconst `0xbb9a8`)

**Media Settings** (`0x02042da1`, id 0): 11 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 5 | Media Select (`296a`) | 10 | 5 | `view/MEDIA_CHANGE` | HDD, Int. Memory, Memory Card |
| 6 | Media Info (`2969`) | 0 | 5 | `view/MEDIA_INFO` opt=1 |  |
| 7 | Media Info (`2969`) | 0 | 5 | `view/MEDIA_INFO` opt=9 |  |
| 8 | Media Info (`2969`) | 0 | 5 | `view/MEDIA_INFO` opt=2 |  |
| 9 | Media Info (`2969`) | 0 | 5 | `view/MEDIA_INFO` opt=4 |  |
| 10 | Media Info (`2969`) | 0 | 5 | `view/MEDIA_INFO` opt=3 |  |
| 11 | Media Info (`2969`) | 0 | 5 | `view/MEDIA_INFO` opt=6 |  |
| 12 | Media Info (`2969`) | 0 | 5 | `view/MEDIA_INFO` opt=5 |  |
| 13 | Format (`26af`) | 0 | 5 | `view/SELECT_FUNC_TARGET_MEDIA` opt=1 |  |
| 14 | Repair Img. DB F. (`2ae1`) | 0 | 5 | `view/SELECT_FUNC_TARGET_MEDIA` opt=2 |  |
| 15 | File Number (`265c`) | 8 | 0 |  | Series, Reset |

**Playback Settings** (`0x02042da3`, id 1): 4 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 19 | Data Code (`247c`) | 8 | 0 |  | Off, Date/Time, Camera Data, Coordinates |
| 20 | Volume (`2d0f`) | 0 | 5 | `view/VOLUME` |  |
| 21 | Download Music (`256c`) | 0 | 5 | `view/USB_BGM_DOWNLOAD` |  |
| 22 | Empty Music (`25bc`) | 0 | 5 | `view/BGM_FORMAT` |  |

**Connection** (`0x02042d9c`, id 2): 17 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 26 | TV Connect Guide (`2ca9`) (válido = 0 de fábrica) | 0 | 5 | `view/TELEVISION_TYPE` |  |
| 27 | TV Type (`2caa`) | 8 | 0 |  | 16:9, 4:3 |
| 28 | Component (`2430`) | 8 | 0 |  | 480i, 1080i/480i, 1080p/480i |
| 29 | Component (`2430`) | 8 | 0 |  | 576i, 1080i/576i, 1080p/576i |
| 30 | Component (`2430`) | 8 | 0 |  | 480i, 1080i/480i, 1080p/480i |
| 31 | Component (`2430`) | 8 | 0 |  | 480i, 1080i, 1080p |
| 32 | Component (`2430`) | 8 | 0 |  | 576i, 1080i, 1080p |
| 33 | HDMI 3D Setting (`272b`) | 8 | 0 |  | Auto, Frame Packing, Side-by-Side |
| 34 | HDMI Resolution (`2739`) | 8 | 0 |  | Auto, 1080p/480p, 1080p/576p, 1080i/480i, 1080i/576i, 720p/480p, 72… |
| 35 | HDMI Resolution (`2739`) | 8 | 0 |  | Auto, 1080p, 1080i, 720p, 480p, 576p, 480i, 576i |
| 36 | HDMI Output Link (`2733`) | 8 | 0 |  | Fixed to 3D, Panel unity |
| 37 | CTRL FOR HDMI (`2471`) | 8 | 0 |  | On, Off |
| 38 | HDMI TC Output (`2757`) | 8 | 0 |  | Off, On |
| 39 | USB Connect (`2cc4`) | 0 | 5 | `view/USB_LAUNCHER` |  |
| 40 | USB Connect Setting (`2cc5`) | 8 | 0 |  | Auto, MTP, Mass Storage |
| 41 | USB LUN Setting (`2943`) | 8 | 0 |  | Multi, Single |
| 42 | Disc Burn (`250c`) | 0 | 5 | `view/USB_OTDISC` |  |

**General Settings** (`0x02042d9f`, id 3): 23 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 46 | 60i/50i Sel (`2301`) | 8 | 5 | `view/60I_50I_EXCHANGE_L` | 50i, 60i |
| 47 | Beep (`23b3`) | 8 | 0 |  | On, Off |
| 48 | LCD Brightness (`290d`) | 10 | 0 |  | Normal, Bright |
| 49 | VF Brightness (`2ce2`) | 0 | 5 | `view/FINDER_BRIGHTNESS` |  |
| 50 | Viewfinder Adjust (`2ce8`) | 0 | 5 | `view/FINDER_AXIS_ADJUST` opt=2 |  |
| 51 | Your Location (`2d73`) | 0 | 5 | `view/LOCATION_DISPLAY` |  |
| 52 | GPS Satellite INFO (`2712`) | 0 | 5 | `view/GPS_INFO` |  |
| 53 | Position (`2a82`) | 0 | 5 | `view/GEO_INFO` |  |
| 54 | GPS Information (`2701`) (válido = 0 de fábrica) | 0 | 5 | `view/GEO_RCV_INFO` |  |
| 55 | GPS Setting (`2703`) | 10 | 0 |  | On, Off |
| 56 | REC Lamp (`2aac`) | 8 | 0 |  | On, Off |
| 57 | Beep (`23b3`) | 8 | 0 |  | On, Off |
| 58 | Remote Ctrl (`2adc`) | 8 | 0 |  | On, Off |
| 59 | A. Shut Off (`2351`) (válido = 0 de fábrica) | 8 | 0 |  | On, Off |
| 60 | Power On By LCD (`2a83`) | 8 | 0 |  | Off, On |
| 61 | Language Setting (`28e6`) | 8 | 5 | `view/LANGUAGE_MENU` | ENGLISH, ENG[SIMP], 日本語, B.MELAYU, ČESKY, DEUTSCH, ΕΛΛHNIKA, ESPAÑO… |
| 62 | Calibration (`23f5`) | 0 | 5 | `view/TP_CALIBRATION` |  |
| 63 | Drop Sensor (`2587`) | 10 | 0 |  | On, Off |
| 64 | 60i/50i Sel (`2301`) | 8 | 5 | `view/60I_50I_EXCHANGE_L` | 50i, 60i |
| 65 | Battery Info (`23a0`) | 0 | 5 | `view/BATTERY_INFO` |  |
| 66 | Eco Mode (`3064`) | 10 | 0 |  | Standard, Max, Off |
| 67 | Demo Mode (`24ed`) | 8 | 5 | `view/PB_DEMO_CAM_SET` | On, Off |
| 68 | Demo Mode (`24ed`) (válido = 0 de fábrica) | 8 | 5 | `view/DEMO_MODE` | On, Off |

**Clock Settings** (`0x02042d9b`, id 4): 4 ítems + 3 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 72 | Date & Time Setting (`2491`) | 0 | 5 | `view/DATE_SET` |  |
| 73 | Area Setting (`232b`) | 0 | 5 | `view/AREA_SET` |  |
| 74 | Auto Clock ADJ (`2387`) | 8 | 0 |  | On, Off |
| 75 | Auto Area ADJ (`237d`) | 8 | 0 |  | On, Off |

### view/TCUB_MENU (TC/UB, desde GLOBAL_MENU) (raíz `0xc0380`, id raíz 28, inconst `0xc03a8`)

**Main Settings** (`0x020427db`, id 0): 8 ítems + 4 relleno

| id | texto (`0x0204....`) | fila | exec | destino (opt_str, opt_num) | opciones (hijos) |
| --- | --- | --- | --- | --- | --- |
| 1 | TC/UB Disp (`2c61`) | 8 | 0 |  | TC, U-Bit |
| 2 | TC Preset (`2c5c`) | 8 | 5 | `view/TC_PRESET` | ${v1:02}:${v2:02}:${v3:02}.${v4:02}, ${v1:02}:${v2:02}:${v3:02}:${v… |
| 3 | UB Preset (`2cb5`) | 8 | 5 | `view/UB_PRESET` | ${v1:02} ${v2:02} ${v3:02} ${v4:02} |
| 4 | TC Format (`2c56`) | 8 | 0 |  | DF, NDF |
| 5 | TC Format (`2c56`) | 8 | 0 |  | DF, NDF |
| 6 | TC Run (`2c5e`) | 8 | 0 |  | Rec Run, Free Run |
| 7 | TC Make (`2c59`) | 8 | 0 |  | Preset, Regenerate |
| 8 | UB Time Rec (`2cb6`) | 8 | 0 |  | Off, On |

## Quién dibuja los rótulos

Camino de un rótulo de fila en `PAS_ListBtnTitleValue` (CAMERA_SETTING_MENU,
IMAGE_QUALITY_SIZE_MENU, SETUP_MENU, TCUB_MENU) [código]:

1. Ghidra `FUN_00881c7c` (`PAS_ListBtnTitleValue`, actualización de una fila)
   pide al `CmnViewMenuDataIF` el texto del título con el slot `+0x34`
   (`getText` por índice visible → `item+0x0c`) y el del valor con `+0x1c` y
   `+0x34` sobre la fila (el texto del hijo elegido).
2. Se los pasa a la fila `PAS_ListBtnTitleValueItem` (vtable `0x11cef20`) por
   sus slots `+0x144` (título) y `+0x148` (valor).
3. `+0x144` (`0x87306c`) llama por la PLT a `GEN_Label::setText(unsigned int)`
   sobre la etiqueta `fila+0xe90`, y `+0x148` (`0x87307c`) sobre `fila+0x1120`.
4. `GEN_Label::setText(unsigned int)` (`0x172f64`, código ARM) guarda el número
   en `+0x28c` y llama por la PLT a
   `ux::gfxeng::TextSprite::setString(unsigned int)` (`0x323f76`, Thumb).
5. `TextSprite::setString(unsigned int)` busca el registro con una función
   interna del gestor de recursos (`0x31c1b0`); no llama a
   `ResourceManagerImpl::getText(unsigned int, UString&)`.

`PAS_ListBtnTitleDTIItem` (My Button, vtable `0x11cc990`, slots `+0x144` y
`+0x148` en `0x87056c` y `0x87057c`) y `PAS_MenuTextItem` (SETTING_MENU,
vtable `0x11bcac8`, llamadas a `GEN_Label::setText` en `0x87c950` y
`0x87c964`) hacen lo mismo. [código]

Respuestas concretas:

- `CmnViewMenuData::getText(item, int)` devuelve el número `0x0204xxxx` del
  hijo `int` (índice absoluto), pero **los widgets no la llaman**: usan el slot
  interno de la vtable. Interceptarla no renombra nada en pantalla.
- Interceptar `TextSprite::setString(unsigned int)` **sí** cambia los rótulos
  del menú, tanto el título como el valor, en todas partes donde aparezca ese
  número.
- Interceptar `GEN_Label::setText(unsigned int)` también serviría y alcanza
  menos cosas (sólo `GEN_Label`).
- `ResourceManagerImpl::getText` no está en este camino: los rótulos lo
  evitan.

## Cómo ocultar, mostrar, renombrar y reordenar

Ninguna de estas recetas se probó en la cámara. Las direcciones de la vtable y
de las tablas son del firmware 1.07.

### Ocultar (recomendado)

Interceptar
`_ZN15CmnViewMenuData12initMenuDataEP21CmnViewMenuItemStructP26CmnViewMenuItemInconstPartP22CmnViewMenuExclusiveIF`.
Argumentos: `r0 = this` (`CmnViewMenuData*`), `r1 = raíz`, `r2 = inconst`,
`r3 = exclusivo`. Devuelve `int`; hay que devolver lo que devuelva la original.

```c
struct item {                     /* CmnViewMenuItemStruct, 0x28 */
    struct item *child; u8 n, pad[3]; int id;
    u32 text, icon, help, value_icon; u8 row, exec, pad2[2];
    int opt_num; const char *opt_str;
};
/* inconst[id*3 + 0] cursor, +1 estado, +2 válido */

int my_initMenuData(void *self, struct item *root, u8 *inc, void *excl)
{
    int r = real_initMenuData(self, root, inc, excl);
    u32 menu = (u32)root - cx_lib_base("/viewUnified7.so");  /* 0xc347c… */
    int c, i;
    for (c = 0; c < root->n; c++) {
        struct item *cat = &root->child[c];
        for (i = 0; i < cat->n; i++)
            if (hide(menu, cat->child[i].id))
                real_setItemValid(self, cat, i, 0);   /* o inc[id*3+2] = 0 */
    }
    return r;
}
```

- Ocultar por `(menú, id)`, no por texto: los textos se repiten entre
  variantes.
- Hay que hacerlo **dentro** del `initMenuData` interceptado, antes de volver.
  Justo después, la vista (CAMERA_SETTING_MENU, Ghidra `FUN_0007ad78`) cuenta
  las filas visibles de cada pestaña y apaga 0 a 3 filas de relleno para que
  sea múltiplo de 4 (una página). Si se oculta después, quedan páginas
  incompletas.
- `setItemValid` con `false` es lo mismo que escribir `inc[id*3+2] = 0`. Las
  dos son seguras con el resto del código: nada de `viewUnified7.so` vuelve a
  poner `true`: las 60 llamadas son del relleno y todas pasan `r3 = 0`
  [código].
- Como el arreglo es estático, lo oculto sigue oculto hasta que se escriba otra
  cosa. Conviene que la lista de ocultos sea fija y se aplique igual en cada
  apertura.

Riesgos:

1. **Pestaña vacía.** Si se ocultan todos los ítems de una pestaña (por
   ejemplo Flash, ids 47 a 49), el relleno también se apaga y la pestaña queda
   con 0 filas. Hay que ocultar también la pestaña: `setItemValid(self, root,
   índice_de_la_pestaña, 0)` (Flash es el hijo 3 de la raíz). El widget arma
   las pestañas con los hijos visibles de la raíz y cada pestaña trae su propio
   ícono en `+0x10`/`+0x18`, así que el resto debería correrse bien [inferido].
   `PAS_ListBtnTitleDTI` admite hasta 6 pestañas.
2. **Opciones.** No ocultar una opción de nivel 3 que pueda ser el valor
   actual: el valor que muestra la fila es el hijo elegido (byte 0 del padre) y,
   si ese hijo no se ve, el índice visible apunta a otra opción.
3. **Posición recordada.** Al abrir, CAMERA_SETTING_MENU llama a
   `PAS_ListBtnTitleValue::setTopItem` y `cursorMoveToById`; no se vio de
   dónde saca la fila de partida. Si es un índice visible guardado, con una
   lista de ocultos fija sigue apuntando a una fila que existe [inferido].
4. **Gris.** `setGreyOut` y las reglas del exclusivo sólo cambian el estado
   (2↔4); un ítem con `válido = 0` sigue oculto aunque lo pongan en gris.
5. `getCountAll` devuelve la cantidad **sin** filtrar; el relleno la usa a
   propósito para ubicar las filas vacías al final. No hay que tocarla.
6. `isItemDisplayed`, `getText`, `getSubItem` y `cursorMoveTo` exportadas no
   hacen falta: los widgets no las usan, y la lógica interna ya salta lo
   oculto.

La lista «Se oculta» de `docs/ui-spec.md` en `(menú, id)`:

| Ítem | Menú | id |
| --- | --- | --- |
| Scene Selection, Auto Back Light | CAMERA_SETTING_MENU | 23, 34 |
| Smile Shutter, Smile Sensitivity, Self-Timer | CAMERA_SETTING_MENU | 42, 43, 26 |
| Flash, Flash Level, Red Eye Reduction | CAMERA_SETTING_MENU | 47, 48, 49, y la pestaña Flash (id 3, hijo 3 de la raíz) |
| Fader | CAMERA_SETTING_MENU | 25 |
| Closer Voice, Audio Mode | CAMERA_SETTING_MENU | 53, 57 |
| Light Bright, Digital Zoom, Conversion Lens | CAMERA_SETTING_MENU | 69, 31 y 32, 33 |
| My Button | CAMERA_SETTING_MENU | 63 |
| Image Size | IMAGE_QUALITY_SIZE_MENU | 10, 11 |
| Download Music, Empty Music, Disc Burn | SETUP_MENU | 21, 22, 42 |
| Your Location, GPS Setting, Auto Clock ADJ, Auto Area ADJ | SETUP_MENU | 51, 55, 74, 75 |
| Demo Mode | SETUP_MENU | 67, 68 |
| Intelligent Auto, Golf Shot | botones (`view/IAUTO_L`, CAMERA_MODE_MENU) | sin tabla |
| Map View, Highlight Movie, Scenario, Highlight Playback | botones de PB_FUNC_MENU [inferido] | sin tabla |

Con eso, Face queda con Face Detection (41) y Flash se queda vacía; hay que
ocultar también la pestaña.

Si en la cámara ocultar diera problemas, lo seguro es gris:
`_ZN15CmnViewMenuData10setGreyOutEP21CmnViewMenuItemStructb` con `(self,
&cat->child[i], 1)` en el mismo punto. La fila queda visible y atenuada, y la
vista comprueba `isItemGreyout` antes de ejecutar (25 llamadas en
`viewUnified7.so`).

Las pantallas de botones (GLOBAL_MENU, CAMERA_MODE_MENU, y probablemente
PB_FUNC_MENU y EDIT_COPY_MENU) no pasan por aquí. Para esconder un botón habría
que trabajar con `LayoutableWidgetBase::setDispState(bool)` sobre su widget, o
cortar la vista en `ViewBase::openView` (ya interceptada en `cxview.c`). Eso no
se analizó.

### Mostrar un ítem que el exclusivo oculta

Mismo punto, después de la original: `inc[id*3+1] = 2` (normal) y
`inc[id*3+2] = 1`. Candidatos en CAMERA_SETTING_MENU: 12 IRIS, 13 Shutter
Speed, 14 AGC Limit, 15 AE Shift, 16 White Balance Shift. Abren
`view/IRIS_L`, `view/SHUTTER_SPEED_L`, `view/AGC_LIMIT`, `view/AE_SHIFT` y
`view/WB_SHIFT` (`opt_str`). Sin probar: el texto del valor sale del hijo
elegido (`Auto`, `F${v1}.${v2}`…), y falta ver que la vista llene esas
variables para esta unidad.

La alternativa sin código sería la propiedad de Backup `0x1070371` = 1
(condición 0). Desbloquearía IRIS, Shutter Speed, AE Shift, WB Shift y Dial
Setting en el exclusivo, pero IRIS, Shutter Speed y AGC Limit también los
oculta la columna de modelo (`0x1070316`). Además, otras partes del firmware
pueden leer esas propiedades; según las notas previas, `FUN_007a3fec`
(Ghidra) decide el alias `view/IRIS` → `view/IRIS_L` por una propiedad de
Backup. No se sabe si es la misma. Antes de escribir Backup hay que leer el
valor actual y respaldarlo (regla 2 de `AGENTS.md`).

### Renombrar

- **Global**, para un número en todas partes: la regla `label` que ya tiene
  `cxtext.c` sobre `TextSprite::setString(unsigned int)`. Alcanza el menú.
- **Sólo en un ítem**: cambiar `item->text` (`+0x0c`) en la tabla, que está en
  `.data` (escribible). Se puede hacer en el `initMenuData` interceptado; la
  fila pide el texto en cada actualización. Sirve con cualquier número que ya
  exista, en cualquier idioma [inferido]. Para un texto nuevo haría falta un
  número inventado que la interceptación de `TextSprite::setString(unsigned
  int)` traduzca con `TextSprite::setString(const char*)`
  (`_ZN2ux6gfxeng10TextSprite9setStringEPKc`). No se analizó.
- La ayuda (`+0x14`) y los íconos (`+0x10`, `+0x18`) se cambian igual.

### Reordenar y reagrupar [inferido]

Como el mapa id → ítem se arma en cada `initMenuData` recorriendo el árbol, y
el arreglo de 3 bytes va por id, el orden lo da sólo la posición en
`cat->child[]`:

- **Reordenar dentro de una pestaña**: antes de llamar a la original, permutar
  las estructuras de 0x28 bytes de `cat->child[]` (con `memcpy`; cada una lleva
  su puntero a hijos), dejando las 3 filas de relleno al final. Una sola vez
  por arranque, porque la tabla es estática.
- **Mover ítems entre pestañas del mismo menú**: armar arreglos nuevos (en
  memoria propia que no se libere) y cambiar `cat->child` y `cat->n`, con 3
  rellenos al final de cada pestaña.
- **Entre menús distintos no**: los ids indexan el arreglo de cada menú, y
  cada vista ejecuta sus ítems con un `switch` por id (76 llamadas a `getId`).

A favor: en código ARM, `viewUnified7.so` sólo referencia la raíz y el arreglo
de cada menú, nunca un ítem por dirección; y el relleno sólo usa las últimas
posiciones (`getCountAll − 1..4`). En contra: no se revisó el código Thumb ni
si alguna vista usa índices fijos de pestaña (por ejemplo, para elegir la
pestaña inicial).

## My Button y los atajos en pantalla

- La lista para asignar es `view/CUSTOMIZE_DTI_L`: una sola pestaña («Main
  Settings») con 61 ítems y 3 rellenos. Ids: 1 White Balance (válido=0); 2 White Balance; 3 Spot Meter/Fcs; 4 Spot Meter; 5 Spot Focus; 6 Exposure; 7 Focus; 8 IRIS; 9 Shutter Speed; 10 AGC Limit; 11 AE Shift; 12 White Balance Shift; 13 Low Lux; 14 3D Depth Adjustment; 15 Auto 3D Lens Adjust; 16 Scene Selection; 17 Cinematone; 18 Fader; 19 Self-Timer; 20 Tele Macro; 21 SteadyShot; 22 SteadyShot; 23 SteadyShot; 24 Digital Zoom; 25 Conversion Lens; 26 Auto Back Light; 27 Dial Setting; 28 Dial Setting; 29 NIGHTSHOT Light; 30 Face Detection; 31 Smile Shutter; 32 Smile Sensitivity; 33 Flash; 34 Flash Level; 35 Red Eye Reduction; 36 Closer Voice; 37 Blt-in Zoom Mic; 38 Auto Wind NR; 39 Wind Noise Reduct.; 40 Audio Mode; 41 Micref Level; 42 Audio Output Timing; 43 Expanded Focus; 44 Guide Frame; 45 Display Setting; 46 Zebra; 47 Peaking; 48 Light Bright; 49 Audio Level Display; 50 REC Mode; 51 Frame Rate; 52 Frame Rate (válido=0); 53 Frame Rate (válido=0); 54 /Setting; 55 Wide Mode; 56 x.v.Color; 57 Image Size; 58 Media Select; 59 LCD Brightness; 60 Your Location; 61 GPS Setting.
- `libObj.so` tiene una copia igual de la tabla (raíz vaddr `0x1123eb0`), que
  usa `ACT_DtiShooting` (`initMenuData` desde `0x8079e0`, exclusivo en
  `this+0x900`) [código]. Por el nombre, son los botones en pantalla de la
  toma [inferido].
- Los números que `FUN_00817b78` (Ghidra) traduce a pantallas, según
  `firmware-analysis.md` (6 `view/EXPOSURE`, 7 `view/FOCUS`, 8 `view/IRIS`,
  9 `view/SHUTTER_SPEED`, 10 `view/AGC_LIMIT`, 11 `view/AE_SHIFT`, 12
  `view/WB_SHIFT`), son los ids de esta tabla. Coinciden texto por texto.
  [inferido por coincidencia]
- El exclusivo de esta lista es `menuExclusiveCustomizeDTI_L`, exportado en
  `libObj.so`. `menuExclusiveCustomizeDTI_L::setState(int i, int v)`
  (`_ZN27menuExclusiveCustomizeDTI_L8setStateEii`, 92 llamadas desde
  `viewUnified7.so`) guarda `v` en `this[5+i]` para `i < 43`: son las
  condiciones, como en el exclusivo de CAMERA_SETTING_MENU. Interceptarla
  permitiría habilitar en My Button ítems que el modelo oculta. [código;
  efecto inferido]
- Dónde se guardan las asignaciones de My Button no se buscó.

## Pendiente

1. Leer en el Backup.bin de la unidad las propiedades `0x1070316` (columna de
   modelo), `0x1070371` (condición 0) y `0x1070408` (todas las condiciones), o
   anotarlas desde `cxg.so` interceptando `BackupManager::Bkup_Read(int,
   void*)`, que `viewUnified7.so` llama por la PLT.
2. Probar en la cámara la receta de ocultar con un ítem inofensivo (por
   ejemplo 34 Auto Back Light) y mirar scroll, cursor y relleno.
3. Probar «mostrar» con 12 IRIS.
4. Revisar el código Thumb de `viewUnified7.so` antes de reordenar.
5. Las tablas de reglas de IMAGE_QUALITY_SIZE_MENU, SETUP_MENU y TCUB_MENU no
   se leyeron; sus exclusivos son otras clases.

## Para repetirlo

Los scripts quedaron en el directorio temporal de la sesión; esto es lo
necesario para rehacerlos:

- PLT: cada entrada de `.plt` hace `add ip, pc, #a; add ip, ip, #b; ldr pc,
  [ip, #c]!`; `pc + 8 + a + b + c` es la ranura de la GOT, y su reubicación en
  `.rel.plt` da el símbolo. La entrada Thumb empieza 4 bytes antes.
- Raíces: en cada llamada a `initMenuData`, `r2 = literal + pc` (de `ldr r2,
  [pc, #…]` y `add r2, pc, r2`) es el arreglo inconst y `r1 = r2 − 0x28` es la
  raíz.
- Texto: registros de `share/app/string_english.uxc` alineados a 4 bytes,
  palabra `(7<<26)|id` y UTF-8 terminado en NUL.
- `arm-linux-gnueabi-objdump -d --no-show-raw-insn` en el contenedor
  `cx580-toolchain` para `libObj.so` y `viewUnified7.so`; Ghidra headless con
  `DecompileAt.java` y `DecompileVtable.java` sobre una copia del proyecto.
