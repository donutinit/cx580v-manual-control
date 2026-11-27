# Análisis del firmware 1.07

26 de noviembre de 2026. Las secciones de pantallas y parámetros salen de
leer cadenas en `firmware/cx580v-1.07/unpacked/`. La de arranque sale de
descompilar con Ghidra. Nada se probó en la cámara. Que una pantalla o un
parámetro exista en el binario no prueba que la CX580V lo ejecute bien.

## Dónde está cada cosa

| Partición | Contenido |
| --- | --- |
| `nflasha3` | Arranque de la parte de cámara: `vmlinux.bin`, `initrd.img`, `av-cam.bin` (5.6 MB), `bonobo.bin`, `sabin/idt_cam.bin` |
| `nflasha7` | FAT con imágenes de la interfaz (`images/*.uxc`) y `config.spec`, la lista de componentes de la compilación |
| `nflasha8` | La aplicación: `lib/libObj.so` (18.8 MB, capa de objetos de cámara), `lib/libmpr.so`, `lib/viewUnified2..7.so` (pantallas), `share/app/view*.uxc` (diseños), `scenario/*.so` |

Versión de la compilación: `12101_12.2012072401`, la misma que guarda
`Backup.bin`. Región `CX38600_U2`.

`config.spec` muestra que la aplicación sale de componentes comunes
(`App-Base`, `AppFw`, `Avbb-*`) más dos específicos de la familia:
`App-Model_112H … CAM-CJ-121H` y `App-View-TP-LEFT_121H … CAM-CJ-121H`.

## Los controles manuales ya están en el firmware

El firmware es compartido con modelos superiores de la misma generación.
Trae pantallas 3D de la TD20 y el dial de la PJ760V.

**Pantallas** en `lib/viewUnified7.so` y `share/app/`:

| Pantalla | Clase | Diseño |
| --- | --- | --- |
| `view/IRIS_L` | `ViewIris_L`, `LayoutIRIS_SETTING` | `viewIris_L.uxc` |
| `view/SHUTTER_SPEED_L` | `ViewShutterSpeed_L`, `LayoutSHUTTER_SPEED_SETTING` | `viewShutterSpeed_L.uxc` |
| `view/AGC_LIMIT` | `ViewAgc_Limit`, `LayoutID_OPAGC_SET_L` | `viewAgc_Limit.uxc` |
| `view/WB_SHIFT` | — | `viewWbShift.uxc` |
| AE Shift | — | `viewAeShift_L.uxc` |
| `view/CAMERA_CTRL_DIAL_L` | dial MANUAL de la PJ760V | `viewCameraCtrlDial.uxc` |

`lib/viewUnified2.so` exporta `CmnViewCameraCommon_L::control_Iris`,
`control_ShutterSpeed`, `control_Exposure` y `manual_FocusDrive`.

**Parámetros** en `lib/libObj.so`:

- Iris: `PARAMID_IRIS_DRIVE`, `PARAMID_IRIS_DRIVE_MODE` con
  `VALUEID_IRIS_AUTO` y `VALUEID_IRIS_MANUAL`.
- Exposición manual: `PARAMID_MANUAL_EXPOSURE_MODE`,
  `PARAMID_MANUAL_EXPOSURE_DRIVE`.
- Ganancia: `PARAMID_AGC_LIMIT`, `PARAMID_AGC_LIMIT_MODE`, `AGC Limit Min/Max`.
- Balance de blancos: `PARAMID_WB_COLOR_TEMP`, `PARAMID_WB_SHIFT_LB` y
  `PARAMID_WB_SHIFT_CC`, `PARAMID_WB_TUNING`.
- **`VALUEID_IRIS_SHUTTER_GAIN_MANUAL_ON` / `_OFF`** y una variante con ISO.
  El nombre describe iris, obturación y ganancia manuales al mismo tiempo,
  como en la HXR-NX70. Es el control independiente que busca el proyecto.
  Falta comprobar que la CX580V lo acepte.

## Qué los esconde

Respuesta, sacada del volcado de la cámara (ver «Resultado del volcado»): la
capa de cámara no bloquea iris, obturación ni límite de ganancia. El ISO sí,
por el factor `0x1f7`. Lo demás lo decide el menú de la CX580V, que no
incluye esos ítems. Nada de esto exige reflashear: los cambios se pueden hacer
en memoria desde `cxg.so`.

