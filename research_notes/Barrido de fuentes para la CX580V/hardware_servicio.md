# Hardware, servicio y control remoto de la HDR-CX580V y su familia (CXD4132, placa VC-653)

Barrido del 26 y 27 de noviembre de 2026. Todo lo descargado está en
`vendor/barrido-fuentes/r6/`
(abreviado `r6/` abajo). Ahí no hay nada ejecutable de terceros salvo el tarball GPL del kernel.
Archivos que vale la pena conservar:

| Archivo en `r6/` | Qué es |
| --- | --- |
| `et_prev/cx570_l2_p1.png`, `cx570_l2_p2.png` | Portada y especificaciones del L2 Ver. 1.5 de la familia CX570E…PJ600VE |
| `et_prev/cx570_l3_p1.png`, `cx570_l3_p2.png` | Portada (con «MODEL GROUP») y p. 2 del L3 Ver. 1.0 de la misma familia |
| `mdu_pj260_l2.txt` | Texto del L2 de PJ260/XR260 (VC-653): notas de servicio y tabla completa de autodiagnóstico |
| `mdu_pj260_l3.txt`, `mdu_pj260_l3.html`, `pj260.css`, `pj260_bd1_items.json` | L3 de PJ260/XR260: lista de piezas y texto con coordenadas del diagrama de bloques general |
| `mdu_cx720.txt` | L3 de CX720V…PJ760VE (placa VC-658): piezas y diagrama de bloques |
| `servlib_sm4.html` | Copia de Wayback de la primera página del manual ADJ de la familia CX580 |
| `dvx/hv4sz.jpg`, `dvx/8WoJS.jpg` | Pinout del conector de servicio CN3504 de la NEX-VG10 y pads JTAG de la NEX-3/5 (dvxuser) |
| `nex7/*.md` | Documentos de reparación y UART de knowXD para la NEX-7 |
| `nx/nx70_cam.pdf`/`.txt`, `nx/nx30j.pdf`/`.txt` | Manuales de operación de la HXR-NX70E/P y la HXR-NX30J |
| `om_hardware.rst` | `hardware.rst` de OpenMemories-Docs |
| `pmca/sony.py`, `pmca/senser.py`, `pmca/README.md` | Implementación pública del protocolo de servicio «Senser» (Sony-PMCA-RE) |
| `gpl/k.tgz` y `gpl/linux/` | Kernel GPL de la HDR-CX210 (misma plataforma) y los archivos extraídos de `mach-cxd4132`, `plat-cxd41xx`, `setup.c`, `drivers/serial` y `.config` |
| `sony_lanc.txt` | Artículo de Sony «What is a LANC jack?» |

Notación: **[V]** leído en la fuente, con URL y página. **[L]** comprobado en el firmware local
(`firmware/cx580v-1.07/unpacked/`, sólo `strings`, sin ejecutar nada). **[I]** inferencia mía.
Nada de esto se probó en la cámara.

## 1. Manuales de servicio: qué existe, dónde y qué trae

### Takeaway
Los manuales de servicio de la familia exacta de la CX580V existen en público y gratis en
elektrotanya: L2 Ver. 1.5 de 37 páginas y L3 Ver. 1.0 de 61 páginas. El L3 trae los esquemas
de la VC-653, pero no lo pude bajar, porque la descarga pasa por un Cloudflare Turnstile que
tiene que resolver una persona. Las notas de servicio (L2) y la lista de piezas y el diagrama
de bloques de la VC-653 (L3 de la PJ260) sí se leen gratis en manualesdeusuario.es. El software
de ajuste se llama **Adjust Station** + **Adjust Manual** («HDR-CX580V_Series_AM»). Sony lo
reparte por su portal de servicio TISS y exige una llave HASP.

