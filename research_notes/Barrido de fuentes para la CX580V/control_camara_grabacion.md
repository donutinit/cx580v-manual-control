# Control de cámara y de grabación en la CX580V: detalle por función, fuentes públicas y cotejo local

Notas del 2026-11-26/27. Retoman el intento anterior interrumpido (sus archivos intermedios en `…/scratchpad/r4/` y el informe del ayudante `handback_foros.md`) y cubren lo que faltó. Etiquetas:

- **[fuente]**: lo leí hoy en la fuente citada (página, issue o archivo de un repositorio).
- **[local]**: lo comprobé hoy con herramientas sólo de lectura (strings, llvm-objdump, mcopy en modo lectura, scripts Python propios que sólo leen) sobre el firmware 1.07 desempacado o sobre los respaldos. No es conocimiento público.
- **[ayudante]**: sale del informe del ayudante del intento anterior y no lo reabrí; tómese como pista.
- **[inferido]**: deducción mía, sin prueba.

Rutas abreviadas: `av-cam` = `firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin`; `lib/` = `firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/`; diccionarios = `firmware/names/*.tsv`. Las direcciones de `av-cam` son desplazamientos del archivo (base de carga `0xc3431000`) o direcciones virtuales `0xc3…`. Nada de esto se probó en la cámara.

**Corrección principal al informe anterior.** `[ADF]PRO_SET_MANUAL_GAIN_LINK` y `[ADF]PRO_SET_AGC_LINK` no tienen que ver con la ganancia de video: son órdenes de **audio** de la entrada «InputPro» (micrófono interno, XLR1, XLR2, tono). La «ruta de ganancia manual por `PRO_SET_MANUAL_GAIN_LINK`» de P1 no existe. Lo que sí trae el motor de AE de `av-cam` es un estado de AGC manual (`uc_AgcManuOn`, `us_AgcManuCont`) paralelo al de iris y obturación (sección 1).

---

## 1. Ganancia manual fija en video (P1) y saltos de 1/3–1/6 de paso

### Takeaway
Ninguna fuente pública sigue la ganancia o el ISO manual de película hasta av-cam en ninguna cámara Sony, ni describe el bucle de AE (paso, histéresis). En la CX580V, `PRO_SET_MANUAL_GAIN_LINK` resultó ser de audio, pero el motor de AE de `av-cam` sí tiene un modo de AGC manual (`uc_AgcManuOn` con su valor `us_AgcManuCont`) junto a `uc_IrisManuOn`/`us_IrisManuCont` y `uc_ShutManuOn`/`us_ShutManuCont`. El diccionario de ObjCamera no tiene ninguna orden con «GAIN» en el nombre. Las vías candidatas para encenderlo son el modo M (`EXP_MODE_MANUAL`), las teclas LANC de ganancia y la tabla de órdenes `0x40xx` de av-cam, y ninguna está trazada.