## Arranque y camino de rescate

Descompilado con Ghidra 12.1.4 (`firmware/ghidra/out/init.c`,
`bootin.elf.c`) y leído en los scripts del updater. Probado sólo en QEMU, no
en la cámara.

1. **Bootloader** (`boot1.bin`): monta la FAT de `nflasha2` y busca
   `\updater\mode`. Si existe, carga el kernel y el initrd del updater desde
   `nflasha1`. Si no, arranca el sistema principal (`nflasha3`, `nflasha8`).
   El updater no depende del firmware principal.
2. **Updater** (`nflasha1`, `UdtrMain.sh` y `execute_ufp.sh`): con sólo `mode`
   entra en modo usuario. Si existe `/setting/updater/comp` (en esta unidad
   existe) borra `mode*` al empezar, así que un arranque al updater basta para
   que el siguiente sea normal. Después espera el firmware por USB, que es
   donde entra el shell de PMCA, y se apaga solo si nadie se conecta.
3. **init del sistema principal** (`nflasha3`, sin modificar): monta
   `/setting` con `noatime,dirsync,sync` y sin `noexec`. Si existe
   `/setting/mode/dmode`, lee `/setting/env.txt` y exporta cada línea
   `VAR=valor`. Después lanza `/usr/bin/bootin.elf`, que hereda el entorno y
   lo pasa a `im.elf`, la app de la cámara, que enlaza `libObj.so`.
   - Sólo lee los primeros `st_blocks` bytes de `env.txt`, que son 32 con
     clusters de 16 KiB.
   - Si en `/setting` quedan menos de 200 KiB libres corre `dosfsck -Y` en
     cada arranque. Hoy hay 448 KiB libres.
   - Un `dmode` vacío no activa ningún modo de depuración, ni en `init` ni en
     `bootin.elf`.
4. **Cargador de Sony** (`sonyld.so`): si el `LD_PRELOAD` apunta a un
   archivo que no existe, el proceso **no arranca** («Cannot open»). Por eso
   la biblioteca va en `/setting`, junto a `env.txt`.

### Prueba de la bandera en la cámara, 26 de noviembre

`cxdump.py guard-flag-test` creó `/setting/updater/mode`, vacío y con el md5
verificado en la cámara. Antes guardó la copia verificada
`backups/20261126-161601-guard-flag-test/`. Al salir del shell la cámara
reinició y volvió a enumerar como `054c:03e2` (el dispositivo USB pasó de 57
a 60), o sea que entró al updater por la bandera. `guard-status` mostró que
el updater la había borrado solo. Al salir volvió a modo normal (`054c:05fc`)
en unos 8 s.

Para entrar al updater desde el modo normal, la cámara pide una confirmación
en pantalla. Mientras no se acepta, PMCA espera y termina sin conectar.

### Seguro de rescate (`preload/cxguard.c`)

Biblioteca de 5 KB, sin libc. En `bootin.elf` crea `/setting/updater/mode`
antes de que corra la app. Si la app falla, el siguiente encendido entra al
updater, y desde ahí `cxdump.py guard-remove` deja la cámara de fábrica. En
QEMU, con el `sonyld.so` de la cámara, carga en `busybox`, `bootin.elf` e
`im.elf` y sólo crea la bandera en `bootin.elf`.

Se instala con `tools/cxdump.py guard-install`. Escribe biblioteca, `env.txt`
y `dmode`, en ese orden, y verifica el md5 en la cámara después de cada
escritura. Antes guarda una copia verificada de `nflasha2`. Si se corta a la
mitad, falta `dmode` y la cámara arranca de fábrica. `guard-remove` borra en
orden inverso.

Nunca se escriben `nflasha1`, el bootloader ni las particiones del firmware
principal.

**Probado en la cámara el 26 de noviembre.** `guard-install` escribió los tres
archivos con el md5 verificado. Antes guardó la copia
`backups/20261126-161904-guard-install/`. La cámara arrancó a modo normal
(`054c:05fc`). Al apagarla y prenderla entró sola al updater (`054c:03e2`),
porque la biblioteca había corrido en `bootin.elf` y había puesto la bandera.
`guard-status` mostró los tres archivos intactos y la bandera ya borrada por
el updater.

