# Exposición manual en modo video: Handycam Sony 2011-2016 y cámaras Sony con Linux

Notas de investigación del 2026-11-26. Convención de fiabilidad:

- **[verificado]**: leí la página fuente y cito o parafraseo su texto.
- **[resumen]**: sale del resumen del buscador o de un fragmento. No leí la página entera, así que hay que confirmarlo antes de citarlo como hecho.
- **[foro]**: testimonio de un usuario, no documentación de Sony.
- **[proyecto]**: hallazgo propio de cx580-pro, ya documentado en `docs/research/`.

## 1. Controles manuales de exposición en modo película de las Handycam 2011-2014, y ganancia frente a ISO

### Takeaway
Las Handycam de consumo de 2011-2013 (CX760V, PJ760V, PJ790V, CX580V, NX30) admiten **un solo** control manual de exposición a la vez: Exposure, IRIS o Shutter Speed. Los otros dos parámetros quedan en automático. La ganancia nunca es manual. Sólo se le puede poner un tope con AGC Limit, de 0 a 24 dB, y desplazar la exposición con AE Shift. Las fuentes de esa época con ganancia manual en dB son la HXR-NX70 (iris en el aro, ganancia y obturación por My Buttons, según un dueño) y la NEX-VG30 (de lentes intercambiables). En 2014 la FDR-AX100/HDR-CX900 añadió un botón GAIN/ISO. Su control único "gain (ISO)" muestra dB en película e ISO en foto.

