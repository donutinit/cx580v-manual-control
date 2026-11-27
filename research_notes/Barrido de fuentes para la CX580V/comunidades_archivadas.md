# Comunidades archivadas y en otros idiomas sobre cámaras Sony (2010-2014): lo que no estaba en el informe anterior

Notas del 2026-11-27. Todo lo citado lo volví a abrir en la captura indicada; las fechas de los mensajes son las del foro. Etiquetas: **[captura]** leído en Wayback, **[vivo]** leído en el sitio actual, **[local]** lectura estática del firmware 1.07 desempacado en `firmware/cx580v-1.07/unpacked/` (sin tocar la cámara ni escribir nada), **[buscador]** sólo un fragmento de un buscador, sin abrir la fuente. Entre corchetes va el pendiente al que sirve (P1…P8).

Copias locales de lo bajado (datos no confiables, nada ejecutado), todas bajo `vendor/barrido-fuentes/r7/`:
- `nexhack-wiki/` (wiki DokuWiki de nex-hack.info) y `pv-wiki/` (su copia en personal-view.com/faqs/sony-hack). Texto extraído en `txt/*.content`.
- `forum/` (55 páginas de hilos de nex-hack.info) → texto en `txt/forum_all.txt`.
- `nhcom/` (12 páginas del foro nex-hack.com de 2012) → `txt/nhcom_all.txt`.
- `pvtalk/` (19 páginas de hilos de personal-view) → `txt/pvtalk_all.txt`.
- `dvx/p1.html`…`p8.html` (hilo de dvxuser completo) → `dvx/thread.txt`.
- `img/hv4sz.jpg` y `img/8WoJS.jpg` (fotos con el pinout del conector, de imgur), `img/av-cam.png` y `img/av-cam-rebase.png` (capturas de IDA de la wiki).
- `cdx/` con los listados de Wayback, incluido `nexhack_discussions.txt` (los 118 hilos con captura).

---

## 1. nex-hack.info (wiki y foro): ¿qué descubrieron que no esté en el informe anterior?

### Takeaway
La wiki y el foro no traen nada específico de handycam ni de la CX580V, pero sí cinco cosas nuevas y útiles: (a) kenan describió en 2013 a `av-cam.bin` como «la aplicación principal de cámara, llamada LIRO», en C++, sin strip y con un **Debug Monitor** que la CX580V también tiene; (b) el árbol del kernel de la NEX-5R/6 traía un **`cxd4132_qemu_defconfig`**, o sea que Sony emulaba la CXD4132 en QEMU; (c) el cambio de región por archivos de preajuste (`PS_UUG`) funcionó en SLT y **falló en todas las NEX de 2.ª generación** que lo intentaron; (d) la 2.ª generación usa un **WBI** («warm boot image», hibernación) en `nflasha5`; (e) el `up.sh` del firmware trae las funciones `modreg` y `modmod`, por las que alguien preguntó en 2013 sin respuesta y que en la CX580V **escriben en `nflasha1`**.

### Cited Findings