Mientras el seguro esté instalado, un encendido sí y otro no la cámara entra
al updater. Si nadie se conecta, se apaga sola.

Riesgo que quedaba antes de la prueba: que la biblioteca no cargara en la cámara aunque cargara en
QEMU. En ese caso `bootin.elf` no arranca, no se crea la bandera y no hay USB.
Quedaría la consola serie de `init` en `ttyAM0` a 115200, que hace falta
ubicar en la placa.

## Tabla de bloqueos (`MWF::InhInfo`)

Descompilado de `libObj.so` y `libMWF.so`. `InhFeatureCheck`
(`libObj.so`, `FUN_00aeb48c` en Ghidra) traduce cada parámetro a un ID de
función y pregunta a `FUN_00af489c(id)`, que llama a
`MWF::InhInfo::GetInhInfo`. La función queda bloqueada si su fila de la tabla
tiene algún bit encendido.

- La tabla son `DefInh::GetFeatureIdMax()` = `0x403` funciones por
  `DefInh::GetFactorTblMax()` = 58 palabras de 32 bits, con
  `GetFactorIdMax()` = `0x739` factores. Los tamaños vienen de
  `libSysDef.so`. El bit `b` de la palabra `w` encendido en la fila `f`
  significa que el factor `w*32+b` bloquea la función `f`.
- Vive en `operator new[]`, y la apunta `MWF::InhInfo::mp_inhTbl`, que está en
  `libMWF.so` + `0x45e5c`. `im.elf` no tiene *copy relocations*, así que ése
  es el puntero vivo.
- Los factores se encienden y apagan con
  `MWF::InhMgr::ChangeInhFactorStatus(factor, bool)`, desde 152 funciones de
  `libObj.so`. `libMWF.so` incluye órdenes de depuración de Sony
  (`MWF::InhMcmd`: `InhInfo`, `AllInhInfo`, `ChangeInhFactor`).

Las direcciones de Ghidra llevan `0x10000` de más respecto del archivo.

### Volcado en la cámara (`preload/cxinh.c`)

Dentro de `im.elf`, `cxg.so` arranca un hilo propio, creado con `clone()` y sin
libc, que sólo lee. A los 20 s y a los 90 s copia las palabras distintas de
cero de la tabla a `/setting/cxinh.bin`, 8 KiB como máximo por copia. Antes
de leer comprueba:

- que `libMWF.so` tenga las instrucciones de la versión 1.07 en `0x1d1d8` y
  `0x1d3a4`;
- que el puntero y la tabla estén dentro de memoria mapeada, según
  `/proc/self/maps`.

Si el archivo ya existe no escribe nada. Se baja con
`cxdump.py guard-fetch --delete` y se lee con `tools/cxinh.py`.

`preload/test/run.sh` lo prueba en QEMU con `sonyld.so` y la `libMWF.so` de
la cámara. Comprueba que `busybox`, `bootin.elf` e `im.elf` se comporten igual
con y sin la biblioteca, que sólo `bootin.elf` cree la bandera, que las copias
salgan con la tabla sin inicializar e inicializada, que los bits marcados con
`OrInhTblValue` se decodifiquen bien y que una copia existente no se
sobrescriba.

### Resultado del volcado, 26 de noviembre

`backups/20261126-164444-inhdump/cxinh.bin`: la cámara arrancó normal con la
biblioteca, estuvo 1.5 min en `Manual Settings` y se copió la tabla a los
20 s y a los 90 s, con estado `ok`. A los 90 s había 272 funciones bloqueadas
por 32 factores activos.

- El bit 0 de la palabra 0 es sólo el resumen («bloqueada por algo»).
- El factor `0x1f7` es el único que bloquea 195 funciones. `DefInh::sm_refTbl`
  (`libSysDef.so`, `0x3a2b8` bytes, las mismas dimensiones que la tabla viva)
  confirma que todas las funciones que puede bloquear están bloqueadas.

