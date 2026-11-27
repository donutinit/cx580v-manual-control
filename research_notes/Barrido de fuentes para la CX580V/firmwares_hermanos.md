# Firmwares de cámaras hermanas de la HDR-CX580V (CXD4132 y cercanas) y reparto de funciones por modelo

Notas del 2026-11-26/27. Todo lo «verificado» se leyó en la fuente o se comprobó en el archivo, con herramientas estáticas (curl -I, sha256sum, strings, nm, readelf y scripts Python propios que sólo leen bytes). No se ejecutó nada de lo descargado, no se tocó la cámara y no se usó Ghidra.

Rutas de trabajo (tmpfs, ~1 GB en uso):
- Base: `vendor/barrido-fuentes/r1/`
- Instaladores: `…/r1/dl/nx70/Update_NXCAMNX70V3.exe`, `…/r1/dl/ea50/Update_NXCAMEA50V200.exe`, `…/r1/dl/ax100/Update_FDRAX100V310.exe`
- Desempacados (se conservan):
  - NX70: `vendor/barrido-fuentes/r1/un/nx70/out/firmware.tar_unpacked/0700_part_image/dev/` (`nflasha3_unpacked_unpacked/`, `nflasha8_unpacked_unpacked/`)
  - EA50: `vendor/barrido-fuentes/r1/un/ea50/out/firmware.tar_unpacked/0700_part_image/dev/` (las mismas dos)
  - AX100: `vendor/barrido-fuentes/r1/un/ax100/out/firmware.tar_unpacked/0700_part_image/dev/` (`nflasha3_unpacked_unpacked/`, `nflasha15_unpacked_unpacked/`; en la AX100 la app va en nflasha15)
- Volcados de cadenas para comparar sin repetir el trabajo: `…/r1/str/{cx580,nx70,ea50,ax100}.uniq` (únicas), `.all.tsv` (archivo↔cadena), `.ids` (MSGID/PARAMID/VALUEID/MID de libObj), `.bkid`, `.adf`, `.liro`
- Scripts de lectura: `vendor/barrido-fuentes/scripts/bkidtab.py` (tabla BKID de libmprctrl.so) y `lits.py` (busca palabras de 32 bits)

## 1. ¿Qué hermanas comparten plataforma con la CX580V y qué controles «pro» traen? SoC y generación

### Takeaway
Las hermanas útiles son CXD4132, igual que la CX580V: HXR-NX70 (2011), NEX-FS100 (2011), NEX-EA50 (2012), NEX-VG20 y NEX-VG900. Para la VG20, la VG900 y la EA50 se verificó en el propio firmware o en el kernel GPL, y para la FS100 ya estaba verificado. La FDR-AX100/HDR-CX900 y la PXW-X70 son de la generación siguiente (CXD90014). El ID de modelo del updater sigue un patrón año/semestre/familia que permite ubicar cada cámara.