### Cited Findings
- [local] `PRO_SET_MANUAL_GAIN_LINK` es de audio. En `av-cam` la cadena `[ADF]PRO_SET_MANUAL_GAIN_LINK COMP to Biz dst=%x, err=%x` (`0x349458`) va en un bloque con `PRO_SET_INT_MIC_SENS`, `PRO_SET_INPUT_TRIM`, `PRO_SET_CH_SOURCE`, `PRO_SET_REC_LEVEL_MODE`, `PRO_SET_AGC_LINK` y `PRO_SET_REC_LEVEL_LIMITTER` (`0x348188`–`0x34975e`). Las órdenes de depuración son `[ADF][DBGCMD]InputPro SetAgcLink`, `InputPro SetManualGainLink`, `InputPro SetIntMicSens` y `InputPro SetRecLevelMode` (`0x402f4`–`0x40408`). La ayuda de la orden dice `arg2:source [0:IntMic, 1:XLR1, 2:XLR2, 3:TONE]` (`0x529bb8`), y el driver tiene `AVIO SetProInputAgcLink(Audio)` (`0x2c44f0`) y `AdfInputProMsgSetAgcLink::ConvProAgcLinkBiz2Avio` (`0x501f14`) — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [fuente] La ZV-E10 tiene el mismo juego de parámetros de audio por canal: `PRM_IN_PRO_SET_AGC_LINKC1E/C2E`, `PRM_IN_PRO_SET_CH_SOURCEC1E`, `PRM_IN_PRO_SET_INPUT_TRIMC1E`, `PRM_IN_PRO_SET_INT_MIC_SENSC1E`, `PRM_IN_PRO_SET_REC_LEVEL_LIMITTERC1E`, `PRM_IN_PRO_SET_REC_LEVEL_MODEC1E`, `PRM_IN_SET_REC_LEVELC1E` y `PRM_IN_SET_REF_LEVELC1E` (líneas 380–433) — [param_vocabulary.txt](https://github.com/cokeenet/zve10-qemu/blob/main/tools/param_vocabulary.txt).
- [local] `av-cam` tiene en `0x49aa40` una tabla de 147 variables del AE, cada una con `{puntero al nombre, tamaño, dirección en RAM}`. Entre ellas: `uc_IrisManuOn` (`0xc3a4f6e9`), `uc_ShutManuOn` (`…6eb`), **`uc_AgcManuOn` (`…6ed`)**, `us_IrisManuCont` (`…6fa`), `us_ShutManuCont` (`…6fc`), **`us_AgcManuCont` (`…6fe`)**, `us_AgcRestoreCont`, `uc_AgcMonFixOn`, `uc_AgcCapFixOn`, `us_HardwareAgcCont`, `ss_AgcContOfs`, `ss_AgcStdgOfs`, `us_NdManuCont`, `uc_IrisHoldOn` y `b_IrisManual`/`b_ShutManual`. Una segunda tabla en `0x4a2a00` asocia cada dirección con un número (`uc_AgcManuOn` → `0x00010b19`, tamaño en los 16 bits altos). La tabla la reconstruyó el intento anterior (`vartables.pkl`) y hoy la volví a leer — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [local] Envolví `av-cam` como ELF (`llvm-objcopy -I binary -O elf32-littlearm`, `.data` en `0xc3431000`) y lo desensamblé como Thumb. Busqué escrituras a esas variables siguiendo cargas de literales y no encontré ninguna: sólo hay lecturas en funciones cortas que empaquetan 8 bytes. La de `0xc3733e34` copia `uc_IrisManuOn` (+0x11), `uc_AgcManuOn` (+0x15) y `uc_ShutManuOn` (+0x13) desde la base `0xc3a4f6d8`, y la de `0xc3733e08` copia `us_AgcManuCont` — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [local] El diccionario de ObjCamera no tiene ninguna orden de ganancia. La familia vecina de `SET_AGC_LIMIT` es `0x75b4` `SET_IRIS_DRIVE_MODE`, `0x75b6` `SET_SHUT_DRIVE_MODE`, `0x75b8` `SET_AUTOLOCK_MODE`, `0x75ba` `SET_AGC_LIMIT`, `0x75bc` `SET_ND_MODE`, `0x75be` `SET_AEL` y `0x75c0` `SET_AEL_MODE`. `firmware-analysis.md` llama «sin nombre» a `0x75be`, pero `MSGID.tsv` le da nombre: `MSGID_SET_AEL_CMD`. Los síncronos son `SYNC_IRIS_DRIVE` `0x6502`, `SYNC_SHUT_DRIVE` `0x6504`, `SYNC_MANUAL_EXPOSURE_DRIVE` `0x6506`, `SYNC_P_SHIFT_DRIVE` `0x6508`, `SYNC_M_SHIFT_DRIVE` `0x6514` y `SYNC_FOCUS_DRIVE` `0x650a`, sin ningún `GAIN_DRIVE` — [MSGID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/MSGID.tsv).
- [local] Parámetros de ganancia: `PARAMID_AGC_LIMIT` `0x96dc`, `AGC_LIMIT_MODE` `0x96dd`, `AGC_LIMIT_POS` `0x9861`, `AGC_LIMIT_MAX` `0x9862`, `AGC_LIMIT_MIN` `0x9863`, `ISO_GAIN_STATUS` `0x9706` y `ISO_GAIN_STATUS_VALID` `0x9720`. Eventos: `NOTIFY_AGC_LIMIT_EVT` `0x76bb`, `NOTIFY_AGC_LIMIT_STATUS_EVT` `0x7704` y `NOTIFY_ISO_GAIN_STATUS_EVT` `0x76c7` — [PARAMID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/PARAMID.tsv), [MSGID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/MSGID.tsv).
- [local] En la ruta de la interfaz, `pvt_SetParamAgcLimit` sólo acepta valores `signed char` múltiplos de 3. Con cualquier otro valor registra `AGCNG1` o `AGCNG2` y no manda nada. Agrega `AGC_LIMIT` como tipo 3 y `AGC_LIMIT_MODE` como tipo 8. La orden que inyecta `cxg.so` no pasa por esta comprobación — [agcparams.c](file:///home/aristotle/temp/cx580-pro/firmware/ghidra/out/agcparams.c), [agc.c](file:///home/aristotle/temp/cx580-pro/firmware/ghidra/out/agc.c).
- [local] Teclas LANC en `libObj.so`: `KEY_LANC_GAIN_MODE`, `KEY_LANC_GAIN_SELECT`, `KEY_LANC_IRIS_MODE`, `KEY_LANC_IRIS_CHANGE`, `KEY_LANC_SHUTTER_MODE`, `KEY_LANC_ONEPUSH_IRIS`, `KEY_LANC_WB_MODE`, `KEY_LANC_WB_PRESET`, `KEY_LANC_ONEPUSH_WB`, `KEY_LANC_FOCUS_MODE/NEAR/FAR`, `KEY_LANC_EXPFOCUS` y `KEY_LANC_PICTURE_PROFILE`, además de `INFO_DATA_CAMERA_GAIN_BKUP` — [libObj.so](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libObj.so).
- [local] La familia `IRIS_SHUTTER_GAIN_MANUAL` es interna del agente de modos. `MID_IRIS_SHUTTER_GAIN_MANUAL_CMD` (`0xa034`) y `MID_IRIS_SHUTTER_GAIN_ISO_MANUAL` (`0xa050`) están junto a `MID_KILL_THREAD_EXC_COMP` y `MID_EXC_SET_STROBE_MODE_CMD`. `VALUEID_EXC_MOVIE_MANUAL_MODE` (`0x102b`) es un elemento de la enumeración de modos de ejecución: `EXC_STILL_*` desde `0x1000` y `EXC_MOVIE_PROGRAM/IRIS/SHUTTER/MANUAL` en `0x1028`–`0x102b`. O sea, es el modo M de película, el equivalente de `EXP_MODE_MANUAL` (`0xf5`) — [libObj-names.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/libObj-names.tsv).
- [fuente] En las Alpha con Android, el factor del modo M se llama igual pero con ISO: `CAUTION_ID_FACTOR_CAM_SET_IRIS_SHUTTER_GAIN_ISO_MANUAL_ON_MSG = 3083` y sus variantes `…_PUSH_MR_1/2/3_ON_MSG = 4166–4168` — [Caution.j](https://github.com/ma1co/OpenMemories-Framework/blob/master/stubs/src/main/jasmin/com/sony/scalar/sysutil/didep/Caution.j).
- [local] Nombres del lazo de AE en `av-cam` (tabla de `0x49aa40` y cadenas cercanas): `us_AELevel`, `us_AELevelLpf`, `ss_AeErrorInteg`, `us_TargetLevel`, `us_FollowLevel`, `us_PreFollowLevel`, `ss_TargetError`, `ss_ModulateValue`, `us_ContLevel`, `uc_AeStable`, `uc_OpmAeStability`/`Cnt`, `us_AgcCont`, `us_AgcContMax`, `AgcContStdBase`, `agc_cont_blend[%x]` (`0x392aa0`), `uc_IrisHuntLimOver`, `uc_RapidNDState`, `us_NdHoldCont` y `uc_NdMonFixOn` — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [fuente] Los análisis de la A6000 no describen el paso del AE. g62 trata la «sensibilidad de seguimiento del AE» y propone imponer la compensación con `MSGID_SET_EXPOSURE_BIAS_CMD`. Marca como bloqueante leer el EV medido, porque lo que `libObj` recibe de av-cam es la velocidad, el diafragma y demás, no el EV — [g62-seguimiento-ae-intervalo.md](https://github.com/nicolvus/a6000-fw-decomp/blob/main/docs/gui/g62-seguimiento-ae-intervalo.md). s58 trata la temporización del sensor, no el lazo de AE — [s58-ae-timing-producers.md](https://github.com/nicolvus/a6000-fw-decomp/blob/main/docs/capture/s58-ae-timing-producers.md).

### Inferences
- [inferido] Lo más probable es que `SET_IRIS_DRIVE_MODE = IRIS_MANUAL` y `SET_SHUT_DRIVE_MODE = SHUT_MANUAL`, que ya funcionan en la cámara, terminen encendiendo `uc_IrisManuOn` y `uc_ShutManuOn`. El tercer miembro, `uc_AgcManuOn` con `us_AgcManuCont`, sería la ganancia manual al estilo de la NX70 que esta familia de firmware heredó. Ninguna orden de ObjCamera con nombre la enciende de forma evidente. Las candidatas, de más a menos probable: (a) el modo M (`SET_EXPOSURE_MODE = EXP_MODE_MANUAL`), que en la v7 hacía llegar los pasos aunque el modelo no lo conociera, combinado con un paso de ganancia (`SYNC_M_SHIFT_DRIVE` `0x6514` o `SYNC_MANUAL_EXPOSURE_DRIVE` `0x6506`); (b) el camino de las teclas LANC `GAIN_MODE`/`GAIN_SELECT`; (c) una de las 169 órdenes `0x40xx` de av-cam (sección 2) que `libObj` no emite.
- [inferido] Los empaquetadores de 8 bytes que leen juntos `IrisManuOn`, `AgcManuOn` y `ShutManuOn` tienen la forma de los datos de cámara que se graban con el clip. Si es así, `tools/cxexposure.py` podría leer en cada clip si av-cam considera manual cada uno de los tres. Falta ubicar esos bytes en el `ARI_DATA` o el MDPM.
- [inferido] Sobre los saltos de 1/3 a 1/6 de paso. Con iris y obturación manuales, el único actuador que le queda libre al AE es el AGC, y lo maneja con un código continuo (`us_AgcCont`, `agc_cont_blend`), filtrado (`us_AELevelLpf`) e integrado (`ss_AeErrorInteg`), no por escalones de 3 dB. Si la escena queda un poco por encima del objetivo con el tope puesto, el AE baja la ganancia en fracciones, y eso se vería como saltos chicos. La segunda hipótesis es un ND o el iris de rescate (`us_NdHoldCont`, `uc_RapidNDState`, `uc_IrisHuntLimOver`), si la unidad tiene ND.
- Experimentos concretos:
  1. Grabar con iris y obturación fijos, tope de 9 dB y una escena estable, y comparar con `cxexposure.py` la ganancia por cuadro contra los saltos de brillo. Si `NOTIFY_ISO_GAIN_STATUS` (`0x9706`, pasos de 3 dB) no cambia en un salto, el salto no viene del escalón de ganancia sino del código fino o de otro actuador.
  2. Inyectar `SET_EXPOSURE_MODE = EXP_MODE_MANUAL` (`0xf5`) después de fijar iris y obturación, dar pasos `SYNC_M_SHIFT_DRIVE` (`0x6514`, mismo formato que `SYNC_IRIS_DRIVE`) y registrar con el gancho de `GetParam` si cambian `ISO_GAIN_STATUS` o `ISO_SPEED_RATE_ACT`.
  3. Probar `SET_AGC_LIMIT` con `AGC_LIMIT_MIN` (`0x9863`) igual a `AGC_LIMIT` y ver si el AE deja de bajar la ganancia del tope (ganancia fija por mínimo igual a máximo, como `ISO_SPEED_RATE_MIN/MAX`). También vale `AGC_LIMIT` con valores que no sean múltiplos de 3, que la interfaz nunca manda.
  4. Estático, antes de tocar la cámara: en Ghidra, partir de los manejadores de la tabla `0x45e920` de av-cam y buscar cuál escribe en la estructura que empieza cerca de `0xc3a4f678` con desplazamiento `0x75` (`uc_AgcManuOn`).

### Gaps
- No encontré ninguna fuente pública con el tamaño de paso, la histéresis o el filtro del AE de una Sony, ni mediciones de saltos con iris y obturación fijos.
- No localicé quién escribe `uc_AgcManuOn`. El acceso es por puntero a estructura, así que hace falta análisis de flujo en Ghidra, que este encargo no permitía correr.
- No hay fuente que relacione los `MSGID_SET_*` de ObjCamera con las órdenes `0x40xx` de av-cam en la CXD4132.
- No sé si la CX580V tiene un ND físico. Las variables `Nd*` pueden ser de otros modelos que comparten este firmware.

---

## 2. av-cam.bin: cargadores, recuperación de nombres, ADF y SDF

### Takeaway
No hay un cargador público de LIRO para Ghidra o IDA. Lo reutilizable es el analizador mínimo de nicolvus (`src/liro.py`: cabecera y tablas de despacho `{orden, manejador, bandera}`) y el mapa de cabecera de knowXD para la NEX-7. En la CX580V basta envolver el blob como ELF en `0xc3431000` para desensamblarlo como Thumb. Adentro hay tablas de nombres de variables y una tabla de despacho de 169 órdenes `0x4000`–`0x40bd`, de la misma familia que las órdenes `0x40xx` que la A6000 manda a `CamMgr`. SDF es «Still DataFlow» (nicolvus). En la CX580V, ADF es el flujo de audio; que la sigla signifique «Audio DataFlow» es inferencia.

### Cited Findings
- [fuente] `src/liro.py` de nicolvus: `parse_header` valida la magia `0x4C49524F`, la versión 4 y nueve punteros (`entry_pre`, `entry_main`, `descriptor`, `suspend`, `resume`…). `parse_dispatch` lee entradas de 12 bytes `{command_id, handler_address (bit 0 = Thumb), flag}`, y las pruebas usan, por ejemplo, la orden `0x40E3` — [src/liro.py](https://github.com/nicolvus/a6000-fw-decomp/blob/main/src/liro.py), [tests/test_liro.py](https://github.com/nicolvus/a6000-fw-decomp/blob/main/tests/test_liro.py).
- [fuente] Cabecera de la NEX-7 según knowXD: `0x00` `add pc, pc, #0x28`, «ORIL» (LIRO en little endian), versión 4 y nueve punteros (`0xC3421050`, `0xC3421078`…). La entrada es ARM y muchos módulos son Thumb. Desplazamientos conocidos: `0x2B7A10` tabla de constantes de bitrate MP4, `0x461092` tabla JPEG y `0x31E390` estilos creativos — [AVCAM_STRUCT.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/AVCAM_STRUCT.md).
- [local] La CX580V tiene la misma cabecera, con base `0xc3431000` (`e28ff028`, `4c49524f`, `4`, `c3431050`…), según la lectura del intento anterior. El ELF envuelto da 2.55 millones de líneas de Thumb coherente con `llvm-objdump --triple=thumbv7` — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [local] Tablas de despacho que encontré con un barrido de entradas `{u32 ≤ 0xffff, puntero Thumb al código, bandera 0/1}`:
  - `0x45e920`: 169 entradas, órdenes `0x0000` y `0x4000`–`0x40bd`, con manejadores en `0xc345afcd`–`0xc345c499`. Son trampolines de unos 10 bytes, así que no traen cadenas propias. Las de `0x4019`–`0x401b`, `0x4026`, `0x4046`, `0x405b`–`0x406d` caen junto a la cadena `Pseudo Rec [SINGLE] ON`.
  - `0x46a484`: 31 entradas (`0x4042`–`0x404c`, `0x4081`, `0x4092`, `0xd000`–`0xd015`), con manejadores que registran `RelMgr) …` y `CapIdTblMgr) Initialize …`.
  - [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin)
- [fuente] En la A6000, `CamMgr 0x30001` recibe las órdenes `0x40f5`, `0x4074`, `0x405c`, `0x408d`, `0x4036`, `0x3024`, `0x302a` y `0x401b`, la misma familia `0x40xx` — [g22-avcam-puente.md](https://github.com/nicolvus/a6000-fw-decomp/blob/main/docs/gui/g22-avcam-puente.md).
- [local] Tablas de nombres de variables en `av-cam`: la de AE en `0x49aa40` (sección 1), otras de 16 bytes en `0x46e274` (`IrisDrvMode`…`ZoomFocusEMG`), `0x46e344` (`uc_IrisInitComp`…), `0x46e494`, `0x46e704`, `0x46e9f4` y `0x4a5fa4`, y nombres de la parte YC/sensor (`us_AgcCont`, `us_gainMax`, `uc_BB_*`, `240fpsFlag`) en `0x52dddc`–`0x52e23e`. El intento anterior las había listado y hoy confirmé una parte — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [local] Prefijos de registro más frecuentes, contados por el intento anterior: `[ADF]` 921, `[SDF]` 621, `[CDIAG]` 243, `[IDT]` 171, `[VEF]` 149, `[ENCVIDSEQ]` 136, `[CodecSeq]` 122 y `[VDF]` 100. `[CDIAG]` es diagnóstico de códec (`SENS_SET_DATA_AREA`, `RunCodec HD REC`…), no un monitor de variables — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [fuente] «S103 — subsistema SDF (Still DataFlow)». En la A6000 la codificación JPEG va como `MSG_ID_SDF_ENCODE_JPEG` «al SDF (el mismo motor de LIRO)» — [s103-sdf.md](https://github.com/nicolvus/a6000-fw-decomp/blob/main/docs/capture/s103-sdf.md), [encodestill-yc-contract.md](https://github.com/nicolvus/a6000-fw-decomp/blob/main/docs/capture/encodestill-yc-contract.md).
- [local] En la CX580V, `[ADF]` es audio: `[ADF][INIT]AUDIOIP INIT COMP`, `[ADF]AUDIO_IP_CONFIG_TYPE_VGS`, `AdfCmdInputCtrl`, `AdfSenserCtrl` y `AdfInputProMsg*`. En la A6000, el bloque de audio también arranca desde `AdfSenserCtrl` — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin), [s69-audio-subsystem.md](https://github.com/nicolvus/a6000-fw-decomp/blob/main/docs/capture/s69-audio-subsystem.md).
- [local] `av-cam` dice en una cadena que es de la plataforma «OPAL»: `%s: Minna-no-SA Unit can't be used on OPAL` (`0x2148c0`). OpenMemories llama «Opal» a la CXD4132 — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).

### Inferences
- [inferido] La tabla `0x45e920` es el `CamMgr` de la CXD4132. Para darle nombre a cada orden habría que (1) seguir cada trampolín hasta su función real y ver qué variables del AE toca, y (2) en `libObj.so`, buscar los inmediatos `0x40xx` dentro de los conversores `Set…ForLiro` de `CamMsgConv`. Así se sabría qué orden de av-cam corresponde a `SET_AGC_LIMIT` y si alguna vecina enciende `uc_AgcManuOn`.
- [inferido] Por analogía con SDF, VDF (video o pantalla) y EDF, ADF sería «Audio DataFlow». No lo vi escrito en ninguna fuente.

### Gaps
- No encontré scripts de Ghidra o IDA publicados que carguen LIRO con nombres. ivica00 tiene `tools/research/liro_*.py` (instantáneas y trazas de tareas en el QEMU de la A6400) que no revisé a fondo.
- Nadie publicó la tabla de órdenes `0x40xx` con nombres para ninguna plataforma.

---

## 3. Balance de blancos y foco por órdenes directas (P3)

### Takeaway
El diccionario de ObjCamera de la CX580V tiene todo lo necesario: modo de WB con temperatura de color (Kelvin), WB Shift en dos ejes con interruptor, WB personalizado (One Push), modo de foco manual, movimiento de foco por dirección y velocidad, lectura de la posición, asistencia de MF y ampliación. Faltan los tipos y rangos de los valores, que no publicó nadie para esta generación y que se pueden leer con el gancho de `IssueCommand` y `GetParam` que ya existe.

### Cited Findings
- [local] Balance de blancos:
  - `SET_WB_MODE_CMD` `0x7546`, con `PARAMID_WB_MODE` `0x958d` y los valores `WB_AUTO` `0xc7`, `WB_DAYLIGHT` `0xc8`, `WB_CLOUDY` `0xc9`, `WB_SHADE` `0xca`, `WB_FLUORESCENT1..4` `0xcb`–`0xce`, `WB_INCANDESCENT` `0xcf`, **`WB_COLOR_TEMP` `0xd1`**, `WB_CUSTOM1..3` `0xc3`–`0xc5` y `WB_CUSTOM_NOW` `0xc6`.
  - Temperatura: `PARAMID_WB_COLOR_TEMP` `0x9589`, `WB_COLOR_FILTER` `0x958a`, `COLOR_TEMP` `0x9812`, `WB_ADJUST_LB/CC` `0x958e`/`0x958f`, `WB_DAYLIGHT_LEVEL` `0x9590`, `WB_TUNING` `0x980d` y `WB_TUNING_AXIS_0/1` `0x980f`/`0x9810`.
  - Shift: `SET_WB_SHIFT_CMD` `0x7548`, `WB_SHIFT_LB` `0x959b`, `WB_SHIFT_CC` `0x959c`, `WB_SHIFT_SW` `0x959d`, `WB_SHIFT_ON/OFF` `0xd4`/`0xd5`.
  - One Push: `CUSTOM_WB_CMD` `0x7542`, `CUSTOM_WB_SAVE_CMD` `0x7544`, `CUSTOM_WB_STBY_CMD` `0x7600` (`CUSTOM_WB_STBY_ON/OFF` `0x2b9`/`0x2ba`), `SYNC_REQ_CUSTOM_WB_DATA_CMD` `0x650c`, `CUSTOM_WB_CH` `0x958b` y `CUSTOM_WB_IN_RANGE` `0x9588`.
  - [MSGID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/MSGID.tsv), [PARAMID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/PARAMID.tsv), [VALUEID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/VALUEID.tsv)
- [local] Foco:
  - Modo: `SET_FOCUS_MODE_CMD` `0x7564`, con `FOCUS_MODE` `0x95df` y los valores `FOCUS_MANUAL` `0x137`, `FOCUS_MOVIE_AUTO` `0x138`, `FOCUS_AF_S/C/A` `0x139`–`0x13b`, `FOCUS_DMF` `0x13d`, `FOCUS_AUTO` `0x13e` y `FOCUS_INF` `0x13f`.
  - Movimiento: `SYNC_FOCUS_DRIVE_CMD` `0x650a` (`FOCUS_SPD` `0x95dd`, `FOCUS_DIR` `0x95de` con `FOCUS_DIR_NEAR/FAR` `0x135`/`0x136`) y `SYNC_FOCUS_CONT_DRIVE_CMD` `0x6516` (`FOCUS_CONT_DIR` `0x9727` con `STOP/NEAR/FAR` `0x219`–`0x21b`).
  - Posición: `FOCUS_POS` `0x95f2`, `FOCUS_POS_MAX` `0x95f3`, `FOCUS_POS_DATA` `0x9662`, `FOCUS_MOVABLE_DIR` `0x9663`, `MF_DISTANCE` `0x9553` y `MF_DIST_NEAR` `0x9554`.
  - Asistencia y ampliación: `SET_MF_ASSIST_MODE_CMD` `0x761e` (`MF_ASSIST_ON/OFF` `0x31b`/`0x31a`), `SET_CAM_EXPAND_FOCUS_MODE_CMD` `0x75d2` (`EXPAND_FOCUS_OFF/ON_MAG1/ON_MAG2` `0x1f3`–`0x1f5`) y `SET_CAM_EXPAND_FOCUS_TIME_CMD` `0x761a`.
  - Otros: `FOCUS_INFINITY_CMD` `0x7568`, `SYNC_AF_MF_CHANGE_ON/OFF_CMD` `0x6522`/`0x6524`, `SYNC_FOCUS_HOLD_CMD` `0x6520`, `SET_TELEMACRO_CMD` `0x7534`, `TOUCH_SPOT_FOCUS_CMD` `0x7524` y `SET_AF_DRIVE_SPEED_CMD` `0x756c`.
  - [MSGID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/MSGID.tsv), [PARAMID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/PARAMID.tsv), [VALUEID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/VALUEID.tsv)
- [local] El peaking no tiene orden en ObjCamera, aunque existe `view/PEAKING` en `libObj.so` y `viewUnified7.so`, así que vive en otra capa (probablemente la de pantalla). `view/WB_SHIFT` y `viewWbShift.uxc` ya existen — [libObj.so](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libObj.so).
- [local] También están en el diccionario los parámetros de «Picture Profile», cada uno con su propio WB Shift: `PP_WB_SHIFT_LB/CC` `0x9675`/`0x9676`, `PP_WB_SHIFT_RGAIN/BGAIN` `0x9677`/`0x9678` y `PP_WB_SHIFT_FILTER_TYPE` `0x9679`, con los valores `PP_WB_SHIFT_LB_CC` `0x1b2` y `PP_WB_SHIFT_RGAIN_BGAIN` `0x1b3` (sección 7) — [PARAMID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/PARAMID.tsv).
- [fuente] En las Sony con Android, la API privada separa `getWhiteBalanceShiftLB/CC` y `getColorTemperatureForWhiteBalance` (ya citado en el informe anterior) — [CameraEx$ParametersModifier.j](https://github.com/ma1co/OpenMemories-Framework/blob/master/stubs/src/main/jasmin/com/sony/scalar/hardware/CameraEx%24ParametersModifier.j).

### Inferences
- [inferido] Que existan `WB_COLOR_TEMP` (`0xd1`) y `PARAMID_WB_COLOR_TEMP` dice que el procesador acepta Kelvin. Es la misma situación que el iris: el firmware lo trae y el menú de la CX580V no lo ofrece. El valor sería un entero en Kelvin, como `setColorTemperature` en Android, pero eso no está comprobado.
- Experimentos concretos:
  1. Registrar con el gancho de `IssueCommandAsync/Sync` (ya está en `cxg.so`) lo que manda la pantalla White Balance de fábrica (Indoor, One Push) para ver tipos y orden de parámetros.
  2. Inyectar `SET_WB_MODE` con `WB_MODE = WB_COLOR_TEMP` y `WB_COLOR_TEMP = 3200` y después `5600`, y comprobar el cambio en un clip con `cxexposure.py`.
  3. Inyectar `SET_WB_SHIFT` con `WB_SHIFT_SW = WB_SHIFT_ON` y `WB_SHIFT_LB = ±n`, y comparar con la pantalla de fábrica `view/WB_SHIFT` que ya abre con Tele Macro en v9.
  4. Para el foco: `SET_FOCUS_MODE = FOCUS_MANUAL` y después `SYNC_FOCUS_DRIVE` con `FOCUS_DIR` y `FOCUS_SPD` en `IssueCommandSync`, como `SYNC_IRIS_DRIVE`. Leer `FOCUS_POS`/`FOCUS_POS_MAX` en las notificaciones con el gancho de `GetParam` para tener posición absoluta.
  5. Antes de mandar nada, pasar cada MSGID por la emulación `EmuInhCheck` para saber si el factor `0x1f7` lo bloquea, como ya se hizo con el ISO.

### Gaps
- No hay fuente pública con la codificación (tipo, unidades, rango) de `WB_COLOR_TEMP`, `WB_SHIFT_LB/CC`, `FOCUS_SPD` ni `FOCUS_POS` en ninguna Sony con Linux.
- No sé dónde está la orden de peaking; no la busqué fuera de ObjCamera.

---

## 4. Formatos: 25p/PAL, 24p/30p en STD, bitrate, límite de grabación (P4)

### Takeaway
Para PAL y 25p hay una vía concreta y reversible. La propiedad de Backup `0x010704fd` existe en la CX580V (vale `01`, NTSC), y un usuario la usó en una HDR-PJ760V con el mismo firmware 1.07 para que la cámara grabara 50i, 50p y 25p. Las propiedades de cadencia, bitrate y códec que se conocen de la NEX-5N también existen en la CX580V, con los mismos atributos y valores que coinciden con los ajustes de fábrica. En la CX580V no hay límite de 29:59 en Backup. STD a 24p o 30p no tiene evidencia: la capa de cámara conoce `FR_30_25P` y `FR_24P`, pero el codificador MPEG-2 es un programa del DSP SA aparte y no apareció nada de MPEG-2 progresivo. Nadie logró un bitrate por encima del de fábrica; los cuatro parches de constantes de knowXD no tuvieron efecto.

### Cited Findings
- [fuente] Issue #714: LAK132 propone `0x010704fd` como valor de PAL/NTSC aunque falte el selector. thijmau (2026-06-22): «I can confirm this works on a Sony HDR-PJ760V with firmware `1.07` … the modification worked flawlessly, and the camera now supports 50i, 50p, and 25p recording modes.» El procedimiento citado es `bk r 0x010704fd` (00 = PAL, 01 = NTSC), `bk w 0x010704fd XX`, `bk s` y sacar la batería. Nadie publicó archivos medidos — [Sony-PMCA-RE #714](https://github.com/ma1co/Sony-PMCA-RE/issues/714).
- [fuente] El fork de LAK132 define `palNtsc` `0x010704fd` (1 B), `palNtscSelector` `0x01070148`, `palNtscSelectorXR` `0x02cf0004`, `recLimit` `0x003c0373`+3 y `recLimit4k` `0x003c04b6`. Su `dump-txt` sólo nombra esas 16 propiedades: no es una lista grande de nombres — [backup.py del fork](https://github.com/LAK132/Sony-PMCA-RE/blob/main/pmca/platform/backup.py). En las BIONZ XR, `0x02cf0004` sólo muestra u oculta el ítem del menú, y `0x01070148` no existe en ellas — [Sony-PMCA-RE #756](https://github.com/ma1co/Sony-PMCA-RE/issues/756).
- [local] Backup.bin de la CX580V, leído sin escribir (BK2, 353 subsistemas, 12 389 propiedades, versión `12101_12.2012072401`, región `CX38600_U2`, suma de control correcta):

  | ID | Valor | Reinicio | attr | Qué es en otras cámaras |
  |---|---|---|---|---|
  | `0x010704fd` | `01` | `01` | `0x70` | PAL/NTSC (00 PAL, 01 NTSC) |
  | `0x01070148` | `00` | — | `0x01`, sólo lectura | selector PAL/NTSC del menú |
  | `0x0107009e` | `01` | `01` | `0x60` | cadencia, NEX-5N: 00 = 60p, 01 = 60i, 02 = 25p, 03 = 24p |
  | `0x010700a1` | `02` | `02` | `0x60` | bitrate AVCHD, NEX-5N/HX60: 4 = 17M, 5 = 24M, 6 = 28M |
  | `0x010700a2` | `02` | `02` | `0x60` | bitrate MP4: 1 = 3/6M, 2 = 12M |
  | `0x01070087` | `03` | `03` | `0x68` | códec: 1 DVD?, 2 M2PS?, 3 AVCHD, 4 MP4, 5 Still, a XAVC S |
  | `0x003c0373..75` | `0d 01 00` | — | `0x00` | límite de grabación, 13 h 01 min 00 s |
  | `0x02cf0004`, `0x01070a47`, `0x01070c9a`, `0x01070cd9`, `0x003c04b6` | no existen | | | selector XR, HDMI limpio A58, cadencias AX100, límite 4K |

  Según el parser de Sony-PMCA-RE, en `attr` el bit `0x01` es sólo lectura, `0x08` dispara callbacks al escribir y `0x74` indica que hay valor de reinicio — [Backup.bin](file:///home/aristotle/temp/cx580-pro/backups/20261126-dump/Backup.bin), [pmca/backup/__init__.py](file:///home/aristotle/temp/cx580-pro/vendor/Sony-PMCA-RE/pmca/backup/__init__.py).
- [fuente] NEX-5N: `0x0107009e` cambia la cadencia (00 60p, 01 60i, 02 25p, 03 24p) y `0x010700a1` el bitrate, pero «this only change menu options, can't raise bitrate for more than original options. We need an custom firmware» — [Sony-PMCA-RE #440](https://github.com/ma1co/Sony-PMCA-RE/issues/440). Tabla de mungewell con los mismos `attr` (`0x60`, `0x60`, `0x68`) y la nota de que su AS100 no le dejó encontrar el control de bitrate — [OpenMemories-Tweak #411](https://github.com/ma1co/OpenMemories-Tweak/issues/411).
- [fuente] El límite de 13 h 01 min ya es el de fábrica en las videocámaras: «all camcorders have this value set to 13h01m00s», dice el comentario del tweak — [tweaks.cpp](https://github.com/ma1co/OpenMemories-Platform/blob/master/api/tweaks.cpp). Copia local: [vendor/OpenMemories-Platform/api/tweaks.cpp](file:///home/aristotle/temp/cx580-pro/vendor/OpenMemories-Platform/api/tweaks.cpp).
- [ayudante] Cuerpos PAL con el tweak quitado muestran los modos NTSC pero graban 50p, y una SLT-A77 se congeló al grabar 24p o 60i ([#490](https://github.com/ma1co/Sony-PMCA-RE/issues/490), [#643](https://github.com/ma1co/Sony-PMCA-RE/issues/643), [#412](https://github.com/ma1co/Sony-PMCA-RE/issues/412)). En la FDR-AX100E el cambio a NTSC funcionó; forzar 4K50p colgó la cámara ([#138](https://github.com/ma1co/Sony-PMCA-RE/issues/138)). No los reabrí.
- [local] Capa de cámara: `SET_MOVIE_FORMAT_CMD` `0x750e` (`NOTIFY_MOVIE_FORMAT_EVT` `0x7645`) con `PARAMID_MOVIE_SIZE` `0x9507`, `FRAME_RATE` `0x9508` y `MOVIE_ASPECT_RATIO` `0x9509`. Valores de tamaño: `HD1080` `0x0e`, `HD720` `0x0f`, `SD` `0x10`, `VGA` `0x11`. De cadencia: `FR_60_50P` `0x12`, `FR_60_50I` `0x13`, `FR_30_25P` `0x14`, `FR_24P` `0x15`. De aspecto: `MOVIE_ASP_16_9/4_3` `0x16`/`0x17` — [VALUEID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/VALUEID.tsv), [MSGID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/MSGID.tsv).
- [local] En `av-cam`, el AE tiene `uc_PAL`, `uc_24pOn`, `uc_30pOn` y `uc_25pOn`. El codificador AVC («mojito») imprime `frame_rate`, `system_frame_rate`, `avchd_30p_type`, `bit_rate`, `hrd_bit_rate_scale` y `hrd_bit_rate_value_minus1` (`0x696b4`–`0x6a13c`, `0x2d1170`–`0x2d1740`), y valida con `INVALID VUI NUM UNITS IN TICK. MUST BE 2002 for 60P` / `…2400 for 50P`, `The input picture size of 1920x1080/24/25/30P must be 1440x1080` y `PSEUDO PROGRESSIVE IS NOT SUPPORTED IN THIS VERSION`. Otras cadenas: `[MOVENCCMD][%d]vbr -> cbr`, `cbrRecMode %d` y la ayuda de depuración `arv2 minorMode HdVideoRecMode: 1 ENC_AVC_1080I, 2 ENC_AVC_1080P_24, 3 ENC_AVC_720P, 4 ENC_AVC_720P_24, 5 ENC_AVC_480I, Progressive(TEST_MODE)` (`0x29795c`). Del MPEG-2 hay `[MPGPINSUPL]Change FrameRate %x->%x, VfcSub = %x`, `SENS_SET_MP2E_PARAM`, `mpeg2SaMgrIf.cpp` y `[MPG_VCPU]Picture_Mode,frame_coding_type,…` — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [local] Los codificadores son programas del DSP SA en `nflasha8/sabin/`: `sa_avce_performance_zma.bin`, `sa_avce_quality_zma.bin`, `sa_mpeg2e_minna.bin`, `sa_mpeg2e_wlpf_zma.bin`, `sa_ac3enc_ac3dec.bin`, `sa_dd51enc.bin`, `sa_bd_lpcmenc.bin`, `sa_zima_startup_*.bin` y otros — [sabin/](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/sabin/).
- [local] La biblioteca de medios conoce calidades MPEG-2 que la CX580V no ofrece. `libmpr.so` tiene `AVI REG <Number> <M2PS/AVC> 101PNV01/M2U<FILENO> <NTSC/PAL> <XP/HQ/SP/LP> <16:9/4:3> <5.1/2> …` y contadores `movieRecInh1 (LP)`, `(SP)`, `(HQ)`. `libObj.so` tiene `CNTMGR GetMovieRemain [ContentType(0:DVD/1:M2PS/2:AVCHD/3:MP4)] [quality(1-10)] [audio(0:AC3_2/1:AC3_51/2:LPCM/3:AAC_1/4:AAC_2)][pictsize(0:1080/1:720/2:VGA)]`. `libmprctrl.so` lee `BKID_MPR_SYS_TVSYSTEM_TYPE`, `BKID_MPR_SYS_M2PS_REC_SUPPORT`, `…_AVCHD_REC_SUPPORT`, `…_MP4_REC_SUPPORT`, `…_TC_UB_SUPPORT` y `…_DUAL_REC_OVER_18MBPS_MOVIE`, y `libObj.so` lee `BKID_APP_MENU_NTSC_PAL` — [lib/](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/).
- [local] Grupos de constantes de bitrate: `libmpr.so` `0x58a6d4` = 28/24/18 Mbps; `av-cam` `0x2a52b0` = 16/9/18 y `0x2a56dc` = 16/6/18/9 Mbps — [libmpr.so](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libmpr.so), [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [fuente] knowXD: «video bitrate (4 patch sets, zero effect)». Su conclusión, en chino: «四组常量补丁均无效；控制点在编码器的速率控制逻辑，不在常量表», es decir, el control está en la lógica de control de tasa del codificador y no en la tabla de constantes. Los parches: `av-cam` `0x2B7A18` → `0x02BDE780` (46 000 000), `0x2B7A1C` («12M») al mismo valor y un trío en `nflasha8` `0x2548590` → (46M, 46M, 30M) — [README](https://github.com/knowXD/nex7-firmware-research/blob/main/README.md), [patch_bitrate.py](https://github.com/knowXD/nex7-firmware-research/blob/main/tools/patch_bitrate.py), [patch_bitrate2.py](https://github.com/knowXD/nex7-firmware-research/blob/main/tools/patch_bitrate2.py), [patch_n8.py](https://github.com/knowXD/nex7-firmware-research/blob/main/tools/patch_n8.py).
- [fuente] Guía de la CX760V/PJ760V: «You can select the frame rate to be used to record high definition image quality (HD) movies» (copia guardada por el intento anterior, página «Frame Rate») — [copia local](file://vendor/barrido-fuentes/hg/04_02_02.html); guía de la CX580V citada en `ui-spec.md`: [Frame Rate](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/04/02/02.html).

### Inferences
- [inferido] Los valores actuales y de reinicio de `0x0107009e` (`01`), `0x010700a1` (`02`) y `0x01070087` (`03`) coinciden con los de fábrica de la CX580V: HD, 60i, REC Mode Standard (HQ). Con los mismos `attr` que en la NEX-5N y la HX60, lo más probable es que el bloque `0x0107` siga el mismo esquema en toda la generación: `0x01070087` = códec (3 AVCHD, 2 M2PS = STD), `0x0107009e` = cadencia, `0x010700a1` = calidad AVCHD (en la CX580V `02` sería HQ y, por orden, 4/5/6 = FH/FX/PS).
- [inferido] Para 25p/PAL el camino más corto es escribir `0x010704fd` = `00` (no es sólo lectura ni dispara callbacks; tiene valor de reinicio). Es una propiedad de Backup reversible, el segundo lugar en el orden de preferencia de `AGENTS.md`. Hay que guardar antes el valor `01` y la copia verificada de `/setting`. Por la experiencia en Alpha, esperar que cambien también la salida de TV/HDMI y la lista de modos, y medir con `ffprobe` la cadencia real de los clips (60i frente a 50i/25p).
- [inferido] STD 30p/24p: la capa de cámara ya tiene los valores (`FR_30_25P`, `FR_24P`) y el AE sus banderas (`uc_30pOn`), pero el MPEG-2 va por `sa_mpeg2e_*` y la única marca de progresivo es de prueba y para AVC. Es probable que el codificador MPEG-2 sólo arme campos. Experimento de bajo riesgo: con STD elegido, registrar qué `SET_MOVIE_FORMAT` manda la interfaz (`MOVIE_SIZE = SD`, `FRAME_RATE = FR_60_50I`) y probar a mandar `FR_24P` sólo con la cámara en espera, sin grabar. El riesgo de cuelgue está documentado en la A77 (dato del ayudante).
- [inferido] Bitrate: `0x010700a1` sólo elige entre modos de fábrica. Las constantes no gobiernan el control de tasa según knowXD, y el control vive en `av-cam` (mojito/ZIMA) más el programa SA. La calidad XP de M2PS que conoce `libmpr` sugiere una ruta de STD de mayor tasa heredada de las handycam DVD/HDD; falta ver si `sa_mpeg2e` la acepta.
- [inferido] El límite de grabación no es problema en la CX580V: Backup ya trae 13:01:00.

### Gaps
- No encontré mediciones publicadas de clips de la PJ760V después del cambio a PAL (ni `ffprobe` ni HDMI).
- No hay ningún resultado público de STD progresivo ni de bitrate mayor en ninguna Sony de esta generación.
- No traduje los nombres `BKID_*` de `libmprctrl.so`/`libObj.so` a números: haría falta seguir en el desensamblado las llamadas a `Backup_read` junto a cada cadena.

---

## 5. Nivel de audio manual (P5)

### Takeaway
El firmware de la CX580V ya trae la cadena completa de nivel de grabación manual, aunque su menú sólo ofrece Micref Level Normal/Low. `libObj.so` tiene `ObjAudioProcessor` con `REC_LEVEL_MODE` y `REC_LEVEL_VALUE` (fijar modo y valor) y acciones de modelo `ActionSetRecLevel`/`ActionSetRecLevelMode`, y `av-cam` tiene `Input SetRecLevel` y `ADJ_CMD_AVBB_ADF_SET_REC_LEVEL(_MODE)`. Sony puso «Audio Rec Level» Auto/Manual en el menú de las handycam de 2013. Faltan la categoría y los MSGID de `ObjAudioProcessor`, que no están en los diccionarios del proyecto, y el rango de valores.

### Cited Findings
- [local] `libObj.so`, componente `[ObjAudioProcessor]`: `ExecCommandSetRecLevelMode`, `SequenceAppSetRecLevelModeCmd`, `SequenceAppSetRecLevelValueCmd`, `SequenceAppGetRecLevelValueCmd`, `PARAMID_REC_LEVEL_MODE`, `PARAMID_REC_LEVEL_VALUE`, `ADF error: SET_REC_LEVEL [%x]`, `OsalSndMsg => ADF Set RecLevelMode : ch 0x[%x] mode 0x[%x] rcvMsgID[%x]`, `MSGID_SET_REC_LEVEL_MODE_CMD_INTERNAL(Ch1/Ch2)`, `MSGID_SET_AGC_LINK_CMD_INTERNAL`, `MSGID_SET_INPUT_TRIM_CMD_INTERNAL(Input1/2)` y `MSGID_INPUT_PRO_SET_REC_LEVEL_LIMITTER_COMP`. En el modelo: `ActionSetRecLevel`, `ActionSetRecLevelMode`, `ActionSetMicLevel`, `prv_SetRecLevel`, `prv_SetRecLevelMode` y `prv_SetMicRefLevel`. Más: `PARAMID_AUDIO_LEVEL`, `AUDIO_LEVEL_LIMIT`, `AUDIO_LEVEL_DETECT`, `LOW_CUT_MODE`, `ZOOM_MIC_LINK_MODE`, `MSGID_AUDIO_LEVEL_EVT`, `MSGID_SET_ZOOM_MIC_LINK_EVT` y `MSGID_SET_LOW_CUT_CMD` — [libObj.so](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libObj.so).
- [local] `av-cam`: `[ADF][DBGCMD]Input SetRecLevel` (`0x402b4`), `Input SetRefLevel`, `Input SetZoomLink`, `Input SetLowCut`, `InputPro SetRecLevelMode/SetRecLevelLimitter`, `[ADF]SetRecLevel` (`0xc8b48`), `[ADF]SetRefLevelNormalLow err %x` / `NormalHigh` / `SET_REF_LEVEL this cmd is N/A`, `AVIO SetRefLevel(Audio)`, `ADJ_CMD_AVBB_ADF_SET_REC_LEVEL`, `…_SET_REC_LEVEL_MODE`, `…_SET_MIC_MODE`, `…_ADJ_MIC_SENSE`, `agcIntMic : Limited ! / Unlimited` (`0x2c314c`), `AdfSenserCtrl::SetParamSetRecLevel`, `AbbMicSensAdjustCtrl` y `[ADF]SetWindNr` — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin).
- [local] No hay pantalla de nivel de audio: las vistas `view/*` de `libObj.so` y `viewUnified*.so` y los `.uxc` de `share/app` no tienen nada de AUDIO, MIC ni REC_LEVEL — [lib/](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/).
- [fuente] A6000: `AdfSenserCtrl::SetParamSetRecLevel`/`…RecMode`, `15AbbRecLevelCtrl` y el registro `[RecLevel]SetRecLevel [volume]` — [s69-audio-subsystem.md](https://github.com/nicolvus/a6000-fw-decomp/blob/main/docs/capture/s69-audio-subsystem.md).
- [fuente] Handycam de 2013 (HDR-CX400E, CX410VE, CX430V, PJ420E, PJ430V, PJ510E…): «Audio Rec Level», Auto («The audio recording level is adjusted automatically») o Manual con botones de subir y bajar, sin rango numérico. «The setting will be reset each time you switch between [Auto] and [Manual]», Manual apaga Closer Voice y Blt-in Zoom Mic, y «The limiter is always active regardless of the setting value of the recording level» — [guía CX430V y familia](https://helpguide.sony.net/gbmig/44528931/v1/gb/contents/09/03/27/27.html). Igual en la PJ650V/PJ660V — [guía PJ650V](https://helpguide.sony.net/gbmig/44501601/v1/gb/contents/09/03/34/34.html).
- [fuente] En la CX760V/PJ760V de 2012, Micref Level sólo tiene Normal («Records various ambient sounds, converting them into the appropriate level») y Low («Records ambient sound faithfully… not suitable for recording conversations»). Lo leí en la copia guardada de la guía — [copia local 03_35_35.html](file://vendor/barrido-fuentes/hg/03_35_35.html); página de la CX580V según el ayudante: [Micref Level](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/03/28/28.html).

### Inferences
- [inferido] Micref Normal/Low se implementa como `SetRefLevelNormalLow`/`NormalHigh` (nivel de referencia) y no como nivel de grabación. El nivel de grabación manual (`REC_LEVEL_MODE` = manual más `REC_LEVEL_VALUE`) es la función de 2013 ya presente en este firmware, quizá por la NX30 con XLR (`InputPro`). El limitador quedaría siempre activo, como dicen las guías de 2013.
- Experimentos concretos:
  1. Con el gancho de `IssueCommandAsync/Sync`, cambiar Micref Level y Wind NR en el menú y anotar categoría, mensaje y parámetros. Así se descubre la categoría de `ObjAudioProcessor` o de `ObjMic`, que no es `0x3100`.
  2. Extraer de `libObj.so` su tabla de nombres de MSGID y PARAMID como se hizo con ObjCamera.
  3. Inyectar «REC_LEVEL_MODE = manual» y `REC_LEVEL_VALUE = n` con pasos chicos, y medir el RMS del audio de un tono constante en clips (`ffmpeg -af volumedetect`) y el medidor de pantalla (Audio Level Display).
  4. Alternativa por el modelo: llamar a la acción de modelo que usa `ActionSetRecLevel` (número sin identificar) con `requestModelExecute`, como se hace con `model/CAMERA`.

### Gaps
- No encontré el rango ni la codificación de `REC_LEVEL_VALUE` en ninguna fuente (las guías de 2013 no dan números).
- No identifiqué la categoría de mensajes de `ObjAudioProcessor` ni el número de acción del modelo de audio.
- Ninguna fuente pública documenta un nivel de audio manual logrado por hack en una Sony.

---

## 6. Propiedades de Backup.bin para handycam (P7)

### Takeaway
Las listas públicas son cortas: el fork de LAK132 nombra 16 propiedades, OpenMemories-Platform las mismas y las tablas de mungewell unas pocas de video. De ellas, en la CX580V existen las de PAL/NTSC, códec, cadencia, bitrate y límite de grabación, con el mismo esquema `0x0107xxxx`. El hallazgo más útil para el proyecto es que ya tiene 30 copias de Backup.bin de hoy (dentro de las copias de `nflasha2` que guarda cada `guard-install`). Compararlas muestra qué propiedades cambian cuando el dueño cambia ajustes, y ése es el método de diff de #597 sin tocar la cámara.

### Cited Findings
- [fuente] Listas públicas de IDs: OpenMemories-Platform `backup.cpp` (`0x01660024`, `0x00e70000`, `0x003e0005`, `0x00e70003`, `0x003c0373..75`, `0x003c04b6`, `0x01070148`, `0x010d008f..b1` idiomas y `0x01640001`) — [backup.cpp](https://github.com/ma1co/OpenMemories-Platform/blob/master/api/backup.cpp); el fork de LAK132 agrega `0x010704fd`, `0x02cf0004`, `0x01070000` (uptime) y textos de autor y copyright `0x02cf0bd1..` — [backup.py](https://github.com/LAK132/Sony-PMCA-RE/blob/main/pmca/platform/backup.py). OpenMemories-Tweak no trae IDs propios: usa los de la plataforma — [OpenMemories-Tweak](https://github.com/ma1co/OpenMemories-Tweak).
- [local] Presencia en la CX580V: ver la tabla de la sección 4. Además `0x00e70000` = `0002408230` (código de producto), `0x003e0005` = «HDR-CX580V», `0x010d008f` = `01` (attr 1). El subsistema `0x0107` tiene 2472 propiedades, `0x003c` 1003 y `0x010d` 298 — [Backup.bin](file:///home/aristotle/temp/cx580-pro/backups/20261126-dump/Backup.bin).
- [local] Diff de 30 versiones distintas de Backup.bin, sacadas con `mcopy -i <imagen> ::/Backup.bin` de cada `backups/2026*-guard-install/nflasha2.bin` y de `20261126-dump`, sin modificar los respaldos. Leer sólo contadores que cambian siempre (`0x0058xxxx`, `0x000d0027`, `0x00bc000x`) y los que cambian pocas veces:

  | ID | Secuencia de valores (hora de la copia) | attr, reinicio | Lectura probable |
  |---|---|---|---|
  | `0x010700b8` | 00 → fc (18:18) → 03 (18:21) → 00 (18:35) → 04 (21:02) → fc (21:09) → 00 (21:13) | `0x60`, 00 | AE Shift (−4…+4); v16 manda +4 |
  | `0x010700ba` | 01 → 02 → 01 → 02 → 01 | `0x70`, 01 | AE Shift apagado/encendido |
  | `0x0107001b` / `0x0107001c` | 00 → 01 / 00 → fd (18:18) → 00 (18:21) | `0x70`, 00 | Exposure manual y su nivel (−3) |
  | `0x0107001f` | 02 → 01 (17:57) | `0x40`, reinicio 01 | ajuste de modo (sin identificar) |
  | `0x0107066a` / `0x0107066b` | 01 ↔ 02 (20:07, 20:14, 20:39, 21:02, 21:13) | `0x70`, reinicio 02 | quizá prioridad de iris/obturación u otro par auto/manual |
  | `0x01070973`, `0x0107098f`, `0x01070092`, `0x010704da`, `0x010703e1` | conmutan pocas veces | `0x40`–`0x70` | sin identificar |

  [Backup.bin extraídos](file://vendor/barrido-fuentes/r4/work/bk/), [script de diff](file://vendor/barrido-fuentes/r4/work/bkdiff.py).
- [local] El `Backup.bin` suelto de `backups/20261126-dump/` (md5 `b128ed9e…`) no es igual al que está dentro del `nflasha2.bin` de esa misma carpeta (`67584618…`). Son dos instantáneas distintas, y los valores de la tabla de la sección 4 coinciden en las dos — [20261126-dump](file:///home/aristotle/temp/cx580-pro/backups/20261126-dump/).
- [local] Nombres `BKID_*` sin número: en `av-cam`, `BKID_SDF_QTBL_C/Y`, `BKID_SDF_RETRY_Q01..05`, `BKID_SDF_USE_STD_QTBL` (tablas de cuantización JPEG en Backup), `BKID_VDF_SRVC_SET_HISTOGRAM_COMP_FREQ`, `BKID_EVF_CTRL_050/052/054` y `BKID_UPM_BOOT_CIPA_FLAG`; en `libmprctrl.so`, los `BKID_MPR_SYS_*` de la sección 4; en `libObj.so`, unos 286, como `BKID_APP_MENU_NTSC_PAL` y `BKID_APP_MENU_BEEP_SETTING` — [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin), [lib/](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/).

### Inferences
- [inferido] `0x010700b8` sería el AE Shift guardado: sus valores −4, +3 y +4 calzan con la v16, que manda +4. Si es así, el AE Shift de fábrica queda guardado entre arranques, y `cxg.so` tendría que tenerlo en cuenta al volver a cargar.
- [inferido] Para nombrar las demás alcanza con cruzar las horas de cada copia con los `cxreq.log` de las carpetas `-dumps`, que registran las acciones de `model/CAMERA` (por ejemplo White Balance Indoor = `0x12`/`0x136 = 9`).
- Experimento concreto, sin escribir nada: cambiar un solo ajuste por vez (HD/STD, Frame Rate, REC Mode, White Balance, Micref Level, Audio Mode, x.v.Color, Cinematone, SteadyShot, TV Type), apagar y bajar `nflasha2` con la copia de solo lectura de `guard-install`/`recon`, extraer Backup.bin con `mcopy` y comparar con `bkdiff.py`.

### Gaps
- No abrí `video_settings.zip` (AS15/QX10/HX60/AS100) ni el gist ILCE-BK4.bt en esta pasada. El informe anterior ya los cita, y lo comprobado en la CX580V cubre los IDs que mencionan las fuentes leídas.
- No sé si escribir `0x0107009e` o `0x01070087` en la CX580V hace algo más que cambiar el menú; `0x01070087` tiene `attr 0x68`, que dispara callbacks al escribir.

---

## 7. Otros: SteadyShot, zoom, Cinematone, perfiles de imagen, x.v.Color, HDMI limpio

### Takeaway
Todos tienen orden en el diccionario de ObjCamera. Lo nuevo es `SET_PICTURE_PROFILE_CMD` (`0x75ae`), con gamma (STD, CINEMA_TONE1/2, ITU709), black gamma, knee, detail, modo de color (incluidos PRO e ITU709) y WB Shift propio: el perfil de imagen de las NX, sin menú en la CX580V. Para HDMI limpio el único truco público es el de la clave de códec de la NEX-5N (`0x01070087 = 0a`), y esa clave existe en la CX580V con el mismo `attr`.

### Cited Findings
- [local] SteadyShot: `SET_MOVIE_STEADYSHOT_CMD` `0x7560` (`MOVIE_STEADYSHOT` `0x95d4`: `OFF` `0x12d`, `CONT` `0x12e`, `ACTIVE` `0x12f`), `SET_MOVIE_STEADY_TYPE_CMD` `0x7562` (`STD/SOFT/HARD` `0x130`–`0x132`) y los eventos `NOTIFY_STEADY_INDICATOR/ACTUATOR/CAUTION` `0x767c`–`0x767e`. Zoom: `SYNC_ZOOM_DRIVE_CMD` `0x6500` (`ZOOM_SPD` `0x9500`, `ZOOM_DIR` `0x9501`: `STOP/WIDE/TELE` 0/1/2), `SYNC_DIRECT_ZOOM_CMD` `0x6526`, `SET_EZOOM_CMD` `0x751a` y `SET_DIRECT_SMART/PRECISION_EZOOM_MODE` `0x7612`/`0x7614`. Cinematone: `SET_CINEMATONE_MODE_CMD` `0x75b0` (`CINEMATONE_MODE` `0x96d0`: `OFF/TYPE1/TYPE2` `0x1c7`–`0x1c9`). x.v.Color: `SET_MOVIE_COLOR_SPACE_CMD` `0x758a` (`MOVIE_COLOR_SPACE` `0x9630`: `COLOR_SPACE_STD/EXPANDED` `0x180`/`0x181`) — [MSGID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/MSGID.tsv), [PARAMID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/PARAMID.tsv), [VALUEID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/VALUEID.tsv).
- [local] Perfil de imagen: `SET_PICTURE_PROFILE_CMD` `0x75ae`.
  - Parámetros: `PP_COLOR_MODE_TYPE/LEVEL` `0x9673`/`0x9674`, `PP_COLOR_DEPTH_*` `0x967a`–`0x967f`, `PP_COLOR_SATURATION/HUE_LEVEL`, `PP_BLACK_LEVEL` `0x9682`, `PP_BLACK_GAMMA_RANGE/LEVEL` `0x9683`/`0x9684`, `PP_GAMMA_TYPE` `0x9685`, `PP_KNEE_MODE/MAX_POINT/SENSE/POINT/SLOPE` `0x9686`–`0x968a` y `PP_DETAIL_*` `0x968b`–`0x9691`.
  - Valores: `PP_GAMMA_STD/CINEMA_TONE1/CINEMA_TONE2/ITU709` `0x1b7`–`0x1ba`, `PP_COLOR_STD/CINEMA_TONE1/CINEMA_TONE2/PRO/ITU709` `0x1ad`–`0x1b1`, `PP_BLACK_GAMMA_HIGH/MID/LOW` `0x1b4`–`0x1b6`, `PP_KNEE_AUTO/MANUAL` `0x1bb`/`0x1bc` y `PP_DETAIL_OFF/ON` `0x1c0`/`0x1c1`.
  - Hay además una tecla LANC `KEY_LANC_PICTURE_PROFILE` y la orden `SET_AUTO_BLACK_BALANCE` `0x75b2`.
  - [PARAMID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/PARAMID.tsv), [VALUEID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/VALUEID.tsv), [libObj.so](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libObj.so)
- [fuente] HDR-CX760V/PJ760V: Cinematone y x.v.Color son ítems On/Off. Cinematone: «The camcorder automatically adjusts the image quality of the movie to resemble the atmosphere of film camera images». HDMI Resolution tiene Auto/1080p-480p/1080i-480i/720p-480p/480p (copias guardadas de la guía) — [03_14_14.html](file://vendor/barrido-fuentes/hg/03_14_14.html), [04_05_05.html](file://vendor/barrido-fuentes/hg/04_05_05.html), [07_11_11.html](file://vendor/barrido-fuentes/hg/07_11_11.html).
- [fuente] HDMI limpio en la NEX-5N: «I changed the codec key (id=0x01070087) from AVCHD, value 03, to XAVC, value 0a … when I press the record button the screen was clean». Lo confirma AstarOfDavid en otra NEX-5N; en una NEX-C3 dio «Error: unknown error» al leer la clave. Leído en la copia guardada del issue — [Sony-PMCA-RE #448](https://github.com/ma1co/Sony-PMCA-RE/issues/448), [copia local i448.txt](file://vendor/barrido-fuentes/i448.txt). [ayudante] Según el mismo hilo, la cámara no graba en ese modo y la NEX-5N vuelve la clave a `03` después de cada escritura.

### Inferences
- [inferido] El perfil de imagen y Cinematone son parientes. Es probable que Cinematone sea un perfil fijo (`PP_GAMMA_CINEMA_TONE1` + `PP_COLOR_CINEMA_TONE1`), así que `SET_PICTURE_PROFILE` podría dar gamma ITU709 o black gamma sin menú. Experimento: registrar qué manda Cinematone On y después inyectar `SET_PICTURE_PROFILE` con `PP_GAMMA_TYPE = PP_GAMMA_ITU709`, comprobando antes el bloqueo con `EmuInhCheck`.
- [inferido] El truco del HDMI limpio podría reproducirse en la CX580V con `0x01070087 = 0a` (o un valor de códec que no exista) desde el updater, pero `attr 0x68` dispara callbacks y la cámara no graba en ese estado. Es para pruebas de salida HDMI, no para el uso normal.

### Gaps
- No encontré nada publicado sobre perfil de imagen, gamma o x.v.Color desbloqueados por hack en handycam.

---

## 8. Cotejo local: método, archivos de trabajo y callejones sin salida

### Takeaway
Todo el cotejo se hizo con lectura estática y sin tocar la cámara ni los respaldos. Los archivos de trabajo quedan en el scratchpad y se pueden reutilizar. Varias búsquedas no dieron resultado y conviene no repetirlas.

### Cited Findings
- [local] Archivos de trabajo (`vendor/barrido-fuentes/r4/work/`):
  - `avcam.elf` y `avcam.thumb.txt`: desensamblado Thumb completo, sin el ruido de nombres de símbolo.
  - `xref.py`: busca lecturas y escrituras de variables de AE por carga de literal.
  - `disp.py`: busca tablas de despacho de av-cam.
  - `bk.py`: lee propiedades de Backup.bin.
  - `bkdiff.py`: compara copias de Backup.bin.
  - `bk/`: las 30 copias de Backup.bin extraídas.
  - Los del intento anterior: `avcam.strings.txt`, `libObj.strings.txt` y `vartables.pkl`.
  - [carpeta de trabajo](file://vendor/barrido-fuentes/r4/work/)
- [local] Callejones sin salida:
  - (1) Las escrituras a `uc_AgcManuOn` no aparecen siguiendo cargas de literal: el código usa punteros a estructura.
  - (2) Los manejadores de `0x45e920` son trampolines, así que buscar cadenas cerca no les da nombre.
  - (3) El `dump-txt` de LAK132 no trae una lista grande de nombres, sólo 16.
  - (4) OpenMemories-Tweak no trae IDs: están en OpenMemories-Platform.
  - (5) Las tablas de nombres de variables de `av-cam` no tienen un comando de depuración evidente para leerlas o escribirlas por nombre: `[CDIAG]` es del códec.
  - (6) Las búsquedas de AGC manual en los análisis de la A6000 (paso del AE, histéresis) no dieron nada.
  - [av-cam.bin](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin), [backup.py del fork](https://github.com/LAK132/Sony-PMCA-RE/blob/main/pmca/platform/backup.py)
- [local] Diferencia con `docs/research/firmware-analysis.md`: la fila `0x75be` «sin nombre» corresponde a `MSGID_SET_AEL_CMD` según `MSGID.tsv` — [MSGID.tsv](file:///home/aristotle/temp/cx580-pro/firmware/names/MSGID.tsv).

### Inferences
- [inferido] El paso estático que más rinde ahora es abrir `avcam.elf` en Ghidra (en otra sesión, fuera de este encargo), con la tabla `0x45e920` como punto de entrada y la tabla de variables de `0x49aa40` para nombrar la estructura del AE. Eso respondería qué orden enciende `uc_AgcManuOn` y cómo se codifica `us_AgcManuCont`.

### Gaps
- No usé archive.org en esta pasada: lo que se buscaba estaba en GitHub, en las guías de Sony y en el firmware local.
- No releí dvxuser, EOSHD ni personal-view sobre bitrate y PAL. El informe del ayudante (`handback_foros.md`) no encontró allí ningún archivo medido.