Para saber qué función consulta `InhFeatureCheck` con cada mensaje, se emuló
en Ghidra (`firmware/ghidra/scripts/EmuInhCheck.java`) con los mensajes
`0x7540`–`0x75e0` de la categoría `0x3100`. Los números de mensaje salen de
las funciones que registran «Issued ObjCamera::MSGID_SET_…»
(`firmware/ghidra/out/setcmds.tsv`).

| Mensaje | ID | Función | Estado en la cámara |
| --- | --- | --- | --- |
| `MSGID_SET_IRIS_DRIVE_MODE_CMD` | `0x75b4` | `0x2d3` | libre |
| `MSGID_SET_SHUT_DRIVE_MODE_CMD` | `0x75b6` | `0x2d4` | libre |
| `MSGID_SET_AGC_LIMIT_CMD` | `0x75ba` | `0x35d` | libre |
| `MSGID_SET_EXPOSURE_MODE_CMD` | `0x7558` | `0x5e` | libre |
| sin nombre | `0x75be` | `0x2d5` | libre |
| `MSGID_SET_ISO_SPEED_RATE_CMD` | `0x75d0` | `0x116` | bloqueada por `0x1f7` |
| `MSGID_SET_AE_METER_MODE_CMD` | `0x7588` | `0x174` | bloqueada por `0x1f7` |
| `MSGID_SET_COLOR_MODE_CMD` | `0x757c` | `0xf9` | bloqueada por `0x1f7` |

Conclusión: en esta unidad la capa de cámara (`libObj.so`) no bloquea iris,
obturación ni límite de ganancia. El factor `0x1f7` corresponde a funciones de
cámara de fotos. Lo que falta es la entrada en el menú de la CX580V. Que el
procesador de cámara (`av-cam.bin`) obedezca esas órdenes sigue sin probarse.

## Cómo la interfaz habla con la cámara

Las pantallas no mandan mensajes de cámara directamente: usan el modelo
`model/CAMERA` del motor de UI. `CmnViewCameraCommon_L` (`libObj.so`, con los
métodos exportados) guarda en su primer campo un `ViewBase*` y manda todo
con:

- `ViewBase::requestModelExecute(modelo, acción, ParamList*)`, en `0x19ba9c`,
  para ejecutar;
- `ViewBase::getModelInfo(modelo, consulta, salida, entrada)`, en `0x19bb40`,
  para consultar;
- `ViewBase::openView(nombre, n)`, en `0x19bde8`, y `closeView(nombre)`, en
  `0x19bef8`, para abrir y cerrar pantallas.

Las cuatro están en `libObj.so` y las pantallas de `viewUnified2..7.so` las
llaman por la PLT. `ParamList` es un puntero a un vector `{inicio, fin,
capacidad}` de `ParamBase*`, y cada `ParamBase` es `{vtable, tipo, clave,
valor}`.

Las acciones del iris según `control_Iris` (las usaría la pantalla
`view/IRIS_L` de la PJ760V):

| Llamada | Acción en `model/CAMERA` | Parámetro |
| --- | --- | --- |
| `control_Iris(3)`, pasar a manual | `0x10` y luego `0x32` | modo = 2 y clave `0x1c4` = 1 |
| `control_Iris(2)`, alternar | `0x10` | 2 o 5 (manual o automático) |
| `control_Iris(0/1)`, un paso | `0x2d` | ± tamaño de paso |

`control_Exposure` usa las acciones `0xf` (modo), `0x22` (paso) y `0x32`.

### Bitácora de la interfaz (`preload/cxview.c`)

`cxg.so` v3 exporta esas cuatro funciones con el mismo nombre. Como va
precargada, las llamadas de las pantallas pasan primero por ella. Cada
envoltura anota la llamada en `/setting/cxreq.log` (48 KiB como máximo, en
hexadecimal, sólo durante los primeros 15 minutos) y pasa los mismos
argumentos a la función original. La original se busca por nombre en la
tabla dinámica de `libObj.so` en memoria (`preload/cxdl.c`, hash SysV).

En QEMU, con una `libcxt.so` que hace de `libObj.so`, se comprobó que
argumentos y retornos llegan intactos, incluido el quinto argumento de
`getModelInfo`, que va por la pila. La búsqueda de símbolos se verificó contra
la `libObj.so` real: las cuatro direcciones coinciden con `readelf`.

### Bitácora de la cámara, 26 de noviembre