**av-cam.bin y arquitectura (P3, P6, P8)**
- [captura] kenan, 2013-02-14: «This is the main cam application, called LIRO. Written in cpp, not stripped with debug console», y cita las cadenas `[        Debug Monitor         ]`, `help    ... Show command list`, `DebugMonitor initialized.` y `Stream Property : 1920x1080@60p/50p(30p/25p)`. En el mismo hilo enumera los procesadores de NEX/SLT: «armv6 inside cxd4115/4132, mips16ase inside tmp19a44fdax. dsp are inside cxd4115/4132, cxd9974/90016, cxd4113», y añade: «there is a linux console accessible thru the usb port by special software». Un día después dice que av-cam.bin «looks not like arm code to me», cosa que contradicen la página de la wiki y el informe anterior. — [hilo 71, About av-cam.bin](http://web.archive.org/web/20130306023740/http://nex-hack.info/talks/discussion/71/about-av-cam-bin-found-in-unpacked-5n-firmware)
- [local] El `av-cam.bin` de la CX580V tiene las mismas cadenas del Debug Monitor (`[        Debug Monitor         ]`, `help    ... Show command list`, `DebugMonitor initialized.`) y una tabla `Stream Property` que incluye `1920x1080@24p`, `1280x720@24p(Not Support)`, `1920x1080@60p/50p(30p/25p)` y `720x480@60i / 576@50i`. No sé por qué canal entra o sale ese monitor (UART, USB u otro).
- [captura] Página de la wiki «nex6-av-cam.bin-reversing», editada por última vez el 2013-06-21: av-cam.bin es «ARM code, and powered with t-kernel RTOS», el IDC de IDA está en `github.com/steelcnn/nex-hack`, lo revertido es sobre todo códec JPEG/video y por eso el autor sospecha que corre «in the Bionz image processing chip or Sony CDX4113GG chip». El módulo se reubica en otra dirección en cada modelo: en la NEX-5N 1.02, rebase a `0xC3421000`. — [nex-hack.info, 2015-02-22](http://web.archive.org/web/20150222161657/http://www.nex-hack.info:80/wiki/development/nex6-av-cam.bin-reversing) · [copia en personal-view](http://web.archive.org/web/20150901083213/http://www.personal-view.com:80/faqs/sony-hack/nex6-av-cam.bin-reversing)
- [captura] Las dos imágenes de esa página. `av-cam-rebase.png` muestra la NEX-6 cargada en IDA con `ORG 0xC34B1000`, la cadena `"ORIL"` (LIRO en little-endian), `DCD 4` (versión) y la tabla de punteros de entrada `0xC34B1050`, `…1078`, `…1088`, `…1098`, `…10A8`, `…10B8`, `…10BC`, `…10C0`. `av-cam.png` muestra «Occurrences of binary ".cpp"», con 573 líneas, como `CS_imaging.cpp`, `CaptureManager.cpp`, `CdcMgrEnc.cpp` y `CaptureUtilitySDF.cpp`. — [av-cam-rebase.png](http://web.archive.org/web/20201102014749/http://www.personal-view.com/faqs/_media/sony-hack/av-cam-rebase.png) · [av-cam.png](http://web.archive.org/web/20201102014749/http://www.personal-view.com/faqs/_media/sony-hack/av-cam.png)
- [vivo] El repositorio `steelcnn/nex-hack` existe: creado el 2013-06-10, último push ese mismo día, con `README.md` (18 B) y `av-cam-NEX6-v1.01.idc.zip` (2 167 901 B). No lo bajé. — [steelcnn/nex-hack](https://github.com/steelcnn/nex-hack)
- [captura] En la wiki, «current-tasks» (2013-09-03, kenan): «Testu is working on the reversing of av-cam.bin with IDA 6.1 … The primary goal is to learn the NEX E-mount lens protocol», y «jwise has an ARM IDA/Hex-Rays license». — [current-tasks](http://web.archive.org/web/20140306210621/http://www.nex-hack.info:80/wiki/development/current-tasks)
- [captura] Vitaliy_Kiselev, 2012-08-17, sobre el `MAIN` de la 1.ª generación (A33): «Yep, big number of references to SWF. Also MicroGL v2.3 is referenced … RS-232C Serial Debug Monitor Ver  0.02». — [hilo 21, Menus, p. 2](http://web.archive.org/web/20120915020828/http://nex-hack.info/talks/discussion/21/menus/p2). Someone100, 2012-08-27: usan «commont table structure for almost everything. From debug monitors to flash state machine. Since most of debug monitors commented out…». — [p. 3](http://web.archive.org/web/20120915004903/http://nex-hack.info/talks/discussion/21/menus/p3)
- [captura] Someone100, 2012-08-31: «They comment out debugging output in public firmware. But they leave the output function. So it's possible to turn the output on. The input is more complecated problem. And they mentioned RS232 in this rotime tables». — [hilo 38, USB Wire Remote](http://web.archive.org/web/20120915061356/http://nex-hack.info/talks/discussion/38/usb-wire-remote)
- [captura] kenan, 2013-03-12: la NEX-3 no graba AVCHD porque «the chip and codec ram is not soldered onto the board». — [hilo 80](http://web.archive.org/web/20130510100718/http://www.nex-hack.info:80/talks/discussion/80/flash-nex-3-with-nex-5-firmware-is-it-possible)
- [captura] kenan, 2014-05-21: «1.gen is based on CXD4115 … CElinux kernel 2.6.22 … 2.gen is based on CXD4132 series, kernel 2.6.29 … 3.gen is based on CXD90014 series, kernel 3.0». — [hilo 86, p. 3](http://web.archive.org/web/20150202094743/http://www.nex-hack.info:80/talks/discussion/86/firmware-unpacker/p3)

**Arranque, WBI y emulación (P6, P8)**
- [captura] kenan, 2013-09-28: «WBI … is the "warm boot image", think of it like the hibernate file on win … Every 2.gen has it», con la línea de `partinf.conf` de la A77 `/dev/nflasha5 36MB(System Group  Warm boot Image  /dev/nflasha5)` y `mount.conf: /dev/nflasha5 /wbi wbi 37748736 0`. Añade: «If the wbi is missing, the cam will cold boot and recreates it, but don't blame me». En el mismo mensaje: «Many other lost backup files, debug tools, doubled files are found in fw already». — [hilo 184](http://web.archive.org/web/20130930002227/http://www.nex-hack.info:80/talks/discussion/184/need-add-support-to-nflasha5-wbi1-file-format)
- [captura] zoiah, 2014-02-16: hizo correr el `orb-server` del firmware de la DSC-QX10 (CXD4132) en `qemu-arm` en modo usuario dentro de un chroot del ext2 del firmware. Para eso enlazó `usr` a `.`, porque el cargador está en `/glib/ld-linux.so.3`, y dejó un `nc -l 60884`, el socket interno que el binario espera. — [hilo 256](http://web.archive.org/web/20140914073849/http://www.nex-hack.info/talks/discussion/256/run-binorb-server-standalone)
- [captura] Someone100, 2012-09-23: el kernel de la cámara «hangs in QEMU». — [hilo 42](http://web.archive.org/web/20121014192318/http://www.nex-hack.info:80/talks/discussion/42/arm-emulator)

**Región, idioma, PAL/NTSC y 24p (P4, P7)**
- [captura] Wiki «languages» (texto de «nick», edición 2013-06-21): en la SLT-A65 J1 se cambió el idioma reemplazando el archivo de región dentro del firmware. En `/0110_backup/SYSAVIP-DSLR/PS_UUG` hay un archivo por región (`01000000_CX86800_J1.bin`, `21000000_CX86801_CEE2.bin` y otros). Hay que cambiar `01000000_CX86800_J1.bin` por el de otra región, corregir las sumas de `/0111_backup_sum/backup`, reempacar, flashear y hacer el reset de fábrica, que «will read the configuration file … and will take the J1 file (J1 is fixed in an eprom inside the camera)». — [languages](http://web.archive.org/web/20140306212845/http://www.nex-hack.info:80/wiki/development/languages)
- [captura] Wiki «gui_languages» (2015-08-10): la región también fija «the video frame rates based on NTSC - 60i or PAL - 50i markets» y el subconjunto de idiomas; J1 no tiene menú de idioma. — [gui_languages](http://web.archive.org/web/20150901083751/http://www.personal-view.com:80/faqs/sony-hack/gui_languages)
- [captura] kenan, 2014-05-05, sobre los archivos de región de la NEX-5T y la NEX-6: «try with tw6 file instead of cn2 … has the same last digit '1'. Afaik this is for the PAL/NTSC version differences. They changed the way this differences are implemented». tonyg responde que con TW6 tampoco funcionó. — [hilo 83, p. 3](http://web.archive.org/web/20140910040238/http://www.nex-hack.info:80/talks/discussion/83/language-modification-topic-for-users/p3)
- [captura] Resultados de 2014 en NEX de 2.ª generación: horace_li (NEX-6) cambió `CX76000_TW6.bin` y `CX76001_CE.bin` por `CX76000_J1.bin`, flasheó bien y el menú siguió en japonés. unreal3000 (NEX-5R, región AP2) cambió `CX75801_AP2.bin` por `CX75800_ALLLANG.bin`, sin idiomas extra ni 60p. NEX7User: «The only confirmed sucessful hacks … are with Gen A77 / A65». — [hilo 86, p. 3](http://web.archive.org/web/20150202094743/http://www.nex-hack.info:80/talks/discussion/86/firmware-unpacker/p3)
- [captura] nonor, 2013-12-17: en una A77 con el idioma cambiado, «If I choose 24p recording format, that freezes the camera, only recovering control after removing battery». — [hilo 83, p. 3](http://web.archive.org/web/20140910040238/http://www.nex-hack.info:80/talks/discussion/83/language-modification-topic-for-users/p3)
- [captura] Vitaliy_Kiselev, 2013-08-28, sobre flashear la actualización de otro país: «Hardware is the same, firmware is also the same». La región no cambia. — [hilo 148](http://web.archive.org/web/20130930002603/http://www.nex-hack.info:80/talks/discussion/148/changing-region-via-firmware-update)
- [captura] StefanH, 2013-03-17: «Did you see fwimage_ext2/bin/up.sh? What do you think about function "modreg"? Is it possible that calling it will change camera region permanently?». En las páginas capturadas nadie le responde. — [hilo 36, p. 4](http://web.archive.org/web/20130409080339/http://www.nex-hack.info:80/talks/discussion/36/language-modification-for-developers/p4)
- [local] En la CX580V, `nflasha8/bin/up.sh` define `func_modreg` y `func_modmod`. Ambas montan `/dev/nflasha1` y reescriben con `ud_datcnv.elf -e` los archivos `dat3` (región) y `dat2` (modelo). El valor debe tener 8 caracteres. En el volcado local, `nflasha1_unpacked/dat2` vale `01 00 01 21` (0x21010001) y `dat3` vale `00 00 00 00`, lo que cuadra con «región 0» de AGENTS.md. **Escribir ahí viola la regla 1 del proyecto.** El mismo `up.sh` tiene los modos `user` (sólo `mode`), `serv` (`mode` + `mode3`, «Service mode»), `ms` (`mode5`, «MS Crypt mode»), `ver` (`mode6`, «Version Skip mode»), `modver` (reescribe `/setting/updater/dat4`) y una opción `keep` comentada.
- [captura] Pedidos sin respuesta técnica: quitar el límite de 29:52 de la NEX-3 (hilo 209, 2013-10-15) y pasar una NEX-7 PAL a 24p (hilo 255, 2014-01-24). — [hilo 209](http://web.archive.org/web/20140914074136/http://www.nex-hack.info/talks/discussion/209/nex-3-video-recording-limit) · [hilo 255](http://web.archive.org/web/20140413013949/http://www.nex-hack.info:80/talks/discussion/255/region-change)

**Paquete de firmware, menús y herramientas de depuración (P2, P3, P6)**
- [captura] «image-structure» (1.ª generación): `usr/bin` trae `mpr_monio`, `debugio`, `mpr_viewLog`, `memtool`, `sen`, `testcmd`, `debugio_core`, `av_conio` y `up`, y los escenarios incluyen `app_repair_snr.esf`. — [image-structure](http://web.archive.org/web/20150901083756/http://www.personal-view.com:80/faqs/sony-hack/image-structure)
- [local] La CX580V conserva parte de ese juego en `nflasha8/bin`: `testcmd.elf`, cuyo uso es `<options> [osal_id] [data] ...` con `--sync` («invoke sync msg instead of async msg»), y `sndcmd.elf`, con `[osal_id] [size]:[data]`, `--sid` («source OSAL_ID (default: 0x00dc0000)») y `--ifile/--ibfile/--obfile`. También están `rcvcmd.elf`, `mwf_monio.elf`, `mpr_monio.elf`, `mpr_viewLog.elf`, `adjstctl.elf`, `ulogio.elf`, `ujlog.elf`, `netblog.elf`, `appdbglv.sh` (niveles de log por vista, modelo y widget, por ejemplo `-q view/TOOL_MENU`) y `change_mode.sh`.
- [local] `change_mode.sh` documenta los valores de `/setting/mode/dmode`: `0` sólo kernel, `1` + infra global, `2` + arranque LIRO, `3` + appFw (por defecto), `1n/2n/3n` sin driver unificado, `3L` LeakTracer, `3w` «sslibc Wfree Check», `3k` «KMC ICE», `3g` gdb en el destino, `3s` gdbserver, más `d` (DUMA) y `e` (Electric Fence). Esto precisa la lista de knowXD que recoge el informe anterior.
- [captura] «usbinfo»: además de PTP y MSC, `libusb.so` declara un descriptor «SEN (could be some sort of console access but uses the 0xFF class codes …)». — [usbinfo](http://web.archive.org/web/20150901083413/http://www.personal-view.com:80/faqs/sony-hack/usbinfo)
- [captura] «scenarios» y «menus»: la interfaz de la 1.ª y la 2.ª generación es «modification of SWF v3», con `app_eps_rsf.rsf` de recursos y `app_top_snr.esf` como escenario inicial. El ESF empieza con `ESFf`, versión mayor y menor, y un indicador de compresión en el byte 26, comprimido con el LZ77 de Sony que empieza en `0xF0`. — [scenarios](http://web.archive.org/web/20150901083356/http://www.personal-view.com:80/faqs/sony-hack/scenarios) · [menus](http://web.archive.org/web/20150831203047/http://www.personal-view.com:80/faqs/sony-hack/menus)
- [captura] «main_code»: «Camera uses customized flash engine for user interface and commands. Flash state machine use table with command identifier and function reference». «main_resources» trae la tabla de tareas µITRON del `MAIN` de 1.ª generación (`tsk_sys_initial`, `tsk_rec_cam`, `tsk_measure` y otras). — [main_code](http://web.archive.org/web/20140306212057/http://www.nex-hack.info:80/wiki/development/main_code) · [main_resources](http://web.archive.org/web/20140306212618/http://www.nex-hack.info:80/wiki/development/main_resources)
- [captura] «server»: en la HDR-AS15 (CXD4132), el servidor del visor en el puerto 10000 permite recorrer directorios (`http://10.0.0.1:10000/sony/../../../../`), sólo para texto. — [server](http://web.archive.org/web/20150122211543/http://www.nex-hack.info:80/wiki/development/server)
- [captura] «fwtool» (2013-09-03): v0.6final reempaqueta de nivel 3 a archivo flasheable y «contains a bug in auto increment of version number»; queda planeada la v0.7. En el foro, kenan anuncia el 2014-05-21 «New release of fwtool v07b12 … changed handling of fsimg», posterior al espejo de erik-smit, que llega a 2014-02-22. El código vivía en un Fossil (`nex-hack.info/vcs/`) con usuario `nex_dev`; Wayback sólo guarda la página de login. — [fwtool](http://web.archive.org/web/20140301205337/http://www.nex-hack.info:80/wiki/development/fwtool) · [hilo 86, p. 3](http://web.archive.org/web/20150202094743/http://www.nex-hack.info:80/talks/discussion/86/firmware-unpacker/p3)
- [captura] «responsibility» (2013-10-06) lista quién hacía qué: kenan (fwtool), Madwyn (GUI), アンドレアス (USB y WLAN), Diamondback (Android), Meawoppl («qeum hacking toward kernel emulation»), Falco (AutoISO), Hdi (protocolo E-mount) y agentjj24 (mapas de memoria). — [responsibility](http://web.archive.org/web/20140302224153/http://www.nex-hack.info:80/wiki/development/responsibility)

**Recuperación (P6)**
- [captura] NEX7User, 2014-05-21 y 22: una NEX-7 con firmware modificado falló al 45 % y quedó «in a 'bricked' state with a blinking red light under the battery». Se recuperó flasheando el firmware oficial, pero el updater no deja bajar de versión, así que tuvo que usar una más nueva cada vez. — [hilo 86, p. 3](http://web.archive.org/web/20150202094743/http://www.nex-hack.info:80/talks/discussion/86/firmware-unpacker/p3)

### Inferences
- El «Debug Monitor» de av-cam es el candidato más directo a una consola de depuración de la parte de cámara [P3, P6, P8]. Como av-cam corre como hilos del kernel (`liro.ko`), su salida probablemente vaya por `printk` o por la UART del kernel, pero eso no está comprobado. Habría que buscar en la CX580V quién referencia esas cadenas en `av-cam.bin`, cómo se activa el monitor y por dónde lee sus órdenes.
- Que el kernel GPL de la NEX-5R/6 traiga `cxd4132_qemu_defconfig` sugiere que Sony tenía un modelo de QEMU para la CXD4132 [P8]. Si el paquete GPL de una handycam CXD4132 (por ejemplo el de la HDR-CX210 que cita el informe) trae ese defconfig y su `mach-cxd4132`, puede servir de base para emular el modo normal. Hay que mirar el árbol GPL antes de suponerlo.
- La técnica de zoiah (qemu-arm en modo usuario, en chroot, con los sockets internos simulados) es el antecedente público más cercano al `preload/test/run.sh` del proyecto, y sugiere el mismo camino para correr `testcmd.elf` o `mwf_monio.elf` fuera de la cámara.
- El fracaso del cambio de región por `PS_UUG` en todas las NEX de 2.ª generación y el «They changed the way this differences are implemented» de kenan apuntan a que en la CXD4132 la región se fija en otro lado. En la CX580V el candidato es `nflasha1/dat3`, que `modreg` reescribe. Escribir ahí está prohibido por la regla 1; lo único posible sin romperla es leer y decodificar `dat3` (`ud_datcnv.elf -d`) para entender el formato [P4].
- El cuelgue al elegir 24p en una A77 con la región cambiada advierte que forzar modos fuera de región puede colgar la parte AV [P4].
- `testcmd.elf` y `sndcmd.elf` mandan mensajes OSAL arbitrarios desde la consola: son herramientas de fábrica para probar órdenes directas sin pasar por `libObj` [P3]. Habría que ver qué colas aceptan y si sirven para foco o balance de blancos, primero en QEMU y luego en la cámara.

### Gaps
- Las páginas `development:emulation`, `lensprotocol`, `webkitbrowser` y `gui_languages` de nex-hack.info sólo tienen capturas de 2016 con el dominio estacionado en GoDaddy, y no se copiaron a personal-view (salvo `gui_languages`). No están en archive.ph (sólo hay capturas de 2021 en adelante, ya estacionadas) ni en los índices de CommonCrawl de 2013 a 2015, que sólo tienen 3 a 9 URLs del foro. La página de emulación se perdió.
- En 2012, `development:general`, `rootfs`, `image-structure`, `menus`, `scenarios`, `lzpt` y `fwpackage` daban «Permission Denied» en nex-hack.info. Sus versiones de 2015 sí están en personal-view y las leí.
- No hay captura del Fossil ni de los adjuntos `fwtool_v06b*.zip` (404). No pude ver el código de fwtool v0.7.
- El hilo 20, «smooth change for shutter, iso/gain and aperture», no tiene captura con estado 200. Tampoco la tienen los hilos 16, 17, 18, 162 y 240.
- No apareció ninguna mención de handycam (CX, PJ o TD) ni de la CX580V en los 118 hilos con captura.

---

## 2. Fandom: ¿qué páginas existen?

### Takeaway
No existe ninguna wiki de nex-hack en Fandom ni en Wikia. La wiki real era una DokuWiki en `nex-hack.info/wiki/`, que en agosto de 2015 se mudó a `personal-view.com/faqs/sony-hack/`. El «402 de Fandom» del informe anterior venía casi seguro de una URL adivinada que no existe.

### Cited Findings
- [vivo] `https://nex-hack.fandom.com/wiki/Special:AllPages` → 404 «This wiki does not exist». Lo mismo con `nexhack`, `sonyhack` y `sony-hack.fandom.com`, todos probados con `navega`.
- [vivo] La búsqueda de comunidades de Fandom no encuentra nada para «nex hack». — [Special:SearchCommunity](https://community.fandom.com/wiki/Special:SearchCommunity?query=nex+hack)
- [captura] La API CDX de Wayback no devuelve ninguna captura para `nex-hack.fandom.com/*`, `nex-hack.wikia.com/*`, `nexhack.fandom.com/*`, `nexhack.wikia.com/*`, `sony-hack.wikia.com/*` ni `sony-nex-hack.wikia.com/*` (consultas del 2026-11-26 y 27, guardadas en `cdx/`).
- [captura] La wiki original se presenta como «Sony NEX and Alpha Hack Wiki», generada con DokuWiki, y su `start` enlaza a «Hacked models», «Firmware package extraction» y «Firmware patching». — [start, 2012-10-13](http://web.archive.org/web/20121013091623/http://www.nex-hack.info/wiki/start)

### Inferences
- No queda nada por buscar en Fandom. Todo el contenido recuperable de esa wiki está en las capturas de nex-hack.info y de personal-view, y ya está leído.

### Gaps
- No sé qué URL exacta dio el 402 en la investigación anterior; su nota no la guardó.

---

## 3. personal-view.com: subpáginas de /faqs/sony-hack/ e hilos del foro

### Takeaway
Las subpáginas sí tienen captura: todas las del índice de 2015 se pueden leer en capturas de 2015 a 2018. Las capturas de 2026 de `changing-languages`, `esf-files`, `firmware-package`, `firmware-files-package` y `device-information-file` dicen «This topic does not exist yet» porque son enlaces rotos del índice nuevo. Eso, junto con que el sitio vivo hoy devuelve respuestas vacías, explica probablemente los 404 del informe anterior. La única página nueva de peso es **fwupdateprocess**, que documenta el protocolo SCSI del updater. Los hilos del foro de personal-view añaden la ubicación del firmware del microcontrolador de lente (TMP19A44), un truco de versión en el archivo de preajuste de región y la opinión de Vitaliy sobre las handycam.

### Cited Findings
- [captura] Índice «hack-development» del 2015-08-10 con todas las subpáginas: beginners-place, current-tasks, responsibility, source-control, fwtool, simple-gui, fwpackage, boot, image-structure, main, device_info, languages, nex5r-6-android-reversing, server, usbinfo, scenarios, menus, gui_languages, rootfs, axfs, lzpt, fwupdateprocess, nex6-av-cam.bin-reversing, fwupdates y sources. — [hack-development, 2015-08-31](http://web.archive.org/web/20150831003933/http://www.personal-view.com:80/faqs/sony-hack/hack-development)
- [captura] **fwupdateprocess** (2015-08-10): el actualizador de Windows usa `SCSI_PASS_THROUGH_DIRECT` hacia un targetID «seemingly hidden», con un CDB de 12 bytes de opcode `0x7A`. Cada orden lleva una cabecera de 32 bytes en little-endian: `0x00` largo de datos, `0x04` siempre 1, `0x06` orden, `0x08` dirección (1 = cámara→PC), `0x0A` secuencia. Órdenes en `libupdaterufp.so`: `0x0001 Init`, `0x0010 Check Guard`, `0x0020 Query Version`, `0x0030 Switch Mode`, `0x0040 Write Firmware`, `0x0100 Complete` y `0x0200 Get State`. El autor avisa que es «early work in progress». — [fwupdateprocess](http://web.archive.org/web/20150901083549/http://www.personal-view.com:80/faqs/sony-hack/fwupdateprocess)
- [captura] «boot»: lista de la imagen de arranque del updater (`bin/keep_backupdata_*.sh`, `loader_writer.sh`, `uc_util_infosector`, `ud_datcnv`, `us_chkver_updater.sh`, `bodylib/libupdaterbody.so` y otros). Someone100 lo comentó el 2012-08-16 y Vitaliy respondió: «all updater related stuff is present here. And it was the reason why you can restore body if you messed up with main executable». — [boot](http://web.archive.org/web/20150901083151/http://www.personal-view.com:80/faqs/sony-hack/boot) · [hilo 26](http://web.archive.org/web/20120817105250/http://www.nex-hack.info:80/talks/discussion/26/boot-image)
- [captura] «lzpt» y «axfs»: `nflasha3` es FAT y `nflasha8` es ext2, los dos en LZPT con bloques de 64 KiB (versión `0x10`; la `0x11`, de 128 KiB, en EA50 y FS700). El initrd de la 2.ª generación es AXFS. — [lzpt](http://web.archive.org/web/20150901083201/http://www.personal-view.com:80/faqs/sony-hack/lzpt) · [axfs](http://web.archive.org/web/20150831203027/http://www.personal-view.com:80/faqs/sony-hack/axfs)
- [captura] Leegong, 2016-04-03 (comentario 215806): «Just found Toshiba TMP19A44 on A55 camera main board, according to A55 level3 repair manual, TMP19A44 is responsibile for E mount communication with lens». ma1co, 2016-04-15 (comentario 216430): «Look at the 0630_ca / 1630_ca folder in the firmware tar. It contains definitely TMP19A44 MIPS code. The 2nd gen devices have a CA_0_0.BOO file (probably the bootloader) and CA_2_0.APP». — [Sony Alpha hacks talk, p. 2 y 3](http://web.archive.org/web/20160428041718/http://www.personal-view.com:80/talks/discussion/12756/sony-alpha-hacks-talk/p3)
- [captura] satmat, 2017-03-13 (comentario 232244): en una RX10 japonesa (3.ª generación) el cambio de archivo de región no bastaba, pero funcionó tras tocar «backup settings file header … looks like byte at 0x38 offset is file version and needs to be incremented for camera to read it». En el mismo hilo Leegong (comentario 232251) escribe: «Just find lots of debug calls in NEX5N_v0102_av-cam!». — [p. 8](http://web.archive.org/web/20170412224448/http://www.personal-view.com:80/talks/discussion/12756/sony-alpha-hacks-talk/p8)
- [captura] Vitaliy_Kiselev, 2016-07-06 (comentario 220246), sobre camcorders: «remember that camcorders are always produced in much smaller quantity, hence almost none of hacks». En el mismo hilo, una AX53 no aceptó instalar apps. — [p. 6](http://web.archive.org/web/20160919100553/http://www.personal-view.com:80/talks/discussion/12756/sony-alpha-hacks-talk/p6)
- [captura] Hilo 382 (2011), sobre la NEX-VG10: Vitaliy responde que el firmware está cifrado y que «Adding low level functions as you described is almost impossible». — [hilo 382](http://web.archive.org/web/20111011013533/http://personal-view.com/talks/discussion/382/sony-nex-vg10-and-nex-cameras-firmware-hack)

### Inferences
- El protocolo de fwupdateprocess es el que Sony-PMCA-RE implementa para entrar al updater. Al proyecto le sirve sólo como documentación del camino de rescate que ya usa [P6], no como capacidad nueva.
- El truco de versión del byte 0x38 de satmat sugiere que el preajuste de región se aplica sólo si su versión cambia. Si en la CX580V existiera un preajuste equivalente (por ejemplo el `Backup.bin` de `nflasha1_unpacked/boot/initrd.img_unpacked/root/`, que usa `recovery_backup.sh`), la versión podría pesar igual [P7]. Es especulación y habría que leerlo.
- Que el código del microcontrolador de lente vaya en la carpeta `0630_ca` del paquete sugiere que en la handycam el control fino de lente (foco, iris) podría vivir en un microcontrolador aparte, no en av-cam [P3]. No sé si la CX580V tiene un microcontrolador así; el manual L3 lo diría.

### Gaps
- La captura de 2026 del hilo 4520 («nex-hack-faq-hardware-hacking», un hilo de nex-hack importado a personal-view) muestra «Maintenance Mode». Antes ese número era otro hilo, «eye-controlled-tv». El sitio vivo devuelve una respuesta vacía con `navega` y archive.ph no lo tiene. **No se pudo leer el FAQ de hardware.**
- No se pudieron leer las fotos de la wiki que acompañan a fwupdateprocess (`fwupdate_1.png` a `fwupdate_7.png`, capturadas en 2016). No las bajé; tienen captura y se pueden revisar si hace falta.
- El listado CDX de hilos de personal-view se cortó a los 294 KB (698 hilos, hasta el 28154). Puede haber hilos de Sony posteriores que no vi.

---

## 4. dvxuser «Sony NEX/Axx Firmware hacking» (8 páginas leídas completas): UART, JTAG, conector y consola

### Takeaway
Hay que corregir el informe anterior. El hilo dice «**112500**Bd», no 115200, y lo dice de la **NEX-VG10** (CXD4115), no de todas las NEX ni de una CXD4132. Lo nuevo y concreto es el **pinout de un conector de servicio de 18 pines (CN3504)**, en una foto con el esquema que sigue en línea: BOOT_MODE_1, dos UART y JTAG del procesador de imagen. Además, la UART0 de una NEX-5N muestra el arranque del kernel del updater y, con BOOT_MODE_1 en alto, la cámara entra en otro modo de arranque. Nadie llegó a un shell.

### Cited Findings
- [vivo] Nexus7, 2011-08-26: «There is this this nice connector in VG10 with two UARTs and even JTAG port. In firmware, there is 112500Bd console active on one UART with servicing commands and possible shell access. Also it may be possible to read full flash image using JTAG or upload dumper code to RAM and execute it. There are also test points for all these pins, some near CPU, some directly behind connector. Similar connector is in NEX series, but I don't have LEVEL 2 service manual to confirm it». — [dvxuser, p. 1](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/)
- [vivo] Nexus7, 2011-08-25: «From parts of firmware, I can see there is debug console with commands to read/write memory». También explica que un `FirmwareUpdater.log` vacío junto a `SPUFirmwareUpdater.exe` activa un registro detallado del protocolo de actualización. — [p. 1](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/)
- [vivo] rdrr, 2012-01-18: «Confirmation of the nice connector on Nex-3/5 for the Image Processor, contains JTAG, UART0 and UART1 (UART2 is pin 3 & 4 of the lens connector) for the Image Processor … Main CPU JTAG is on the underside». Remite a dos fotos de imgur y a manuales de servicio L2/L3 de la NEX-3/5 subidos a 2shared. — [p. 1](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/)
- [vivo] La foto `https://i.imgur.com/hv4sz.jpg` (copia en Wayback desde el 2023-04-30) muestra la placa de la VG10 y el esquema del **CN3504, 18P**: 1 REG_GND, 2 BOOT_MODE_1, 3 EVER_3.15V, 4 OR_UART0_RXD, 5 OR_UART0_TXD, 6 OR_UART1_RXD, 7 OR_UART1_TXD, 8 JTAG_NSRST, 9 OR_TDO, 10 OR_RTCK, 11 OR_TCK, 12 OR_TMS, 13 OR_TDI, 14 NTRST, 15 JTAGSEL_0, 16 REG_GND y 17-18 a tierra, con puntos de prueba CL1205 a CL1212 junto al conector. La segunda foto, `https://i.imgur.com/8WoJS.jpg`, «NEX-3/5 Main CPU JTAG», marca TDI, TRST, TDO, TMS, TCK, DINT y !EJE en los puntos CL3001 a CL3007, alrededor del BGA IC3001. — [hv4sz.jpg](https://i.imgur.com/hv4sz.jpg) · [8WoJS.jpg](https://i.imgur.com/8WoJS.jpg); copias locales en `img/`
- [vivo] rdrr, 2012-05-15, en una NEX-5N: «UART0 shows the following just after the camera is reset for a firmware update: … Uncompressing Linux……… done, booting the kernel.» y «booting with BOOT_MODE_1 high results in no serial output and the SD card light flashing on and off at about one second intervals». El 2012-06-04, ya actualizada: «no other output on UART0 unfortunately». — [p. 2](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/page-2)
- [vivo] someone1.00, 2012-08-02: «Camera has no gdb inside. At least in firmware». El 2012-08-11: «Emulation is not possible. QEMU crashes with original kernel». — [p. 4](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/page-4) · [p. 7](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/page-7)
- [vivo] El resto de las 158 entradas (p. 3 a 8) son pedidos, discusión legal y la colecta para comprar cámaras, que se mudó a nex-hack.com. Nadie reporta haber abierto un shell por UART ni usado el JTAG.
- [local] La CX580V guarda en `/setting` (`nflasha2_unpacked/kemco.txt`) una línea de arranque del kernel con `console=ttyAM0,115200n8`, `cfg.uart=5`, `klog.addr=0x833E0080 klog.size=0x20000`, `tmonitor.addr=0x8F900000` y `exception.reboot=1`. Coincide con la consola en `ttyAM0` a 115200 que ya anota firmware-analysis.md.

### Inferences
- «OR_» en las señales de la VG10 corresponde casi seguro a «Ortus», el nombre interno de la CXD4115 según OpenMemories. En la CXD4132 («Opal/Avip?») las señales del manual L3 deberían llevar otro prefijo. No hay ninguna foto o pinout publicado de la CXD4132 [P6].
- Por las señales DINT y EJE, el «Main CPU JTAG» de la NEX-3/5 es un EJTAG MIPS. Sería entonces el microcontrolador Toshiba TMP19A44, el «main CPU» en la nomenclatura del manual de Sony, y no el ARM del BIONZ. Esto concuerda con la lista de procesadores de kenan.
- BOOT_MODE_1 en alto con el LED de la SD parpadeando parece un modo de arranque alternativo de la boot ROM, quizá desde SD o en espera de USB. Si la CX580V expone una señal así, sería un camino de rescate que no depende de `nflasha1` [P6]. No está probado en ninguna cámara.
- La cifra de 112500 parece un error de tipeo por 115200. El `kemco.txt` de la CX580V dice 115200.

### Gaps
- No hay pinout publicado para ninguna placa CXD4132. Los manuales de servicio L2/L3 de la NEX-3/5 que enlaza rdrr estaban en 2shared y no los busqué ni los bajé.
- Nadie documentó qué órdenes acepta la consola de servicio de la VG10 ni qué muestra la UART1.

---

## 5. Menús ocultos y modos de servicio en handycam y NEX (2010-2014)

### Takeaway
No encontré ninguna combinación de teclas, toque ni archivo en la tarjeta que abra un menú oculto en una handycam de 2010 a 2014, ni nada que cambie región o PAL/NTSC desde la cámara. El servicio de Sony entra por **USB con el programa de ajuste SeusEX y una llave HASP**, y en las handycam con disco de 2007 eso incluye «Destination Data Write», o sea la región. El firmware de la CX580V tiene piezas de ese mundo (`adjstctl.elf`, un `.hasp` en `/setting`, el modo `serv` de `up.sh`), sin documentación pública.

### Cited Findings
- [vivo] Manual de servicio DCR-SR32E a SR82E (2007), sección «6-2. SERVICE MODE»: «The adjustment software (SeusEX) can change operational coefficients of signal processing, EVR data, etc. same as the adjustment remote commander. The SeusEX performs two-way communication between PC and camcorder using the USB terminal». Para conectar: «1) Connect the HASP key to the USB terminal of the PC. 2) Connect the PC and camcorder with the USB cable. 3) Start the SeusEX». El índice del manual incluye «1-4 Destination Data Write», «USB Serial No Input», «MAX GAIN adj» y «AWB adj». — [manualshive, DCR-SR42, p. 49](https://manualshive.com/en/manual/sony/dcr-sr42/392430?page=49)
- [buscador] Un manual de servicio de la HDR-CX11E/CX12 pide usar el manual «together with the Automatic Adjustment Program» y la llave HASP con SeusEX, que se piden al servicio técnico de cada región. servlib.com está estacionado y no lo pude abrir. — [servlib, CX12](https://servlib.com/sony/movie/hdr-cx11e-hdr-cx12-hdr-cx12e-sm3.html?start=40)
- [vivo] Existe en elektrotanya un manual **«SONY HDR-CX570 HDR-CX580 HDR-CX590 HDR-PJ580 HDR-PJ590 HDR-PJ600 L3 VER1.0»** (PDF de 61 páginas, 6,1 MB). No lo bajé. — [elektrotanya](https://elektrotanya.com/sony_hdr-cx570_hdr-cx580_hdr-cx590_hdr-pj580_hdr-pj590_hdr-pj600_l3_ver1.0.pdf/download.html)
- [captura] NEX7User, 2014-05-24, sobre la SLT: «reset process for SLT series is key combination of menu and movie record (red button) + replay (> button) held for a while while turning camera on». Antes, unreal3000 recordaba vagamente una combinación de botones que dejaba la cámara en un estado de recuperación, sin encontrarla. — [hilo 86, p. 3](http://web.archive.org/web/20150202094743/http://www.nex-hack.info:80/talks/discussion/86/firmware-unpacker/p3)
- [captura] Única técnica por archivo en la tarjeta que apareció, y es de una cámara profesional: en la PMW-F5, el archivo de ajustes `Private/Sony/PRO/CAMERA/PMWF5/001.ALL` se edita en la línea 150 (por ejemplo `150=4096,2160,1,25,…`) y se vuelve a calcular su md5 en la primera línea para activar 4K. Vitaliy lo publicó el 2014-08-27. — [hilo 267](http://web.archive.org/web/20141001075101/http://www.nex-hack.info:80/talks/discussion/267/sony-f5-pro-camera-4k-enabling)
- [vivo] En las guías de Sony, el «Demo Mode» de Alpha y Cyber-shot sólo reproduce películas protegidas de la tarjeta. Es una función documentada, no un modo oculto. — [guía de ayuda, Demo Mode](https://helpguide.sony.net/dsc/2410/v1/en/contents/0601_demo_mode.html)
- [vivo] Para cámaras de cinta de 2005 circulaba que con el control remoto de ajuste (RM-95) se podía pasar una FX1 de NTSC a PAL. En DV Info Net lo desmintieron: el artículo original hablaba de habilitar la entrada FireWire en MiniDV PAL antiguas. — [DV Info Net, 2005](https://www.dvinfo.net/forum/sony-hvr-z1-hdr-fx1/47330-turn-your-sony-ntsc-camera-into-pal-article-claims-possible-sony-remote-print.html)
- [local] En la CX580V: `nflasha2_unpacked/.hasp` (88 bytes en cero), `bin/adjstctl.elf`, el modo `serv` de `up.sh` y los escenarios `MPR_SCN_SET_FACTORY_MODE.so` y `MPR_SCN_EXEC_FACTORY_MODE.so`.

### Inferences
- En estas handycam, la región la escribe en fábrica o en el servicio técnico el programa de ajuste por USB («Destination Data», así en 2007). En la CX580V lo más probable es que acabe en `nflasha1/dat3` vía `modreg`. No hay evidencia de un camino de usuario [P4]. Habría que leer el manual L3 por si nombra el modo de servicio o el conector, y seguir en el firmware quién usa `adjstctl.elf` y `.hasp`, sin ejecutar nada.
- El modo `serv` de `up.sh` (`mode` + `mode3`) arranca el updater en modo servicio. Puede ser el lado de la cámara de SeusEX, pero no está comprobado. Como sólo crea banderas en `/setting/updater/`, el mismo sitio donde la guarda del proyecto ya escribe `mode`, sería una prueba de bajo riesgo si se confirma antes en QEMU qué hace el updater con `mode3` [P6].

### Gaps
- No encontré combinaciones de teclas documentadas para handycam CX o PJ de 2011 a 2013, ni para NEX. Busqué en inglés, japonés y ruso.
- No leí el manual L3 de la CX580 (no se descarga, por las reglas). No sé si trae el conector de depuración, puntos de prueba de UART o la sección de ajustes.

---

## 6. Foros en ruso, japonés, chino, alemán, español y portugués

### Takeaway
Casi nada. El buscador disponible sólo cubre EE. UU.; Yandex pidió captcha; DuckDuckGo bloqueó el cliente; Bing con `site:` no devolvió nada; las búsquedas internas de Qiita, Zenn y Hatena no dieron resultados. Lo único que apareció en chino es una copia en CSDN de un repositorio de 2018 sobre la NEX de 1.ª generación. No encontré ningún hack de handycam (PAL/NTSC, idioma, 25p, bitrate) en ningún idioma.

### Cited Findings
- [vivo] CSDN aloja «NEX1stGenRE：索尼NEX（第一代）逆向工程» (zip de 4 KB, 2021-05-10), descrito como el estudio de «USB描述符、调试函数及区域补丁机制» (descriptores USB, funciones de depuración y el mecanismo del parche de región). Es una copia de `jeanthom/NEX1stGenRE` (2018-06-28). Ese repositorio analiza el «Jap2Eng patcher» de Someone 1.00 («It appears that the patching utility is just patching binary»), compara los firmwares de la NEX-3 y la NEX-5 V5 (cambian `deviceInfo.xml`, `av.bin`, `initreg.bin`, `CA_FRAM.BIN` y `CA_FROM.BIN`) y señala en `main` funciones de depuración tipo printf y assert. — [búsqueda en CSDN](https://so.csdn.net/so/search?q=%E7%B4%A2%E5%B0%BC%20%E7%9B%B8%E6%9C%BA%20%E5%9B%BA%E4%BB%B6%20fwtool) · [jeanthom/NEX1stGenRE](https://github.com/jeanthom/NEX1stGenRE)
- [captura] En nex-hack hubo hispanos y rusos, pero escribieron en inglés por regla: Someone100 pidió «Guys please use english» cuando azya escribió en ruso el 2012-08-15 ([hilo 21](http://web.archive.org/web/20120817100942/http://www.nex-hack.info:80/talks/discussion/21/menus)). 25111993 escribió en ruso el 2013-03-12 proponiendo flashear firmware de NEX-5 en la NEX-3, y kenan le respondió con lo del chip AVCHD. — [hilo 80](http://web.archive.org/web/20130510100718/http://www.nex-hack.info:80/talks/discussion/80/flash-nex-3-with-nex-5-firmware-is-it-possible)
- [vivo] En 2025, ProVideoCoalition (Allan Tépper) describe cómo volver «quasi-worldcam» muchas Sony con Sony-PMCA-RE. Las handycam de la lista son todas CXD90014 (AX33, AX53, CX450, CX625 y otras), de 2014 en adelante; no hay ninguna CXD4132. — [ProVideoCoalition, 2025-07-14](https://www.provideocoalition.com/how-to-make-many-sony-cameras-worldcams-ntsc-pal-derived-rates-free-of-charge/)

### Inferences
- La comunidad de 2012 a 2014 se concentró en foros en inglés (dvxuser, nex-hack.info, nex-hack.com de EOSHD, personal-view). Los foros en otros idiomas, por lo que alcancé a ver, sólo repetían el cambio de idioma de nex-hack.

### Gaps
- **4pda.to y forum.ixbt.com**: la búsqueda no encontró hilos de firmware de handycam. Yandex exigió captcha y no lo resolví. Faltaría usar la búsqueda interna de cada foro, que depende de Google o de sesión.
- **5ch/2ch, Hatena, Qiita, Zenn**: sin resultados para «ハンディカム ファームウェア 解析», «HDR-CX ファームウェア 改造», `fwtool` ni `Sony-PMCA-RE`. Hatena devolvió 404 en `hatenablog.com/search`. No busqué en archivos de logs de 5ch (por ejemplo, log-soku).
- **Zhihu, Bilibili y foros chinos**: Bilibili no trajo nada pertinente y Zhihu no aparece con `site:`.
- **slashcam.de, dslr-forum.de y foros en español y portugués**: sin resultados pertinentes.
- **rutracker**: no se intentó, por las reglas.

---

## 7. ¿Hay firmware Sony con símbolos, binarios de depuración o documentación interna filtrada?

### Takeaway
No encontré ningún firmware de depuración ni compilación «ilog» o DUMA filtrada. Lo que hay son: (a) firmwares de producción con mucha información de depuración (av-cam sin strip según kenan, nombres de 573 `.cpp`, herramientas de prueba que vienen en el paquete); (b) anotaciones de terceros (el IDC de steelcnn); (c) **manuales de servicio L2/L3** en sitios públicos, incluido el L3 de la CX580; (d) el programa de ajuste SeusEX, que no es público y exige llave HASP. No bajé nada de eso.

### Cited Findings
- [captura] kenan, 2013-02-14: av-cam.bin «not stripped with debug console». — [hilo 71](http://web.archive.org/web/20130306023740/http://nex-hack.info/talks/discussion/71/about-av-cam-bin-found-in-unpacked-5n-firmware)
- [captura] kenan, 2013-09-28: «Many other lost backup files, debug tools, doubled files are found in fw already». — [hilo 184](http://web.archive.org/web/20130930002227/http://www.nex-hack.info:80/talks/discussion/184/need-add-support-to-nflasha5-wbi1-file-format)
- [vivo] `steelcnn/nex-hack` tiene el IDC (script de IDA con nombres) del av-cam de la NEX-6 1.01, de 2,1 MB, de 2013-06-10. Según la wiki «This IDC is incomplete». — [steelcnn/nex-hack](https://github.com/steelcnn/nex-hack)
- [vivo] Manuales de servicio publicados: L3 de la HDR-CX570/580/590 y PJ580/590/600 en elektrotanya ([enlace](https://elektrotanya.com/sony_hdr-cx570_hdr-cx580_hdr-cx590_hdr-pj580_hdr-pj590_hdr-pj600_l3_ver1.0.pdf/download.html)); L2 y L3 de la ILCE-6000 en elektrotanya según AudioMark, 2015-02-17 ([hilo 272](http://web.archive.org/web/20150222161404/http://www.nex-hack.info:80/talks/discussion/272/ilce-6000-level-2-3-document)); el pinout del puerto de 24 pines de la A57 «available in the level 3 document», que AudioMark compró en manualsparadise, 2014 ([hilo 266](http://web.archive.org/web/20150207021057/http://www.nex-hack.info:80/talks/discussion/266/a57-comms-port)); L2 y L3 de la NEX-3/5 en 2shared, según rdrr en 2012 ([dvxuser, p. 1](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/)).
- [vivo] SeusEX y su llave HASP se consiguen sólo por el servicio técnico regional (manual de la DCR-SR42, sección 6-2). — [manualshive](https://manualshive.com/en/manual/sony/dcr-sr42/392430?page=49)
- [captura] Someone1.00, 2012-08-11, en nex-hack.com: la cadena de compilador del firmware de 1.ª generación es `GCC: (GNU) 4.1.1 20080610 (Sony CE Linux 4.5.1)`. — [nex-hack.com, tema 1077](http://web.archive.org/web/20121201065520/http://www.nex-hack.com:80/topic/1077-technical-questions-camera-related-clarifications/)
- [captura] anmcy, 2012-11-08, en nex-hack.com: en el kernel GPL de la NEX-5R/6, `linux/arch/arm/configs` trae `cxd4132_net_defconfig`, `cxd4132_qemu_defconfig`, `cxd4132_wlan_nfs_defconfig` y `cxd4132_wlan_set_defconfig`; «looks like sony use qemu for development». También trae `CONFIG_LZ77_INFLATE=y`. jerzees, 2012-11-01: el kernel de Sony arranca en QEMU hasta «Error: unrecognized/unsupported processor variant». — [nex-hack.com, tema 1081, p. 2](http://web.archive.org/web/20121122145921/http://www.nex-hack.com:80/topic/1081-how-can-i-help-arm-emulator/page__st__20)
- [captura] Someone1.00, 2012-10-04: «Every single console output in main was replaced with stubs … The have something called binary log (blog)». — [nex-hack.com, tema 1116, p. 2](http://web.archive.org/web/20121121182026/http://www.nex-hack.com:80/topic/1116-linux-activity-test/page__st__20)

### Inferences
- El manual L3 de la CX580 es el documento público con más chances de traer los puntos de prueba de UART, BOOT_MODE y JTAG de esta placa [P6]. Está en un sitio de terceros y las reglas prohíben bajarlo; si se decide consultarlo, lo debería hacer el dueño.
- El `cxd4132_qemu_defconfig` es lo más parecido a «documentación interna» útil para P8, y viene en el GPL que Sony publica, no en una filtración.

### Gaps
- No busqué el IDC de steelcnn por dentro (no lo bajé), así que no sé cuántos nombres trae ni si sirven para la CX580V.
- No encontré firmware de depuración, `.map`, encabezados ni SDK internos filtrados de BIONZ en ninguna fuente.

---

## 8. Personas y alias con notas técnicas útiles sobre esta generación, y dónde está su material

### Takeaway
El material técnico de 2011 a 2014 sale de unas diez personas, casi todas con seudónimo y sin rastro posterior. Lo útil para la CX580V está en la wiki de personal-view y en tres hilos: el 71 de nex-hack (kenan), el de dvxuser (Nexus7, rdrr) y el 1081 de nex-hack.com (anmcy, jerzees).

### Cited Findings
- **Vitaliy_Kiselev**: administrador de nex-hack.info y de personal-view. La wiki está en `personal-view.com/faqs/sony-hack/` (capturas de 2015 a 2018). — [hack-development](http://web.archive.org/web/20150831003933/http://www.personal-view.com:80/faqs/sony-hack/hack-development)
- **Someone1.00 / Someone100**: primer descifrado de la 1.ª generación, parche de la A55, «Jap2Eng patcher» de la NEX-3; trabajó sobre el updater y los escenarios. Escribió en dvxuser, nex-hack.com y nex-hack.info en 2012. — [dvxuser](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/page-2) · [hilo 21](http://web.archive.org/web/20120915020828/http://nex-hack.info/talks/discussion/21/menus/p2)
- **kenan**: fwtool v0.6 a v07b12 (reempaque y recifrado), notas sobre LIRO/av-cam, WBI, generaciones y región. — [hilo 71](http://web.archive.org/web/20130306023740/http://nex-hack.info/talks/discussion/71/about-av-cam-bin-found-in-unpacked-5n-firmware) · [hilo 184](http://web.archive.org/web/20130930002227/http://www.nex-hack.info:80/talks/discussion/184/need-add-support-to-nflasha5-wbi1-file-format)
- **oz_paulb (Paul Bartholomew)**: fwtool original (noviembre de 2012) y LZPT. — [fwtool](http://web.archive.org/web/20140301205337/http://www.nex-hack.info:80/wiki/development/fwtool)
- **Nexus7** (dvxuser, 2011, cuenta nueva a propósito: «Sony isn't really happy when hackers are having fun») y **rdrr** (dvxuser, 2012): conector, UART, JTAG y las fotos de imgur. — [dvxuser](https://www.dvxuser.com/threads/sony-nex-axx-firmware-hacking.262509/)
- **Testu / testu** y **steelcnn** (posiblemente la misma persona, por fechas y tema; no comprobado): IDA del av-cam y el IDC. — [current-tasks](http://web.archive.org/web/20140306210621/http://www.nex-hack.info:80/wiki/development/current-tasks) · [steelcnn/nex-hack](https://github.com/steelcnn/nex-hack)
- **Diamondback**: Android de la NEX-5R/6 y la herramienta `headerfix` para odex. — [nex5r-6-android-reversing](http://web.archive.org/web/20140306213301/http://www.nex-hack.info:80/wiki/development/nex5r-6-android-reversing)
- **hazard**: pidió capturas USB del actualizador «to get a console or other "debug" communication through usb» (hilo 76, 2013-02-27). Es probable, no comprobado, que sea el autor de las páginas fwupdateprocess, server, usbinfo y axfs, todas en primera persona.
- **anmcy, jerzees, REVENGE, zoiah**: emulación (QEMU, defconfigs, orb-server en modo usuario). — [tema 1081](http://web.archive.org/web/20121122145921/http://www.nex-hack.com:80/topic/1081-how-can-i-help-arm-emulator/page__st__20) · [hilo 256](http://web.archive.org/web/20140914073849/http://www.nex-hack.info/talks/discussion/256/run-binorb-server-standalone)
- **nick**: cambio de idioma de la SLT-A65 J1 (página «languages»).
- **Leegong, satmat**: protocolo E-mount y TMP19A44, idioma de la RX10, y la nota de las llamadas de depuración en av-cam (personal-view, 2016-2017).
- **AudioMark**: manuales L3 de la A57 y la ILCE-6000.
- **jeanthom**: NEX1stGenRE (2018).

### Inferences
- Ninguna de estas personas trabajó con handycam. El único puente con la CX580V es la arquitectura común de la 2.ª generación (CXD4132, LIRO, `up.sh`, herramientas de prueba) y el conector de servicio, que en la CX580V habría que ubicar con el manual L3.

### Gaps
- No encontré material posterior de Nexus7, rdrr, kenan ni testu fuera de estos foros.