### Cited Findings
- HXR-NX70 V3.01: el FDAT declara `crypterName: CXD4132`, `model: 0x11010007`, `region: 0`, `version: '3.01'`. El instalador se llama `…V3`. Compilación `11107_06.2012032902`. — [config.yaml NX70](file://vendor/barrido-fuentes/r1/un/nx70/out/config.yaml) · [version.txt NX70](file://vendor/barrido-fuentes/r1/un/nx70/out/firmware.tar_unpacked/0700_part_image/dev/nflasha3_unpacked_unpacked/initrd.img_unpacked/version.txt)
- NEX-EA50 V2.00: `crypterName: CXD4132`, `model: 0x22010015`, `region: 0`, `version: '2.00'`, compilación `12215_03.2013052701`, `lib/githash` = `7996051`. — [config.yaml EA50](file://vendor/barrido-fuentes/r1/un/ea50/out/config.yaml)
- El fwtool en C de nex-hack ya tenía estos IDs: `0x22010015` NEX-EA50, `0x11010009` NEX-FS100, `0x21010061` NEX-FS700. NEX-VG20, VG30 y VG900 figuran como `0xffffffff` («model_type unknown»). — [fwt_util.c, espejo de nex-hack](https://github.com/erik-smit/nex-hack/blob/master/src/fwt_util.c)
- Los kernels GPL de la NEX-VG900 y de la NEX-VG20 tienen `CONFIG_ARCH_CXD4132=y`, `CONFIG_ARCH_CXD4132BASED=y`, `CONFIG_NR_CPUS=3` y `CONFIG_PREEMPT_RT=y`. El de la VG20 es el mismo archivo `DI/common/WNSvHvNnIw_d3BdCO3kJlA/linux-kernel.tar.gz` (33 451 761 B, SHA-256 `5877fcf65bdeec38…`) que enlaza la DEV-5. El de la VG900 pesa 33 520 329 B, SHA-256 `e72d3cd69d426d33…`, y trae perfiles `cxd4132b2_*` y `cxd4132c2_*`. — [OSS NEX-VG20](https://oss.sony.net/Products/Linux/DI/NEX-VG20.html) · [OSS NEX-VG900](https://oss.sony.net/Products/Linux/DI/NEX-VG900.html) · [kernel VG900](https://prodgpl.blob.core.windows.net/download/DI/NEX-VG900/linux-kernel.tar.gz)
- FDR-AX100 V3.10: `crypterName: CXD90014`, `model: 0x41040033`, `version: '3.10'`, compilación `44133_07.2016032301`. devices.yml también da CXD90014 para la AX100 y la HDR-CX900 (`0x41040034`). — [config.yaml AX100](file://vendor/barrido-fuentes/r1/un/ax100/out/config.yaml) · [devices.yml](file:///home/aristotle/temp/cx580-pro/vendor/fwtool.py/devices.yml)
- Componentes de la compilación. La capa de cámara y AV es la misma rama en las tres: `Camera-Agent/Liro/User_111H`, `Avbb-*_111H`, `Mw-framework_111H`, `Diadem-SysDef_111H`. Cambian la vista y el modelo de UI:

  | Cámara | Vista | Modelo de UI |
  | --- | --- | --- |
  | CX580V | `App-View-TP-LEFT_121H` | `App-Model_112H` |
  | NX70 | `App-View-TP-SEMIPRO_111H` | `App-Model_111H` |
  | EA50 | `App-View-TP-M_122H` | `App-Model_112H` |

  NX70 y EA50 traen además `Avbb-Tcub_111H` (código de tiempo); la CX580V no. — [config.spec NX70](file://vendor/barrido-fuentes/r1/un/nx70/out/firmware.tar_unpacked/0700_part_image/dev/nflasha3_unpacked_unpacked/initrd.img_unpacked/tmp/config.spec) · [config.spec CX580V](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha7_unpacked/config.spec) · cadenas `/vobs/…` en `…/r1/str/*.uniq`
- `CXD90009` («Sirius», el de la NEX-FS700 y la PXW-FS5) es un front-end (DFE) entre sensor y SoC, no el SoC. — [OpenMemories-Docs hardware.rst](https://github.com/ma1co/OpenMemories-Docs/blob/master/hardware.rst)
- Controles pro según el firmware de cada una (pantallas `view/*` que la CX580V no tiene):
  - NX70: `view/GAIN`, `view/GAIN_L`, `view/AGCLIMIT`, `view/TCPRESET`, `view/UBPRESET`, `view/ASSIGN_BUTTON`, `view/RECFORMAT`, `view/MARKER`, `view/PEAKING_L`, `view/SIMULTANEOUSREC` y `view/LAST_SCENE_REVIEW`.
  - EA50: unas 190, entre ellas `view/GAIN_H/M/L`, `view/GAINISO_SELECT`, `view/ISO`, `view/WB_TEMPSET`, `view/WBSHIFT_LB/CC/R_GAIN/B_GAIN/TYPE`, `view/AUDIO_LIMIT`, `view/INPUT1_TRIM`, `view/INPUT2_TRIM`, `view/XLR_AGC_LINK`, `view/FRAMERATE_50/60`, `view/60I_50ISEL`, `view/24P_OUTPUT`, `view/25P_OUTPUT`, `view/PICTURE_PROFILE`, `view/ZEBRA_*`, `view/KNEE_*` y `view/DVMODE`.

  — comparación de `…/r1/str/{cx580,nx70,ea50}.uniq`

### Inferences
- Patrón del ID de modelo del updater: `0xYS01nnnn` para videocámaras, `…02…` para DSC y `…03…` para ILC. `Y` es el año (1 = 2011, 2 = 2012, 3 = 2013, 4 = 2014) y `S` el semestre de lanzamiento (1 primavera, 2 otoño). Ejemplos: CX700 `0x11010005`, NX70 `0x11010007`, FS100 `0x11010009`, TD10 `0x11010011`, PJ590V/CX580V `0x21010001`, PJ760V `0x21010031`, TD20 `0x21010052`, FS700 `0x21010061`, AS10/AS15 `0x22010005`, EA50 `0x22010015`, PJ790V `0x31010060`, AX100 `0x41040033`. Es un patrón deducido de devices.yml, nex-hack y los FDAT, no documentado. Sirve para P6/P8 (saber qué updater acepta qué FDAT).
- La HXR-NX30 no tiene firmware público. Que sea «una PJ760 con otro firmware», como dice el foro citado en el informe previo, sigue sin poder comprobarse. Por la PJ760V (`0x21010031`, CXD4132), su SoC sería CXD4132, pero es inferencia.
- La VG30 salió con la EA50 y la VG900 en 2012. Su SoC sería CXD4132 por la VG900 (verificada), pero no se comprobó.

### Gaps
- Faltan el ID de modelo de la NX30, la VG20, la VG30 y la VG900: no hay FDAT público y nex-hack tampoco los tenía.
- La FS700 (`0x21010061`) y la HXR-MC2500 tienen kernel GPL, pero no se verificó su SoC.

## 2. ¿Qué firmwares de hermanas existen públicamente? (URL, versión, fecha, tamaño, estado)

### Takeaway
Ninguna handycam de consumo CXD4132 de 2011 a 2013 recibió actualización pública. Tampoco la NX30, la VG20, la VG30 ni la VG900. Sí hay actualizaciones, todas en línea, para las CXD4132 profesionales (NX70 V3.01, FS100 V2, EA50 V2.00) en download.pro.sony, para las Action Cam AS15 y AS30V, y para las NEX-5N/7/F3/5R/6 de foto. De la generación siguiente hay paquetes de AX100, CX900 y PXW-X70. Los .exe de NXCAM no los desempaca fwtool tal cual, pero llevan el DAT sin comprimir dentro.

### Cited Findings
Comprobado con HEAD o descarga el 2026-11-26:

| Modelo | Archivo, URL | Versión | Fecha | Tamaño (B) | Estado |
| --- | --- | --- | --- | ---: | --- |
| HXR-NX70 | [Update_NXCAMNX70V3.exe](https://download.pro.sony/23/09/MRyPBMNFC6/Update_NXCAMNX70V3.exe) | 3.01 (FDAT) | anuncio 2012-04-05 | 51 790 640 | en línea; SHA-256 `cbca61c19a04b2e6ac89d0377c89040c41b7d92a8aa472299c785d0223db838c` |
| NEX-EA50 | [Update_NXCAMEA50V200.exe](https://download.pro.sony/23/09/nsw_V6Gm3R/Update_NXCAMEA50V200.exe) | 2.00 | 2013-07-04 | 53 147 200 | en línea; SHA-256 `287f0c03ae674d0bc0d55d82ce6f1bf56173734ea93727b1b57e645ca18f9d13` |
| NEX-FS100 | [Update_NXCAMFS100V2.exe](https://download.pro.sony/23/09/CkACN4lltT/Update_NXCAMFS100V2.exe) | 2.00 | 2012-04-19 | 46 730 032 | en línea; no descargado (sólo se leyó el primer MiB) |
| FDR-AX100 | [Update_FDRAX100V310.exe](https://di.update.sony.net/HDR/gNo6e1b24q/Update_FDRAX100V310.exe) / [.dmg](https://di.update.sony.net/HDR/gNo6e1b24q/Update_FDRAX100V310.dmg) | 3.10 | Last-Modified 2016-04-25 | 71 970 552 / 82 307 792 | en línea; SHA-256 del .exe `b8481b1e715aa03b27fd81573bf1bb60b69dad44d575607a32de10e9294bc37e` |
| HDR-CX900 | [Update_CAMHDRCX900V300.exe](https://di.update.sony.net/HDR/4wA5Y7PABT/Update_CAMHDRCX900V300.exe) / [.dmg](https://di.update.sony.net/HDR/4wA5Y7PABT/Update_CAMHDRCX900V300.dmg) | 3.00 | Last-Modified 2015-03-11 | 72 511 576 / 84 892 311 | en línea |
| HDR-AS15 (CXD4132) | [V3.00 .exe](https://di.update.sony.net/HDR/oeNlZt8xaj/Update_CAMAS15V300.exe) · [V2.00 .exe](https://di.update.sony.net/HDR/DYgqRGkitd/Update_CAMAS15V200.exe) | 3.00 / 2.00 | 2013-06-10 / 2013-04-25 (Last-Modified) | 68 005 976 / 70 656 088 | en línea |
| PXW-X70 | [PXW-X70_V303_J.zip](https://download.pro.sony/23/09/FyyA3zBFYz/PXW-X70_V303_J.zip); en Wayback también Update_PXWX70V200/V301/V302.dmg en di.update.sony.net/HXR | 3.03 | — | — | ver [Sony Japón](https://www.sony.jp/xdcam/update/pxw-x70.html) |

- Contenido de las actualizaciones de Sony Japón:
  - [NX70J](https://www.sony.jp/xdcam/update/HXR-NX70J.html) (2012-04-05, para Ver.2 o menor): velocidad de zoom, grabación simultánea en memoria y tarjeta, 1920×1080/30p y 1280×720/60p, botón asignable, Last Scene Review, marco de caras y zebra/histograma en la salida externa.
  - [EA50JH](https://www.sony.jp/ls-camera/update/nex-ea50jh.html) (2013-07-04, para Ver.1.00): ajuste del botón de grabación HD y grabación en formato DV.
  - [FS100J](https://www.sony.jp/xdcam/update/NEX-FS100J.html) (2012-04-19, para Ver.1): 60/50 Hz (NTSC/PAL) conmutable, perfiles de cámara en tarjeta, foco ampliado 4×/8×, ISO en pantalla, pies de foco y ángulo de obturación.
- La lista de actualizaciones profesionales de Sony Japón tiene NEX-EA50JH, NEX-FS700, HXR-NX3, NX5J, NX5R, NX70J, NX800, NX80, NEX-FS100J y PXW-X70. No están la HXR-NX30J ni la HXR-NX3D1, y `sony.jp/xdcam/update/HXR-NX30J.html` devuelve 404. — [sony.jp/xdcam/update](https://www.sony.jp/xdcam/update/)
- Callejones sin salida:
  - HXR-NX30U (EE. UU.) y HXR-NX30E (RU): «Unfortunately, there currently are no downloads for this product». — [NX30U](https://www.sony.com/electronics/support/professional-camcorders-handheld-camcorders/hxr-nx30u/downloads) · [NX30E](https://www.sony.co.uk/electronics/support/professional-camcorders-handheld-camcorders/hxr-nx30e/downloads)
  - NEX-VG30, VG900 y VG20 (EE. UU.): sólo Imaging Edge Desktop y PlayMemories Home. En el catálogo japonés: «※現在提供されているアップデート情報はありません» (no hay información de actualizaciones). — [VG30 EE. UU.](https://www.sony.com/electronics/support/camcorders-and-video-cameras-interchangeable-lens-camcorders/nex-vg30/downloads) · [catálogo JP VG30](https://support.d-imaging.sony.co.jp/www/cscs/firm/?mdl=NEX-VG30&lang=ja)
  - En el catálogo japonés, HDR-PJ760V y HDR-TD20V dan el mismo aviso. HDR-TD10, PJ790V y CX700V (EE. UU.) sólo listan PlayMemories Home e Imaging Edge Mobile.
  - En el archivo de actualizaciones Handycam de Sony Japón (captura de 2014) sólo aparecen FDR-AX1 V2.00 (2014-01-30) y NEX-VG10 (2011-12-15), sin entradas de 2012 ni 2013. — [2011.html en Wayback](http://web.archive.org/web/20140828175823/http://www.sony.jp:80/handycam/update/2011.html) · [index 2014 en Wayback](http://web.archive.org/web/20140525211407/http://www.sony.jp:80/handycam/update/index.html)
- En Wayback (CDX de `di.update.sony.net/HDR/`, `/NEX/` y `/HXR/`), los únicos paquetes de firmware CXD4132 son de AS15/AS30V y de las NEX de foto: [Update_NEX7V103.exe](http://di.update.sony.net/NEX/yPM9KL6Nlx/Update_NEX7V103.exe) (52 784 221 B), Update_NEX5NV102.exe (52 980 652 B), Update_NEXF3V101.dmg, Update_NEX5RV103 y Update_NEX6V103. En `/HXR/` sólo hay modelos de 2014 en adelante (PXW-X70, FDR-AX1, HXR-NX3, NX5R, FS5, FS7, X160/X180, Z100). No hay ninguna VG20/VG30/VG900/NX30. — listados CDX en `…/r1/cdx/`
- Las URL `support.d-imaging.sony.co.jp/download/IMT/Update_CAMAS15V300.exe`, `…AS30V300.exe` y `…FDRAX100V310.exe`, que todavía muestra el catálogo japonés, devuelven 404. Las de di.update.sony.net funcionan.
- fwtool con los .exe de NXCAM (NX70, EA50) falla con `Exception: Unknown exe file` (`unpackInstaller`, fwtool.py línea 103): no hay sección `_winzip_` ni LZH al final. El DAT (`\x89UFU\r\n\x1a\n`) va sin comprimir dentro del .exe: en la NX70 de 0x8044c a 49 035 620, en la EA50 de 0x80242 a 50 282 218 y en la FS100 empieza en ~0x80444 (FDAT de 43 449 344 B). Cortado con `dd` a `FirmwareData_x.dat`, fwtool lo desempaca sin errores. El .exe de la AX100 lo desempaca directo. — [fwtool.py](file:///home/aristotle/temp/cx580-pro/vendor/fwtool.py/fwtool.py) · [log NX70](file://vendor/barrido-fuentes/r1/un/nx70.log)

### Inferences
- Para la CX580V, la única hermana de consumo con paquete descargable es la AS15. Es Action Cam y sin pantalla táctil, así que aporta poco a P1/P2. La familia útil es la profesional de la misma rama de código (NX70, FS100, EA50).
- Que el instalador japonés de la NX70 diga «V3» y el FDAT «3.01» indica que Sony republicó el paquete con un arreglo menor sin cambiar el nombre.

### Gaps
- No se descargaron ni desempacaron FS100 V2, AS15, CX900, X70 ni las NEX de foto (tope de 3 paquetes). La FS100 tiene la misma estructura de .exe que la NX70, comprobado en su primer MiB.
- No se encontró ninguna lista pública de ma1co (fwtool, OpenMemories-CI, issues) con URLs de firmware de estas hermanas: en los clones locales sólo hay URLs de ILCE-6000/6300.

## 3. Verificación local de NX70 V3.01, EA50 V2.00 y AX100 V3.10 frente a la CX580V 1.07, y cuál comparar primero

### Takeaway
Conviene empezar por la NX70 V3.01, que ya está desempacada. Es la única hermana con interfaz de handycam táctil que fija la ganancia en video, con `view/GAIN` y `ViewGain` en `viewUnified2.so` y la misma API `ViewBase::requestModelExecute/getModelInfo`. Comparte la capa de cámara (`Camera-*_111H`) y la familia `IRIS_SHUTTER_GAIN_MANUAL` sin la variante ISO, y su libObj no tiene ningún ID de ganancia que la CX580V no tenga. Basta leer en Ghidra qué acción de `model/CAMERA` y qué mensaje de ObjCamera manda `ViewGain` para saber qué inyectar en la CX580V (P1). Después vienen la EA50, para Kelvin, NTSC/PAL, 24p/25p y audio (P3, P4, P5), y la AX100, cuyo libObj exporta `CmnViewCameraCommon_L::control_Gain` y `control_WBColourTemp` como referencia limpia. La comparación también corrige dos cosas del informe previo: `PRO_SET_MANUAL_GAIN_LINK` es de audio, y la ganancia manual en video de NX70, EA50 y AX100 sale de órdenes que la CX580V ya tiene.

### Cited Findings
- Bibliotecas presentes (tamaño y SHA-256 abreviado):

  | Binario | CX580V 1.07 | NX70 3.01 | EA50 2.00 |
  | --- | --- | --- | --- |
  | `lib/libObj.so` | 18 812 212 `0fed10ff…` | 13 892 988 `a18e7f80…` | 20 414 076 `48143793…` |
  | `lib/libMWF.so` | 254 232 `f02d2202…` | 218 004 `5b89b70c…` | 180 340 `baac1d71…` |
  | `lib/libSysDef.so` | 311 276 `3d23f35c…` | 260 848 `b3dd2178…` | 392 452 `99585fd9…` |
  | `lib/viewUnified*.so` | 2..7 (v2: 241 028 `f6740348…`) | sólo 2 (1 441 084 `82c7e3c4…`) | ninguna (las vistas van en libObj) |
  | `av-cam.bin` | 5 615 780 `779fad33…` | 5 575 876 `ad58bae3…` | 6 331 252 `e54a5e96…` |
  | `bonobo.bin` | 172 144 `9f65a2f3…` | 172 144 `9cd15477…` | 172 144 `9f65a2f3…` (igual a la CX580V) |

  AX100: `nflasha15_unpacked_unpacked/lib/libObj.so` 18 125 156 B, SHA-256 `01542f08baf175b37503127d8a7a8952ce5cdc1f40f119b1be52c4a11ddff13d`; `libMWF.so` 237 996 B; `libSysDef.so` 610 588 B; `viewUnified2..7.so`. — `sha256sum` sobre las rutas de arriba
- Presencia de cadenas clave en todo el firmware:

  | Cadena | CX580V | NX70 | EA50 | AX100 |
  | --- | --- | --- | --- | --- |
  | `VALUEID_IRIS_SHUTTER_GAIN_MANUAL_ON` | sí | sí | no | no |
  | `VALUEID_IRIS_SHUTTER_GAIN_ISO_MANUAL_ON` | sí | no | sí | sí |
  | `MSGID_SET_ISO_SPEED_RATE_CMD` | sí | sí | sí | sí |
  | `SetManualExposureModeForLiro` | sí | no | sí | sí |
  | `[ADF]PRO_SET_MANUAL_GAIN_LINK` | sí | sí | sí | sí |
  | `view/GAIN` | no | sí | sí | no |
  | `view/AGC_LIMIT` | sí | no (`view/AGCLIMIT`) | sí | sí |
  | `view/WB_TEMPSET` | no | no | sí | no |
  | `view/AUDIO_REC_LEVEL` | no | no | no | sí |
  | `PARAMID_REC_LEVEL_VALUE` | sí | no | sí | sí |
  | `PARAMID_WB_COLOR_TEMP` | sí | sí | sí | sí |
  | `view/60I_50ISEL`, `view/FRAMERATE_50` | no | no | sí | no |
  | `VALUEID_FR_24P`, `VALUEID_FR_30_25P` | sí | sí | sí | sí |
  | `BKID_APP_MENU_NTSC_PAL` | sí | sí | sí | sí |

  — `…/r1/str/*.uniq`
- NX70, ganancia (P1). Su libObj no tiene ningún MSGID, PARAMID ni VALUEID de ganancia que la CX580V no tenga. Las órdenes de cámara candidatas son `MSGID_SET_ISO_SPEED_RATE_CMD` (en la bitácora interna: «SeqSetISOSpeedRate) CamManager ---> CamLIRO [MSGID_SET_ISO_SPEED_RATE_CMD:PID_SET_ISO_SPEED_RATE]»), `MSGID_SET_AGC_LIMIT_CMD`, `SET_EXPOSURE_MODE` con `VALUEID_EXP_MODE_MANUAL`, y `SYNC_MANUAL_EXPOSURE_DRIVE` (`SetManualExposureDriveForLiro` sí está). No tiene `SetManualExposureModeForLiro`. A diferencia de la CX580V, tiene `PARAMID_ISO_SPEED_RATE_MENU_STD_RANGE_MIN/MAX(_SET/_ACT)` y `PARAMID_DATA_ISO_MENU_STD_RANGE_MIN/MAX`. — `…/r1/str/nx70.ids`, `nx70.all.tsv`
- NX70, la pantalla. `viewUnified2.so` exporta `ViewGainToInstance` en 0x8fbad (Thumb), `ViewAgcLimitToInstance` en 0x5bb89, `ViewIris_LToInstance` en 0x95d05 y `ViewShutterSpeed_LToInstance` en 0xb0eb5. Trae las clases `LG_viewgain::LayoutGAIN_SETTING` y `ViewGain`, el diseño `share/app/viewGain.uxc` (SHA-256 `03498dd0…`) y `menuExclusiveAgcLimit`. Importa por la PLT `ViewBase::requestModelExecute(char const*, unsigned long, ParamList*)`, `getModelInfo`, `openView` y `closeView`, igual que las pantallas de la CX580V. El libObj de la NX70 exporta `ACT_GainValue::cast` y `ACT_Gain_L::cast`. — [viewUnified2.so NX70](file://vendor/barrido-fuentes/r1/un/nx70/out/firmware.tar_unpacked/0700_part_image/dev/nflasha8_unpacked_unpacked/lib/viewUnified2.so) (`nm -D`)
- EA50, ganancia: `view/GAIN_H/M/L`, `view/GAINISO_SELECT`, `view/ISO`, `ACT_Gain_BI`. Tiene `IRIS_SHUTTER_GAIN_ISO_MANUAL` y no `IRIS_SHUTTER_GAIN_MANUAL`, y además `MID_HYBRID_01/02_SET_ISO_SPEED_RATE_CMD` y `MSGID_SET_GAIN_UP_LONGFLASH_CMD`. — `…/r1/str/ea50.ids`
- AX100, ganancia y Kelvin. Su libObj exporta:
  - `CmnViewCameraCommon_L::control_Gain(int, int)` en 0x69c861
  - `isGainModeAuto()` en 0x69bf75 e `isISOModeAuto()` en 0x69b711
  - `control_WBColourTemp(int)` en 0x69c715
  - `control_ProgramAe(int)` en 0x69cbd5
  - `control_WBShift(int)` en 0x69d64d
  - `control_Iris(int)` en 0x69d1ad

  `viewUnified2.so` y `viewUnified7.so` de la AX100 importan tres de ellos cada una. Hay además `ACT_IsoSensitivity`, `ACT_GainValue`, «Err (pvt_SetParamIsoSpeedRate)», «ISO/Gain Numer», `ViewAudioRecLevel` y `ViewAudioRecLevelMusiCam`. En la CX580V el mismo objeto sólo exporta `control_Iris`, `control_ShutterSpeed`, `control_Exposure`, `control_AEShift`, `control_WBShift`, `control_FocusMode`, `manual_FocusDrive`, `resetTimeCode` y `resetUserBit`. — [libObj.so AX100](file://vendor/barrido-fuentes/r1/un/ax100/out/firmware.tar_unpacked/0700_part_image/dev/nflasha15_unpacked_unpacked/lib/libObj.so) (`nm -D -C`)
- **Corrección: `PRO_SET_MANUAL_GAIN_LINK` es una orden de audio.** En `av-cam.bin` de la CX580V está en 0x3494be, dentro del bloque `[ADF]` (flujo de audio) junto con `PRO_SET_CH_SOURCE` (0x348e3c), `PRO_SET_REC_LEVEL_MODE` (0x3490fa), `PRO_SET_AGC_LINK` (0x349329) y `PRO_SET_REC_LEVEL_LIMITTER` (0x34975e). En libObj, `PARAMID_AGC_LINK` sólo aparece en mensajes de `[ObjAudioProcessor]`. Las diez órdenes `[ADF]PRO_SET_*` (AGC_LINK, CH_SOURCE, HP_MONITOR, HP_OUT, INPUT_TRIM, INPUT_TYPE, INT_MIC_SENS, MANUAL_GAIN_LINK, REC_LEVEL_LIMITTER, REC_LEVEL_MODE) son idénticas en los av-cam de CX580V, NX70 y EA50, y la EA50 tiene `view/XLR_AGC_LINK`. — [av-cam.bin CX580V](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/av-cam.bin) (`strings -t x`) · `…/r1/str/*.adf`
- Audio (P5). El ObjAudioProcessor de la CX580V tiene la misma ruta de nivel de grabación manual que la EA50: «OsalSndMsg => ADF SetRecLevel : ch 0x[%x] value 0x[%x]», «ADF Set RecLevelMode : ch, mode», `MSGID_SET_REC_LEVEL_MODE_CMD_INTERNAL(Ch1)`/`(Ch2)`, `PARAMID_REC_LEVEL_MODE`, `PARAMID_REC_LEVEL_VALUE`, `MSGID_SET_INPUT_TRIM_CMD_INTERNAL(Input1/2)` y `PARAMID_AUDIO_LEVEL_LIMIT`. La NX70 no tiene `PARAMID_REC_LEVEL_VALUE`. — `…/r1/str/cx580.all.tsv`, `ea50.all.tsv`
- Formatos (P4). Las cuatro tienen `PARAMID_FRAME_RATE`, `VALUEID_FR_24P/30_25P/60_50I/60_50P` y `PARAMID_VIDEO_OUT_NTPAL`. Sólo la EA50 tiene `VALUEID_EXC_MOVIE_FR_*`, `PARAMID_24P_30P_OUTPUT`, `PARAMID_EXC_FRAME_RATE` y las pantallas `FRAMERATE_50/60`, `60I_50ISEL`, `SETUP_MOV_FORMAT_50/60` y `24P_OUTPUT`/`25P_OUTPUT`. — `…/r1/str/*.ids`
- `…ForLiro` (conversores hacia av-cam): 156 en la CX580V, 120 en la NX70 y 190 en la EA50. La NX70 sólo agrega `SetZoomSpeedSettingForLiro`. La EA50 agrega, entre otros, `SetIrisDialForLiro`, `SetShutLowLimitForLiro`, `SetPushAutoIrisModeForLiro`, `SetWBLockForLiro` y `SetGainUpLongFlashForLiro`. — `…/r1/str/*.liro`
- Menús (P2). El libObj de la CX580V exporta `CmnViewMenuData::initMenuData(CmnViewMenuItemStruct*, CmnViewMenuItemInconstPart*, CmnViewMenuExclusiveIF*)` en 0x1864b4 y `CmnViewMenuData::setItemValid(CmnViewMenuItemStruct*, int, bool)` en 0x185a9c. También exporta `setGreyOut` (0x186de8), `isItemDisplayed` (0x186ea8), `getId` (0x185f70), `getSubItem` (0x1870e8) y `cursorMoveToById` (0x187040). `viewUnified7.so` importa 20 símbolos `CmnViewMenuData`, entre ellos `initMenuData` y `setItemValid`; `viewUnified4.so` importa 12 (también esos dos) y `viewUnified6.so` importa `initMenuData`. El libObj de la NX70 tiene los mismos (`initMenuData` en 0x2c859b, `setItemValid` en 0x2c7f95). — [libObj.so CX580V](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libObj.so) (`nm -D`)
- `bonobo.bin` es idéntico, byte a byte, en la CX580V 1.07 y la EA50 2.00 (`9f65a2f3…`). El resto de binarios difiere en todos los pares.

### Inferences
- P1. La NX70 fija la ganancia en video con las mismas órdenes de cámara que tiene la CX580V. Por eliminación, `ViewGain` termina en `SET_ISO_SPEED_RATE` (con ISO como «ganancia», igual que el «ISO/Gain» de la AX100) o en `SYNC_MANUAL_EXPOSURE_DRIVE`. Lo más probable es que vaya junto a `EXP_MODE_MANUAL` o `IRIS_SHUTTER_GAIN_MANUAL_MODE = ON`. Eso contradice la conclusión del informe previo de que el ISO es sólo de foto: EA50 y AX100, cámaras de video con manual total, también usan la familia ISO. La prueba v15 de la CX580V (ISO aceptado y no aplicado) se hizo en prioridad de iris u obturación, no en `EXP_MODE_MANUAL`. **Qué comprobar en la CX580V:** primero, leer en Ghidra `ViewGain` de la NX70 (desde `ViewGainToInstance`, 0x8fbad) para obtener la acción de `model/CAMERA` y las claves de su `ParamList`, y seguirla en el libObj de la NX70 hasta el `ObjMsg`. Luego repetir en la CX580V la inyección v15 de `SET_ISO_SPEED_RATE` con la capa de cámara en `EXP_MODE_MANUAL` (como v7) y con `IRIS_SHUTTER_GAIN_MANUAL_MODE = ON`, y leer `ISO_SPEED_RATE_ACT` y la ganancia del clip con `tools/cxexposure.py`.
- P1, otra vía. Como la NX70 usa `App-Model_111H` y la CX580V `App-Model_112H`, la numeración de acciones de `model/CAMERA` puede diferir. El mensaje de ObjCamera (`0x3100`) es más portable que la acción del modelo de UI.
- P2. `setItemValid(item, índice, bool)` es el candidato más fuerte al punto donde la CX580V esconde ítems que su `viewUnified7.so` sí trae (IRIS, SHUTTER, AGC_LIMIT, WB_SHIFT). Está exportado y viewUnified7 lo llama por la PLT, así que `cxg.so` puede interceptarlo como ya hace con `ViewBase::*`. **Qué comprobar:** envolver `setItemValid` e `initMenuData` y registrar puntero, índice, valor e `getId()` del ítem al abrir `Manual Settings`, y después forzar `true` para los ítems de exposición.
- P5. El nivel de audio manual existe en el ObjAudioProcessor y en el av-cam de la CX580V, sin pantalla. Las cámaras que lo usan, NX70 y EA50, lo mueven con diales físicos; la AX100 lo hace con `view/AUDIO_REC_LEVEL`. **Qué comprobar:** registrar en `cxg.so` los `MSGID_SET_REC_LEVEL_MODE_CMD_INTERNAL` que se emitan (si alguno) y probar una inyección con `PARAMID_REC_LEVEL_MODE` en manual y `PARAMID_REC_LEVEL_VALUE`, midiendo el nivel en un clip. Falta encontrar la categoría y el número de esos mensajes de audio, que no están en los diccionarios del proyecto (`firmware/names/`).
- P3. Kelvin: `PARAMID_WB_COLOR_TEMP` y `VALUEID_WB_COLOR_TEMP` están en la CX580V, y la AX100 trae `control_WBColourTemp(int)` exportado como modelo de llamada. Leer esa función da la acción y el rango que espera la capa de cámara.
- P4. El 50/60 Hz de la FS100 llegó por firmware (V2), y EA50 y AX100 traen `BKID_APP_MENU_NTSC_PAL`. El cambio NTSC/PAL es software más una propiedad de Backup, no hardware (inferencia).
- La NX70 tiene un solo `viewUnified2.so` de 1.4 MB con sus pantallas. La CX580V reparte en `viewUnified2..7` pantallas de varios modelos (3D de la TD20, dial de la PJ760V), lo que encaja con una imagen compartida por toda la familia `TP-LEFT_121H`.

### Gaps
- Sin desensamblar (Ghidra fuera del encargo) no se sabe qué acción y qué claves manda `ViewGain` de la NX70 ni `control_Gain` de la AX100. Es lo que decide P1.
- No se comparó `viewGain.uxc` de la NX70 con los `.uxc` de la CX580V, ni se probó cargar pantallas de otra cámara (sería mezclar binarios y no se recomienda).
- La AX100 es CXD90014: sus números de MSGID y PARAMID y su `ObjMsg` no están verificados frente a la CX580V.

## 4. ¿Cómo decide el firmware qué funciones ofrece cada modelo?

### Takeaway
Hay dos niveles. Primero, Sony compila una imagen por familia de UI: las capas de cámara, AV y middleware son comunes (`_111H`), y cambian `App-View-TP-*` y `App-Model_*`. Dentro de una familia (PJ590V/CX580V, que comparten ID de modelo, y las pantallas de PJ760V y TD20 en la misma imagen), las funciones las eligen en tiempo de ejecución propiedades de Backup.bin con nombre `BKID_*`. En el firmware de la CX580V aparecen `BKID_APP_PARAM_UI_TYPE`/`UI_SUB_TYPE`, `BKID_APP_MENU_NTSC_PAL`, `BKID_DISPLAY_PROJECTOR_SUPPORT_MODEL`, `BKID_MPR_SYS_TVSYSTEM_TYPE`, `BKID_MPR_SYS_TC_UB_SUPPORT` y `BKID_MODEL_NAME`. A eso se suma la tabla de bloqueos `DefInh`, ya documentada. Nadie ha publicado los números de estas propiedades para handycam.

### Cited Findings
- Nombres de propiedades de Backup en el firmware de la CX580V (448 `BKID_*` distintos; 447 en la NX70, 949 en la EA50, 987 en la AX100):
  - libObj imprime «BKID_APP_PARAM_UI_TYPE     : 0x%02X(%02d)» y «BKID_APP_PARAM_UI_SUB_TYPE : 0x%02X(%02d)».
  - `[ObjDisplay]` imprime «BKID_APP_MENU_NTSC_PAL : 0x%x» y «BKID_DISPLAY_PROJECTOR_SUPPORT_MODEL : 0x%x».
  - `[ObjEffect]` dice «BackUp Read Error BKID_APP_MENU_NTSC_PAL».
  - `libmprctrl.so` lee `BKID_MODEL_NAME`, `BKID_MPR_SYS_TVSYSTEM_TYPE`, `BKID_MPR_SYS_TC_UB_SUPPORT` y `BKID_MPR_SYS_M2PS_REC_SUPPORT`, y aparecen además `BKID_MPR_SYS_AVCHD_REC_SUPPORT`, `…MP4_REC_SUPPORT`, `…MULTI_REC_SUPPORT`, `…GPS_TYPE` y `…MEDIA_TYPE`.
  - `viewUnified7.so` tiene «[V_CUSTOMIZE_DTI_L]  in BKID_APP_VAL_CMN_PARAM_MENU_ALL_IN_ON».

  — `…/r1/str/cx580.all.tsv`, `cx580.bkid`
- `libmprctrl.so` trae una tabla `{id, puntero a nombre}` de 160 BKID, todas `0x0005xxxx` (tamaños de JPEG, de `BKID_IMAGING_DATSZ_DRJPG_VERT_STD_EXFINE_MAX` = `0x00050000` a `BKID_IMAGING_DATSZ_JPGSCN_4_3_TGT` = `0x000500a7`). Es idéntica en CX580V y NX70; 136 de los 160 pares coinciden con los 685 de la EA50. — [libmprctrl.so CX580V](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libmprctrl.so) · `…/r1/str/*.bkidtab.tsv` (script `bkidtab.py`)
- Palabras de 32 bits en `libObj.so` de la CX580V que coinciden con IDs de Backup publicados para otras cámaras: `0x010704fd` (modo de video PAL/NTSC, «confirmed HX60») 7 veces, `0x0107009e` (modo de captura AVCHD) 7, `0x010700a1` (bitrate AVCHD) 7 y `0x003e0005` (nombre del modelo) 2. No aparecen `0x01070148` (selector PAL/NTSC visible), `0x003c0373` (límite de grabación) ni `0x010d008f` (idiomas). Los significados vienen de la tabla ya reunida en el proyecto. — script `lits.py` sobre [libObj.so CX580V](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libObj.so) · [tabla de IDs del proyecto](file:///home/aristotle/temp/cx580-pro/research_notes/Ingeniería%20inversa%20cámaras%20Sony%20Linux/proyectos_comunidad.md)
- libObj tiene en las cuatro cámaras las clases `ModelIdTable`, `ModelIdSoTable` (con `getInstance`), `ModelCaution` y la biblioteca `ModelCautionConfig.so` (`m_factorTbl`, `getModeTable`, `getCautionTable`). La CX580V nombra en libObj los `.so` que carga: `viewUnified2.so` … `viewUnified7.so`; la NX70 sólo `viewUnified2.so`. — `…/r1/str/*.all.tsv`
- En `/setting` de la CX580V (`nflasha2`) hay `DmmConfig.bin` (62 358 B), `.hasp` (88 B, todo ceros), `kemco.txt` (parámetros del kernel: `rootfstype=axfs`, `tmonitor`, `memrsv`), `Backup.bin` y `Backup.bak` (437 176 B). libObj dice `DmmConfig=/usr/share/dmm/DmmConfig.bin` y existe `kmod/dmm.ko`, el gestor de memoria. — [nflasha2_unpacked](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha2_unpacked/)
- Búsqueda pública (GitHub, búsqueda de código): `BKID_APP_PARAM_UI_TYPE` y `BKID_MPR_SYS_TVSYSTEM_TYPE` → 0 resultados. `IRIS_SHUTTER_GAIN_MANUAL` → sólo el repositorio público del propio proyecto. En los clones locales, `BKID_` aparece en trabajos de A5000 y ZV-E10, no de handycam. El de la A5000 mide propiedades con `bk r` y diff alternando un ajuste en el menú (beep `0x01070026`, zebra `0x010701B8/B9`, peaking `0x01070352/0360`), y anota que el símbolo de zebra es `BKID_RENDERER_ZEBRA` aunque su número esté en `0x0107`. — [propid-mapping-measured.md (a5000-cfw)](https://github.com/Oein/a5000-cfw/blob/main/docs/RE/backup/0002.%20propid-mapping-measured.md)

### Inferences
- La CX580V y la PJ590V comparten el ID `0x21010001` y por tanto el mismo paquete. Lo que las distingue (proyector, idiomas, región) tiene que estar en Backup.bin: `BKID_DISPLAY_PROJECTOR_SUPPORT_MODEL` es el caso evidente. Del mismo modo, `BKID_APP_PARAM_UI_TYPE`/`UI_SUB_TYPE` son los mejores candidatos a la «propiedad de Backup.bin» que, según `firmware-analysis.md`, decide en `FUN_007a3fec` si usar los alias `_L` (pantallas de la PJ760V). **Qué comprobar en la CX580V (P2/P7):** en Ghidra, seguir las referencias a la cadena «BKID_APP_PARAM_UI_TYPE     : 0x%02X(%02d)» hasta la constante que se pasa a `Backup_read`. Luego leer ese ID en el `Backup.bin` del respaldo, sin escribir, y compararlo con lo que esperaría una PJ760V (`0x21010031`, otra imagen, así que no sirve de diff directo).
- Que `0x010704fd`, `0x0107009e` y `0x010700a1` aparezcan como constantes en libObj sugiere que la CX580V usa para video los mismos IDs que la HX60 y la NEX-5N (P4/P7). Hay que confirmarlo leyendo esas claves del `Backup.bin` del respaldo y viendo su valor (por ejemplo, NTSC = 1 en una unidad NTSC).
- El 50/60 Hz de la FS100 añadido por firmware y la EA50 con `view/60I_50ISEL` indican que el sistema de TV de estas cámaras es una propiedad (`BKID_MPR_SYS_TVSYSTEM_TYPE`, `BKID_APP_MENU_NTSC_PAL`) y no hardware. El precedente de la NEX-5N (el tweak de 60p cambiaba el menú pero no la grabación, PMCA-RE #329) obliga a medir el clip antes de dar nada por hecho.

### Gaps
- No se obtuvieron los números de `BKID_APP_PARAM_UI_TYPE`, `BKID_APP_MENU_NTSC_PAL`, `BKID_MPR_SYS_TVSYSTEM_TYPE` ni `BKID_DISPLAY_PROJECTOR_SUPPORT_MODEL`: están como inmediatos en el código y hay que desensamblar.
- No se encontró documentación pública sobre `ModelIdSoTable`, `DmmConfig.bin` ni `.hasp` en estas cámaras. Por el contexto (`ViewBase::getModelInfo`), «Model» parece el modelo MVC de la UI y no el producto, pero no está comprobado.

## 5. Código GPL de las hermanas: ¿algo más que el kernel ya revisado?

### Takeaway
Para todas las CXD4132 (VG20, VG900, EA50, FS700, FS100, NX3D1, MC2500, CX210…) Sony sólo publica el kernel 2.6.29 y los mismos cinco RPM de userland (busybox, dosfstools, e2fsprogs, hostname, procps). No hay encabezados de la aplicación, toolchain ni drivers propietarios. NX70 y NX30 no tienen página propia (la NX70 remite a la FS100). Sólo la PXW-X70, de otra generación, añade glibc, bibliotecas de gcc, ALSA y GStreamer.

### Cited Findings
- [NEX-VG20](https://oss.sony.net/Products/Linux/DI/NEX-VG20.html): kernel `DI/common/WNSvHvNnIw_d3BdCO3kJlA/linux-kernel.tar.gz` (33 451 761 B, Content-MD5 `WNSvHvNnIw/d3BdCO3kJlA==`, el mismo que la DEV-5) y los cinco RPM `sony-target-srel-*` de `DI/common`.
- [NEX-VG900](https://oss.sony.net/Products/Linux/DI/NEX-VG900.html): kernel de 33 520 329 B y los mismos RPM.
- En la categoría DI de oss.sony.net están HDR-CX210, CX180, TD10, MV1, DEV-5, DEV-50, HXR-MC50, NEX-VG10/VG20/VG900, NEX-5N, FDR-AX100 y HDR-CX900, entre otras. No están HDR-PJ760V, CX760V, PJ590V, CX580V, TD20V ni NEX-VG30. — [oss.sony.net DI](https://oss.sony.net/Products/Linux/DI/category01.html)
- B2B: [NEX-EA50](https://oss.sony.net/Products/Linux/B2B/NEX-EA50.html) (kernel de 33 520 455 B y los cinco RPM), [NEX-FS700](https://oss.sony.net/Products/Linux/B2B/NEX-FS700.html) (33 517 568 B, los mismos RPM), NEX-FS100 (32 653 519 B), HXR-NX3D1, HXR-MC2500 y HXR-MC88. `B2B/HXR-NX70.html` y `B2B/HXR-NX30.html` → 404. — [oss.sony.net B2B](https://oss.sony.net/Products/Linux/B2B/category01.html)
- [PXW-X70](https://oss.sony.net/Products/Linux/B2B/PXW-X70.html): kernel de 53 971 010 B; `sony-target-dev-glibc-2.11.2-08020203.src.rpm` (15 679 356 B); `sony-target-dev-gcc-gplv3-libs-4.5.1-08020104.src.rpm` (66 385 109 B); alsa-lib 1.0.19, busybox, e2fsprogs 1.42, iptables 1.4.10, libnl 3.2.22, rpm 4.4.2.3, util-linux-ng 2.17.2; glib 2.42.0 y gstreamer/gst-plugins 1.0.4.
- Todos los kernels CXD4132 se subieron el 2019-11-12 (Last-Modified).

### Inferences
- Para P2, P6 y P8 los paquetes GPL de las hermanas CXD4132 no aportan más que el kernel ya revisado. A lo sumo, los perfiles `cxd4132b2_*`/`c2_*` de la VG900 indican revisiones del chip, cosa que no se comprobó. El paquete de la X70 sí da una versión concreta de glibc (2.11.2) y de gcc (4.5.1) de Sony, aunque para CXD90014. Puede orientar el toolchain, sin garantía de que sea el de la CX580V.

### Gaps
- No se descargaron los kernels de EA50, FS700, NX3D1 y MC2500 para compararlos con el de la CX210. Por tamaño y fecha parecen variantes del mismo árbol.

## 6. FDR-AX100/HDR-CX900 y PXW-X70 como referencia de ganancia manual

### Takeaway
La AX100 (y su gemela CX900) es CXD90014, hay firmware en línea y fwtool la desempaca directo. Su libObj exporta las funciones de UI que la CX580V no tiene: `control_Gain(int,int)`, `isGainModeAuto`, `isISOModeAuto`, `control_WBColourTemp`, `control_ProgramAe` y la pantalla `ViewAudioRecLevel`. Su ganancia manual va por la familia ISO (`ACT_IsoSensitivity`, «ISO/Gain», `pvt_SetParamIsoSpeedRate`, `IRIS_SHUTTER_GAIN_ISO_MANUAL`). La PXW-X70 tiene firmware V3.03 y GPL con userland, pero no se desempacó.

### Cited Findings
- AX100 V3.10: instalador de 71 970 552 B, FDAT `0x41040033`/3.10/CXD90014, particiones `nflasha3` y `nflasha15`. — [Update_FDRAX100V310.exe](https://di.update.sony.net/HDR/gNo6e1b24q/Update_FDRAX100V310.exe) · [config.yaml AX100](file://vendor/barrido-fuentes/r1/un/ax100/out/config.yaml)
- Símbolos y cadenas de la AX100: ver la sección 3 (direcciones de `control_Gain` 0x69c861, `control_WBColourTemp` 0x69c715 y demás). Pantallas `view/AUDIO_REC_LEVEL`, `view/AUDIORECLEVELMUSICAM`, `view/EXTAUDIOINPUT`, `view/IRIS_BI`, `view/WBSHIFT_BI`, `view/AGC_LIMIT(_L)` y `view/AE_SHIFT(_L)`; no hay `view/GAIN`. IDs nuevos frente a la CX580V: `MSGID_SET_HYPER_GAIN_MODE_CMD`, `PARAMID_FORCE_EXPOSURE_LEVEL(_CONTROL_MODE)`, `PARAMID_EXPOSURE_LEVEL_ID` y `PARAMID_AGC_AE_OUT_OF_RANGE`. — `…/r1/str/ax100.uniq`, `ax100.ids`
- La guía de la AX100 dice que el mismo control muestra dB en película (−3 a +33 dB) e ISO en foto. — [Adjusting the gain (ISO)](https://helpguide.sony.net/gbmig/45346551/v1/en/contents/TP0000390091.html) (ya citado en el informe previo)
- PXW-X70: Ver.3.03 en [Sony Japón](https://www.sony.jp/xdcam/update/pxw-x70.html) ([zip](https://download.pro.sony/23/09/FyyA3zBFYz/PXW-X70_V303_J.zip)); en Wayback, Update_PXWX70V200.dmg (76 966 181 B), V301.dmg (86 428 025 B) y V302.dmg (86 427 260 B) en di.update.sony.net/HXR. No está en devices.yml.

### Inferences
- La AX100 confirma lo mismo que la NX70: en video, la ganancia manual se maneja con la orden ISO (mostrada en dB) y no con una orden «GAIN» aparte. Para P1 eso vuelve a poner `SET_ISO_SPEED_RATE` en el centro, ahora con el modo de exposición correcto.
- `control_Gain(int, int)` recibe dos enteros, quizá paso y modo, por analogía con `control_Iris(int)`. Desensamblarla es la forma más corta de ver qué acción de `model/CAMERA` usa una handycam para la ganancia manual, aunque la numeración de CXD90014 puede no coincidir con la CX580V.
- La PXW-X70 es CXD90014 por generación (2014, XAVC) y por su paquete GPL con glibc 2.11 y GStreamer. No se comprobó en un FDAT.

### Gaps
- No se desempacaron la CX900 ni la X70: el .zip de la X70 no se probó con fwtool y la CX900 duplicaría a la AX100.
- Falta desensamblar `control_Gain` y `control_WBColourTemp` de la AX100 y `ViewGain` de la NX70. Es el siguiente paso, con el Ghidra del proyecto.