`backups/20261126-174933-dumps/cxreq.log`: v3 instalada, con la secuencia
Exposure, White Balance, Focus y Low Lux hecha a mano. La interfaz funcionó
normal. Son 657 líneas: 27 `openView`, 13 `closeView`, 273
`requestModelExecute` y 343 `getModelInfo`.

Pantallas: `view/GLOBAL_MENU → view/CAMERA_SETTING_MENU → view/EXPOSURE`
(también `view/WHITE_BALANCE` y `view/FOCUS`), todas con `openView(nombre, 0)`.

| Acción del usuario | `model/CAMERA` | Parámetro |
| --- | --- | --- |
| Exposure → Manual / Auto | `0xf` | `0x12d` = 1 / 2 |
| Exposure + / − | `0x22` | `0x17e` = +1 / −1 |
| White Balance → Indoor / Auto | `0x12` | `0x136` = 9 / 1 |
| Focus → Manual / Auto | `0xe` | `0x124` = 1 / 2 (más `0x32`, `0x1c4`) |
| Focus cerca / lejos | `0x19` | `0x161` = 1 / 0, `0x162` = 4 |
| Low Lux On / Off | `0x2a` | `0x19a` = 1 / 2 |
| Entrar / salir de una pantalla de ajuste | `0x30` | `0x1bf` = 1 / 2, `0x1c0` = 0 |

Coincide con `control_Exposure`, que usa las acciones `0xf` y `0x22` y las claves
`0x10e+0x1f` y `0x17e`. Por el mismo camino, `control_Iris` y
`control_ShutterSpeed` usan:

| Control | Acción | Clave | Valores |
| --- | --- | --- | --- |
| Modo AE | `0x10` | `0x12a` | 2 prioridad de iris, 3 prioridad de obturación, 5 automático (PJ760V) |
| Paso de iris | `0x2d` | `0x1a7` | ± paso |
| Paso de obturación | `0x2c` | `0x1a8` | ± paso |
| Al volver a automático | `0x1d` | `0x16a` | 1 |

La consulta `0xf` devuelve en esta cámara `0x12b` = `0x12c` = 6: el modo AE de
fábrica de la CX580V es 6, no 5.

En `libObj.so`, `FUN_007a3fec` arma el alias de los nombres del menú
(`view/IRIS` → `view/IRIS_L`, `view/EXPOSURE` → `view/EXPOSURE_L`…) y decide
si usarlo según una propiedad de `Backup.bin`. `FUN_00817b78` traduce los
ítems de My Button a pantallas: 6 `view/EXPOSURE`, 7 `view/FOCUS`,
8 `view/IRIS`, 9 `view/SHUTTER_SPEED`, 10 `view/AGC_LIMIT`, `0xb`
`view/AE_SHIFT`, `0xc` `view/WB_SHIFT`.

### Redirección de prueba (v4)

Si existe `/setting/cxmode.txt` (lo escribe
`cxdump.py guard-install --mode iris`), al abrir `view/EXPOSURE` se abre
otra pantalla:

| Modo | Pantalla |
| --- | --- |
| `iris` | `view/IRIS` |
| `shutter` | `view/SHUTTER_SPEED` |
| `agc` | `view/AGC_LIMIT` |
| `aeshift` | `view/AE_SHIFT` |
| `wbshift` | `view/WB_SHIFT` |

Sin el archivo, o con un modo desconocido, no redirige nada. Probado en QEMU;
todavía no en la cámara.

## Capa de cámara (`ObjCamera`)

Desde la v6, `cxg.so` también intercepta `MWF::ObjIf::IssueCommandAsync` e
`IssueCommandSync` (`libMWF.so`, en `0x23c94` y `0x23b58`), que `libObj.so`
llama por la PLT. Anota los mensajes de la categoría `0x3100` con todos sus
parámetros. `tools/cxlogdecode.py` los traduce con los diccionarios.

### Diccionarios (`firmware/names/`)

En `.data` de `libObj.so` hay arreglos de punteros a nombres, uno por número.
Para anclarlos se usaron números conocidos:

