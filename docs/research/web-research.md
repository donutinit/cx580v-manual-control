# Sony HDR-CX580V

Investigación y comprobaciones en la cámara del proyecto, 26 de noviembre de 2026.

## 1. Código fuente GPL/LGPL de Sony

Los manuales de la CX580V y de la PJ760V indican seleccionar **HDR-CX210E** en el portal de código fuente. Ambos remiten a `PMHOME/LICENSE` para leer las licencias de la unidad. La página de distribución publica el kernel y cinco RPM de código fuente. [Manual CX580V](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/09/05/01/01.html) · [Manual PJ760V](https://helpguide.sony.net/gbmig/44196351/v1/us/contents/09/05/01/01.html) · [Página de distribución](https://oss.sony.net/Products/Linux/DI/HDR-CX210.html)

| Paquete | Tamaño en bytes | Estado de la URL |
| --- | ---: | --- |
| [Linux](https://prodgpl.blob.core.windows.net/download/DI/HDR-CX210/linux-kernel.tar.gz) | 33 460 435 | HTTP 200 |
| [BusyBox 1.13.4](https://prodgpl.blob.core.windows.net/download/DI/common/2IcQ0W8AW0nHa8TTgS1N8A/sony-target-srel-busybox-1.13.4-06000007di1.src.rpm) | 2 110 488 | HTTP 200 |
| [dosfstools 2.11](https://prodgpl.blob.core.windows.net/download/DI/common/ZTURzbxaANJNCL_pztv+Rg/sony-target-srel-dosfstools-2.11-06000005.src.rpm) | 76 084 | HTTP 200 |
| [e2fsprogs 1.40.8](https://prodgpl.blob.core.windows.net/download/DI/common/c0jUlaf6n3Ddq7PEr_+8SA/sony-target-srel-e2fsprogs-1.40.8-06000006.src.rpm) | 4 096 703 | HTTP 200 |
| [hostname 2.95](https://prodgpl.blob.core.windows.net/download/DI/common/6xNk6Se_zMlPNVNZp8wrrA/sony-target-srel-hostname-2.95-06000005.src.rpm) | 14 523 | HTTP 200 |
| [procps 3.2.7](https://prodgpl.blob.core.windows.net/download/DI/common/f587Q5O2xaSoLv4bPE7F1g/sony-target-srel-procps-3.2.7-06000006.src.rpm) | 290 029 | HTTP 200 |

Los tamaños son los `Content-Length` de peticiones HEAD. Inspeccioné el archivo del kernel completo en un temporal. El `Makefile` declara **Linux 2.6.29.6-rt24** y la configuración incluida activa `CONFIG_ARM`, `CONFIG_ARCH_CXD4132`, `CONFIG_SMP` y `CONFIG_PREEMPT_RT`. El tarball contiene el árbol del kernel, incluidos varios perfiles `cxd4132*_defconfig`. La versión del kernel instalado y el binario que implementa los menús siguen por identificar. [Distribución Sony](https://oss.sony.net/Products/Linux/DI/HDR-CX210.html)

SHA-256 observado del tarball `94c6f24d51dfcd24eea917c0cefdde551a5e44865768bf03d76b77e6360a061f`.

La cámara del proyecto expuso `PMHOME` como FAT de 25 MiB protegido contra escritura. Comprobé `LICENSE/GPL_LGPL.TXT`, `CLIBRARY.TXT`, `LIBJPEG.TXT`, `OPENSSL.TXT` y `ZLIB.TXT`. El texto GPL/LGPL contiene las licencias y carece de un inventario por versión.

Inspeccioné `PMHOME.EXE` con 7-Zip en solo lectura. Es el instalador de PlayMemories Home para Windows. Revisé tanto el gabinete `PMB.cab` con los archivos de la aplicación como el gabinete interno de `PMB.msi`, que contiene bibliotecas de Visual C++ y un ejecutable de configuración. No apareció una imagen del firmware.

## 2. OpenMemories y compatibilidad

OpenMemories identifica la HDR-PJ590V y la HDR-PJ760V como CXD4132. Ambas figuran sin PlayMemories Camera Apps. La CX580V está ausente de la tabla. La configuración del kernel que Sony indica para este modelo apunta a CXD4132. El SoC exacto de la unidad y su región siguen pendientes de confirmar. [Tabla de OpenMemories](https://openmemories.readthedocs.io/devices.html) · [Fuentes del kernel](https://oss.sony.net/Products/Linux/DI/HDR-CX210.html)

OpenMemories: Tweak documenta un almacén de ajustes `Backup.bin` y su protección de escritura en equipos compatibles. Entre las funciones probadas en esos equipos enumera el límite de grabación, los idiomas y el selector PAL/NTSC. El proyecto sitúa el modo de servicio de Sony en el contexto de calibración. No hay evidencia de propiedades equivalentes para controlar iris, obturación o ganancia en la CX580V ni de un mapa validado de su memoria interna. [OpenMemories: Tweak](https://github.com/ma1co/OpenMemories-Tweak) · [Sony-PMCA-RE](https://github.com/ma1co/Sony-PMCA-RE)

## 3. Actualizaciones oficiales

El catálogo japonés de Sony devolvió HTTP 200 y «actualmente no hay información de actualizaciones» para HDR-CX580V, CX570E, PJ580V, PJ590V, PJ600V, XR260V, CX760V, PJ710V, PJ760V, TD20V, CX730E y PJ740VE. El enlace «información de actualizaciones» de la ficha japonesa de la PJ590V lleva a ese catálogo. [CX580V](https://support.d-imaging.sony.co.jp/www/cscs/firm/?mdl=HDR-CX580V&lang=ja) · [Ficha PJ590V](https://support.sony.jp/electronics/support/memory-camcorders-hdr-pj-series/hdr-pj590v/software/00375006) · [PJ760V](https://support.d-imaging.sony.co.jp/www/cscs/firm/?mdl=HDR-PJ760V&lang=ja) · [TD20V](https://support.d-imaging.sony.co.jp/www/cscs/firm/?mdl=HDR-TD20V&lang=ja)

Las fichas estadounidenses consultadas sólo mostraban PlayMemories Home. La búsqueda no localizó un instalador para la **versión 1.07** instalada en esta CX580V ni una imagen de fábrica de los modelos consultados. El catálogo refleja la disponibilidad actual y deja abierta la posibilidad de una distribución histórica o de servicio. [CX580V](https://www.sony.com/electronics/support/product/hdr-cx580v) · [PJ580V](https://www.sony.com/electronics/support/memory-camcorders-hdr-pj-series/hdr-pj580v) · [PJ710V](https://www.sony.com/electronics/support/product/hdr-pj710v/downloads) · [PJ760V](https://www.sony.com/electronics/support/product/hdr-pj760v/downloads)

Repetí la búsqueda con Camoufox por modelo, versión, código de producto y nombres de paquetes de actualización. Las fichas de Estados Unidos y Canadá listan PlayMemories Home y Sony Japón indica que actualmente no ofrece actualizaciones. El catálogo de Internet Archive devolvió un manual del usuario como único resultado para el modelo exacto. Algunas fichas, entre ellas la de DriverOwl, tardaron más de lo permitido por la consulta y quedaron sin comprobar directamente. [Sony EE. UU.](https://www.sony.com/electronics/support/product/hdr-cx580v/downloads) · [Sony Canadá](https://www.sony.ca/en/electronics/support/memory-camcorders-hdr-cx-series/hdr-cx580v/downloads) · [Sony Japón](https://support.d-imaging.sony.co.jp/www/cscs/firm/?mdl=HDR-CX580V&lang=ja) · [Manual en Internet Archive](https://archive.org/details/manualsbase-id-26688)

## 4. Controles de fábrica

Los ajustes principales están en `MENU → Camera/Mic → Manual Settings`. Tele Macro está en `Camera Settings`. [Menú CX580V](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/index.html) · [Menú CX760V/PJ760V](https://helpguide.sony.net/gbmig/44196351/v1/us/contents/07/index.html)

| Control | HDR-CX580V | HDR-CX760V/PJ760V |
| --- | --- | --- |
| Exposure | Ajuste general de brillo con +/− | Ajuste general de brillo, asignable al dial |
| Focus | Auto o manual cerca/lejos por pantalla táctil | Auto o manual por pantalla táctil o dial |
| White Balance | Auto, exterior, interior y One Push. La medición One Push se fija antes de grabar | Los mismos modos y White Balance Shift, asignable al dial |
| IRIS y Shutter Speed | Se muestran en los datos de reproducción. No aparecen como ajustes de captura | Prioridad de iris y prioridad de obturación, ambas asignables al dial |
| Ganancia | Aparece en los datos de reproducción | AGC Limit limita la ganancia automática de 0 a 24 dB en incrementos de 3 dB |
| AE Shift | Ausente del menú documentado | Compensación de exposición automática, asignable al dial |
| Spot Meter/Focus, Spot Meter y Spot Focus | Ajustes por punto en pantalla | Ajustes por punto en pantalla |
| Low Lux y Tele Macro | Disponibles | Disponibles |
| Acceso rápido | My Button permite asignar funciones a accesos táctiles. El esquema de controles no incluye dial MANUAL | Botón y dial MANUAL, con Focus asignado de fábrica |

[Balance de blancos CX580V](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/03/01/01.html) · [Controles CX580V](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/02/02/01/01.html) · [My Button](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/03/29/29.html) · [AGC Limit](https://helpguide.sony.net/gbmig/44196351/v1/us/contents/07/03/09/09.html) · [Dial MANUAL](https://helpguide.sony.net/gbmig/44196351/v1/us/contents/04/02/07/07.html)

**El dial de la PJ760V trabaja con modos de prioridad.** Al fijar Exposure, IRIS o Shutter Speed, se cancelan los otros dos. Además, la CX580V usa un sensor de 1/3.91 pulgadas y lente G 12×, mientras la PJ760V usa un sensor de 1/2.88 pulgadas y lente Zeiss 10×. La comparación debe incluir la lógica de exposición y las diferencias ópticas antes de asumir compatibilidad. [Funcionamiento del dial](https://helpguide.sony.net/gbmig/44196351/v1/us/contents/04/02/07/07.html) · [Especificaciones CX580V](https://pro.sony/s3/cms-static-content/operation-manual/4416298211.pdf) · [Especificaciones PJ760V](https://www.sony.jp/handycam/products/archive/HDR-PJ760V/spec.html)

Para medir el comportamiento de fábrica, la CX580V permite reproducir muestras con `MENU → Setup → Playback Settings → Data Code → Camera Data`. Ahí se pueden anotar obturación, apertura y ganancia en clips de una misma escena, variando un solo ajuste por toma. Para validar una implementación futura, cada valor fijado tendría que persistir al cambiar otro ajuste y al pasar de espera a grabación. El menú de reproducción sirve para contrastar los valores disponibles. [Data Code](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/07/06/06.html)

## 5. Servicio, hardware y estado de la unidad

Un catálogo de manuales lista documentación de servicio para CX570E/CX580E/CX580V/CX580VE/CX590V, nivel 2 v1.5 de 37 páginas y nivel 3 v1.0 de 61 páginas. Las entradas identifican los documentos. Siguen sin verificar el SoC, el controlador del lente, el mapa de particiones, la estructura de la aplicación, la consola de servicio y la recuperación de una unidad que no arranca. El manual público ubica el botón RESET y explica que inicializa ajustes, incluido el reloj. [Catálogo de manuales de servicio](https://www.serviceandusermanuals.com/085---sony-hcd-h501-to-sony-hdz.html) · [Controles y RESET](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/02/02/01/01.html)

El manual de la CX580V identifica una toma A/V Remote. Sony documenta que varios equipos de esta generación transportan LANC por ese tipo de conector. Queda por comprobar la compatibilidad de accesorios y funciones en esta unidad. El inventario público de Sony sólo acredita un posible punto de partida para controles remotos de fábrica. [Conectores de la CX580V](https://pro.sony/s3/cms-static-content/operation-manual/4416298211.pdf) · [Conectores LANC según Sony](https://www.sony.com/electronics/support/memory-camcorders-hdr-cx-series/hdr-cx380/articles/00027561)

### Observaciones por USB

1. La pantalla quedó en «Preparing…» dentro de «USB Connect». Linux registró errores de descriptor y asignación de dirección `-71` en los puertos `1-9` y `1-8`.
2. Tras el último intento del usuario, a las 13:10, la CX580V enumeró en `1-9` como `054c:05fc`, Mass Storage Hi-Speed (480 Mbit/s). No se aisló qué causó el fallo intermitente.
3. El primer LUN informó medio ausente (0 bytes). El segundo expuso el volumen `PMHOME`, montado por el sistema en solo lectura. Se comprobó la carpeta `LICENSE` por el sistema de archivos.
4. La consulta de identificación y versión confirmó el modelo HDR-CX580V, código comercial `0002408230` y firmware `1.07`. El código comercial y el PID USB identifican el producto en estas interfaces. La consulta se efectuó sin transmitir una imagen de firmware. Después se restauró el controlador Mass Storage y se comprobó de nuevo el montaje de `PMHOME` en solo lectura.
5. Al cambiar la cámara a MTP, enumeró como `054c:05fd` y gPhoto2 2.5.28 la abrió como `USB PTP Class Camera`. Consulté `--summary` y `--list-config`. La unidad anuncia descarga de archivos y ninguna operación de captura. Las propiedades anunciadas incluyen batería y valores auxiliares. No aparecieron propiedades de iris, obturación, ganancia, foco ni balance de blancos. El almacenamiento anunciado era un medio extraíble de 0 bytes.

Sony documenta dos conexiones físicas, el cable USB-A integrado y la toma mini-AB bajo la pantalla. Para reproducir la conexión estándar conviene usar una a la vez y elegir `USB Connect` desde el LCD. `Multi` es el ajuste habitual de USB LUN y `Single` está disponible como prueba de compatibilidad. [Conectores](https://pro.sony/s3/cms-static-content/operation-manual/4416298211.pdf) · [Diagnóstico USB](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/08/02/11/11.html) · [USB LUN Setting](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/07/15/15.html)

### Alimentación externa y toma DC IN

El updater de la unidad exige adaptador de corriente. La comprobación de guarda con el ID `0x21010001` y región 0 respondió «AC adapter required» con la cámara alimentada por batería y USB. El manual indica que la carga por USB funciona con la cámara apagada. [Carga con adaptador](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/03/02/01/01.html) · [Carga por USB](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/03/02/02/02.html)

Sony lista el **AC-L200** y el AC-VQV10 como «Fully compatible» con la HDR-CX580V. El manual del AC-L100/AC-L200 indica salida de **8.4 V DC**. La corriente nominal varía entre 1.5 y 1.7 A según listados de vendedores. [Compatibilidad AC-L200](https://support.d-imaging.sony.co.jp/www/cscs/accessories/detail.php?area=us&lang=en&value1=HDR-CX580V&value2=AC-L200) · [Manual AC-L100/AC-L200](https://www.sony.jp/ServiceArea/impdf/pdf/42896871M-JP.pdf)

No apareció un dibujo oficial de la clavija ni el esquema de la toma de la CX580V. Como referencia sirve el manual de servicio nivel 2 de la **DCR-HC40**, que usa el AC-L25A/B. Los listados de repuestos tratan AC-L25 y AC-L200 como intercambiables; esa equivalencia de clavija es una inferencia. [Servicio DCR-HC40](https://schematicsforfree.com/files/Video/Camcorders/Sony%20Dcrhc40.pdf)

| Terminal de `J901 DC IN` en la HC40 | Señal | Función |
| --- | --- | --- |
| Pin central | `ACV_UNREG` | Positivo de la entrada |
| Casquillo | `ACV_GND` | Negativo, a tierra a través de un filtro |
| Contacto de interruptor | `BATT/XEXT` | Llega al micro de control (`HI CONTROL`, pin 77) |

El mismo micro tiene una entrada `ACV SENS` con divisor resistivo, así que en la HC40 la cámara dispone de dos indicios de alimentación externa: el voltaje en la entrada y el interruptor mecánico de la toma. El símbolo dibuja el contacto del interruptor tocando el casquillo, lo habitual para un contacto cerrado en reposo. [Esquema de J901](img/hc40-j901-dc-in-schematic.png) · [Diagrama de bloques](img/hc40-j901-dc-in-block.png) · [Entradas del micro](img/hc40-hi-control-acv-sens.png)

En la CX580V el usuario localizó tierra y otros dos contactos en la toma, lo que cuadra con esa disposición de tres terminales. Midió con multímetro, sin alimentación:

| Entre | Resistencia |
| --- | ---: |
| Tierra (G) y pin central | 146 kΩ |
| Pin central y el contacto contiguo a G | 92 kΩ |
| G y el contacto contiguo | 238 kΩ |

146 + 92 = 238, así que las tres lecturas describen una cadena `G — 146 kΩ — pin central — 92 kΩ — tercer contacto`. El tercer contacto sólo tiene camino resistivo hacia el pin central y está abierto respecto a tierra en reposo, a diferencia del símbolo de la HC40. Los 146 kΩ a tierra son compatibles con un divisor de medida en la entrada, como el `ACV SENS` de la HC40. Interpretación pendiente de prueba: el pin central es la entrada positiva y el tercer contacto es una línea de habilitación o detección referida a la entrada, que la clavija lleva a tierra al entrar. Falta comprobar con la fuente si la cámara reconoce el adaptador con el tercer contacto libre o puenteado a tierra.

## 6. Cámaras emparentadas

### Placa VC-653 y repuestos

En la fotografía del paso 31 del desmontaje de iFixit se distingue la serigrafía **VC-653** en la placa de una HDR-CX580V. El catálogo de Sears para ese modelo incluye un despiece del conjunto óptico y lista el flexible FP-1352 como pieza común a CX580V, PJ580V, PJ260V y XR260V. Para la **PJ580V**, el mismo catálogo identifica `A1861127A` como placa VC-653 y `A1852708A` como repuesto que la sustituye. Queda pendiente el número de conjunto exacto de la CX580V. [Desmontaje iFixit, paso 31](https://www.ifixit.com/Guide/Sony+Handycam+HDR-CX580V+Motherboard+Replacement/87910#s166990) · [Foto de la placa](https://guide-images.cdn.ifixit.com/igi/J3soWElxwYtpM56w.huge) · [Despiece CX580V](https://www.searspartsdirect.com/model/4w1uflxxjp-000996/sony-hdr-cx580v-digital-camcorder-parts) · [FP-1352](https://www.searspartsdirect.com/product/2gy6kfcako-0003-474/id-a1864823a) · [VC-653 PJ580V](https://www.searspartsdirect.com/product/20nlmlazrg-0003-474/id-a1861127a)

Los manuales de servicio de **PJ260/PJ260V y XR260V** publican la lista de piezas y el diagrama de bloques de la VC-653. El diagrama agrupa CPU, procesamiento audiovisual y control del lente en IC1301 y muestra una interfaz LANC para A/V Remote. Sony distingue los conjuntos de servicio `A-1861-125-A` para PJ260 y `A-1861-124-A` para XR260. También documenta ajustes regionales y datos propios de la unidad al cambiar la placa. La serigrafía compartida sirve para buscar circuitos equivalentes. El número de conjunto y las calibraciones importan al evaluar un repuesto. [Servicio Nivel 3 PJ260/XR260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor) · [Servicio Nivel 2](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/21myl) · [Despiece del lente, p. 18](https://www.manualslib.com/manual/864065/Sony-Hdr-Pj260.html?page=18)

En las páginas 10 y 12 del catálogo de la CX580V, consultadas con Camoufox, aparecen la placa de lente **LD-279** `A1862435A` y el conjunto óptico **LSV-1400A** `A1733588A`. Los despieces de la PJ260V/XR260V también identifican una LD-279, con otros números de repuesto y un conjunto óptico LSV-1450A. Esto permite priorizar la comparación del circuito de control del lente y preservar las diferencias de óptica y calibración. [LD-279 en CX580V](https://www.searspartsdirect.com/model/4w1uflxxjp-000996/sony-hdr-cx580v-digital-camcorder-parts?page=10) · [LSV-1400A en CX580V](https://www.searspartsdirect.com/model/4w1uflxxjp-000996/sony-hdr-cx580v-digital-camcorder-parts?page=12) · [Despiece PJ260V/XR260V](https://www.manualslib.com/manual/864065/Sony-Hdr-Pj260.html?page=18)

El diagrama de la PJ260 y los proyectos de control remoto LANC dan una pista para probar funciones de grabación, zoom y enfoque mediante un accesorio externo. Falta confirmar la compatibilidad de la CX580V con un controlador concreto y qué órdenes acepta. El nivel de soporte cambia según el modelo. [Diagrama LANC en el manual de la PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor) · [Proyecto Arduino LANC](https://github.com/AlexNe/arduino_lanc_sample)

Sony confirma que la CX580V es compatible con el VCT-60AV. El manual del mando enumera grabación, foto, zoom, encendido y cuadrícula. También lista para la CX580V el GP-AVT1, cuyo mando ofrece grabación, foto y zoom. Ninguno documenta mandos de exposición. Sony documenta MTP como modo de transferencia en la CX580V. La cámara queda fuera de los modelos que figuran en Camera Remote Toolkit, y la consulta de la unidad en MTP confirmó que sus propiedades de cámara no exponen los ajustes de captura. [VCT-60AV en la CX580V](https://support.d-imaging.sony.co.jp/www/cscs/accessories/detail.php?area=us&lang=en&value1=HDR-CX580V&value2=VCT-60AV) · [Manual VCT-60AV](https://www.manualowl.com/m/Sony/VCT-60AV/Manual/358481?page=2) · [Compatibilidad GP-AVT1](https://support.d-imaging.sony.co.jp/www/cscs/accessories/detail.php?area=us&lang=en&value1=HDR-CX580V&value2=GP-AVT1) · [Manual GP-AVT1](https://www.sony.jp/ServiceArea/impdf/pdf/41466450M-JP.pdf) · [Modo MTP](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/07/14/14.html) · [Camera Remote Toolkit](https://pro.sony/en_CA/digital-imaging/camera-remote-toolkit)

### Referencias para el control manual

| Familia | Evidencia | Aplicación al proyecto |
| --- | --- | --- |
| **CX580V / PJ580V / PJ260V / XR260V** | Serigrafía VC-653 en la CX580V, catálogo de la PJ580V y manuales de servicio de PJ260V/XR260V. [Foto](https://guide-images.cdn.ifixit.com/igi/J3soWElxwYtpM56w.huge) · [Manual](https://www.manualslib.com/manual/864065/Sony-Hdr-Pj260.html?page=18) | Familias prioritarias para comparar esquemas y referencias de piezas. |
| **CX760V / PJ760V / HXR-NX30** | Los esquemas identifican la familia de placa **VC-658**. Los manuales del dial describen la misma exclusión entre exposición, iris y obturación. Un dueño de NX30U reporta la misma limitación en DV Info Net. [Servicio CX760/PJ760](https://www.manualslib.com/manual/992902/Sony-Hdr-Cx720v.html) · [Servicio NX30](https://www.manualslib.com/manual/3893802/Sony-Nx30u.html) · [Manual de uso NX30, p. 40](https://manualzz.com/doc/19383775/sony-hxr-nx30p-operating-guide) · [Reporte del dueño](https://www.dvinfo.net/forum/sony-4k-ultra-hd-handhelds/531451-new-sony-ax-53-testing-2-print.html) | Referencia de menús y modos de prioridad. La placa principal difiere de la CX580V. |
| **HXR-NX70E/U** | Los manuales documentan ajustes independientes de iris, ganancia y obturación con el selector en MAN y visualización simultánea de los tres valores. El manual de servicio identifica la placa **VC-633**. Un dueño de NX70U y CX760V describe en DV Info Net cómo consulta la ganancia de la CX760V durante la reproducción. [Operación NX70E](https://pro.sony/s3/cms-static-content/operation-manual/4273940131.pdf) · [Servicio NX70](https://www.manualslib.com/manual/1815448/Sony-Hxr-Nx70c.html) · [Prueba del dueño](https://www.dvinfo.net/forum/archive/index.php/t-507570.html) | Referencia de cómo se ajustan y muestran los parámetros. |

La NX70 recibió una actualización en 2012. Los reportes de instalación de DV Info Net llevaron a buscar el paquete en otras regiones. La ficha de la **NX70E v3.00** se abrió con Camoufox y lista ambos sistemas operativos. En Sony Japón encontré las páginas de descarga de la **HXR-NX70J**. Comprobé mediante HEAD que los archivos que enlazan siguen disponibles y coinciden con el tamaño publicado. [Ficha NX70E](https://www.sony.com/electronics/support/software/mpengb00000321) · [Hilo histórico](https://www.dvinfo.net/forum/sony-nxcam-avchd-camcorders/505775-nx70-firmware-release-date-3.html) · [Anuncio NX70J](https://www.sony.jp/xdcam/update/HXR-NX70J.html)

| Plataforma | Archivo que lista Sony para NX70J | Tamaño | Estado |
| --- | --- | ---: | --- |
| [Windows](https://www.sony.jp/xdcam/update/HXR-NX70J_win.html) | `Update_NXCAMNX70V3.exe` | 51 790 640 bytes | HTTP 200 |
| [Mac](https://www.sony.jp/xdcam/update/HXR-NX70J_mac.html) | `Update_NXCAMNX70V3.dmg` | 49 417 815 bytes | HTTP 200 |

### Código fuente publicado para la NX70

El manual de la NX70E dirige al paquete GPL de la **NEX-FS100**, publicado en el catálogo profesional. Inspeccioné el tarball de 32 653 519 bytes, SHA-256 `984523cd5041b1eddc37328d24202ced8656abd5fc92391ec4811a339c5c62a3`. Su `Makefile` declara **Linux 2.6.29.6-rt24** y su `.config` selecciona **ARM CXD4132**, igual que el paquete HDR-CX210E que indica el manual de la CX580V. [Manual NX70E, notas de licencia](https://pro.sony/s3/cms-static-content/operation-manual/4273940131.pdf) · [Fuentes NEX-FS100](https://oss.sony.net/Products/Linux/B2B/NEX-FS100.html) · [Fuentes HDR-CX210E](https://oss.sony.net/Products/Linux/DI/HDR-CX210.html)

Comparé los archivos regulares por ruta y SHA-256. Las 361 entradas de `drivers/media` y `drivers/video` son idénticas en ambos paquetes. En el código específico de la plataforma hay diferencias.

| Directorio | Idénticos | Distintos | Sólo en CX210E |
| --- | ---: | ---: | ---: |
| `arch/arm/mach-cxd4132` | 40 | 24 | 9 |
| `arch/arm/plat-cxd41xx` | 32 | 17 | 4 |

La coincidencia permite estudiar una base de Linux común entre estas cámaras. La configuración incluida y el código de plataforma difieren. Los paquetes GPL no identifican la capa de la aplicación que implementa los modos de exposición.

### Medición en clips AVCHD

Felix Holderer publicó archivos MTS originales de una **HXR-NX70E**. Consulté los primeros 2 MiB de tres clips con HTTP Range y leí sus metadatos H.264 con ExifTool 13.50. [Prueba y clips originales](https://www.film-tv-video.de/equipment/2011/10/29/steht-einer-im-regen-und-filmt-test-hxr-nx70/) · [Código del decodificador H.264](https://github.com/exiftool/exiftool/blob/master/lib/Image/ExifTool/H264.pm)

| Clip | Apertura grabada | Obturación indicada | Ganancia | Programa |
| --- | ---: | ---: | ---: | --- |
| [1080/50p PS](https://www.film-tv-video.de/wp-content/uploads/2016/02/01_NX70_50p_PS.MTS) | f/5.6 | 1/302 s | 0 dB | Manual |
| [1080/50i LP](https://www.film-tv-video.de/wp-content/uploads/2016/02/05_NX70_50i_LP.MTS) | f/5.6 | 1/611 s | 0 dB | Manual |
| [1080/25p FX](https://www.film-tv-video.de/wp-content/uploads/2016/02/06_NX70_25p_FX.MTS) | f/5.6 | 1/611 s | 0 dB | Manual |

```bash
exiftool -ee -a -s -G1 -Model -FNumber -ExposureTime -Gain \
  -ExposureProgram -ApertureSetting -WhiteBalance clip.MTS
```

Los tres clips registran `ApertureSetting=Auto` junto con `ExposureProgram=Manual`. Para comparar tomas conviene guardar el valor efectivo (`FNumber`) y el modo que reporta cada etiqueta. Las muestras son de archivo y sus parámetros de captura no se cambiaron mediante un protocolo controlado.

El comando del proyecto genera una tabla TSV con los valores distintos que ExifTool lee del H.264 de cada clip. Sólo extrae los campos necesarios para comparar la exposición, sin incluir GPS ni otros datos del archivo.

```bash
.venv/bin/python tools/cxexposure.py auto.MTS manual-bajo.MTS manual-alto.MTS low-lux.MTS
```

Comprobé que resume los tres ejemplos NX70E y devuelve un error cuando un archivo falta o carece de los campos de exposición. Los campos que falten en un clip se muestran con una raya. Las etiquetas repetidas con diferentes valores indican que el ajuste varió dentro de los datos leídos.

### Primera prueba en la HDR-CX580V

Descargué los cuatro clips nuevos de la SD por MTP y los guardé en `captures/exposure-test/`. La SD de 64 GB figuró como `/store_00010001` y los cuatro archivos se transfirieron desde la carpeta de la fecha que tenía la cámara. FFprobe identifica MPEG-2 Program Stream, 720 × 480 a 30000/1001 fotogramas por segundo y audio AC-3. La cámara estaba en `STD Quality`. La lectura de metadatos AVCHD en la CX580V queda pendiente. [Ajuste HD/STD de la CX580V](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/04/03/03.html)

Los cuatro originales contienen registros Sony `MVI_DATA` y `ARI_DATA` en paquetes PES de tipo `0xBD`, subflujo `0xFF`. ExifTool no mostró etiquetas de exposición en estos MPG. La fecha y hora del primer `ARI_DATA` de cada clip coinciden con su nombre al interpretar los cuatro bytes en `0xA3` como un entero big-endian, con el año desde 1984 y campos de 6, 4, 5, 5, 6 y 6 bits para año, mes, día, hora, minuto y segundo. En las muestras de la DCR-SR72E del autor del análisis, el campo equivalente comienza en `0xA4`. [Análisis de los originales DCR-SR72E](https://forum.videohelp.com/threads/274961-Overlay-Time-Date-Code-On-MPEG-2-File/page2)

| MPG | Duración | Registros de cada tipo | Luma central mediana | `0xA8` | `0xAC` | `0xAE` |
| --- | ---: | ---: | ---: | --- | --- | --- |
| `20261127134044` | 5.005 s | 10 | 96.0 | `00b4` | `007d` | `0106` |
| `20261127134111` | 5.519 s | 11 | 104.8 | `00b4` | `005a`, `0064` | `0206` |
| `20261127134129` | 4.519 s | 9 | 17.0 | `0190` | `00fa` | `0100` |
| `20261127134156` | 7.014 s | 14 | 85.2 | `00b4` | `005a`, `0064`, `007d` | `0206` |

La luma es la mediana de `YAVG` de FFmpeg en los 97 fotogramas del intervalo 0.5–3.7 s, medida en la mitad central del ancho y alto. Los campos hexadecimales muestran todos los valores distintos en cada clip, con offsets relativos al inicio de `ARI_DATA`.

#### Cotejo con la pantalla de la cámara

Copié las cuatro fotos originales de `/mnt/carrera/scratchpad/sonystills/` a `captures/exposure-test/data-code/` y comprobé que son idénticas. Los encuadres y los valores de reproducción permiten asociarlas con los MPG. La primera foto también muestra el número de archivo `100_0145` a los dos segundos. Los valores de la tabla se transcribieron de `MENU → Setup → Playback Settings → Data Code → Camera Data`. [Data Code de la CX580V](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/07/06/06.html)

| MPG | Foto de Data Code | Modo | Iris | Obturación en la foto | Ganancia |
| --- | --- | --- | ---: | ---: | ---: |
| `20261127134044` | [IMG_0934.JPG](../../captures/exposure-test/data-code/IMG_0934.JPG) | Manual | f/1.8 | 1/125 s | 6 dB |
| `20261127134111` | [IMG_0935.JPG](../../captures/exposure-test/data-code/IMG_0935.JPG) | Auto | f/1.8 | 1/100 s | 6 dB |
| `20261127134129` | [IMG_0937.JPG](../../captures/exposure-test/data-code/IMG_0937.JPG) | Manual | f/4.0 | 1/250 s | 0 dB |
| `20261127134156` | [IMG_0938.JPG](../../captures/exposure-test/data-code/IMG_0938.JPG) | Auto | f/1.8 | 1/100 s | 6 dB |

Las dos tomas Manual tienen combinaciones distintas de iris, obturación y ganancia. En ambas tomas Auto, `1/100` coincide con la foto y los demás registros muestran que la obturación varió durante la grabación. El encuadre también cambia dentro y entre tomas. La serie permite validar los metadatos, pero el efecto de Low Lux queda por aislar. Sony documenta el ajuste manual de Exposure como un control general de brillo. [Exposure](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/03/05/05.html) · [Low Lux](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/03/07/07.html)

#### Campos verificados y lectura reproducible

| Offset relativo a `ARI_DATA` | Campo | Codificación verificada |
| --- | --- | --- |
| `0xA8–0xA9` | Iris | Entero big-endian en centésimas. `00b4` es f/1.8 y `0190` es f/4.0 |
| `0xAA–0xAB` | Campo adyacente al de obturación | `0001` en los cuatro originales |
| `0xAC–0xAD` | Obturación | Entero big-endian como denominador en segundos con `0xAA–0xAB = 0001`. `0064` es 1/100 s, `007d` es 1/125 s y `00fa` es 1/250 s |
| `0xAE` | Modo de exposición | `01` Manual, `02` Auto |
| `0xAF` | Ganancia | Valor en dB para los códigos observados `00` y `06` |

`tools/cxexposure.py` lee los paquetes originales con la firma `ARI_DATA` seguida de `01 05 ff ff SONY 00` y el modelo `HDR-CX580V` en `0x18B`. El TSV lista los valores distintos por orden de aparición. Los MPG se leen sin ExifTool. Para AVCHD el comando usa las etiquetas H.264 de ExifTool. La segunda prueba añade el balance de blancos y el modo de enfoque en STD.

```bash
.venv/bin/python tools/cxexposure.py captures/exposure-test/*.MPG
```

Sumas SHA-256 desde `captures/exposure-test/`.

```text
d66aaf2e1f086a2b41af4fbdbdc8a934c79454399ef778fa387eab9019934d70  20261127134044.MPG
03f310fed3ab986fcfb88dc69bf93618b46a2e10e3deb4e7a24c5ca2191d97af  20261127134111.MPG
51dba9f80c623d2f9f782cfba57b5941956867cc63d0f58aff59fa43bac0252c  20261127134129.MPG
78eef6898358572666abdf792c5c645916aef17ca4660295bee6fe585cb3a185  20261127134156.MPG
10993482756c1af1c4e290a799b0fe6c30f18374eb5464b34ef8b6717b9949e5  data-code/IMG_0934.JPG
413f617d41ad24ddafd495fc96c6bb93981255dadbec432c55c7357f9e0ca978  data-code/IMG_0935.JPG
dff852a4ebbe4c6dd372a83229d41141d6e7fb452300da9180e8b4c2484d9dd4  data-code/IMG_0937.JPG
8e68899ec935b2ce193bcd782d8745209be6cf67c6bb4c9513e0bd80c314205c  data-code/IMG_0938.JPG
```

### Cadencia de ARI_DATA y campos de color y foco

La patente de Sony describe un bloque `ARI_DATA` asociado al fotograma I de cada GOP y la figura 6 incluye un ejemplo de su estructura en grabación DVD. `F_NUM`, `EXP_TM` y `EXP_PRG` del ejemplo ocupan los bytes 72, 74–77 y 78. Esa secuencia coincide con los campos verificados en los MPG de la CX580V al sumar `0x60`. Los campos siguientes tienen otra disposición. El byte de ganancia está en `0xAF` en los originales y la misma suma lo situaría en `0xB2`. [Patente Sony US7962002B2, figuras 5 y 6](https://patents.google.com/patent/US7962002B2/en)

Crucé las posiciones de los paquetes ARI con `pkt_pos` y `best_effort_timestamp_time` de FFprobe. Los cuatro archivos tienen exactamente un registro por fotograma I, siempre antes del paquete de video correspondiente. Los fotogramas I consecutivos están separados por 15 fotogramas, alrededor de 0.5005 s. En el segundo clip, el registro cambia de 1/100 a 1/90 alrededor de 2.002 s y vuelve a 1/100 en 4.004 s. En el cuarto, los primeros cambios a 1/100 y 1/90 aparecen en 2.002 y 4.505 s. Los dos clips Manual mantienen los mismos valores en toda la muestra. Los tiempos son relativos al primer fotograma I de cada clip.

```bash
ffprobe -v error -select_streams v:0 -skip_frame nokey -show_frames \
  -show_entries frame=best_effort_timestamp_time,pkt_pos,pict_type -of json clip.MPG
```

La documentación de Sony incluye campos de modo de balance de blancos, modo de foco y distancia al sujeto en esta familia de estructuras. Las cuatro fotos de Data Code muestran el mismo icono One Push. En los 44 registros, el candidato `0xB8` vale `02`, y los bytes `0xBE–0xBF` varían entre `000a` y `0050`. Hay que cotejar esas posiciones con tomas de ajustes conocidos para asignarles un significado. [Patente Sony US7796862B2, figura 8](https://patents.google.com/patent/US7796862B2/en) · [Iconos y modos de balance de blancos](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/03/01/01.html)

### Segunda prueba en la cámara

Grabar cinco clips de unos cinco segundos en la SD con la cámara apoyada, encuadre fijo, zoom fijo y la misma luz tenue. Dejar Exposure en Auto. WB, Focus y Low Lux están en `MENU → Camera/Mic → Manual Settings`. HD y STD se eligen en `MENU → Image Quality/Size → HD/STD Setting`. [Exposure](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/03/05/05.html) · [Focus](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/03/06/06.html) · [Low Lux](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/03/07/07.html) · [HD/STD Setting](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/04/03/03.html)

| Toma | Formato | White Balance | Focus | Low Lux |
| --- | --- | --- | --- | --- |
| 1 | STD | Auto | Auto | Off |
| 2 | STD | Indoor | Auto | Off |
| 3 | STD | Indoor | Manual hacia lejos, con el objeto de prueba fijo | Off |
| 4 | HD 60i | Indoor | Auto | Off |
| 5 | HD 60i | Indoor | Auto | On |

Las tomas 1–2 aíslan balance de blancos en STD. Las tomas 2–3 aíslan el modo de foco. Las tomas 4–5 prueban Low Lux y si la CX580V escribe en AVCHD las etiquetas que ya se leyeron en la NX70E. Sony especifica 6 lx a 1/60 s con el ajuste de fábrica y 3 lx a 1/30 s con Low Lux. La obturación y la ganancia efectivas de la prueba se medirán a partir de los originales. [Manual de Sony, especificaciones p. 84](https://pro.sony/s3/cms-static-content/operation-manual/4416298211.pdf#page=84)

Los originales de la segunda serie están en `captures/color-focus-low-lux/`. La cámara presenta por MTP los videos del formato seleccionado. Las dos tomas HD se transfirieron en HD Quality y las cinco tomas STD nuevas aparecieron al seleccionar STD Quality y reconectar.

#### Resultados de las dos tomas HD

En HD, el dispositivo MTP mostró las dos tomas `.MTS` de la SD. Las copias coinciden en tamaño con los objetos MTP y se decodificaron desde el inicio sin errores. FFprobe indica H.264 en MPEG-TS, 1920 × 1080 entrelazado con campo superior primero. ExifTool 13.50 identificó la cámara como `HDR-CX580V` y leyó iris, obturación, ganancia, exposición, balance de blancos y foco en los metadatos H.264.

| Archivo | Ajuste previsto | Duración | Iris | Obturación | Ganancia | Luma central |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `20261127140833.MTS` | Low Lux Off | 4.032 s | f/1.8 | 1/60 s | 6 dB | 82.19 |
| `20261127140856.MTS` | Low Lux On | 3.008 s | f/1.8 | 1/60 s | 9 dB | 88.50 |

En esta escena la toma con Low Lux registró 3 dB más de ganancia, con el mismo iris y tiempo de exposición. La luma es la mediana por cuadro de `YAVG` en el cuarto central de la imagen entre 0.5 y 2.5 s, con 60 cuadros por archivo. En ambos clips ExifTool reportó `Program AE`, `Focus = Auto (0.2)` y `WhiteBalance = Daylight` (código `3`). El ajuste previsto para las tomas era Indoor. Queda pendiente comprobar cómo se corresponde la etiqueta H.264 con el menú de la cámara.

```bash
.venv/bin/python tools/cxexposure.py captures/color-focus-low-lux/*.MTS
exiftool -ee -a -s -G1 -H264:WhiteBalance -H264:Focus captures/color-focus-low-lux/*.MTS
```

#### Resultados de las cinco tomas STD

Los cinco originales nuevos son MPEG-2 entrelazado de 720 × 480. En cada archivo los metadatos de la tabla se mantuvieron estables. El iris fue f/1.8 y la obturación 1/60 s en los cinco. Las dos parejas Auto/Indoor tienen el mismo iris, obturación y ganancia dentro de cada pareja. La imagen cambia de un tono neutro a uno azulado al cambiar a Indoor. La última toma conserva Indoor y los parámetros de exposición de la cuarta, y el objeto cercano queda desenfocado.

| MPG | Duración | Registros ARI | Ganancia | WB en `0xB8` | Foco en `0xBA` | `0xBC–0xBF` |
| --- | ---: | ---: | ---: | --- | --- | --- |
| `20261127140557` | 3.510 s | 7 | 9 dB | `00` Auto | `00` Auto | `00000014` |
| `20261127140620` | 4.526 s | 9 | 9 dB | `11` Indoor | `00` Auto | `00000014` |
| `20261127140641` | 3.518 s | 7 | 6 dB | `00` Auto | `00` Auto | `00000014` |
| `20261127140651` | 3.533 s | 7 | 6 dB | `11` Indoor | `00` Auto | `00000014` |
| `20261127140731` | 6.520 s | 13 | 6 dB | `11` Indoor | `01` Manual | `ffffffff` |

Los cambios en `0xB8` asignan `00` a Auto y `11` a Indoor. Los cuatro originales de la primera prueba conservan `02` con el icono One Push. El cambio en `0xBA` asigna `00` a foco Auto y `01` a Manual. El campo en `0xBC–0xBF` es candidato a distancia de enfoque. En Auto, `0x14` equivale a 20 en decimal y ExifTool mostró `Auto (0.2)` en los dos clips HD de la misma escena. Al enfocar hacia lejos en STD el campo pasó a `ffffffff`. La unidad de medida y el significado de ese valor quedan por verificar. El lector incluye el modo de foco STD y las tres asignaciones de WB verificadas.

El tono de las dos tomas HD coincide visualmente con el de las tomas STD en Indoor. El lector presenta la etiqueta `Daylight` que entrega ExifTool en HD y la asignación validada `Indoor` en STD. Para confirmar los nombres de balance de blancos en HD hace falta una serie AVCHD que cambie expresamente entre modos.

```bash
.venv/bin/python tools/cxexposure.py captures/color-focus-low-lux/*.{MPG,MTS}
```

Sumas SHA-256 desde `captures/color-focus-low-lux/`.

```text
760907902cc28bd4b4b572f253557da3f3af3e0ccc7598bb54866581cba5df5a  20261127140557.MPG
88c2a015b10092433af8d66dae61026cb79ba9e4607e01caf0402b36933ea131  20261127140620.MPG
34e5e684e6752df5c3d38e6622bb4572e1099f44fd97fac3d8cd223c7e04da75  20261127140641.MPG
0d19b383f556c8f5276b127436aaa3b49792b886447936eef1e33a16fe56e7b3  20261127140651.MPG
b363b765951e1d66affd1ad47d058158c33429d23dc9a0621ef248a10a60a933  20261127140731.MPG
f42a92cebe7acf68c71579e64a748ec2586565fa7e634de25efe62814308a58e  20261127140833.MTS
6a55686fe915b667236af8bf8496e59d6c477f62d080f9e830a60fa69557a17a  20261127140856.MTS
```