### Cited Findings
- **[V] L2 de la familia CX580V.** Título: «HDR-CX570E/CX580E/CX580V/CX580VE/CX590V/PJ580/PJ580E/PJ580V/PJ580VE/PJ590V/PJ600E/PJ600V/PJ600VE SERVICE MANUAL LEVEL 2», Ver. 1.5, 2013.05, Revised-5. Documento **9-834-634-36**, que reemplaza a 9-834-634-35. Archivo `983463436.pdf`, 37 páginas, 7.6 MB. Índice de notas de servicio: 1-1 alimentación durante la reparación, 1-2 cambio de la VC-653, 1-3 archivo Destination Data, 1-4 MS-478, 1-5 lente, 1-6 autodiagnóstico, 1-7 error del lente de desplazamiento, 1-8 GPS, 1-9 LD-279, 1-10 HDD, 1-11 descarga del flash LED y 1-12 unidad de proyector. Las versiones 1.1 a 1.5 cambian notas de servicio, vistas despiezadas y accesorios. — [elektrotanya, página de descarga del L2](https://elektrotanya.com/sony_hdr-cx570_hdr-cx580_hdr-cx590_hdr-pj580_hdr-pj590_hdr-pj600_l2_ver1.5.pdf/download.html) (vista previa p. 1, `r6/et_prev/cx570_l2_p1.png`)
- **[V] L3 de la familia CX580V.** «SERVICE MANUAL LEVEL 3», Ver. 1.0, 2011.12, documento **9-834-634-11**, archivo `983463411.pdf`, 61 páginas, 6.1 MB, categoría CAMCORDER. En el «MODEL GROUP» de la portada, la CX580V figura como Non Projector Model, Non HDD Model, IM Model (memoria interna) y GPS Model. — [elektrotanya, página de descarga del L3](https://elektrotanya.com/sony_hdr-cx570_hdr-cx580_hdr-cx590_hdr-pj580_hdr-pj590_hdr-pj600_l3_ver1.0.pdf/download.html) (vista previa p. 1, `r6/et_prev/cx570_l3_p1.png`)
- **[V] Cómo se baja.** La página de elektrotanya es gratuita, con un límite de 10 archivos por día, pero el botón pasa por Cloudflare Turnstile antes de llegar a `/cgi-bin/download2.cgi` (en el L2, `fid=344046`). No intenté saltarlo. — [HTML de la página del L2](https://elektrotanya.com/sony_hdr-cx570_hdr-cx580_hdr-cx590_hdr-pj580_hdr-pj590_hdr-pj600_l2_ver1.5.pdf/download.html)
- **[V] Manuales hermanos en elektrotanya.** PJ260/XR260: L3 Ver. 1.0 y L2 Ver. 1.1 a 1.5. CX720V…PJ760VE: «VER.1.0 LEVEL3 FULL», L3 Ver. 1.0 y L2 Ver. 1.3 con «SUPPLEMENT-1». HXR-NX30C/E/J/N/P/U: un manual «VER1.1» sin nivel indicado. — [listado PJ260](https://elektrotanya.com/showresult?what=sony+hdr-pj260&kategoria=camcorder&kat2=all) · [listado CX720](https://elektrotanya.com/showresult?what=sony+hdr-cx720&kategoria=camcorder&kat2=all) · [listado NX30](https://elektrotanya.com/showresult?what=sony+hxr-nx30&kategoria=camcorder&kat2=all)
- **[V] Lectura gratis sin descarga.** manualesdeusuario.es publica el texto del L2 de PJ260/XR260 (Ver. 1.2 y 1.3 en el historial) y del L3 de PJ260/XR260 (`983463611.pdf`, Ver. 1.0, 2011.12) con lista de piezas y diagrama de bloques general (4 hojas). También el L3 de CX720V…PJ760VE (`983465611.pdf`, Ver. 1.0, 2012.02, placa VC-658), con lista de piezas y diagrama de bloques general (5 hojas). Su índice menciona además «Power Block Diagram», «Frame Schematic Diagram» y los esquemas, pero el visor sólo carga texto hasta el diagrama de bloques. No hay páginas de esquemas ni de placas impresas legibles. — [L2 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/21myl) · [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor) · [L3 CX720/PJ760](https://manualesdeusuario.es/sony/camcorder/hdr-cx720v/user-manual/24o54)
- **[V] Manual de ajuste (ADJ) de la familia CX580V.** Documento **9-834-634-56**, `983463456.pdf`, «HDR-CX570E/…/PJ600VE_ADJ», Ver. 3.0, 2015.03. Texto: «For adjusting these models, the Adjust manual and the Adjust station are required.» La instalación remite al ADJ de la HDR-TG5/TG7VE. Se baja «HDR-CX580V_Series_AM_Ver_□.□_x64.zip» de la página TISS y se instala con `Setup.msi`. Nota 3: «To perform Destination Data Write for this model, the Adjust manual of the DSC-WX50 series must have been installed.» Por el historial de versiones, el programa incluye «White/RTS Defect adj», «White defect adj», «INTERNAL MEMORY ADJUSTMENTS», «HDD SYSTEM ADJUSTMENTS», «Projector Data Reference/Input», «Destination data write», «SWITCH CHECK» y «Roll GYRO Check» (eliminado en 1.4). La versión 3.0 trae «Modification of HASP Key authentication». — [Wayback de servlib, serv.man4](http://web.archive.org/web/20191120075240/https://servlib.com/sony/movie/hdr-cx570e-hdr-cx580e-hdr-cx580v-hdr-cx580ve-hdr-cx590v-hdr-pj580-hdr-pj580e-hdr-pj580v-hdr-pj580ve-hdr-pj590v-hdr-pj600-hdr-pj600e-hdr-pj600v-hd-sm4.html) (`r6/servlib_sm4.html`)
- **[V] Notas de servicio de la VC-653 (L2 PJ260, sección 1).** Si se alimenta el terminal de batería con una fuente regulada de 8.4 V, «about 10 seconds after power is supplied … the power is shut off»; el remedio es usar el adaptador de CA (1-1). Al cambiar la VC-653 hay que hacer «DESTINATION DATA WRITE», «USB SERIAL SAVE» y «USB SERIAL No. INPUT» desde el Adjust Manual en el Adjust Station (1-2). En modelos con memoria interna se corren además los scripts `11DdmMmcReadTestMINI_v4.vbe` y `11DdmMmcMakePartition_v4.vbe`, bajados de TISS con la palabra clave «IM_VC_TOOL», con la cámara conectada por USB «by SEUS driver». Durante el procedimiento la cámara queda «running with blue color monitor» (1-2, Fig. 1). El archivo Destination Data se actualiza desde TISS («Destination File For ‘model name’.zip») (1-3). — [L2 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/21myl), pp. 1-1 a 1-3 (`r6/mdu_pj260_l2.txt`)
- **[V] Código de documento del L2 de PJ260.** El texto que leí no lo da. Su L3 es 9-834-636-11 (`983463611.pdf`). — [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor)
- **[V] servlib.com ya no existe.** El dominio está aparcado y en venta, y de sus páginas «sm2/sm3/sm4» de la familia CX580 Wayback sólo guarda la sm4 (ADJ). — [servlib.com](https://servlib.com/) · [CDX de Wayback](http://web.archive.org/cdx/search/cdx?url=servlib.com/sony/movie/hdr-cx570e*&output=txt)

### Inferences
- **[I]** Para P6, lo que falta está casi seguro en el L3 de 61 páginas: el esquema de la VC-653 (en el L3 de la PJ260, las divisiones 2/13 a 4/13 son IC1301, «CPU, AV SIGNAL PROCESS, LENS CONTROL, MODE CONTROL, HDMI PROCESS») y las placas impresas con sus puntos de prueba «CL». El dueño puede bajarlo gratis desde un navegador normal resolviendo el Turnstile. Conviene bajar también el L3 de PJ260/XR260 (misma VC-653) y el «LEVEL3 FULL» de la CX720/PJ760 para cotejar.
- **[I]** Que el ajuste de destino de una handycam dependa del Adjust Manual de una Cyber-shot (DSC-WX50) cuadra con lo que ya se sabe: estas handycam comparten plataforma y software de servicio con las Cyber-shot CXD4132.

### Gaps
- No leí el L3 de la CX580V. No sé si su lista de piezas confirma el sensor IMX116 ni qué puntos de prueba muestra la VC-653.
- No encontré el ADJ completo (sólo su primera página) ni descripciones públicas de lo que ajusta cada ítem del Adjust Manual.
- Callejones: manualslib y manualzz bloquean con Cloudflare, aun con Camoufox y salida por Japón. pro.sony bloquea con Akamai («Access Denied»), aun con curl y con `navega run`. archive.org no tiene copia de los PDF de pro.sony.

## 2. Consola serie, JTAG y caminos físicos de recuperación (P6)

### Takeaway
La consola de la CX580V es la **UART0 del CXD4132**, a 115200 8N1, y el `init` de fábrica
abre en ella una shell `ash` (`shell_loop` sobre `/dev/ttycons`). Ninguna fuente pública
ubica los pads en la VC-653. El precedente más cercano es la NEX-VG10 (CXD4115), que tiene un
conector de servicio de 18 contactos sin poblar con BOOT_MODE_1, UART0, UART1 y JTAG completo.
En la NEX-5N (CXD4132) alguien conectó la UART0 y sólo vio el descompresor del kernel al
arrancar el updater. No hay reporte público de una handycam o una NEX recuperada por UART.

### Cited Findings
- **[V] Configuración de UART en el kernel GPL de la plataforma (HDR-CX210, Linux 2.6.29.6-rt24).**
  - `plat-cxd41xx/cfg.c`: `unsigned long cxd41xx_uart_bitmap = 0x1UL; /* bit0=UART0 */` y `module_param_named(uart, cxd41xx_uart_bitmap, …)`. El comentario lo describe como «UART ports which is handled by tty driver».
  - `mach-cxd4132/cxd4132.c`: registra cada `uart_devs[i]` sólo si `cxd41xx_uart_bitmap & (1 << i)`, y define `UART_NR 3`.
  - `drivers/serial/amba2.c`: el driver se llama `ttyAM`, con consola `ttyAM0`.
  - `.config`: `CONFIG_CXD4132_DEBUG_UART0=y`, `CONFIG_CXD4132_DEBUG_UART_NO=0`, `CONFIG_CMDLINE=""`, `CONFIG_CMDLINE_PHYS_OFFSET=0x13000`.
  - `arch/arm/kernel/setup.c`: si en `PHYS_OFFSET+0x13000` hay un bloque que empieza con «kemco», lo toma como línea de órdenes («cmdline override at %p»). Si no, usa la de serie, que está vacía.

  — [kernel GPL HDR-CX210](https://prodgpl.blob.core.windows.net/download/DI/HDR-CX210/linux-kernel.tar.gz) (`r6/gpl/linux/…`)
- **[L] La línea de órdenes real de la CX580V vive en `/setting/kemco.txt`.** Está en `nflasha2_unpacked/kemco.txt` y empieza con «kemco». Contiene `console=ttyAM0,115200n8`, `cfg.uart=5` (bits 0 y 2: Linux registra UART0 y UART2 y deja libre la UART1), `cfg.poff=0,10`, `xrstreq=1`, `exception.reboot=1`, `softlockup_panic=1`, `wdt.mode=1 wdt.timeout=500`, `error.led=0,0` y `klog.addr=0x833E0080 klog.size=0x20000`. El `vmlinux.bin` 1.07 tiene la misma cadena «kemco» junto a «cmdline override at %p». — [kemco.txt local](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha2_unpacked/kemco.txt)
- **[L] Shell en la consola.** El `/sbin/init` de la CX580V tiene `shell_loop`, `/dev/ttycons` y `/bin/ash`, igual que el de la NEX-7. La descompilación del proyecto ya lo muestra (`firmware/ghidra/out/init.c`, líneas 708 a 722). — [initrd local](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/initrd.img_unpacked/sbin/init)
- **[V] NEX-VG10 (CXD4115): conector CN3504 de 18 contactos, sin poblar, en el borde de la placa principal.** Pinout según la imagen de rdrr: 1 REG_GND, 2 BOOT_MODE_1, 3 EVER_3.15V, 4 OR_UART0_RXD, 5 OR_UART0_TXD, 6 OR_UART1_RXD, 7 OR_UART1_TXD, 8 JTAG_NSRST, 9 OR_TDO, 10 OR_RTCK, 11 OR_TCK, 12 OR_TMS, 13 OR_TDI, 14 NTRST, 15 JTAGSEL_0, 16 REG_GND, 17 y 18 a tierra. — [imagen de rdrr](http://i.imgur.com/hv4sz.jpg) (`r6/dvx/hv4sz.jpg`), enlazada en el [hilo de dvxuser, p. 1](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/)
- **[V] Hilo «Sony NEX-axx firmware hacking» de dvxuser.**
  - Nexus7, 26 de agosto de 2011: «There is this this nice connector in VG10 with two UARTs and even JTAG port». «In firmware, there is 112500Bd console active on one UART with servicing commands and possible shell access». «There are also test points for all these pins, some near CPU, some directly behind connector».
  - rdrr, 18 de enero de 2012: el conector del procesador de imagen de las NEX-3/5 «contains JTAG, UART0 and UART1 (UART2 is pin 3 & 4 of the lens connector)», y «Main CPU JTAG is on the underside».

  — [dvxuser, p. 1](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/)
- **[V] Pads JTAG en la NEX-3/5.** La imagen anotada «NEX-3/5 Main CPU JTAG» marca CL3001 a CL3007 junto a IC3001: EJE, TMS, TRST, DINT, TCK, TDI y TDO. — [imagen de rdrr](http://i.imgur.com/8WoJS.jpg) (`r6/dvx/8WoJS.jpg`)
- **[V] Lo único publicado que se vio por UART en una CXD4132 (NEX-5N).** rdrr, 15 de mayo de 2012, según el resumen del lector: la UART0 mostró sólo una «E» y la línea «Uncompressing Linux… done, booting the kernel.» al reiniciar la cámara para actualizar el firmware. «booting with BOOT_MODE_1 high results in no serial output», y la luz de la SD parpadeaba más o menos una vez por segundo. El 4 de junio de 2012 escribió: «Did the firmware update, no other output on UART0 unfortunately». — [dvxuser, p. 2](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/page-2)
- **[V] NEX-7 (knowXD, noviembre de 2026).** El README dice que el kernel mapea `/dev/ttycons` a `ttyAM0` y que `init` levanta incondicionalmente una `ash` ahí. Dice haber encontrado «若干 2×2 测试焊盘阵列，其中一个对地为 0Ω», pero no reporta haberse conectado. `REPAIR_NEX7.md` da por hecho «3.3V TTL，115200 / 38400 8N1，GND 与屏蔽罩通» y una shell root, sin evidencia de prueba. `DISASSEMBLY_UART.md` es un plan: buscar TX midiendo con el voltímetro en el arranque y conectar sólo GND y TX al RXD de un adaptador de 3.3 V. — [README](https://github.com/knowXD/nex7-firmware-research) · [REPAIR_NEX7.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/REPAIR_NEX7.md) · [DISASSEMBLY_UART.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/DISASSEMBLY_UART.md) (`r6/nex7/`)
- **[V] Órdenes `bootrom` y `bootloader` en la NEX-7.** En el updatershell de Sony-PMCA-RE respondieron con error («DeviceIoControl 31») y dejaron el endpoint USB colgado; hubo que sacar la batería. — [NAND_DUMP.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/NAND_DUMP.md)
- **[V] Señales de reset en la VC-653 (L3 PJ260, diagrama de bloques 1/4).** IC4801 («HI CONTROL, LANC INTERFACE», hoja 12/13, cristal X4801 de 32.768 kHz) maneja `XSYS_RST`, `XFLASH_RST`, `XRESET_REQ`, `XPWR_OFF`, `XCS_FRONT` y el bus serie `FR_SI/SO/SCK` hacia IC1301. — [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor), p. 4-1

### Inferences
- **[I]** Lo que hay que buscar en la VC-653 es la UART0 del CXD4132. La consola va ahí, y la salida temprana del descompresor también, si la CX580V se compiló como la CX210. Esto concuerda con que rdrr la viera en una NEX-5N. La UART2 también queda en manos de Linux (`cfg.uart=5`), así que podría ser una segunda consola o el canal de algún periférico. La UART1 queda para otro uso.
- **[I]** El patrón de la VG10 (un conector FPC sin poblar o una fila de pads «CL» con UART0, UART1, JTAG y BOOT_MODE) es el candidato más probable en la VC-653. «OR_» parece el prefijo de Ortus, el nombre interno del CXD4115 según OpenMemories, así que en la CXD4132 las señales tendrían otro prefijo. La alimentación de esa interfaz en la VG10 es «EVER_3.15V», lo que sugiere lógica de unos 3.1 a 3.3 V. Hay que medir antes de conectar.
- **[I]** Por la experiencia de rdrr, en el updater la UART0 puede quedarse callada después del descompresor: el updater tiene su propio kernel y su propia línea de órdenes, que no vi. El arranque normal sí debería volcar la consola completa, porque `kemco.txt` declara `console=ttyAM0`.
- **[I] Riesgo nuevo para P6:** `/setting/kemco.txt` es la línea de órdenes del sistema principal. Si se corrompe o se borra (por ejemplo, si `dosfsck -Y` en el arranque toca la FAT de `/setting`), el kernel arranca con una línea vacía, sin `root=` ni `console=`, y no hay sistema principal ni consola. El updater no depende de ella, porque viene de `nflasha1`. Hay que tratarla como intocable, igual que `Backup.bin`.
- **[I] Bitácora post mortem sin UART.** El kernel guarda su bitácora en RAM en una dirección fija (`klog.addr=0x833E0080`, 128 KiB), y con `exception.reboot=1` un pánico reinicia en caliente. Con la guarda puesta, ese reinicio entra al updater. Si el bootloader y el kernel del updater no pisan esa zona, desde el updatershell se podría leer la bitácora del arranque fallido con `/dev/mem`. Está sin comprobar: se probaría provocando un reinicio controlado y leyendo esa zona, sólo lectura y con riesgo bajo.
- **[I]** BOOT_MODE_1 alto en la NEX-5N dejó la UART muda y la luz de la SD parpadeando. Eso apunta a un modo de la boot ROM, quizá descarga por USB o por serie. Sería un camino de rescate por hardware independiente de la flash, pero nadie lo documentó para la CXD4132.

### Gaps
- Ubicación física de la UART0 y del JTAG en la VC-653. Sólo la puede dar el L3 de 61 páginas (esquemas 2/13 a 4/13 y placas impresas) o una inspección de la placa. La foto de iFixit (paso 31) no tiene resolución para pads.
- Nivel de voltaje exacto de la UART en la CXD4132: ninguna fuente lo mide.
- Si hay prompt de cargador (`boot1`) por la UART: ningún reporte lo muestra. En la NEX-5N sólo apareció «E» antes del descompresor.
- No hay reporte público de una handycam CXD4132 conectada por UART ni de una recuperación por UART o JTAG en ninguna cámara Sony de 2010 a 2014.
- Comprobación en la cámara, con riesgo medio por el desarme y los flexibles: conectar sólo GND y el RXD del adaptador al TX candidato, a 115200, encender en modo normal y esperar el texto del kernel y luego la `ash`. No conectar VCC. No escribir nada por la consola hasta confirmar el eco.

## 3. Flash OneNAND y programación

### Takeaway
La OneNAND de 256 MiB de la CX580V no es un chip suelto. En esta familia, Sony apila la DRAM y
la NAND en un MCP montado encima del SoC (package-on-package). En la VC-653 todo el conjunto es
IC1301 («AVY1G2G04B», «Not supplied»). No hay TSOP que pinzar, así que leer o escribir la flash
fuera del circuito exige retrabajo BGA de un PoP. En la práctica, el rescate por hardware pasa
por la UART o el JTAG, no por un programador.

### Cited Findings
- **[V]** «The DRAM and NAND flash are usually combined in the same package (multi-chip package, MCP) and then stacked on top of the SoC (package-on-package, PoP). This is why the SoC package itself is never visible in disassembly photos.» — [OpenMemories hardware.rst](https://github.com/ma1co/OpenMemories-Docs/blob/master/hardware.rst) (`r6/om_hardware.rst`)
- **[V]** VC-653 (PJ260): «IC1301 (Not supplied) IC AVY1G2G04B». En la VC-658 (CX720…PJ760) el equivalente es «IC1301 (Not supplied) IC AVY2G2G05A». Ninguna de las dos listas tiene otro chip de NAND para el firmware. — [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor), p. 2-5 · [L3 CX720](https://manualesdeusuario.es/sony/camcorder/hdr-cx720v/user-manual/24o54)
- **[V]** La memoria interna de grabación es otro chip, un e-MMC Toshiba en la placa LD-279: «IC8101 THGBM4G7D2GBAIE» en la PJ260. El L2 lo nombra en E:94:00, «Inspect the flash memory (IC8101 on the LD-279 board)». En la VC-658 son THGBM4G8D4GBAIE y THGBM4G9D8GBAII. — [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor) · [L2 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/21myl), p. 1-6
- **[L]** El driver de la OneNAND está en `unidrv1.ko` (cadenas `onenand_register`, «OneNAND Unknown Device ID %d», «OneNAND: bootblock found at %d», «OneNAND:BOOTDATA: signature not found.»). O sea, el driver de la flash es propietario y no está en el kernel GPL; el de la UART sí (`drivers/serial/amba2.c`). — [unidrv1.ko local](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha3_unpacked_unpacked/initrd.img_unpacked/kmod/unidrv1.ko)
- **[V] Contraste con la NEX-7.** Usa una NAND Toshiba TC58NVG0S3ETA00 en TSOP48 de 128 MiB SLC, y aun así su autor desaconseja leerla en circuito con pinza («48 脚 0.5mm 间距难压准… 断电时 SoC 会经 ESD 二极管给总线倒灌»). Recomienda desoldar, leer dos veces y recalcular el ECC al escribir. — [REPAIR_NEX7.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/REPAIR_NEX7.md)

### Inferences
- **[I]** «AVY1G2G04B» se lee como 1 Gb de una memoria y 2 Gb de otra. 2 Gbit son 256 MiB, la OneNAND que reporta la cámara. Sería entonces el MCP de 1 Gb de DRAM y 2 Gb de OneNAND encima del CXD4132. Esto es inferencia por el nombre; ningún documento lo dice.
- **[I]** Para P6, la escala de rescate queda así: (1) la bandera del updater, ya probada; (2) la `ash` por UART0 si el kernel y el `init` llegan a correr; (3) JTAG si el bootloader o el kernel no arrancan, lo que exige ubicar los pads y una herramienta que entienda el ARM11 MPCore; (4) la boot ROM, si BOOT_MODE expone un modo de descarga. Reballear un PoP no es razonable.

### Gaps
- Referencia Samsung exacta de la OneNAND: no está en ninguna lista de piezas, porque va dentro del MCP «Not supplied». Las cadenas de `unidrv1.ko` tampoco traen una tabla de IDs legible.
- No hay ningún reporte público de lectura o escritura con programador en una cámara Sony con PoP de esta época.

## 4. Software y modos de servicio, códigos de error

### Takeaway
El software de servicio de Sony para esta familia es Adjust Station + Adjust Manual. Por USB
habla el protocolo **Senser**, que Sony-PMCA-RE implementa en público como `serviceshell`. La
CX580V trae ese servicio: `sen.elf`, `libsencore.so`, `/setting/.hasp` y `adjstctl.elf`, cuyo
formato «adjust block/code» coincide con el `PFUNC_AdjustControl` de PMCA-RE. Esto corrige a
`proyectos_sony_nuevos.md`, que dice que la CX580V «no tiene modo de servicio Senser». El
servicio cambia el destino (región), el número de serie USB, la sensibilidad del giroscopio,
los defectos de píxel, el micrófono y otros datos de ajuste. Para un arranque fallido, el
código **E:20:00** («Flash memory data are rewritten») es el que delataría un dato de flash o
de Backup corrupto.

### Cited Findings
- **[V] Senser en Sony-PMCA-RE.** README: «Sony cameras have a USB mode called *senser mode*, which is used during servicing for calibration and other things. It can also be used to execute code on the running system.» Hace falta un driver libusb.
  - Enumeración: `SONY_ID_PRODUCT_SENSER = [0x02a9, 0x0336]`. El cambio de modo usa `SONY_VendorRequest_StartSenser = (1, 0x37ff, 0xd7aa)` y, después, una autenticación de tres rondas con SHA-1 o SHA-256 (`senserKeysSha1`/`senserKeysSha256`).
  - Paquetes: cabecera `size, pFunc, sequence, version, miconType, offsetType, response`. Funciones: `PFUNC_ProductInfo 0x10`, `FirmwareUpdate 0x20`, `MiconAccess 0x30`, `AdjustControl 0x40`, `TestMode 0xff00`, `FileControl 0xff01`, `MemoryDump 0xff03`.
  - Órdenes: `READ_HASP (0, 0x1f)`, `TERM_CHANGE (0, 0xf1)`, `SENSER_TERM_MODE (0x601, 0x1f)`, `BACKUP_READ/WRITE/SAVE (0x603, 1/2/3)`, `BACKUP_ID1 (0x603, 15)`.
  - El backend abre una terminal en el endpoint 1 (`setTerminalEnable`, `readTerminal`, `writeTerminal`) y lee y escribe archivos y Backup.

  — [README](https://github.com/ma1co/Sony-PMCA-RE/blob/master/README.md) · [pmca/usb/sony.py](https://github.com/ma1co/Sony-PMCA-RE/blob/master/pmca/usb/sony.py) · [backend/senser.py](https://github.com/ma1co/Sony-PMCA-RE/blob/master/pmca/platform/backend/senser.py)
- **[L] Pila Senser en la CX580V 1.07.**
  - `libsencore.so`: «CORE_START MODE:SENSER», `senser_usb_mode_start`, `senser_normal_mode_start`, `RECV_SENSER_PACKET`, `pFunc_level_A/B/C`, «USB Event: COMPLETE_AUTHENTICATION», `/setting/.hasp`, `Read_Hasp`, `Regist_HaspId`, `REMOTE_ADJUST`, `@TERM_MODE`, `terminal_change_function_is_disabled`, `TEST_SIO_SEND`, y la ruta de compilación `Etools-SENSER_111H`.
  - `sen.elf`: `/setting/sen/smode`, `/var/.sen/.smode`, `SENSER_MODE = %d`.
  - `adjstctl.elf`: «adjust block: 0x%04x, code: 0x%04x OSAL ID: 0x%08x cmdname: %s», con órdenes como `ADJSTART`, `ADJEND`, `ADJ_MIC_SENSE`, `SET_MIC_MODE`, `SET_REC_LEVEL_MODE`, `S_WB_MODE`, `S_DEV_MANUAL_CCADJ`, `A_AS_GYRO_ADJ`, `A_AS_HALL_ADJ`, `S_LC_SET_ALENS_PINT_ADJ`, `G_SAVE_ADJ_DATA_TO_BKDATA`, `RAM_READ/WRITE`, `GPIO_READ`, `FCT_MODE`, `@DBG_MODE`, `SET_DIAG_MODE`, `MPR_CMD_MAKE_PARTITION` y `MPR_CMD_FORMAT`.

  — archivos locales [`lib/libsencore.so`](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libsencore.so), [`bin/sen.elf`](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/bin/sen.elf), [`bin/adjstctl.elf`](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/bin/adjstctl.elf)
- **[V] Qué cambia el servicio** (L2 PJ260, VC-653): «DESTINATION DATA WRITE» (destino o región; también borra el error de memoria interna tras cambiar la placa), «USB SERIAL SAVE» y «USB SERIAL No. INPUT» (un ID único que la placa nueva no trae), «GYRO sensor sensitivity adj» (PITCH/YAW, del adhesivo del lente o del sensor), datos del proyector, «Measure to E:62:11/12» en la pestaña SERVICE, y la pantalla del acuerdo de licencia de mapas del GPS. — [L2 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/21myl), pp. 1-1 a 1-7
- **[V] Autodiagnóstico de la familia VC-653.** «This function consists of two display; self-diagnosis display and service mode display». El código parpadea a 3.2 Hz; C la corrige el cliente y E el técnico. Códigos útiles para un arranque fallido:
  - **E:20:00** «Flash memory data are rewritten. Make flash memory data correct value», con nota de usar DESTINATION DATA WRITE.
  - E:31:00 «Drive fault».
  - C:13:01 a C:13:04, errores de medio.
  - C:32:60 o E:61:00, no puede inicializar el foco; revisar el pin 7 de CN4002 en la LD-279 y el IC5501.
  - E:61:10, zoom.
  - **E:61:30** «Reset position detection error on the stepper iris initializing».
  - E:62:00 a E:62:20, SteadyShot, lente de desplazamiento y termistor.
  - E:92:01, corriente de batería.
  - E:94:00 y E:94:01, memoria flash interna (IC8101 de la LD-279).
  - E:94:02 «BGM data error».

  — [L2 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/21myl), pp. 1-4 a 1-6 (tabla completa en `r6/mdu_pj260_l2.txt`)
- **[V]** En los issues de Sony-PMCA-RE no encontré ningún reporte de serviceshell en una handycam. Los que mencionan serviceshell o Senser son de Alpha y Cyber-shot (por ejemplo, el #715, ILME-FX3A, «serviceshell authentication timeout»). La única handycam que sale con «serviceshell HDR» es el #728 (HDR-CX405, «App install does not work»), que no leí. — [búsqueda de issues](https://github.com/ma1co/Sony-PMCA-RE/issues?q=serviceshell)
- **[V] Para la CXD90057, otro investigador ya documentó** Senser en 3 rondas → PID `054C:0336` → archivos «Jiritsu» → root. — `research_notes/Barrido de fuentes para la CX580V/proyectos_sony_nuevos.md` (no lo repito)

### Inferences
- **[I]** El «Adjust Manual» de Sony y el `serviceshell` de PMCA-RE hablan el mismo protocolo. El `pFunc 0x40 AdjustControl (block, code)` de PMCA-RE corresponde al «adjust block / code» que `adjstctl.elf` reenvía por OSAL a la capa de cámara, y la llave HASP del PC corresponde a `/setting/.hasp` en la cámara. Esto deja ver que el servicio de fábrica puede tocar micrófono y nivel de grabación (`ADJ_MIC_SENSE`, `SET_REC_LEVEL_MODE`, que sirven a P5), balance de blancos y matriz de color (`S_WB_MODE`, `S_DEV_MANUAL_CCADJ`, para P3), lente y foco (`S_LC_*`, `A_AS_*`) y Backup.
- **[I] Para P6 y P7:** si la CX580V acepta la autenticación de PMCA-RE, el modo Senser daría, en modo normal y sin el updater, una terminal por USB, lectura y escritura de archivos y lectura y escritura de Backup por ID (`0x603, 1/2`). Sería un segundo canal de diagnóstico. No sirve si el sistema principal no arranca, porque lo lanza `sen.elf` desde el modo normal.
- **[I]** `/setting/sen/smode` parece elegir el modo de Senser al arrancar (USB o normal). Es otro archivo de `/setting` que el proyecto debe respaldar y no tocar a ciegas.
- **[I]** La «service mode display» del autodiagnóstico (historial de códigos) en estas handycam probablemente se lee por el Adjust Manual, no con una combinación de teclas. El L2 no da ninguna combinación.
- **Comprobación en la cámara:** correr `pmca-console serviceshell` contra la CX580V en modo normal y ver si cambia a `054c:02a9` o `0336` y pasa la autenticación. Riesgo bajo a medio: es el modo de servicio de Sony, pero `start()` lee la HASP, la terminal cambia `TERM_MODE` y en Windows hace falta reemplazar drivers. Hay que evitar las órdenes de escritura (`BACKUP_WRITE`, `FileControl` de escritura) hasta tener respaldo.

### Gaps
- No hay combinación de teclas pública para menús ocultos en handycam de 2011 a 2013. La búsqueda sólo devolvió el botón RESET, que borra ajustes y reloj.
- No sé si las llaves Senser públicas (SHA-1/SHA-256) valen para la CXD4132 de 2012. PMCA-RE no lista handycam con serviceshell.
- No sé qué otras propiedades de Backup cambian el «DESTINATION DATA WRITE» del Adjust Manual de la DSC-WX50. Eso serviría para P4 (región y PAL) y P7.

## 5. Control remoto por LANC (A/V Remote) y SIRCS (P1, P3)

### Takeaway
La CX580V lleva LANC por su conector A/V Remote de 10 contactos. El diagrama de bloques de la
VC-653 lo dibuja de punta a punta: `CN0201 (A/V R) → LANC_SIG → IC4302 (LANC DRIVER) → IC4801
(LANC INTERFACE)`. La lista pública de órdenes LANC «especiales» (byte 0 = 0x28) tiene foco
manual cerca/lejos, AF sí/no, iris más abierto/más cerrado, iris automático y reset de balance
de blancos. **No tiene órdenes de ganancia** ni de obturación para cámaras modernas. Por
infrarrojo, la CX580V probablemente no tiene receptor: el control remoto y el sensor RS-450-T
son sólo de los modelos con proyector.

### Cited Findings
- **[V] LANC en la VC-653.** IC4302 BD8331GU, «DC/DC CONVERTER, LANC DRIVER» (hoja 11/13), con `LANC_IN/OUT` y `LANC_SIG`. IC4801 MB95005ABGL, «HI CONTROL, LANC INTERFACE» (hoja 12/13). `LANC_SIG` llega al conector CN0201 marcado «A/V R», que está en los flexibles FP-1321/FP-1323. Esto sale de las etiquetas y sus coordenadas en el texto del diagrama (`r6/pj260_bd1_items.json`). — [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor), p. 4-1, y lista de piezas «CN0201 1-815-794-13 CONNECTOR (MULTIPLE)»
- **[V] Sony, artículo 00027562 «What is a LANC jack?».** LANC ha estado en sus videocámaras como «2.5mm plug jack», como «Multi- function 10 pin "D" connector labeled as A/V, A/V Remote (A/V R), AVR, or at times A/V OUT» y como «Multi / Micro USB terminal… for 2013 and select 2014 camera models only». «2013 and 2014 camcorder models require the use of Sony VMC -AM1 adaptor.» «2015 and later models are not compatible with LANC anymore.» — [Sony USA](https://www.sony.com/electronics/support/memory-camcorders-hdr-cx-series/hdr-cx380/articles/00027561) (`r6/sony_lanc.txt`)
- **[V] Pinout del conector «D» de 10 contactos** (pinouts.ru, sin verificar por el sitio, «LANC interface available in some models only?»): 1 audio izquierdo, **2 LANC datos**, **3 tierra de S-video y LANC**, **4 LANC +5 V**, 5 croma, 6 audio derecho, **7 habilitación de LANC (a tierra por 100 kΩ; sin la resistencia sale video compuesto por el 9)**, 8 retorno común, 9 video compuesto, 10 luminancia. Cable de A/V: VMC-15FS. — [pinouts.ru](https://pinouts.ru/DigitalCameras/sony_10pin_lanc.shtml)
- **[V] Protocolo LANC (boehmel.de).**
  - Bit de 104 µs, 8 bytes por telegrama, telegramas cada 20 ms en PAL y 16.6 ms en NTSC. «A command is valid after 3...4 telegrams.» Poner LANC a tierra más de 140 ms enciende la cámara.
  - Órdenes especiales a videocámara (byte 0 = 0x28): `0x41` «Auto-Focus on/off (not if there is a real switch at the camera)», `0x45` «Focus manual far», `0x47` «Focus manual near», `0x49` «White balance toggle (only cameras until approx. 1996)», `0x77` «White balance reset (not if white balance is selected via menu)», `0x4B`/`0x51` contraluz, `0x53` «Exposure auto/man. toggle (models of the early 90's) / Iris more close», `0x55` «Iris more open», `0xAF` «Iris auto», `0x61` «Shutter (models of the early 90's)», `0x21` «grid (AVCHD)», zoom variable `0x00`–`0x0E` y `0x10`–`0x1E`, y fijo `0x35`/`0x37`/`0x39`/`0x3B`.
  - No hay órdenes de ganancia, AGC ni AE shift.

  — [boehmel.de/lanc.htm](http://www.boehmel.de/lanc.htm)
- **[L] Qué maneja la CX580V por LANC.** `libObj.so` trae `InfraLanc`, `InfraLancCommand`, `InfraLancStatusDisplay`, «[INFRALANC]chgLancCmd word0=%02x,word1=%02x», `KEY_LANC_HIGHEND_REMOTE_CONTOLLER` (con `callback_lanc_highend_remote_contoller`), `KEY_LANC_HOUSING`, `KEY_LANC_LINE_HOUSINGMIC`, `KEY_LANC_INP_HOTSHOE_STROBE` y elementos de estado para el control remoto (`stsDispItem_AutoIris`, `_Gain`, `_ElectronShutter`, `_WhiteBalance`, `_AutoFocus`, `_StepZoom`, `_Fader`, `_TallySts`). — [libObj.so local](file:///home/aristotle/temp/cx580-pro/firmware/cx580v-1.07/unpacked/nflasha8_unpacked_unpacked/lib/libObj.so)
- **[V] SIRCS para videocámaras.** Mismas órdenes que las VCR con otro código de dispositivo: «Camcorders and any VCRs using 8mm tape use VCR2, which is 7 and 185», «and for additional camcorder codes 217 and 26.241(memory stick)».
  - Dispositivo 217: 30 autofoco, 31 foco manual, 32 conmutar AF/MF, 34 foco lejos, 35 foco cerca, 36 balance de blancos (conmutar), 59 reset de balance, 37/40 contraluz, 48 obturador (foto), 26/27/28/29 zoom T/W normal y rápido, 19/20 start/stop.
  - Dispositivo 185: 21 foto, 25 start/stop, 26/27 zoom, 14 a 17 y 24 navegación.
  - No hay iris, ganancia ni obturación de video.

  — [hifi-remote, Sony VCR](https://hifi-remote.com/sony/Sony_vcr.htm)
- **[V]** En boehmel, SIRCS y LANC comparten temporización: «SIRCS is pulsed with 40 kHz», y las órdenes IR y CTRL-S van «shifted right one bit, compared to LANC». — [boehmel.de/lanc.htm](http://www.boehmel.de/lanc.htm) · [boehmel.de/sircs.htm](https://boehmel.de/sircs.htm)
- **[V] Mando infrarrojo sólo en los modelos con proyector.** La guía de ayuda titula la sección «Wireless Remote Commander (HDR-PJ260/PJ260V/PJ580/PJ580V/PJ600/PJ600V)». En la lista de piezas, el probable receptor IR «IC4001 RS-450-T» aparece marcado «(Projector Model)». En la VC-658, «IC9601 RS-470» está en la placa FR-319. — [guía CX580V](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/08/02/01/01.html) · [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor), lista de la LD-279

### Inferences
- **[I] P3 (foco):** LANC `0x28 0x45`/`0x47` (foco manual lejos/cerca) y `0x41` (AF sí/no) son las órdenes con más probabilidad de funcionar, porque el firmware tiene el camino «HIGHEND_REMOTE_CONTROLLER» de los mandos tipo RM-1000BP. No dan una posición absoluta, sólo pasos, pero bastan para probar si el foco manual entra por LANC sin pasar por la interfaz.
- **[I] P1 (ganancia):** LANC no ofrece orden de ganancia. Los estados `stsDisp_Gain` y `stsDisp_ElectronShutter` sugieren que la cámara *informa* ganancia y obturación al mando, lo que serviría para leer el valor real de ganancia sin `cxexposure.py`. Están sin probar.
- **[I] Balance (P3):** sólo hay `0x77` (reset del balance). No hay una orden de temperatura de color por LANC.
- **[I] SIRCS:** la CX580V (sin proyector) no debería tener receptor IR montado. `INFRASIRCS` estaría en su firmware sólo porque el firmware es común con la PJ580/PJ590/PJ600.
- **Comprobación en la cámara:** un Arduino con un emisor LANC, por ejemplo [AlexNe/arduino_lanc_sample](https://github.com/AlexNe/arduino_lanc_sample), conectado a los contactos 2 (datos), 3 (tierra) y 7 (100 kΩ a tierra) del A/V R. Primero escuchar los 8 bytes de estado, después mandar `0x28 0x45`/`0x47` y `0x28 0x55`/`0x53`. Riesgo bajo: el bus es de colector abierto y lo alimenta la cámara. Hay que confirmar el pinout midiendo antes, porque pinouts.ru no está verificado. Tener a mano el VMC-15FS ayuda a identificar los contactos.

### Gaps
- No apareció ninguna lista de qué órdenes LANC acepta una handycam de 2011 a 2013. Tampoco una tabla de las «guide codes» o del estado que usan los mandos RM-1000BP o RM-1BP con estos modelos.
- No encontré tablas públicas de SIRCS con iris, ganancia u obturación de video. Ninguna tabla interpreta los componentes INFRALANC o INFRASIRCS del firmware.

## 6. Qué integrado hace qué (sensor, lente, audio, power IC, táctil y LCD)

### Takeaway
Con el L3 de la PJ260 (misma VC-653) y el de la CX720/PJ760 se identifica casi toda la placa:

- **CXD4132**: va en el PoP IC1301.
- **Power IC o micro de sistema**: Fujitsu **MB95005ABGL «Charon»** (IC4801), que además hace la interfaz LANC.
- **Estabilizador y motores del lente**: Renesas **R2J30503LG «Bonobo»** (IC5501, núcleo H8; su firmware es el `bonobo.bin` del proyecto).
- **Iris**: es de motor paso a paso y hace también de obturador mecánico.
- **Audio**: cadena BD8871GW (amplificador de micrófono y auriculares) → NN12926A (procesador de audio, controlado por SPI) → CS48L10 (DSP de audio) y CS42L52 (DAC y amplificador de parlante).
- **Sensor**: muy probablemente **IMX116**, el mismo CM-123 de la PJ260.
- **Táctil**: resistivo, leído por ADC.

### Cited Findings
- **[V] VC-653 (lista de piezas del L3 PJ260, p. 2-5, y diagrama de bloques 4-1):**

  | Ref. | Pieza | Función |
  | --- | --- | --- |
  | IC1301 | AVY1G2G04B, no suministrado | «CPU, AV SIGNAL PROCESS, LENS CONTROL, MODE CONTROL, HDMI PROCESS» |
  | IC1302 | AK8153AU-L | «CLOCK GENERATOR», cristal X1302 de 60 MHz |
  | IC2601 | BD8871GW-E2 | «HEADPHONE AMP, MIC AMP» (5/13) |
  | IC2701 | CS42L52-CNZR | «D/A CONVERTER, SPEAKER AMP» (6/13) |
  | IC2801 | NN12926A-BB | «AUDIO PROCESSOR» (7/13): recibe `INT_MIC_L/R/F`, `EXT_MIC_L/R`, `MICAMP_OUT_L/R` y `XCS_IC_2801` y `AV_SO/SCK` desde IC1301 |
  | IC3001 | CS48L10-CWZR | «AUDIO DSP» (8/13) |
  | IC3301 | SN65LVDS301 | Transmisor serie del LCD, sólo en modelos con proyector |
  | IC4301 | BD8330AGU | DC/DC |
  | IC4302 | BD8331GU | DC/DC y driver LANC |
  | IC4303 | R1207N823B | Regulador |
  | IC4304 | BD2224G | Regulador |
  | IC4801 | MB95005ABGL-G-135 | «HI CONTROL, LANC INTERFACE» |
  | IC9101 | MAX14593 | Gestión de energía del USB |
  | IC9102 | BD8668GW | «SMART POWER SELECTOR» |
  | IC9103, IC9104 | BD11600NUX | «USB SWITCH» |
  | IC9105 | TPS2553 | Interruptor de alimentación del USB |

  En la VC-658 se repiten IC1302, IC2701, IC3001, IC4301 a IC4304, IC4801 y IC9101 a IC9105. — [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor) · [L3 CX720](https://manualesdeusuario.es/sony/camcorder/hdr-cx720v/user-manual/24o54)
- **[V] Power IC.** OpenMemories lista «MB95005ABGL | Charon» como power IC («responsible for starting up the BIONZ processor. It also manages the real time clock»). En la VC-653, ese chip es IC4801, con el cristal de 32.768 kHz y las líneas de reset, `XPWR_OFF` y el bus serie `FR_*`. — [OpenMemories hardware.rst](https://github.com/ma1co/OpenMemories-Docs/blob/master/hardware.rst) · [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor), p. 4-1
- **[V] Estabilizador y motores del lente.** OpenMemories: «R2J30503LG | Bonobo | Renesas H8 core» en la sección Image Stabilization. En la LD-279 (placa del lente de la PJ260 y de la CX580V) es IC5501, junto a IC5502 R2A30427BM e IC5503 MM3404A28. El L2 manda revisar «the focus motor drive IC (IC5501…)» en C:32:60 y el «zoom motor drive circuit (IC5501…)» en E:61:10. El L2 dice «on the VC-653 board», pero la lista de piezas y la sección 1-7 lo ubican en la LD-279. En la LD-297 de la CX720/PJ760 están IC5601 R2J30503LG, tres LV8417CS (no suministrados), dos R2A30427BM e IC5201 BU24130GU. — [OpenMemories](https://github.com/ma1co/OpenMemories-Docs/blob/master/hardware.rst) · [L2 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/21myl), pp. 1-4 a 1-6 · [L3 CX720](https://manualesdeusuario.es/sony/camcorder/hdr-cx720v/user-manual/24o54)
- **[V] Iris.** El bloque del lente aparece en el diagrama como «IRIS (SHUTTER)». El código E:61:30 dice «Reset position detection error on the stepper iris initializing… confirm that the iris blades in lens are working… check the iris motor drive IC in lens drive block (CN4002 …on LD-279 board)». — [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor), p. 4-1 · [L2 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/21myl), p. 1-5
- **[V] Sensor.** El L3 de la PJ260 dice «CP001 (Not supplied) IMX116CHL-13» en la CM-123 («CP001 (CMOS IMAGER) is not supplied, but this is included in CM-123 COMPLETE BOARD»), conjunto A-1851-196-A. Para la CX580V, el L2 de su familia da «4.6 mm (1/3.91 type) CMOS sensor… Max. 20.4 mega (6 016 × 3 384) pixels… Gross: Approx. 5 430 000 pixels; Effective (movie, 16:9): Approx. 5 020 000». Según el resultado del buscador, el catálogo de Sears de la CX580V lista «Pcb cm-123, Part #A1851196A». No lo pude ver abriendo la página. — [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor), lista de la CM-123 · [L2 CX580, p. 2](https://elektrotanya.com/sony_hdr-cx570_hdr-cx580_hdr-cx590_hdr-pj580_hdr-pj590_hdr-pj600_l2_ver1.5.pdf/download.html) (`r6/et_prev/cx570_l2_p2.png`) · [Sears CX580V](https://www.searspartsdirect.com/model/4w1uflxxjp-000996/sony-hdr-cx580v-digital-camcorder-parts)
- **[V] Sensor de la CX760/PJ760:** IMX077LHK-13 en la CM-121. — [L3 CX720](https://manualesdeusuario.es/sony/camcorder/hdr-cx720v/user-manual/24o54)
- **[V] Táctil y LCD.** «TOUCH PANEL INTERFACE Q6001, Q6002» en la PD-459 con señales analógicas `TP_X/Y`, `TP_SEL1`, `TP_SEL2`, `TP_R` y `TP_BOT`, y «3.0 INCH WIDE COLOR LCD LCD901 (LCD MODULE)» con bus `LCD_R/G/B_0-7`, `LCD_CLK`, `XCS_LCD`, `AV_SO/SCK` y `PANEL_NOR/REV`. En los modelos sin proyector, el LCD va por el flexible FP-1325; en los con proyector, por el par SN65LVDS301/302. Especificación de la CX580V: 7.5 cm, 921 600 puntos (1920 × 480). — [L3 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/25kor), pp. 4-1 y 4-2 · [L2 CX580, p. 2](https://elektrotanya.com/sony_hdr-cx570_hdr-cx580_hdr-cx590_hdr-pj580_hdr-pj590_hdr-pj600_l2_ver1.5.pdf/download.html)
- **[V] Giroscopio:** SE7501 (PITCH/YAW) en la MS-478. — [L2 PJ260](https://manualesdeusuario.es/sony/camcorder/handycam-hdr-pj260/user-manual/21myl), p. 1-4

### Inferences
- **[I] P5 (audio):** el nivel de grabación lo puede fijar el SoC por SPI en el procesador de audio IC2801 (NN12926A, `XCS_IC_2801`) o en el DSP IC3001. El servicio tiene órdenes `ADJ_MIC_SENSE` y `SET_REC_LEVEL_MODE`, así que hay control programable de la ganancia del micrófono. Por la forma del diagrama, el CS42L52 sólo maneja parlante y DAC, aunque tenga preamplificador de micrófono propio. Que la ruta de los micrófonos internos no pase por él es lectura del diagrama, sin esquema.
- **[I] P1 (saltos de exposición):** el iris es de motor paso a paso y lo maneja el Bonobo. Los saltos esporádicos de 1/3 a 1/6 de paso que ve el proyecto podrían venir de la cuantización del iris o de su recalibración, por ejemplo al cambiar el zoom, si la ganancia y la obturación quedan fijas. Es una hipótesis que hay que medir.
- **[I] ND:** ninguna fuente menciona un filtro ND en el LSV-1400A, y en `av-cam.bin` no encontré cadenas claras de ND. Con lo que hay, «iris» en esta cámara significa apertura real de láminas.
- **[I]** «Charon» (IC4801) es el destino de lo que `upm.ko` manda por `sio` (teclas de energía, `XPWR_OFF`) y decide el reset del SoC (`XSYS_RST`, `XRESET_REQ`). Un cuelgue total del SoC con el power IC vivo se resuelve cortando la alimentación (batería o DC), no con RESET.

### Gaps
- Datasheets públicos: no los hay de NN12926A (Panasonic), BD8871GW, BD8330/BD8331 (piezas a medida de ROHM), MB95005ABGL ni R2J30503LG.
- Del CS42L52, el resumen de búsqueda de sus hojas de datos (mirrors con revisiones DS680A1, DS680F1 y DS680F2) habla de preamplificador de micrófono de 16 a 32 dB en pasos de 1 dB, PGA y ALC. No abrí el PDF oficial, y en esta placa el CS42L52 no parece estar en la ruta de los micrófonos.
- No hay confirmación directa de que la CX580V use exactamente la CM-123 con IMX116, ni datasheet público del IMX116 con su rango de ganancia analógica.
- No encontré el modelo del controlador táctil (parece no haberlo, porque es resistivo por ADC) ni el del panel LCD.

## 7. HXR-NX30 y HXR-NX70: cómo ofrecen iris, ganancia y obturación (hueco del informe anterior)

### Takeaway
La NX70 deja iris, ganancia y obturación manuales a la vez con el selector en MAN: ganancia de
0 a 21 dB, obturación de 1/6 a 1/10000 e iris de F1.8 a F9.6. La NX30, en cambio, es una
PJ760 con otra caja: su dial MANUAL no permite fijar Exposure, IRIS y Shutter Speed al mismo
tiempo («最後に設定した機能のみが有効になります») y la ganancia sólo tiene tope (AGC Limit, de
0 a 24 dB en video).

### Cited Findings
- **[V] HXR-NX70E/NX70P, Operating Guide 4-273-940-13(1).**
  - p. 49: «You can adjust the iris, gain, shutter speed or white balance manually. Set the AUTO/MANUAL switch to MAN.» Notas: «If you set the AUTO/MANUAL switch to AUTO, manual adjustments of iris, gain, shutter speed, white balance are canceled.» «[AE Shift] is not effective temporarily while you adjust the iris, gain and shutter speed all manually.»
  - p. 49, iris: «expressed as an F value between F1.8 and F9.6». «The F value becomes close to F3.4 as the zoom position changes from W to T even when you open the aperture by setting the F value lower than F3.4, such as F1.8.»
  - p. 50, ganancia: «You can adjust the gain manually when you do not want to use the AGC (automatic gain control)… Select the gain value between 0 dB and 21 dB».
  - p. 50, obturación: «You can adjust the shutter speed in a range of 1/6 second through 1/10000 second.»
  - p. 80: AGC Limit de «[Off] (21dB, the default setting), [18dB], [15dB], [12dB], [9dB], [6dB], [3dB] and [0dB]», y «If you adjust the gain manually, you cannot obtain the effect of [AGC Limit].»
  - Por defecto, «[Gain] (p. 50), [Shutter Speed] (p. 81), [AE Shift] (p. 81) are registered» en los botones asignables.

  — [Manual NX70E/P (copia de la Universidad de Cambridge)](https://www.educ.cam.ac.uk/services/avs/loanstock/manuals/user-manual-sony-nx70.pdf) (`r6/nx/nx70_cam.pdf`)
- **[V] HXR-NX30J, 取扱説明書 4-425-005-01(1).**
  - p. 37: «絞り（アイリス）優先で撮る» y nota de que el F vuelve a F3.4 al ir hacia T.
  - p. 38, dial MANUAL: se le asignan [Focus], [Exposure], [IRIS], [Shutter Speed], [AE Shift] y [White Balance Shift]. Nota: «[Exposure]、[IRIS]、[Shutter Speed]は、同時に設定できません。最後に設定した機能のみが有効になります。»
  - p. 68, AGC Limit: Off o 24/21/18/15/12/9/6/3/0 dB; «動画のとき：24dB / 静止画のとき：21dB / ［Smth Slw REC］のとき：15dB».

  — [Manual NX30J (contents.sony.jp)](https://contents.sony.jp/professional/manual_pdf/c_c/4425005011.pdf) (`r6/nx/nx30j.pdf`)

### Inferences
- **[I] P1:** la NX30, la hermana profesional de la PJ760, mantiene la exclusión entre Exposure, IRIS y Shutter Speed, y sólo da tope de ganancia. La NX70, con otra placa (VC-633), sí tiene ganancia manual de 0 a 21 dB. El límite de la CX580V es de interfaz y de producto, no de la arquitectura CXD4132, lo que cuadra con lo que el proyecto ya consiguió con iris y obturación. Que la NX70 no tenga AGC Limit con ganancia manual sugiere que, por dentro, «ganancia manual» y «tope AGC» son modos distintos, lo mismo que se ve entre `PRO_SET_MANUAL_GAIN_LINK` y `PRO_SET_AGC_LINK` en `av-cam.bin`.

### Gaps
- No pude abrir el manual en inglés de la NX30U (pro.sony responde 403 o «Access Denied» y manualzz bloquea). El japonés cubre el mismo contenido.
- Callejón: `sony.jp/…/4477121011.pdf` resultó ser de la HXR-NX3 (2013), no de la NX30.