| Diccionario | Base | Ancla |
| --- | --- | --- |
| `MSGID.tsv`, asíncronos | `0x74d5` | `SET_IRIS_DRIVE_MODE_CMD` = `0x75b4`; los 66 números conocidos coinciden |
| `MSGID.tsv`, síncronos (`MSGID_SYNC_*`) | `0x6500` | `SYNC_IRIS_DRIVE` = `0x6502`, `SYNC_SHUT_DRIVE` = `0x6504`, `SYNC_LENS_COVER_STATUS` = `0x6518`, vistos en la cámara |
| `PARAMID.tsv` | `0x9500` | `IRIS_DRIVE_MODE` = `0x96d3`, el que agrega `pvt_SetParamIrisDriveMode` |
| `VALUEID.tsv` | `0` | `IRIS_AUTO`/`MANUAL` = `0x1ca`/`0x1cb`, de `pvt_ChangeIrisDriveModeParamApp2Obj` |

`libObj-names.tsv` son los nombres internos del agente de cámara (`MID_*`),
sacados corriendo sus funciones de depuración en QEMU
(`preload/test/names.c`). Ahí están `MID_IRIS_SHUTTER_GAIN_MANUAL_CMD`
(`0xa034`) y `VALUEID_EXC_MOVIE_MANUAL_MODE` (`0x102b`).

### Formato de `ObjMsg` (`libMWF.so` 1.07)

- `ObjMsg[0]` apunta al cuerpo: `+4` categoría, `+8` mensaje, `+0xc` un
  `ParamMap` con `{cuenta, primer bloque}`.
- Cada bloque es `{siguiente, 5 × {id, valor, puntero al tipo}}`.
- El tipo `[0]` vale 3 (entero de 8 bits), 7 (entero), 8 (sin signo) o 10
  (puntero). Para los enteros, el valor va directo.
- ObjCamera arma sus mensajes así: `ObjMsg(0x3100, mensaje)`, después
  `SetCommonRelayParam(1, ParamStd<uint,8>(secuencia))` y los `AddParam`.
  `cxg.so` arma los suyos igual (`inject_n` en `preload/cxobj.c`).

### Lo que manda la interfaz

| Pantalla | Mensaje a ObjCamera |
| --- | --- |
| Iris manual | `SET_EXPOSURE_MODE`, `EXPOSURE_MODE = EXP_MODE_IRIS` (`0xf3`) |
| Iris ± | `SYNC_IRIS_DRIVE`, `IRIS_DRIVE = ±1` |
| Shutter manual | `SET_EXPOSURE_MODE`, `EXP_MODE_SHUTTER` (`0xf4`) |
| Shutter ± | `SYNC_SHUT_DRIVE`, `SHUT_DRIVE = ±1` |
| Automático (con `aeauto 6`) | `SET_EXPOSURE_MODE`, `EXP_MODE_AUTO_2` (`0xf7`) |
| AE Shift | `SET_EXPOSURE_BIAS`: `EXPOSURE_BIAS_SW`, `EXPOSURE_BIAS` (tipo 3, −4..+4), `EXPOSURE_BIAS_STEP` |

## Control manual independiente, probado en la cámara