### Cited Findings
**HDR-CX760V / PJ710 / PJ710V / PJ760 / PJ760V (2012)**
- [verificado] La guía de ayuda cubre HDR-CX760V, HDR-PJ710, PJ710V, PJ760 y PJ760V. Su menú Camera/Mic incluye White Balance, Spot Meter/Fcs, Spot Meter, Spot Focus, **Exposure**, Focus, **IRIS**, **Shutter Speed**, **AGC Limit**, **AE Shift**, **White Balance Shift**, **Low Lux**, Scene Selection, Cinematone, Tele Macro, Zebra, Peaking y My Button, entre otros. — [Contents list CX760V/PJ760V](https://helpguide.sony.net/gbmig/44196351/v1/us/contents/contentslist.html)
- [verificado] Al dial MANUAL se le puede asignar Focus (por defecto), Exposure, IRIS, Shutter Speed, AE Shift o White Balance Shift. Texto literal: *"If you set either one of [Exposure], [IRIS], or [Shutter Speed], the settings of other 2 items are canceled."* También: *"if you set [Exposure] after setting [AE Shift] manually, [Exposure] will override [AE Shift]."* La página no menciona ganancia ni ISO. — [MANUAL dial CX760V/PJ760V](https://helpguide.sony.net/gbmig/44196351/v1/us/contents/04/02/07/07.html)
- [verificado] IRIS es un modo de prioridad (*"You can shoot images with the IRIS priority mode"*). Si el iris se abre más que F3.4 en gran angular, *"the IRIS is reset to F3.4 as you move the zoom lever to the T (Telephoto) side."* — [IRIS CX760V/PJ760V](https://helpguide.sony.net/gbmig/44196351/v1/us/contents/07/03/07/07.html)
- [resumen] AGC Limit está en Camera/Mic → Manual Settings → AGC Limit. Valores: Off (ganancia automática) o de 24 dB a 0 dB, con ganancia automática "hasta el límite elegido". En película el tope es 24 dB. Los topes de foto y de Smooth Slow REC difieren entre versiones de la guía: 18 dB y 12 dB en una, 21 dB y 15 dB en la versión GB. — [AGC Limit GB 44196361](https://helpguide.sony.net/gbmig/44196361/v1/gb/contents/07/03/09/09.html) · [AGC Limit (2D only) 44501451](https://helpguide.sony.net/gbmig/44501451/v1/gb/contents/07/03/09/09.html) · [PJ760V Operating Guide p. 70](https://www.manualowl.com/m/Sony/HDR-PJ760V/Manual/269460?page=70)
- [resumen] La HDR-PJ790V tiene página de AGC Limit en su manual "Handycam User Guide" (p. 153). — [PJ790V AGC Limit](https://www.manualsdir.com/manuals/445601/sony-hdr-pj790v.html?page=153)

**Testimonios de dueños sobre CX760V, NX30 y NX70**
- [foro] Cliff Totten, 2012-05-09, sobre la CX760V: *"You cannot adjust "gain", "shutter" and "iris" at the same time. You can pick only 1 to set."* Su alternativa es fijar la obturación y ajustar con EV ±, y admite que no sirve para todo. Dice que la ganancia no se ve en pantalla al grabar, sólo en reproducción, y que en la NX70 no pasa eso. Según él, la CX760V llega a 24 dB y la NX70 se queda en 21 dB. Rey Lowe, 2012-05-28, responde que la NX70 también llega a 24 dB con Low Lux. Paul Rickford, 2012-06-02, asignó "gain limit" a un My Button. — [DV Info Net «Sony HDR-CX760V: A Warning»](https://www.dvinfo.net/forum/archive/index.php/t-507570.html)
- [foro] Alister Chapman sobre la HXR-NX30 (2012), citado por Ron Evans el 2012-05-24: *"It doesn't appear possible to get full independent control of iris, shutter and gain"*. Sigue: *"You can choose to manually control the exposure, shutter and iris, but then the others become automatic"*, y *"You can set an AGC gain limit which can be as low as 0db and this stays set while you change the exposure."* Ron Evans, 2012-06-13, opina que la NX30 es *"exactly the PJ760"* con otro firmware y adaptador XLR. Describe el dial de la CX700 con *"focus, exposure, iris, shutter speed, AE shift, white balance shift"*. — [DV Info Net, hilo NX30E p. 3](https://www.dvinfo.net/forum/sony-nxcam-avchd-camcorders/506579-new-sony-hxr-nx30e-palmcorder-advanced-stabilization-3.html)
- [foro] Mike Beckett, 2011-06-20, sobre la HXR-NX70: el iris se asigna al aro del objetivo. Ganancia y obturación se ajustan con "my buttons" táctiles: se toca el ícono, se cambia el valor y se pulsa OK. *"They are independently controlled"* en modo manual, pero *"gain and shutter cannot be assigned to physical buttons"*. — [DV Info Net, Mike Beckett's NX70 owner's report p. 4](https://www.dvinfo.net/forum/sony-nxcam-avchd-camcorders/497197-mike-becketts-nx70-owners-report-4-print.html)
- [resumen] Un fragmento del manual de la NX70 en ManualsHive dice que la ganancia se ajusta con el selector AUTO/MANUAL en MAN, de 0 a 21 dB, y el iris con el aro manual. La página bloqueó la lectura (Anubis). — [ManualsHive HXR-NX70](https://manualshive.com/en/manual/sony/hxr-nx70/1323632?page=2)
- [proyecto] `docs/research/web-research.md` ya registra que los manuales de la NX70E/U documentan iris, ganancia y obturación independientes con el selector en MAN y los tres valores a la vista. Hoy el PDF oficial devolvió 403 y no lo pude releer. — [Manual de operación NX70E](https://pro.sony/s3/cms-static-content/operation-manual/4273940131.pdf)

**NEX-VG30 (2012, Handycam de lentes intercambiables)**
- [verificado] *"You can set the brightness of the movies you want to shoot by using IRIS, Shutter Speed and Gain (ISO)."* Con IRIS en manual, *"The Shutter Speed and gain (ISO) are adjusted to the ideal brightness automatically"*. AE Shift no tiene efecto si IRIS, Shutter Speed y Gain (ISO) están los tres en manual, así que se admite exposición totalmente manual. — [Manual NEX-VG30H p. 49](https://www.manualsdir.com/manuals/441586/sony-nex-vg30h.html?page=49)
- [resumen] Ganancia manual de unos 0 a 30 dB. — [Manual NEX-VG30H p. 71](https://www.manualsdir.com/manuals/441586/sony-nex-vg30h-nex-vg30.html?page=71) · [Handbook NEX-VG30H p. 50](https://www.manualowl.com/m/Sony/NEX-VG30H/Manual/329092?page=50)

**FDR-AX100 / HDR-CX900 (2014): primera Handycam de consumo con ganancia manual**
- [verificado] El botón GAIN/ISO alterna la ganancia entre manual y automática, y el dial MANUAL la ajusta. *"At default, you can set in the range -3dB to +33dB."* Con Smth Slw REC el rango es *"-3dB - +27dB"*. En foto, *"the setting displayed will change to ISO and the range you can adjust will be ISO160 - ISO12800."* La función se justifica para cuando *"you do not want to boost the gain using the AGC"*. — [Adjusting the gain (ISO)](https://helpguide.sony.net/gbmig/45346551/v1/en/contents/TP0000390091.html)
- [verificado] Hay páginas separadas para IRIS, velocidad de obturación, ganancia (ISO), Exposure y filtro ND. — [Changing the brightness manually](https://helpguide.sony.net/gbmig/45346551/v1/en/cover/level3_10.html)
- [verificado] El botón PROGRAM AE pone en automático IRIS, ganancia (ISO), obturación y Exposure. La pantalla muestra IRIS, ganancia y obturación de izquierda a derecha, cada uno con su ícono de automático o manual. — [Adjusting the exposure AX100](https://helpguide.sony.net/gbmig/45346551/v1/en/contents/TP0000390092.html)
- [verificado] AGC Limit en la AX100/CX900: Off (por defecto) o 24, 21, 18, 15, 12, 9, 6, 3 y 0 dB. *"The gain is automatically adjusted up to the selected limit."* Una ganancia baja reduce el ruido, pero *"the shutter speed becomes slower to compensate"*. — [AGC Limit AX100/CX900](https://helpguide.sony.net/gbmig/45346551/v1/en/contents/TP0000391605.html)
- [resumen] Una guía posterior, de la FDR-AX700 (2017, fuera del periodo), dice que AGC Limit no tiene efecto cuando la ganancia se ajusta a mano. — [AGC Limit AX700](https://helpguide.sony.net/cam/1710/v1/en/contents/TP0001492402.html)

**HDR-CX580V (proyecto)**
- [proyecto] Con `cxmode-v9` y `preload/cxg.so`, el proyecto ya logró en la CX580V iris y obturación manuales e independientes. Lo hizo con `SET_SHUT_DRIVE_MODE = SHUT_MANUAL` / `SET_IRIS_DRIVE_MODE = IRIS_MANUAL` sobre las prioridades de fábrica. Con iris y obturación fijos, cada paso de AE Shift inyecta un tope de ganancia (`SET_AGC_LIMIT`) y la cámara acepta todos (`ret=0`). — `docs/research/firmware-analysis.md`, «Control manual independiente, probado en la cámara».

### Inferences
- Entre 2011 y 2013 el patrón de Sony parece ser de producto, no de hardware. La NX30 sería una PJ760 con otro firmware (Evans) y aun así sólo ofrecía un control manual a la vez, mientras la NX70 de la misma época sí separaba iris, ganancia y obturación. Esto apoya lo que el proyecto ya comprobó: el bloqueo está en la interfaz o el menú, no en el procesador de cámara.
- En estas Handycam "Exposure" es un modo aparte, excluyente con IRIS y Shutter. Probablemente mueve iris, obturación y ganancia a la vez, como describe la AX100 (*"The setting value of the IRIS/gain/shutter speed changes according to the number of dial rotations"*). Si es así, Exposure no sirve para fijar la ganancia.
- Desde 2014 Sony nombra el control "gain (ISO)": es uno solo, con dB en película e ISO en foto. Eso sugiere que en firmware posterior ambos son vistas de un mismo parámetro de ganancia del sensor y que la unidad depende del modo.

### Gaps
- No conseguí releer el manual oficial de la HXR-NX70 (el PDF de pro.sony dio 403 y ManualsHive bloqueó la lectura). Falta la cita textual del rango de ganancia y de si iris, ganancia y obturación pueden estar en manual a la vez.
- No verifiqué páginas de HDR-PJ790V, HDR-TD20 ni HDR-PJ590V más allá de la existencia de AGC Limit en la PJ790V. Por la familia de producto se espera el mismo conjunto que en la PJ760V, pero no está confirmado.
- No encontré ninguna Handycam de consumo de 2011-2013, salvo NX70 y VG30/VG20, con ganancia manual fija en video.

## 2. Cámaras Sony con Android (PlayMemories Camera Apps, `com.sony.scalar.hardware.CameraEx`): ISO y ganancia al grabar video

### Takeaway
La API privada de Sony para las apps PlayMemories expone **dos parámetros distintos**: `setISOSensitivity(int)` y `setCameraGain(int)`. El segundo, con `getSupportedCameraGains()` y `AutoCameraGainListener`, aparece marcado "added in V3". Es decir, Sony separa ganancia e ISO en su capa de cámara. No encontré ninguna app de la comunidad (BetterManual y otras) documentada para fijar ISO o ganancia durante la grabación de video. Las conocidas son de fotografía.

### Cited Findings
- [verificado] En los stubs de `CameraEx$ParametersModifier` de OpenMemories-Framework aparecen `getISOSensitivity()`, `setISOSensitivity(I)`, `getSupportedISOSensitivities()`, `getSupportedExpandedISOSensitivities()`, `setISOAutoMax/Min(I)` y `setIsoAutoMinShutterSpeedMode/Value`. Por separado, `getCameraGain()I`, `setCameraGain(I)V` y `getSupportedCameraGains()` llevan el comentario `; added in V3`. También están los métodos de estabilización de película `get/setMovieAntiHandBlurMode`. — [CameraEx$ParametersModifier.j](https://github.com/ma1co/OpenMemories-Framework/blob/HEAD/stubs/src/main/jasmin/com/sony/scalar/hardware/CameraEx%24ParametersModifier.j)
- [verificado] `CameraEx` define `setAutoCameraGainListener(...)` (interfaz `AutoCameraGainListener.onChanged(int, CameraEx)`, "added in V3") y, aparte, `setAutoISOSensitivityListener(...)`. También define las constantes `RECORDING_MODE_MOVIE = 1`, `MOVIE_MODE_SINGLE = 0` y `MOVIE_MODE_MULTI_STREAMING = 1`. — [CameraEx.j](https://github.com/ma1co/OpenMemories-Framework/blob/HEAD/stubs/src/main/jasmin/com/sony/scalar/hardware/CameraEx.j) · [CameraEx$AutoCameraGainListener.j](https://github.com/ma1co/OpenMemories-Framework/blob/HEAD/stubs/src/main/jasmin/com/sony/scalar/hardware/CameraEx%24AutoCameraGainListener.j)
- [verificado] `com.sony.scalar.media.MediaRecorder$Parameters` sólo trae formato (AVCHD, MPEG4, XAVCS, XAVCS_4K), tasa de bits, cuadros por segundo, tamaño, código de tiempo y grabación en bucle. No hay parámetros de exposición: la exposición del video se controla desde `CameraEx`. — [MediaRecorder$Parameters.j](https://github.com/ma1co/OpenMemories-Framework/blob/HEAD/stubs/src/main/jasmin/com/sony/scalar/media/MediaRecorder%24Parameters.j)
- [verificado] BetterManual (obs1dium), para la A5100, ofrece modo manual y prioridad de apertura con obturación, apertura, ISO (manual o automático) y compensación. Las funciones son de foto: disparo simple y ráfagas, timelapse, bracketing y desactivar el NR de larga exposición. No menciona video y advierte: *"Do not try to install it on cameras without a dedicated manual mode!"*. El README dice que ya no se mantiene. — [BetterManual](https://github.com/obs1dium/BetterManual)
- [verificado] Otros repositorios comunitarios reproducen el stub `setISOSensitivity` / `getSupportedISOSensitivities` para la NEX-5T. — [nex5t-clean-hdmi, stub CameraEx](https://github.com/Trifocals7733/nex5t-clean-hdmi)

### Inferences
- `setCameraGain` aparece en la V3 de la API, al lado de un ISO que existía desde antes. Lo más probable es que exista para los modos o modelos donde la cámara expresa la sensibilidad en dB (video o cuerpos de cine) y no en ISO. Esto calza con la hipótesis de que en las cámaras de Sony el ISO es el parámetro de foto y la ganancia, el de película.
- Si la CX580V (CXD4132, Linux, sin Android) comparte linaje con esa capa, el equivalente de `setCameraGain` sería el que acepta su `av-cam`, no `SET_ISO_SPEED_RATE`. En la CX580V el proyecto sólo encontró `SET_AGC_LIMIT` y no un "set gain" directo. Habría que buscar en `libObj.so` parámetros tipo `CAMERA_GAIN`, `GAIN_DRIVE` o `MANUAL_GAIN`.

### Gaps
- No encontré documentación ni testimonios sobre si `setISOSensitivity` o `setCameraGain` funcionan mientras `MediaRecorder` graba en las cámaras PMCA, ni sobre la regla "ISO AUTO only in movie". La búsqueda de código en GitHub quedó cortada por el límite de la API, así que no pude revisar qué apps llaman a `setCameraGain`.
- No sé qué modelos reportan una lista no vacía en `getSupportedCameraGains()`.

## 3. Protocolos remotos (Camera Remote API, PTP/MTP, LANC/A/V-R) y hacks: ¿alguien logró ganancia o ISO fijos en video?

### Takeaway
No encontré ningún hack, mod de firmware, propiedad de Backup.bin, protocolo remoto ni app documentado públicamente que haya logrado ganancia o ISO fijos, ni exposición totalmente manual, en video en una Handycam de consumo de esta época. La Camera Remote API rechaza ISO según el modo y el modelo (403, lista de APIs disponibles por modo). La implementación PTP de Sony en gphoto2 sólo conoce una propiedad ISO, sin ganancia. LANC permite ganancia manual sólo si la cámara implementa la orden. El único resultado positivo conocido es el del propio proyecto: tope de ganancia con `SET_AGC_LIMIT` desde `cxg.so`, con iris y obturación fijos.

### Cited Findings
- [verificado/foro] Camera Remote API en una A7R: ISO no estaba en la lista de APIs disponibles y *"Forcing" a setIsoSpeedRate command to the camera didn't work either and I get an error code 403: Forbidden.* Le sugirieron actualizar Smart Remote Control a 3.10, sin que se confirmara la solución. — [Stack Overflow 27490209 (copia kiwix)](https://kiwix.lema.org/stackoverflow.com_en_all_2023-11/questions/27490209/sony-camera-api-change-exposure-parameters)
- [resumen/foro] Con "M" en el dial físico, el único `shootMode` que devuelve la API es "still", y algunas APIs sólo están disponibles en ciertos `shootMode` (pregunta sobre la a6300). — [Stack Overflow 45664275](https://kiwix.lema.org/stackoverflow.com_en_all_2023-11/questions/45664275/still-and-movie-shootmode-on-a6300)
- [resumen/foro] Para el ISO automático se recomienda revisar los valores *available* y no los *supported*, porque algunos sólo se pueden usar en ciertos modos. — [Stack Overflow 37703461](https://kiwix.lema.org/stackoverflow.com_en_all_2023-11/questions/37703461/how-i-can-set-auto-iso-by-sony-remote-camera-api)
- [resumen] La Camera Remote API beta (2013) se lanzó con ocho cámaras compatibles, entre ellas las action cam HDR-AS30 y HDR-AS15. — [RedShark News 2013](https://www.redsharknews.com/technology-computing/item/1050-sonoy-releases-camera-apis-puts-developers-in-control)
- [verificado] Propiedades Sony en libgphoto2: `PTP_DPC_SONY_ISO 0xD21E` (marcada `/* ? */`), `PTP_DPC_SONY_ISO2 0xD226`, `PTP_DPC_SONY_ShutterSpeed 0xD20D`, `PTP_DPC_SONY_MovieRecordingState 0xD21D` y `PTP_DPC_SONY_ExposureCompensation 0xD224`. Para la serie QX: `QX_ISO 0xD6F2` y `QX_ISOAutoMinimum/Maximum 0xD6DB/0xD6DC`. No hay ninguna propiedad de ganancia (gain/AGC). — [libgphoto2 ptp.h](https://github.com/gphoto/libgphoto2/blob/master/camlibs/ptp2/ptp.h)
- [proyecto] En la CX580V en MTP (`054c:05fd`), gPhoto2 2.5.28 vio una "USB PTP Class Camera" que sólo descarga archivos. No anunció operaciones de captura ni propiedades de iris, obturación, ganancia, foco o balance. — `docs/research/web-research.md`, punto 5 de las pruebas USB.
- [resumen] En las Handycam con terminal multi A/V de 10 pines, LANC está presente pero sólo se accede con un cable adaptador (Sony J-6082-535-A) o con controles como el RM-AV2. — [Wikipedia LANC](https://en.wikipedia.org/wiki/LANC)
- [resumen] El control LANC RM-1000BP tiene botones para obturación, ganancia y balance en automático o manual. Con cámaras más viejas sólo funciona un subconjunto, según las órdenes que implemente la cámara. — [ProVideo Coalition, reseña RM-1000BP](https://www.provideocoalition.com/review_sony_rm_1000bp_lanc_remote_control/)
- [proyecto/verificado] `web-research.md` registra que OpenMemories-Tweak documenta Backup.bin y su protección de escritura, con funciones probadas como límite de grabación, idiomas y PAL/NTSC. No hay evidencia de propiedades de Backup.bin para iris, obturación o ganancia. — [OpenMemories-Tweak](https://github.com/ma1co/OpenMemories-Tweak) · [Sony-PMCA-RE](https://github.com/ma1co/Sony-PMCA-RE)
- [proyecto] `cxg.so` v9c: con iris y obturación fijos, cada paso k de AE Shift inyecta `SET_AGC_LIMIT` con `AGC_LIMIT = (k+4)·3 dB` y `AGC_LIMIT_MODE = ON`. Subir el paso aumenta brillo y ruido, bajarlo los reduce, y la cámara acepta cada límite. — `docs/research/firmware-analysis.md`.

### Inferences
- La CX580V no tiene Wi-Fi, así que la Camera Remote API (Wi-Fi, 2013 en adelante) no le aplica. En los modelos que sí la tienen, el patrón de 403 y de APIs disponibles según el modo indica que el servidor de la cámara publica ISO sólo donde su capa de cámara lo permite. Es el mismo tipo de inhibición que el factor `0x1f7` de la CX580V.
- LANC por el Multi Terminal es la vía remota más plausible para pedir ganancia manual. Sólo serviría si `av-cam`/la interfaz de la CX580V implementa la orden, y para el modo de consumo eso no está documentado.

### Gaps
- No obtuve el PDF de referencia de la Camera Remote API (sección "Supported API groups for each compatible cameras"), así que no hay una cita oficial de que `setIsoSpeedRate` no esté disponible con `shootMode = movie`.
- No encontré la lista de órdenes LANC que aceptan las Handycam 2011-2013 por el Multi Terminal, ni si incluye ganancia o iris.
- No encontré issues ni publicaciones de Sony-PMCA-RE u OpenMemories sobre Handycam con exposición o ganancia manuales.

## 4. Testimonios de foros sobre cómo trabar la ganancia en Handycam Sony

### Takeaway
La técnica que se repite en los foros es **AGC Limit bajo (incluso 0 dB) más un control de prioridad o AE Shift**. Funciona como tope, no como valor fijo: la ganancia baja de ese tope cuando hay luz de sobra. Con 0 dB el automático sólo trabaja con iris y obturación. Nadie en los foros reporta una ganancia fija verdadera en una Handycam de consumo de 2011-2013.

### Cited Findings
- [foro] NX30 (2012): el AGC Limit puede bajar a 0 dB y *"stays set while you change the exposure"* (Chapman vía Evans). Jay West, 2012-06-13, menciona *"the ability to lock-down the AGC"* como ventaja de la NX30 sobre la CX760. — [DV Info Net, NX30E p. 3](https://www.dvinfo.net/forum/sony-nxcam-avchd-camcorders/506579-new-sony-hxr-nx30e-palmcorder-advanced-stabilization-3.html)
- [foro] CX760V: obturación manual y EV ±, más "gain limit" en un My Button. — [DV Info Net t-507570](https://www.dvinfo.net/forum/archive/index.php/t-507570.html)
- [resumen/foro] HVR-Z5/HDR-FX1000 (2009): un usuario recomienda tope de ganancia en 0 dB, AE Shift para ajustar la exposición e interruptores de ganancia manual con valores fijos. Otro dice que no hay otra forma de ajustar la ganancia que cambiar el AGC Limit a mano. — [DV Info Net, «Can I Adjust Video Gain AGC Sensitivity»](https://www.dvinfo.net/forum/sony-hvr-z5-hdr-fx1000/465503-can-i-adjust-video-gain-agc-sensitivity.html)
- [resumen/foro] HVR-V1: con AGC Limit en 0 dB, el modo automático depende sólo de iris y obturación. — [DV Info Net, «Video Gain»](https://www.dvinfo.net/forum/sony-hvr-v1-hdr-fx7/170460-video-gain.html)
- [verificado] La propia Sony explica el costo: con poca ganancia *"the shutter speed becomes slower to compensate for the lack of brightness"*. — [AGC Limit AX100/CX900](https://helpguide.sony.net/gbmig/45346551/v1/en/contents/TP0000391605.html)

### Inferences
- Con iris y obturación fijos, la ganancia efectiva es el mínimo entre la que pide el AE y el tope. Si la escena está subexpuesta para ese par iris/obturación, la ganancia queda clavada en el tope, y en la práctica es fija. Si sobra luz, el AE la baja. Para una ganancia fija verdadera hay que impedir que el AE la baje: un modo manual de ganancia, AE lock o un AGC "mínimo" si existe en `av-cam`. La prueba v9c del proyecto encaja con ese modelo, aunque no se midió la ganancia grabada en escenas con luz de sobra.

### Gaps
- No encontré mediciones en foros (por ejemplo, con Data Code) que muestren si la ganancia baja del tope de AGC Limit en escenas luminosas con iris y obturación fijos. Se puede medir en la CX580V con `tools/cxexposure.py`.
- No encontré nada sobre "Low Lux" como forma de cambiar el tope. El hilo de la CX760V sólo indica que en la NX70 Low Lux eleva el máximo de 21 a 24 dB.

## 5. Pistas técnicas: por qué `SET_ISO_SPEED_RATE` puede no tener efecto en modo película

### Takeaway
Varias pistas apuntan a que en estas Handycam el ISO es un parámetro del modo foto y la película usa la ganancia AGC en dB. En la AX100, el mismo control "gain (ISO)" cambia de unidad según el modo. La API de las Sony con Android separa `setISOSensitivity` de `setCameraGain`. En el firmware de la CX580V, `MSGID_SET_ISO_SPEED_RATE_CMD` está inhibido por el mismo factor (`0x1f7`) que inhibe el modo de medición y el modo de color, ambas funciones de foto, mientras `SET_AGC_LIMIT`, iris y obturación pasan libres. Aunque se quite la inhibición, no hay garantía de que `av-cam` aplique ISO en película.

### Cited Findings
- [verificado] AX100/CX900: el mismo control muestra dB en película (−3 a +33 dB) y pasa a *"ISO and the range you can adjust will be ISO160 - ISO12800"* en foto. — [Adjusting the gain (ISO)](https://helpguide.sony.net/gbmig/45346551/v1/en/contents/TP0000390091.html)
- [verificado] NEX-VG30: el control se llama "Gain (ISO)" en película. — [Manual NEX-VG30H p. 49](https://www.manualsdir.com/manuals/441586/sony-nex-vg30h.html?page=49)
- [verificado] API PMCA: `setCameraGain` / `getSupportedCameraGains` / `AutoCameraGainListener` ("added in V3") separados de `setISOSensitivity` / `AutoISOSensitivityListener`. — [CameraEx$ParametersModifier.j](https://github.com/ma1co/OpenMemories-Framework/blob/HEAD/stubs/src/main/jasmin/com/sony/scalar/hardware/CameraEx%24ParametersModifier.j)
- [resumen] Los topes de AGC Limit dependen del modo: 24 dB en película y menos en foto y en Smooth Slow REC, con cifras que difieren entre versiones de la guía. Es decir, la cámara maneja límites de ganancia distintos por modo. — [AGC Limit GB](https://helpguide.sony.net/gbmig/44196361/v1/gb/contents/07/03/09/09.html)
- [proyecto] En la CX580V, `MSGID_SET_ISO_SPEED_RATE_CMD` (`0x75d0`) consulta la función `0x116`, bloqueada por el factor `0x1f7`. El mismo factor bloquea `SET_AE_METER_MODE` (`0x7588`) y `SET_COLOR_MODE` (`0x757c`). Iris (`0x75b4`), obturación (`0x75b6`), AGC Limit (`0x75ba`) y modo de exposición (`0x7558`) están libres. En `libObj.so` aparecen `PARAMID_ISO_GAIN_STATUS`, `VALUEID_IRIS_SHUTTER_GAIN_MANUAL_ON/OFF`, una variante `IRIS_SHUTTER_GAIN_ISO_MANUAL`, `PARAMID_AGC_LIMIT` y `PARAMID_AGC_LIMIT_MODE`. — `docs/research/firmware-analysis.md`, «Qué los esconde» y «Pendiente».
- [verificado] Cámaras de fotos con Linux de la misma época: la NEX-7 (2011) *"provides full control over movie exposure--even allowing adjustment during recording"* en modos P, A, S y M. Imaging Resource no dice nada del ISO en video en esa página. — [Imaging Resource, NEX-7 Video](https://imaging-resource.com/PRODS/NEX7/NEX7VIDEO.HTM)
- [resumen] La NEX-5 original grababa película en automático total, sin control de obturación, apertura ni ISO. Una actualización de firmware de 2010 añadió apertura constante, pero la cámara seguía eligiendo ISO y obturación. En la NEX-5N, si el modo de exposición lo permite, se puede cambiar obturación, apertura, ISO y compensación incluso durante la grabación. — [Luminous Landscape, firmware NEX](https://luminous-landscape.com/sony-nex-firmware-update/) · [Imaging Resource, NEX-5N Operation](https://old.imaging-resource.com/PRODS/NEX5N/NEX5NA3.HTM)
- [resumen, fuente no identificada] Una reseña de la NEX-7 diría que en video el ISO se reduce a 100–3200 y que Auto ISO no está disponible en modo Manual. El buscador no atribuyó la cita a una URL concreta, así que no hay que usarla sin confirmar. Candidatas: [dpreview NEX-7 p. 8](https://www.dpreview.com/reviews/sonynex7/8) · [What Digital Camera NEX-7](https://www.whatdigitalcamera.com/reviews/sony-nex-7/4)

### Inferences
- Lo más probable es que en el firmware de la CX580V `SET_ISO_SPEED_RATE` sea la orden del modo foto (la CX580V saca fotos). El factor `0x1f7` sería "no estamos en un contexto de foto con exposición manual". En película, la sensibilidad la decide el AE con la ganancia AGC y su tope (`AGC_LIMIT`). `PARAMID_ISO_GAIN_STATUS` sugiere un estado común que se muestra como ISO o como dB según el modo, igual que en la AX100.
- `IRIS_SHUTTER_GAIN_ISO_MANUAL` probablemente es la variante del modo manual total para foto (con ISO), e `IRIS_SHUTTER_GAIN_MANUAL` la de película (con ganancia). Esto es especulación por los nombres. La NX70, que tiene manual total en película, podría usar la segunda.
- Si se borra en memoria el bit `0x1f7` de la función `0x116` y la orden pasa, hay tres resultados posibles: (a) `av-cam` lo aplica también en película, (b) lo guarda sólo para la próxima foto, o (c) lo rechaza. Para distinguirlos hay que medir la ganancia grabada (byte `0xAF` de `ARI_DATA` en MPG, o ExifTool en AVCHD) con iris y obturación fijos y la luz constante.
- Como el modelo de UI excluye Exposure y las prioridades entre sí, el modo `IRIS_SHUTTER_GAIN_MANUAL_ON` es probablemente la vía "oficial" que usaría la NX70. Si la CX580V lo acepta, podría dar ganancia manual en película sin tocar ISO.

### Gaps
- No encontré documentación pública de Sony que diga explícitamente "ISO only applies to stills; movie uses AGC gain" para las Handycam de 2011-2013. La relación se deduce de la AX100, la VG30, la API PMCA y el firmware del proyecto.
- No encontré la fuente primaria de la restricción "ISO AUTO only in movie" en Alpha/NEX de 2011-2012 (A65, A77, NEX-7). Sólo hay resúmenes sin una URL atribuible.
- No hay datos públicos sobre `PARAMID_ISO_GAIN_STATUS` ni `IRIS_SHUTTER_GAIN_ISO_MANUAL` fuera del firmware del propio proyecto.