| Versión | Qué hace | Resultado en la cámara |
| --- | --- | --- |
| v5 `aemode 1` | modo 1 en el modelo | la cámara no lo adopta y las pantallas dejan de dar pasos |
| v7 `objmanual 1` | `EXP_MODE_MANUAL` en la capa de cámara | los pasos llegan, pero el modelo no conoce el modo M, lo toma por Intelligent Auto y abre un aviso en cada cambio |
| v8 `holdmanual 1` | prioridades normales; al pasar de una a otra, `cxg.so` inyecta `SET_SHUT_DRIVE_MODE = SHUT_MANUAL` (`0x96d6 = 0x1cd`) o `SET_IRIS_DRIVE_MODE = IRIS_MANUAL` (`0x96d3 = 0x1cb`) | **funciona**: la obturación se quedó en 1/15 mientras el iris iba de F1.8 a F10; la ganancia compensaba hasta su máximo |
| v9c `gainshift 1` | cada paso k de AE Shift inyecta `SET_AGC_LIMIT` con `AGC_LIMIT = (k+4)·3` dB (tipo 3) y `AGC_LIMIT_MODE = ON`; el AE Shift real pasa intacto | **funciona**: con iris y obturación fijos, subir AE Shift aumenta brillo y ruido, bajarlo los reduce; la cámara acepta cada límite (`ret=0`) |
| v10 | `cxg.so` recuerda qué está manual en la cámara y lo reporta a cada pantalla, y repite el manual del otro control en cada prioridad | los dos se quedan fijos, pero al volver a una pantalla su valor aparece vacío y el automático de una pantalla pone las dos en automático |
| v11b | el primer paso en una pantalla cuyo control no es la prioridad actual se convierte en «poner esta prioridad» (acción `0x10`), para que el modelo vuelva a mostrar el valor; el automático de una pantalla pasa a la prioridad del otro control e inyecta `IRIS_AUTO` o `SHUT_AUTO` | el valor reaparece al primer toque; la obturación vuelve exacta, el iris vuelve dos pasos abajo (F2.8 → F2.2) |
| v11c | la cámara se queda en prioridad de iris y la obturación se fija sólo por `SHUT_DRIVE_MODE` | sin pasos de obturación: el código del primer toque convertía cada paso en otro cambio de modo (error propio); no prueba si la cámara los aceptaría |
| v11d | el otro control se fija antes y después del cambio de prioridad | el que entra vuelve a su valor de fábrica (1/60, F1.8). La segunda inyección del mismo mensaje dio `ret=ffff` |
| v11e | AE lock (`SET_AELOCK_MODE_CMD 0x7610`, `AELOCK_MODE 0x9795`: `NORMAL_AELOCK_ON 0x2d6`, `AELOCK_OFF 0x2d8`, tipo 7) alrededor del cambio | los valores se guardan, pero el que entra queda uno o dos tercios de paso fuera (F2.8 → F2.4, 1/180 → 1/125). La bitácora muestra que la cámara rechazó cada desbloqueo (`ret=ffff`): el AE lock se quedó puesto |
| v11f | desbloqueo diferido y reintentado; `cxg.so` intercepta `MWF::ObjMsg::GetParam` para leer `FNO` (`0x951b`), `FNO_CURRENT_MIN` (`0x9787`) y `SHUT_SPEED_NUMER/DENOM` (`0x951c/0x951d`, tipo 6) cuando `ObjCamera` procesa las notificaciones; guarda el valor de cada control en su prioridad y, al volver, da pasos `SYNC_IRIS_DRIVE` / `SYNC_SHUT_DRIVE` (signed char, `IssueCommandSync`, como `pvt_SetIrisDrive`) hasta que la cámara reporte el mismo | **funciona**: el dueño confirmó que iris y obturación vuelven a su valor, y que el automático de cada pantalla deja fijo el otro control |
| v12 `isoshift 1` | cada paso de AE Shift manda `SET_ISO_SPEED_RATE` (ISO 100–4000, mínimo y máximo iguales) | la interfaz lo rechaza antes de mandarlo: `ret=a000`, el código de bloqueo de `InhFeatureCheck` (función `0x116`, factor `0x1f7`) |
| v14 `unlockiso 1` | `cxg.so` intercepta `MWF::InhInfo::GetInhInfo` y contesta libre la función `0x116` | pasa el ISO automático; los fijos siguen bloqueados por otra función que depende del valor |
| v15 `autounlock 1` | si una orden propia vuelve con `0xa000`, se contestan libres las funciones que la bloquearon (aprendió `0x12a`–`0x132`, una por valor de ISO) y se reintenta | la cámara acepta cada ISO fijo (`ret=0`), pero no lo aplica: `ISO_SPEED_RATE_ACT` sigue en automático y la ganancia no cambia |
| v13 / v15b `expogain 1` | AE Shift enciende la exposición manual de fábrica (`SET_MANUAL_EXPOSURE_MODE 0x7552`, `0x95bd = 0xea`) y cada paso es `SYNC_MANUAL_EXPOSURE_DRIVE` (`0x6506`) | al encenderla la cámara regresa iris y obturación a su programa; al volver a una prioridad se apaga y sus pasos (desbloqueada la función `0x111`) no mueven nada |
| v16 `gainshift 1` + `gainlock 4` | la pantalla AE Shift elige el tope de ganancia; a la cámara le llega siempre AE Shift +4 y `cxg.so` le devuelve a la pantalla su propio valor al leer `EXPOSURE_BIAS_SET/ACT` (`0x9655/0x9658`) | **funciona**: con iris y obturación fijos la ganancia queda en el tope elegido; cada control se mueve sin tocar los otros. Quedan saltos esporádicos de 1/3 a 1/6 de paso, en estudio |

Detalles:

- El aviso «Turn off Intelligent Auto» aparece al poner prioridad de
  obturación incluso sin inyección. Parece venir de la propia CX580V con
  Intelligent Auto encendido.
- La pantalla AGC Limit de la PJ760V se abre pero no manda órdenes.
- La Exposure de fábrica y las prioridades se excluyen en el modelo: activar
  una apaga la otra.
- El modo AE vuelve a 6 (`EXP_MODE_AUTO_2`) en cada arranque.
- La capa de cámara rechaza (`ret=ffff`) un mensaje si otro con el mismo MSGID
  sigue en curso. Dos inyecciones iguales seguidas fallan; la segunda tiene que
  esperar a la siguiente llamada de la interfaz.
- Al volver a la prioridad de un control, la cámara no lo deja donde estaba:
  según lo que se haga antes del cambio, lo regresa a su valor de fábrica o
  lo pone uno o dos pasos fuera. Por eso v11f corrige con pasos, leyendo el
  valor real.
- El signo de los pasos: +1 en el iris cierra (sube el F). En la obturación,
  `cxg.so` supone que +1 la hace más rápida y corrige el signo si el primer
  paso se aleja del objetivo.
- Si el F mínimo que reporta el lente cambió (zoom), el iris no se corrige,
  porque el mismo F ya no corresponde a la misma apertura.
- La ganancia real llega en `NOTIFY_ISO_GAIN_STATUS` (`PARAMID_ISO_GAIN_STATUS
  0x9706`) como `VALUEID_DB_*`: `0x1e8` = 0 dB, cada valor siguiente +3 dB, hasta
  `0x1f2` = 30 dB (`0x1e6`/`0x1e7` = −6/−3 dB).
- El error `0xa000` de `IssueCommandAsync/Sync` es «bloqueado» y sale de
  `InhFeatureCheck` (`libObj.so`), que consulta `InhInfo::GetInhInfo` por la PLT.

## Para repetirlo

```bash
cd firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked
strings -n 4 lib/libObj.so | grep -E '^VALUEID_(IRIS|MANUAL|AGC)' | sort -u
strings -n 4 lib/viewUnified7.so | grep -E '^view/'
strings -n 4 lib/libObj.so | grep -c InhFeatureCheck
```

## Pendiente

1. **ISO.** Es la vía de la ganancia manual en este firmware:
   `MSGID_SET_ISO_SPEED_RATE_CMD`, `PARAMID_ISO_GAIN_STATUS` y el modo
   `IRIS_SHUTTER_GAIN_ISO_MANUAL`. Está bloqueado por el factor `0x1f7`, que
   ninguna de las 152 llamadas de `libObj.so` enciende con un número fijo.
   Para destrabarlo sin tocar el firmware, se puede borrar en memoria el bit
   `0x1f7` de la función `0x116` en `DefInh::sm_refTbl`, que está en `.data`
   de `libSysDef.so` y es escribible. Falta saber si el procesador de cámara
   aplica ISO en video.
2. **Menú.** `viewUnified7.so` tiene las pantallas de la PJ760V
   (`view/IRIS_L`, `view/SHUTTER_SPEED_L`, `view/AGC_LIMIT`, `view/WB_SHIFT`).
   `menuExclusiveCameraSetting` (vtable en `0xd7188` de Ghidra) sólo aplica
   29 reglas de exclusión entre ítems y recibe la lista de ítems ya armada.
   Falta encontrar dónde se arma esa lista para la CX580V.
3. **Órdenes directas.** Resuelto: `cxg.so` arma `ObjMsg` con las funciones de
   `libMWF.so` y las manda desde los hilos de la interfaz (dentro de las
   envolturas de `ViewBase` y `ObjIf`), nunca desde un hilo propio, porque los
   hilos propios no tienen TLS.
4. Averiguar si `av-cam.bin` cumple las órdenes. Se verifica grabando y
   leyendo los metadatos con `tools/cxexposure.py`.
