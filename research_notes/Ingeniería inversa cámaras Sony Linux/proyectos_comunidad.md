# Proyectos comunitarios de ingeniería inversa en cámaras Sony con Linux (2010-2016)

Notas de investigación del 2026-11-26. Etiquetas de evidencia usadas en cada hallazgo:

- **[código]**: lo leí en el código o en archivos de datos del repositorio (clon local en el scratchpad, commit indicado por fecha).
- **[issue]**: lo afirma alguien en un issue de GitHub. Es un reporte de usuario, no está verificado en código.
- **[foro]**: foro, blog o prensa. Es una afirmación sin verificar.
- **[repo tercero]**: documentación de un repositorio ajeno a ma1co. Cuenta como reporte de su autor, no como verificación.

Las fechas de "última actividad" salen de `gh api` (fecha del último commit en la rama por defecto) consultado el 2026-11-26.

---

## 1. Qué soportan y qué logran Sony-PMCA-RE, OpenMemories-*, fwtool.py y nex-hack. ¿Alguno cubre la CXD4132 de las handycam de 2012?

### Takeaway
Casi todo el ecosistema es obra de una sola persona (ma1co) y sí cubre la CXD4132 de las handycam, pero sólo en **modo updater** («updatershell»: shell USB, volcado de flash y lectura y escritura de propiedades de Backup.bin). Las handycam CXD4132 no tienen Android/PMCA, así que no hay apps propias, telnet ni código corriendo en el firmware principal. Lo único que se ha logrado en estos modelos es desbloquear idiomas o PAL/NTSC y volcar la flash. Lo más parecido a lo que hace cx580-pro (LD_PRELOAD en modo normal vía `/setting/env.txt`) es un intento de 2026 en una NEX-7 (CXD4132), que **dejó la cámara inservible**.

### Cited Findings

**Inventario por proyecto**

| Proyecto | Qué hace | Modelos y plataforma | Cómo mete código | Licencia | Último commit | Personas |
|---|---|---|---|---|---|---|
| [Sony-PMCA-RE](https://github.com/ma1co/Sony-PMCA-RE) | Herramienta USB (`pmca-console`, `pmca-gui`) con tres modos: instalador de apps PMCA, updatershell y serviceshell | Updatershell: CXD4105, MB8AC102, CXD4115, CXD4120, **CXD4132** y CXD90014. No CXD90045 ni CXD90057 (firma criptográfica) | Instala APK en cámaras PMCA; en el updater carga `libupdaterbody.so` dentro de un FDAT propio; en «senser mode» usa el protocolo de servicio | MIT | 2022-08-18 (release v0.18 del 2022-04-08) | ma1co (50 de 50 commits en el historial clonado) |
| [fwtool.py](https://github.com/ma1co/fwtool.py) | Desempaca actualizaciones FDAT, msfirm, DSLR y ASH, y volcados `dd` de `/dev/nflasha` | CXD90045, CXD90014, CXD4132, CXD4120, CXD4115, CXD4105/MB8AC102, CXD4108 | No mete código: sólo desempaca | MIT | 2022-09-14 | ma1co. Es un port del fwtool de nex-hack |
| [OpenMemories-Tweak](https://github.com/ma1co/OpenMemories-Tweak) | App Android: quita el límite de 30 min, desbloquea idiomas, PAL/NTSC, quita el aviso NTSC, protección de Backup, Wi-Fi, telnet y adb | Sólo cámaras con PMCA (Android) | Se instala como APK vía Sony-PMCA-RE | MIT | 2018-02-28 (release v0.11 del 2017-10-27) | ma1co |
| [OpenMemories-Framework](https://github.com/ma1co/OpenMemories-Framework) | Stubs de las API Android de Sony (`com.sony.scalar.*`) para escribir apps | Cámaras Android: desde la NEX-5R y la NEX-6 (2012) | APK | MIT | 2018-05-29 | ma1co |
| [OpenMemories-Platform](https://github.com/ma1co/OpenMemories-Platform) | Biblioteca nativa C/C++ para los modos MAIN, UPDATER y ANDROID en las generaciones GEN1, GEN2 y GEN3 | GEN2 = CXD4132 (ver `pack.py`) | Binarios ELF o `.so` que se enlazan contra bibliotecas de Sony de la cámara | MIT | 2022-06-09 | ma1co |
| [OpenMemories-AppStore](https://github.com/ma1co/OpenMemories-AppStore) | Tienda de apps en la cámara | PMCA | APK | MIT | 2017-10-26 | ma1co |
| [OpenMemories-AppList](https://github.com/ma1co/OpenMemories-AppList) | Índice de apps de terceros | PMCA | — | sin licencia | 2019-03-25 | ma1co y contribuidores |
| [OpenMemories-Launcher](https://github.com/ma1co/OpenMemories-Launcher) | Lanzador Android para camcorders y action cams que reemplaza la app de streaming en vivo | Camcorders y Action Cam con Android (CXD90014) | APK con `applicationId com.sony.imaging.app.livestreaming` | sin licencia | 2018-05-02 | ma1co |
| [OpenMemories-CI](https://github.com/ma1co/OpenMemories-CI) y [ma1co/qemu](https://github.com/ma1co/qemu) | Emulación QEMU de BIONZ en GitHub Actions | CXD4108, CXD4115, **CXD4132** (con firmware de DSC-QX10), CXD90014, CXD90045 | — | qemu: NOASSERTION; CI: sin licencia | CI 2021-11-12; qemu 2021-06-19 | ma1co |
| [OpenMemories-Symbols](https://github.com/ma1co/OpenMemories-Symbols) | `bin2elf.py`: convierte `av.bin` (µITRON) en ELF con símbolos | DSC-F828, H2, DSLR-A700, G1, T100, G3, HX5V, HX20, NEX-3 y **HXR-NX5** (`av.bin` y `b_cam.bin`) | — | sin licencia | 2022-07-17 | ma1co |
| [SA-DSP](https://github.com/ma1co/SA-DSP) | Módulo de procesador para Ghidra que decompila «sabin», el DSP SA dentro de BIONZ | BIONZ | — | sin licencia | 2021-08-09 | ma1co |
| [OpenMemories-Docs](https://github.com/ma1co/OpenMemories-Docs) ([sitio](https://openmemories.readthedocs.io/)) | Tabla de dispositivos generada desde `devices.yml` y documentación de hardware | Todas | — | sin licencia | 2022-07-16 | ma1co |
| [PMCADemo](https://github.com/ma1co/PMCADemo) | App de ejemplo | PMCA | APK | MIT | 2018-10-30 | ma1co |
| nex-hack ([espejo fossil en GitHub](https://github.com/erik-smit/nex-hack)) | fwtool en C: descifra, desempaca, reempaca y vuelve a cifrar | NEX y SLT de la época de la CXD4115 y la CXD4132 | Firmware parchado y reflasheado (prueba de concepto en la SLT-A55) | GPLv2 o posterior | espejo: 2014-02-22; fwtool «v0.6final (2013-08-07)» | Someone1.00, Vitaliy Kiselev, oz_paulb (Paul Bartholomew), kenan, erik-smit |

**Sony-PMCA-RE: detalle verificado**
- [código] El updatershell compila tres cuerpos: `libupdaterbody_gen1.so` para CXD4105, CXD4115, CXD4115_ilc y CXD4120, `libupdaterbody_gen2.so` para **CXD4132** y `libupdaterbody_gen3.so` para CXD90014. Empaqueta el cuerpo en un cramfs (`/bodylib/libupdaterbody.so`) dentro del FDAT de cada modelo de `devices.yml`. — [pack.py](https://github.com/ma1co/Sony-PMCA-RE/blob/master/updatershell/pack.py)
- [código] Comandos del shell: `info`, `shell`, `push`, `pull`, `bootloader` (vuelca el cargador de arranque), `bootrom`, `install` (APK), `tweak` y `bk` con `r`, `w`, `patch` («Write backup property by patching the backup file»), `s` (sync), `lock` y `unlock`. — [pmca/platform/__init__.py](https://github.com/ma1co/Sony-PMCA-RE/blob/master/pmca/platform/__init__.py)
- [código] Según el README, el modo updater «sólo se usa para ejecutar código propio; el firmware queda intacto», y las CXD90045 y CXD90057 no son compatibles porque su firmware va firmado. — [README](https://github.com/ma1co/Sony-PMCA-RE/blob/master/README.md)
- [código] El README agradece a nex-hack: «Without the work done by the people at nex-hack, this wouldn't have been possible». — [README](https://github.com/ma1co/Sony-PMCA-RE/blob/master/README.md)
- [código] Métricas: 3133 estrellas, 339 forks y 513 issues abiertos. El repositorio se creó el 2015-05-12. — [API de GitHub](https://api.github.com/repos/ma1co/Sony-PMCA-RE)

**fwtool.py y la lista de dispositivos (la fuente de verdad del soporte)**
- [código] `devices.yml` trae 143 modelos, 42 de ellos CXD4132. Los CXD4132 son DSC-HX9V, HX20, HX30, HX50, HX100V, HX200V, HX300, QX10, QX100, RX1, RX1R, RX100, RX100M2, WX60, WX80, WX100, WX200, WX300, **HDR-AS10, HDR-AS15, HDR-AS30V, HDR-CX560, HDR-CX700, HDR-MV1, HDR-PJ590V, HDR-PJ760V, HDR-PJ790V, HDR-TD10, HDR-TD20**, NEX-3N, NEX-5N, NEX-5R, NEX-5T, NEX-6, NEX-7, NEX-F3, SLT-A37, A57, A58, A65, A77 y A99. — [devices.yml](https://github.com/ma1co/fwtool.py/blob/master/devices.yml)
- [código] Códigos de modelo de las handycam CXD4132: PJ590V `0x21010001` (el mismo que reporta la CX580V de este proyecto), PJ760V `0x21010031`, PJ790V `0x31010060`, TD10 `0x11010011`, TD20 `0x21010052`, CX560 y CX700 `0x11010005`, MV1 `0x31010090`, AS10 y AS15 `0x22010005`, AS30V `0x22010050`. Ninguna tiene el campo `apps`, o sea, ninguna tiene PMCA. — [devices.yml](https://github.com/ma1co/fwtool.py/blob/master/devices.yml)
- [código] La HXR-NX70, la HDR-CX580 y la NEX-VG20/VG30/VG900 **no figuran** en `devices.yml`. La NEX-5, la NEX-3, la SLT-A55 y la NEX-VG10 son CXD4115 con `key: ilc`. — [devices.yml](https://github.com/ma1co/fwtool.py/blob/master/devices.yml)
- [código] Historia de altas: CX560 (2018-02-08), PJ590V (2020-01-08), MV1 (2020-06-26), TD10 (2020-10-26), PJ760V (2020-11-02), PJ790V (2021-01-03), TD20 (2022-02-04). — [historial de devices.yml](https://github.com/ma1co/fwtool.py/commits/master/devices.yml)
- [código] El README indica que es un port del fwtool de nex-hack y que acepta `.exe`, `FirmwareData.dat` o un volcado `dd if=/dev/nflasha`. — [README de fwtool.py](https://github.com/ma1co/fwtool.py/blob/master/README.md)
- [código] En la página de dispositivos, la columna «Updatershell Support» se marca si el modelo tiene `model` y su arquitectura está en la lista CXD4105, MB8AC102, CXD4115, CXD4115_ilc, CXD4120, CXD4132 y CXD90014. «Serviceshell» aparece marcado en todos. — [devices.rst](https://github.com/ma1co/OpenMemories-Docs/blob/master/devices.rst)

**Hardware (documentación de ma1co)**
- [código] Tabla de SoC: CXD4132 «3x ARM11 MPCore, Opal / Avip?, ene. 2011, DSC-TX100, DSC-RX100, NEX-6». CXD4120 «Baccarat, HDR-AX2000, HXR-NX5». CXD4115 «Ortus / Elvis?». CXD90014 «Musashi / Kojiro?, 4x Cortex-A5». BIONZ corre Linux y µITRON en sus núcleos ARM, y el DSP SA ejecuta «sabin». La CXD4132 llega a 1080p60 sin códec externo. — [hardware.rst](https://github.com/ma1co/OpenMemories-Docs/blob/master/hardware.rst)

**OpenMemories-CI y la CXD4132**
- [código] `test_cxd4132.py` emula `MACHINE='cxd4132'` con firmware de DSC-QX10 y NAND de 64 MiB. Los tests son `testUpdaterKernel`, `testLoader2Updater` y `testUpdaterUsb`: cubren el kernel y el USB del **updater**, no la app principal. — [test_cxd4132.py](https://github.com/ma1co/OpenMemories-CI/blob/master/tests/test_cxd4132.py)

**OpenMemories-Tweak y Framework**
- [código] Tweak es «compatible con todas las cámaras con PMCA». Sus funciones son el límite de grabación, los idiomas, PAL/NTSC, el aviso NTSC, la protección y, en la pestaña de desarrollador, Wi-Fi, telnet (puerto 23) y adb. — [README de Tweak](https://github.com/ma1co/OpenMemories-Tweak/blob/master/README.md)
- [código] La lista de cámaras Android empieza con la NEX-5R (2012-08-29, API 1.0) y la NEX-6. Incluye camcorders y action cams **CXD90014** de 2014 a 2016: AS100V, AZ1, AX30/AXP35, AX33, AS200V, X1000V, CX450/CX455/CX485, CX625/CX675/CX680/PJ675/PJ680, AX40/AX55, AX53, AS50, AS300 y X3000. — [Cameras.md](https://github.com/ma1co/OpenMemories-Framework/blob/master/docs/Cameras.md)

**nex-hack (2012-2015)**
- [foro] El primer «custom firmware» para la SLT-A55 (2012-08-14) sólo cambiaba el ítem *Demo mode* por la cadena «Someone1.» y subía la versión a 2.01. Vitaliy Kiselev lo llamó «just proof of concept» y Someone100 avisó que «no debe instalarse si no vas a ayudar con el desarrollo». — [hilo archivado en nex-hack.info](http://web.archive.org/web/20120817101001/http://www.nex-hack.info/talks/discussion/4/first-custom-firmware-for-slt-a55)
- [foro] En agosto de 2012, kenan describió así la arquitectura: «the main application does not run on main cpu but custom chips. the µlinux/busybox only deals with minor parts of the system». Otro miembro preguntó por el estado en 2013 y la respuesta fue que el proyecto avanza «en el tiempo libre» de los desarrolladores. — [Development status topic](http://web.archive.org/web/20130510094109/http://www.nex-hack.info/talks/discussion/68/development-status-topic) · [What's the status](http://web.archive.org/web/20130710102714/http://www.nex-hack.info/talks/discussion/109/whats-the-status-of-the-project)
- [foro] El 12 de agosto de 2015, Vitaliy Kiselev anunció: «All wiki contents moved to http://www.personal-view.com/faqs/#sony-hacks-information». El registro en el foro era sólo por invitación. — [Wiki contents moved](http://web.archive.org/web/20150816112025/http://www.nex-hack.info/talks/discussion/273/wiki-contents-moved)
- [foro] La wiki en personal-view (última edición: 2015-08-10, vitaliy_kiselev) contiene fwtool, el formato del paquete de firmware, scripts de imagen de arranque, el archivo de información del dispositivo, cambio de idioma, el subsistema Android, el servidor web integrado, «Scenarios», archivos esf, AXFS, LZPT/TPZL, el proceso de actualización y el reversing de `av-cam.bin` de la NEX-6. — [sony-hack:hack-development (archivo)](http://web.archive.org/web/20260502175813/https://www.personal-view.com/faqs/sony-hack/hack-development)
- [código] El `fwtool.c` de nex-hack dice «written (reverse-engineered) by Paul Bartholomew, released under the GPL (originally based on "pr.exe" from nex-hack.info)». Su historia va de v0.5 (2012-11-19, oz_paulb) a v0.6beta (2013-05-16, kenan: reempaque y recifrado). — [espejo de erik-smit](https://github.com/erik-smit/nex-hack)
- [foro] El titular «46 Mbit en la NEX-5N» de 2012 no se sostiene. El cuerpo del artículo dice «Sources say that this hack can allow 46Mbit on the older NEX 5», y una actualización aclara que el video era de la NEX-5, no de la 5N. No hay constancia de un firmware flasheado. — [EOSHD, 2012-08-11](https://www.eoshd.com/news/sony-nex-5n-hack-is-go-46mbit-1080p-tested)

**Lo más cercano a cx580-pro: NEX-7 (CXD4132), 2026**
- [repo tercero] knowXD/nex7-firmware-research (creado el 2026-11-17, MIT para el código y CC BY-SA 4.0 para la documentación) trabaja con una NEX-7, CXD4132 y «Linux 2.6.29 RT». Lo que le funcionó: un parche a la tabla de cuantización JPEG en `av-cam`, el límite de grabación de 29:50 a 13 h, «verskip» (flashear firmware reempacado en la misma versión o en una anterior) y un volcado de la NAND. Lo que no: el bitrate (cuatro juegos de parches sin efecto), Android/PMCA (el kernel no tiene binder), los menús (`.uxc` es un formato privado) y las matrices de color. — [README](https://github.com/knowXD/nex7-firmware-research)
- [repo tercero] **Inutilizó la cámara** «para inyectar código sin modificar el firmware». Escribió `/setting/mode/dmode` con el byte `x` (0x78) y `/setting/env.txt` con `LD_PRELOAD=/setting/ldtest.so`. Con un `dmode` que no empieza por `3`, `sen.elf` no arranca: pantalla negra, sin LED y sin USB. La reparación propuesta es `echo 3 > /setting/mode/dmode` y borrar `env.txt` desde una shell por UART (`ttyAM0`) o con un programador NAND. — [REPAIR_NEX7.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/REPAIR_NEX7.md) · [REVIVAL_RUNBOOK.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/REVIVAL_RUNBOOK.md)
- [repo tercero] Mapa del arranque: `init` lee `dmode` y carga `env.txt` (sólo en el modo «oreore», `set_oreore_env()` hace putenv). Opcionalmente fija `LD_PRELOAD` (kmcsup, eFence o DUMA), lanza `bootin.elf`, que hace fork de **`im.elf`** (enlazado con `libObj.so` para la UI y con libIMDB, y carga `/system/av-cam.bin` en LIRO), y lanza `sen.elf`. Banderas de `dmode`: `3` = appFw, `0` = sólo kernel, `k`, `g` (GDB), `d` (DUMA), `e` (eFence) y `L` (LeakTracer). — [APPLAYER_RE.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/APPLAYER_RE.md) · [RESCUE_PLAN.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/RESCUE_PLAN.md)
- [repo tercero] El repositorio incluye `src/ldtest.c` (un preload mínimo con syscalls directas que escribe `/setting/ldtest.log`), `zebra_v1..3.c`, `scn_stub.c` (reemplazo de un plugin de escenario `/usr/scenario/*.so`) y `nandump*.c` (volcador estático). Sobre la cebra dice «hay pistas en cadenas, sin localizar»: `libObj` tiene `ExecCommandSetZebra` y `MSGID_SET_ZEBRA_CMD`. — [árbol del repo](https://github.com/knowXD/nex7-firmware-research/tree/main/src) · [CAPABILITIES.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/CAPABILITIES.md) · [OFFSETS.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/OFFSETS.md)

### Inferences
- Ninguna herramienta pública ejecuta código en el **firmware principal** de una CXD4132 sin Android. Las rutas existentes son el updater (Sony-PMCA-RE), el modo de servicio y reflashear firmware reempacado (knowXD). Si cx580-pro tiene un `.so` corriendo dentro de `im.elf` en modo normal y probado en la cámara, sería, hasta donde encontré, el primer caso documentado en público en una CXD4132 sin PMCA.
- El fallo de knowXD confirma que el gancho `dmode` + `env.txt` existe en la CXD4132 y que un `dmode` inválido impide arrancar la app. También deja en evidencia el valor de la bandera del updater que cx580-pro activa en cada arranque, porque el updater (`nflasha1` y `boot1`) no depende de `/setting/mode/dmode` para arrancar. Esto es inferencia: no lo comprobé en la NEX-7.
- Como `pack.py` agrupa toda la CXD4132 en GEN2, el soporte de updatershell para la CX580V es, en la práctica, el de la PJ590V (mismo código de modelo, `0x21010001`).

### Gaps
- No pude leer la wiki original de nex-hack.info: la captura de 2016 ya está estacionada en GoDaddy. Tampoco pude leer las subpáginas de personal-view.com (`/faqs/sony-hack/fwupdateprocess` devuelve 404 y WebFetch tiene vetado archive.org). No sé si allí había algo sobre handycam.
- No verifiqué que haya funcionado el updatershell en una PJ590V, PJ760V o PJ790V: en los issues sólo aparecen el alta del código de modelo y volcados. Hay éxitos reportados con la TD10, la TD20 y la CX560 (ver la sección 2).
- No encontré código ni documentación del «modo oreore» fuera de knowXD.

---

## 2. ¿Alguien hizo UI propia, menú propio o firmware propio para handycam (controles manuales, desbloqueo 24p/25p, ajustes de Backup.bin)?

### Takeaway
No encontré ninguna UI, menú ni firmware propio para handycam CXD4132. Lo hecho en handycam se limita a **desbloquear idiomas o PAL/NTSC** con el `tweak` del updatershell (TD10, TD20, CX560V) y a pedidos de soporte sin resolver (PJ710VE). En camcorders CXD90014 con Android existe OpenMemories-Launcher para correr apps. Los controles manuales en apps propias sólo existen en Alpha con PMCA (BetterManual, para la A5100).

### Cited Findings
- [issue] **HDR-TD10**: ma1co la agregó tras el brute force del código de modelo. El 2020-10-27 el usuario reportó: «I've run the pmca-gui on my HDR-TD10 camera and managed to unlock all the languages». ma1co señaló que la TD10 tiene **dos procesadores de imagen** y un USB secundario oculto junto al expulsor de batería, sin uso conocido. — [Sony-PMCA-RE #212](https://github.com/ma1co/Sony-PMCA-RE/issues/212)
- [issue] **HDR-TD20 japonesa**: idiomas desbloqueados el 2022-02-07 con la pestaña Tweaks de pmca-gui. La elección de idioma no se guardaba hasta apagar el «demo mode». Con otra TD10, en junio de 2022, el tweak terminó en «Done» y pantalla negra sin efecto (sin resolver). — [Sony-PMCA-RE #212](https://github.com/ma1co/Sony-PMCA-RE/issues/212)
- [issue] **HDR-CX560V**: tiene el mismo código de modelo que la CX700. Al desbloquear idiomas la cámara se colgaba. ma1co lo atribuyó a «a problem in Sony's Backup_write() code which causes the camera to lock up» y lo resolvió cambiando `Backup_guess_protection()` (2018-02-16). El usuario confirmó el éxito el 2018-02-20. — [OpenMemories-Tweak #255](https://github.com/ma1co/OpenMemories-Tweak/issues/255)
- [código] Ese arreglo es el commit «Backup_guess_protection: Read id1 from preset data» del 2018-02-16. — [historial de OpenMemories-Platform](https://github.com/ma1co/OpenMemories-Platform/commits/master)
- [issue] **HDR-MV1**: se agregó al updatershell el 2020-06-26 para desbloquear el inglés. — [OpenMemories-Tweak #378](https://github.com/ma1co/OpenMemories-Tweak/issues/378)
- [issue] **HDR-PJ710VE** (2012, hermana de la PJ760): `guess_firmware` encontró `gen2_0x21010031_0.dat`, pero el modelo sigue como «Unknown device». El usuario subió volcados y el issue sigue abierto sin respuesta desde el 2023-03. Quería pasar de PAL a NTSC. — [Sony-PMCA-RE #390](https://github.com/ma1co/Sony-PMCA-RE/issues/390)
- [issue] **HDR-CX405** (CXD90014, 2026-10): con el serviceshell responde «No tweaks available» y no hay opción de 60p ni de PAL/NTSC. — [Sony-PMCA-RE #743](https://github.com/ma1co/Sony-PMCA-RE/issues/743)
- [issue] En la HDR-AS15 (CXD4132), mungewell usó el updatershell para leer y escribir claves. El updater no trae `bk.elf`, así que lo recreó con `echo` desde una copia. El `bk.elf` de la AS15 y el de la AS100 «no son compatibles entre sí». — [OpenMemories-Tweak #229](https://github.com/ma1co/OpenMemories-Tweak/issues/229)
- [código] OpenMemories-Launcher es el «Android launcher app for Sony camcorders & action cams. Replaces the live streaming application». — [README](https://github.com/ma1co/OpenMemories-Launcher/blob/master/README.md) · [build.gradle](https://github.com/ma1co/OpenMemories-Launcher/blob/master/app/build.gradle)
- [código] BetterManual («An app for improved manual shooting with the Alpha 5100») y FocusBracket aparecen en la lista de apps. — [apps.yaml](https://github.com/ma1co/OpenMemories-AppList/blob/master/apps.yaml) · [obs1dium/BetterManual](https://github.com/obs1dium/BetterManual)
- [issue] **NEX-5N** (CXD4132): el tweak de 60i/p muestra 60 en el menú y al grabar, pero el archivo sale a 50i. Lo confirman dos usuarios más: una SLT-A77 en 2024 y otra NEX-5N en 2025. — [Sony-PMCA-RE #329](https://github.com/ma1co/Sony-PMCA-RE/issues/329)
- [issue] HDMI limpio en la NEX-5N (2023-10): fijar el códec `0x01070087` a `0a` (XAVC, que la cámara no tiene) deja la pantalla y el HDMI sin OSD al grabar, pero ya no graba en la cámara. Se revierte volviendo a `03`. No funcionó en la NEX-C3 (CXD4115: «unknown error»). — [Sony-PMCA-RE #448](https://github.com/ma1co/Sony-PMCA-RE/issues/448)
- [foro] En la a5000, `bk.elf w 0x01070a47 00` quita el overlay de HDMI. Se entra por telnet con OpenMemories-Tweak. — [Hackaday, 2022-11-25](https://hackaday.com/2022/11/25/telnet-gets-stubborn-sony-camera-under-control)
- [repo tercero] En la NEX-7, los menús son `.uxc`, «formato privado de Sony, sin editor». — [CAPABILITIES.md](https://github.com/knowXD/nex7-firmware-research/blob/main/docs/CAPABILITIES.md)
- [foro] En 2012, nex-hack ya tenía el hilo «Sony A55 manual control workaround for video» y varios hilos de pedidos para la VG900 y la VG10. Sólo vi los títulos en el índice del archivo. — [captura del índice CDX de nex-hack.info](https://web.archive.org/cdx/search/cdx?url=nex-hack.info/talks/discussion/*&output=txt&fl=original,timestamp&collapse=urlkey&filter=statuscode:200)

### Inferences
- Para handycam CXD4132 el estado del arte público es sólo de **datos**: cambiar propiedades desde el updater. No hay nada de **código** en modo normal ni ningún trabajo sobre la UI (`libObj.so` o `.uxc`). Un menú propio dentro de `im.elf` no tiene precedente público que reutilizar.
- El caso de la NEX-5N (el menú dice 60 y graba 50) sugiere que cambiar propiedades de Backup.bin no garantiza que cambie el comportamiento del pipeline AV (µITRON en `av-cam.bin`). Sirve de advertencia para cualquier «desbloqueo 24p/25p» por propiedades.

### Gaps
- No encontré hilos ni código sobre desbloquear 24p/25p en handycam CXD4132, ni sobre iris, obturación o ganancia manuales en ellas. Busqué en GitHub (issues de PMCA-RE, Tweak y fwtool) y en la web general.
- No hay datos sobre la HXR-NX70 en ningún repositorio de ma1co.

---

## 3. Backup.bin: qué se desbloquea por propiedades y qué por parches de código. ¿Hay listas de IDs para handycam?

### Takeaway
Las listas públicas de IDs vienen sobre todo de Alpha, Cyber-shot y Action Cam (AS15, AS100, HX60, QX10, ILCE-6000). No hay una lista específica de handycam. El formato es `ID = subsistema(16 bits) << 16 | parámetro(16 bits)`, y el subsistema `0x0107` concentra los ajustes de usuario y video. Por propiedades se logra lo que el firmware ya soporta: idiomas, PAL/NTSC, límite de grabación, códec y modo dentro de lo ofrecido, overlay de HDMI. Todo lo nuevo (más bitrate, más calidad JPEG) exige parchar código y reflashear.

### Cited Findings

**Funcionamiento**
- [issue] ma1co, el 2017-11-25: «When the camera boots, the contents of Backup.bin are copied to memory. Any call to `Backup_read`, `Backup_write` … reads or modifies the [copia en memoria]». Propuso compilar la lista de claves en OpenMemories-Platform (`api/backup.cpp`). — [fwtool.py #6](https://github.com/ma1co/fwtool.py/issues/6)
- [código] API nativa de Sony expuesta en el driver: `Backup_get_datasize(id)`, `Backup_get_attribute(id)`, `Backup_read(id, addr)`, `Backup_write(subsystem_id, id, addr)`, `Backup_sync_all()` y `Backup_protect(mode, overwrite_data, size)`. Constantes: `BACKUP_ATTR_READ_ONLY 1`, offsets del *preset data* (versión `0x0c`, id1 `0x28`, región `0xc0`) y errores 1, 3, 4 y 5. — [drivers/backup.h](https://github.com/ma1co/OpenMemories-Platform/blob/master/drivers/backup.h)
- [código] `BackupPatchDataInterface` borra temporalmente el bit 1 del atributo (solo lectura), escribe y restaura el atributo: así funciona `bk patch` con propiedades protegidas. — [pmca/platform/backup.py](https://github.com/ma1co/Sony-PMCA-RE/blob/master/pmca/platform/backup.py)
- [código] Hay un `AdvancedProtectionTweak` para cámaras japonesas (commit 2017-10-28) y la protección se lee y escribe con `Backup_get_protection` y `Backup_set_protection`. — [api/tweaks.cpp](https://github.com/ma1co/OpenMemories-Platform/blob/master/api/tweaks.cpp) · [historial](https://github.com/ma1co/OpenMemories-Platform/commits/master)
- [issue] RomanHargrave, en 2025, sobre el template de 010 Editor `ILCE-BK4.bt` para Backup.bin de la ILCE-6000 FW 3.20: «the upper 16 bits is the subsystem ID and the lower 16 the parameter». Subsistemas con nombre: ObjRenderer `0x29`, ObjDisplay `0x2A`, ObjEffect `0x35`, Device Information `0x3E`, INFRAINPUT `0x7A`, Touch panel `0x7B`, Hotshoe `0x7C`, SIRCS `0x7D`, Font `0x81`, UDATE `0x90`, PSM `0xA1/0xA2`, Network Sharing Limits `0xFD`, User Settings `0x0107`, Dial `0x011B` y otros. — [gist ILCE-BK4.bt](https://gist.github.com/RomanHargrave/9b7a194ee166972b3c21b892df126bf5) · [Sony-PMCA-RE #597](https://github.com/ma1co/Sony-PMCA-RE/issues/597)
- [issue] Método para encontrar claves: volcar, cambiar un ajuste desde el menú, volver a volcar y comparar byte a byte, excluyendo lo que cambia solo (la hora, por ejemplo). Hay un aviso: escribir con `bk w` no siempre replica el efecto del menú («additional stuff … with the camera's IPC system when properties are changed via the menu»). El caso es el IR remote `0x01070C7B`, que no se activó en una 7RM2. — [Sony-PMCA-RE #597](https://github.com/ma1co/Sony-PMCA-RE/issues/597)
- [issue] RomanHargrave: en la ILCE-6000, `liro.ko` carga `av-cam.bin` y su dirección base se lee en `/sys/module/liro/parameters/adr`. — [Sony-PMCA-RE #597](https://github.com/ma1co/Sony-PMCA-RE/issues/597)
- [issue] mungewell: «Most of the 'hacking' with this project relies on the features already built into the Sony FW». Los menús «seem to be limited to what the camera officially supports». — [OpenMemories-Tweak #411](https://github.com/ma1co/OpenMemories-Tweak/issues/411) · [OpenMemories-Tweak #238](https://github.com/ma1co/OpenMemories-Tweak/issues/238)
- [issue] En 2026 se reportó que la región no se puede cambiar con `bk w` ni con `bk patch`. Los servicios comerciales que dicen «convertir la región» no han publicado su método. — [Sony-PMCA-RE #716](https://github.com/ma1co/Sony-PMCA-RE/issues/716)

**IDs conocidos (con su fuente)**

| ID | Tamaño | Significado | Fuente y modelos |
|---|---|---|---|
| `0x00e70000` | 5 | código de producto | [código: backup.py](https://github.com/ma1co/Sony-PMCA-RE/blob/master/pmca/platform/backup.py) |
| `0x003e0005` | 16 | nombre del modelo | ídem |
| `0x00e70003` | 4 | número de serie | ídem |
| `0x003c0373..75` | 3×1 | límite de grabación H/M/S (por defecto 0:29:50; desbloqueado 13:01:00) | [código: tweaks.py](https://github.com/ma1co/Sony-PMCA-RE/blob/master/pmca/platform/tweaks.py) |
| `0x003c04b6` | 2 | límite 4K en segundos (300 → 0x7fff) | ídem |
| `0x01070148` | 1 | selector PAL/NTSC visible en el menú | [código: backup.cpp](https://github.com/ma1co/OpenMemories-Platform/blob/master/api/backup.cpp) |
| `0x010d008f..+34` | 35×1 | idiomas habilitados (1 = sí, 2 = no) | ídem |
| `0x01640001` | 1 | instalador de apps por USB | ídem |
| `0x01660024` | 8 | versión de la plataforma Android | ídem |
| `0x02cf0004` | 1 | `palNtscSelectorXR` (cámaras recientes) | [fork LAK132, backup.py](https://github.com/LAK132/Sony-PMCA-RE/blob/main/pmca/platform/backup.py) |
| `0x010704fd` | 1 | modo de video 0 = PAL, 1 = NTSC («confirmed HX60») | ídem y [hoja de mungewell](https://github.com/ma1co/fwtool.py/files/1503349/video_settings.zip) |
| `0x01070000` | 8 | uptime | [fork LAK132](https://github.com/LAK132/Sony-PMCA-RE/blob/main/pmca/platform/backup.py) |
| `0x02cf0bd1/0bd2/0c11/0c12` | 0x2f | fotógrafo y copyright | ídem |
| `0x01070087` | 1 | códec: 1 DVD?, 2 M2PS?, 3 AVCHD, 4 MP4, 5 Still, 7 DVD?, `a` XAVC S | [OpenMemories-Tweak #411](https://github.com/ma1co/OpenMemories-Tweak/issues/411) (HX60) y NEX-5N en [#448](https://github.com/ma1co/Sony-PMCA-RE/issues/448) |
| `0x010700a1` | 1 | bitrate AVCHD: 4 = 17, 5 = 24, 6 = 28 Mbit/s | [Tweak #411](https://github.com/ma1co/OpenMemories-Tweak/issues/411) y [PMCA-RE #440](https://github.com/ma1co/Sony-PMCA-RE/issues/440) (NEX-5N) |
| `0x010700a2` | 1 | bitrate MP4 (1 = 3/6, 2 = 12 Mbit) | [Tweak #411](https://github.com/ma1co/OpenMemories-Tweak/issues/411) |
| `0x0107009e` | 1 | modo de captura AVCHD: 0 p60/p50, 1 i60/i50, 3 p24 (HX60); en la NEX-5N: 0 60p, 1 60i, 2 25p, 3 24p | [hoja de mungewell](https://github.com/ma1co/fwtool.py/files/1503349/video_settings.zip) y [PMCA-RE #440](https://github.com/ma1co/Sony-PMCA-RE/issues/440) |
| `0x0107009f` | 1 | cuadros MP4: 0 = 60/50, 2 = 30/25, 4 = 120/100, 5 = 240/200 | [hoja de mungewell](https://github.com/ma1co/fwtool.py/files/1503349/video_settings.zip) (AS100, AS15, HX60, QX10) |
| `0x010700a5`, `0x010709e6`, `0x01070b48`, `0x01070c9a`, `0x01070cc3/cc4/cc5` | 1 | tamaño MP4, modo XAVC S, resolución MP4 (1 = 1920x1080, 2 = 1440x1080, 3 = 1280x720, 5 = 640x480), volteo de imagen y cambio L/R del audio MP4 | ídem |
| `0x01070a47` | 1 | overlay en HDMI (00 = apagado) en la a5000 | [Hackaday](https://hackaday.com/2022/11/25/telnet-gets-stubborn-sony-camera-under-control) |
| `0x01070C7B` | 1 | `SIRCS_REMOTE_MODE` (ILCE) | [PMCA-RE #597](https://github.com/ma1co/Sony-PMCA-RE/issues/597) |

- [código] La hoja `video_settings.xls` de mungewell (adjunta en fwtool.py #6, 2017-11-25) tiene columnas para AS100 NTSC/PAL, **AS15** (CXD4132), HX60 y **QX10** (CXD4132). — [video_settings.zip](https://github.com/ma1co/fwtool.py/files/1503349/video_settings.zip)

**Por parche de código (no por propiedades)**
- [issue] Subir el bitrate por encima de lo que ofrece el menú: «this only change menu options, can't raise bitrate for more than original options. We need an custom firmware». — [Sony-PMCA-RE #440](https://github.com/ma1co/Sony-PMCA-RE/issues/440)
- [repo tercero] En la NEX-7, con firmware reempacado y flasheado: la tabla de cuantización JPEG (`av-cam` en `0x6D032`) funcionó. Los cuatro parches de bitrate no tuvieron efecto, porque «el control está en la lógica de control de tasa del codificador, no en tablas de constantes». — [README de nex7-firmware-research](https://github.com/knowXD/nex7-firmware-research)
- [foro] El parche de la SLT-A55 (2012) cambiaba una cadena del menú: era prueba de concepto y no era reversible. — [hilo en nex-hack](http://web.archive.org/web/20120817101001/http://www.nex-hack.info/talks/discussion/4/first-custom-firmware-for-slt-a55)

### Inferences
- Los IDs `0x0107xxxx` de video y `0x0107009e` (modo de captura) parecen compartirse entre generaciones: HX60 y AS100 son CXD90014, NEX-5N y AS15 son CXD4132. Es plausible que la CX580V use los mismos para AVCHD, pero falta verificarlo con un diff de volcados de la CX580V. Que un modo aparezca en el menú no garantiza que se grabe así (caso NEX-5N, #329).
- Los controles de exposición manual (iris, obturación, ganancia) no figuran en ninguna lista pública de propiedades. Es coherente con que cx580-pro haya tenido que llegar a ellos por código (`view/IRIS` y compañía) en lugar de por Backup.bin.

### Gaps
- No hay una lista de IDs de Backup.bin para handycam CXD4132 (PJ590V, PJ760V, TD20, CX580V) en ningún repositorio público que encontré.
- No pude abrir el `.xls` completo con sus celdas por modelo. Sólo extraje las cadenas, así que la asignación exacta de valores a cada cámara en la hoja está incompleta.

---

## 4. ¿Hay toolchains, SDK o binarios nativos de ejemplo para el Linux 2.6.x ARM de estas cámaras?

### Takeaway
Sí, pero son pocos. OpenMemories-Platform es el único «SDK» nativo: usa Linaro GCC 4.9 arm-linux-gnueabi, se enlaza contra las bibliotecas de Sony que ya están en la cámara mediante stubs y cubre la CXD4132 como GEN2. En la práctica sólo se usó para el updatershell. Para apps hay Android 2.3.7 (sólo PMCA). knowXD publicó ejemplos mínimos para NEX-7 (preload con syscalls directas, volcador de NAND, stub de escenario).

### Cited Findings
- [código] OpenMemories-Platform recomienda GCC de «https://releases.linaro.org/components/toolchain/binaries/4.9-2017.01/arm-linux-gnueabi/» (`CC = arm-linux-gnueabi-gcc`), con `-Wall -Werror -pedantic`. Las bibliotecas estándar de Sony que enlaza son c, dl, gcc_s, m, pthread, rt y stdc++. GEN1 usa stlport estático y `-nostdlib`. — [vars.mk](https://github.com/ma1co/OpenMemories-Platform/blob/master/vars.mk)
- [código] APIs y drivers por modo: en MAIN GEN2 están `usbcmd`, `backup`, `bootloader`, `mms_ioc` y la biblioteca `backup`. En UPDATER GEN2 se suman `android_data_backup`, `backup`, `bootloader` y `mms_ioc`. En todos los modos hay `properties`, `tweaks`, `util` y `mem`, y GEN2 y GEN3 traen además `backup_senser` y `osal_uipc` (mensajería: `osal_snd_msg`, `osal_snd_sync_msg`, `osal_valloc_msg_wait`). — [vars.mk](https://github.com/ma1co/OpenMemories-Platform/blob/master/vars.mk) · [drivers/osal_uipc.h](https://github.com/ma1co/OpenMemories-Platform/blob/master/drivers/osal_uipc.h)
- [código] Los stubs en ensamblador (`stubs/backup.S`, `osal_uipc.S`, `usbcmd.S`, agregados el 2022-02-07) permiten enlazar sin copiar las `.so` de Sony. Hay ejemplos `examples/main`, `examples/updater` y `examples/android`; el de `main` es un `int main(){return 0;}` vacío. Otros commits: «Boot ROM dumper» (2018-06-26) y la API de escritura de Backup (2022-06-09). — [árbol de OpenMemories-Platform](https://github.com/ma1co/OpenMemories-Platform) · [historial](https://github.com/ma1co/OpenMemories-Platform/commits/master)
- [código] El updatershell incluye OpenMemories-Platform como `platform`, compila con `MODE = UPDATER` para GEN 1, 2 y 3 y produce `libupdaterbody_gen$(GEN).so`. — [updatershell/Makefile](https://github.com/ma1co/Sony-PMCA-RE/blob/master/updatershell/Makefile)
- [código] Las apps PMCA tienen que ser compatibles con Android 2.3.7, y la cámara acepta certificados debug y release. — [README de Sony-PMCA-RE](https://github.com/ma1co/Sony-PMCA-RE/blob/master/README.md)
- [repo tercero] Ejemplos nativos para la NEX-7: `ldtest.c` (constructor que hace `svc 0` para open, write y close, sin libc), `zebra_v1..3.c` (busca símbolos «maps-based, no weak symbols»), `scn_stub.c` (reemplaza un módulo de escenario con `sid = 0x010A0000`), `nandump*.c` y `tpzl.c`. — [src/](https://github.com/knowXD/nex7-firmware-research/tree/main/src)
- [código] Emulador: un fork de QEMU «for Sony BIONZ image processors» y CI con firmware real de CXD4132 (DSC-QX10), sólo en modo updater. — [ma1co/qemu](https://github.com/ma1co/qemu) · [OpenMemories-CI](https://github.com/ma1co/OpenMemories-CI)
- [código] Ingeniería inversa del µITRON: `bin2elf.py` con símbolos de `av.bin` (por ejemplo `act_tsk`, `sta_tsk`, `cre_flg`) para varios modelos, incluida la HXR-NX5 (CXD4120). No hay ninguno CXD4132. — [known_files.yaml](https://github.com/ma1co/OpenMemories-Symbols/blob/master/known_files.yaml)
- [código] Para el DSP SA hay un módulo de procesador para Ghidra. — [SA-DSP](https://github.com/ma1co/SA-DSP)
- [foro] En 2012 nex-hack usaba Code::Blocks, Pelles C, Fossil, HxD, ImDisk y Ext2FSD, todas herramientas de Windows. No hay rastro de un toolchain para la cámara. — [wiki sony-hack en personal-view (archivo)](http://web.archive.org/web/20260502175813/https://www.personal-view.com/faqs/sony-hack/hack-development)

### Inferences
- Para cx580-pro, lo reutilizable de OpenMemories-Platform son las firmas de la API de Backup (`Backup_read`/`Backup_write`/`Backup_get_attribute`/`Backup_protect`), los stubs `osal_uipc` y la técnica de enlazar contra las `.so` de la cámara. No hay un framework de UI nativo que heredar.
- Que knowXD escriba sus preloads sin libc y con syscalls directas sugiere que tuvo problemas de enlace o carga en la NEX-7 (sus notas mencionan «verificar la carga de un .so ARM mínimo» como pendiente). Es inferencia: no está demostrado que su `.so` llegara a cargarse.

### Gaps
- No encontré ningún binario nativo de terceros que corra en **modo normal** en una CXD4132 sin Android, salvo los intentos fallidos de knowXD.
- No verifiqué si Sony publica un toolchain en oss.sony.net para estos modelos. Sólo existe la referencia a las fuentes del kernel que ya está en `docs/research/web-research.md`.

---

## 5. Estado y actividad de las comunidades hoy

### Takeaway
ma1co dejó de hacer commits en 2022 (su último comentario en issues de Sony-PMCA-RE que encontré es del 2022-08-17), pero los issues siguen muy vivos en 2026, sobre todo en torno a desbloquear idiomas y PAL/NTSC en Alpha recientes. El trabajo nuevo ocurre en forks (LAK132) y en repositorios personales de investigación, varios hechos con IA en 2026. nex-hack murió en 2015. No encontré Discord ni Telegram específicos.

### Cited Findings
- [código] Últimos commits de ma1co: Sony-PMCA-RE el 2022-08-18 (`pushed_at` dice 2024-07-27, pero la única rama, `master`, termina el 2022-08-18), fwtool.py el 2022-09-14, OpenMemories-Symbols el 2022-07-17 y OpenMemories-Docs el 2022-07-16. — [API de GitHub de ma1co](https://api.github.com/users/ma1co/repos)
- [issue] Comentarios recientes de ma1co en Sony-PMCA-RE: el más nuevo que devolvió la API (comentarios ordenados por fecha) es del 2022-08-17, en el issue #323. — [comentarios de issues de Sony-PMCA-RE](https://api.github.com/repos/ma1co/Sony-PMCA-RE/issues/comments?sort=created&direction=desc)
- [issue] Issues nuevos en 2026: #743 (CX405, 2026-10), #716 (A7V, A7RVI y FX5; 2026-06 a 08), #718 (A7IV, 2026-11), #717 («Request for backup dumps», 2026-11), #712 (RX100M2 japonesa, 2026-06) y #715 (FX3A serviceshell, 2026-07). — [issues de Sony-PMCA-RE](https://github.com/ma1co/Sony-PMCA-RE/issues)
- [código] El fork LAK132/Sony-PMCA-RE va 21 commits por delante de upstream (2026-05 a 2026-11): tweak de modo PAL/NTSC, comandos `dump`, `stat`, `jpull` y `dump-txt` (vuelca Backup a texto con nombres de propiedades conocidas), atributos legibles, props nuevas y soporte de nix-shell y udev. — [comparación con upstream](https://github.com/ma1co/Sony-PMCA-RE/compare/master...LAK132:Sony-PMCA-RE:main)
- [issue] En #716 hay vendedores que ofrecen «convertir región» por correo y otros usuarios piden que publiquen el método. Un usuario cuenta que pasó una a7 IV de NTSC a PAL «with the works of @LAK132 and the help of Claude». — [Sony-PMCA-RE #716](https://github.com/ma1co/Sony-PMCA-RE/issues/716)
- [código] Forks con push en noviembre de 2026: jonathanware, fsegouin, PrintStructor y LAK132. También hay forks para Apple Silicon (Erindale, tonysmith1sme) y un «ILCE-Toolkit» (CaskLabs, 2026-10). — [forks de Sony-PMCA-RE](https://github.com/ma1co/Sony-PMCA-RE/forks)
- [repo tercero] Repositorios de investigación de 2026: knowXD/nex7-firmware-research (NEX-7, 2026-11-17; «A student's attempt … with AI assistance») y CliffVale/sony-a6000-firmware-research (A6000, 2026-08/09). Este último llama a la CXD90014 «Qualcomm CXD90014», un error: la propia documentación de ma1co la presenta como un BIONZ de Sony. Conviene leerlo con cautela. — [nex7-firmware-research](https://github.com/knowXD/nex7-firmware-research) · [sony-a6000-firmware-research](https://github.com/CliffVale/sony-a6000-firmware-research) · [hardware.rst](https://github.com/ma1co/OpenMemories-Docs/blob/master/hardware.rst)
- [foro] Investigación reciente de RomanHargrave (2025) en la ILCE-6000 y la 7RM2. — [Sony-PMCA-RE #597](https://github.com/ma1co/Sony-PMCA-RE/issues/597)
- [foro] Mr_BananaPants anunció en Reddit (2026-10) que agregó shutter angle a una a7 IV aprovechando un «path traversal bug» para obtener el firmware descifrado. No hay repositorio, y un intento posterior de activar 8K dejó la cámara en un bucle de arranque. Es una plataforma moderna, fuera del período estudiado. — [Digital Camera World](https://www.digitalcameraworld.com/cameras/mirrorless-cameras/videographer-hacks-sony-a7-iv-to-add-this-feature-i-found-a-path-traversal-bug-allowing-me-to-get-the-fully-decrypted-firmware)
- [foro] La compilación de recursos para A6000/A6400 (docodethatmatters.com) no lista Discord ni Telegram de Sony. Las comunidades que nombra son Sony Alpha Forum, DPReview, AlphaShooters, Cloudy Nights y r/a6000, r/a6400 y r/SonyAlpha. Las tiendas PlayMemories cerraron sus compras el 2023-09-30 y las descargas el 2025-08-31. — [Sony A6000/A6400 Hacks, Mods and Community Resources](https://docodethatmatters.com/sony-a6000-a6400-research/)
- [foro] Los hilos del foro nex-hack.info siguen archivados hasta 2014-2015 y el dominio terminó estacionado. Los mayores posteadores eran Vitaliy_Kiselev (223 posts), Someone100 (82) y kenan (48). — [Wiki contents moved (archivo)](http://web.archive.org/web/20150816112025/http://www.nex-hack.info/talks/discussion/273/wiki-contents-moved)
- [foro] Magic Lantern es sólo para Canon. Según un resumen de búsqueda, su página «About» lista a Sony entre las marcas con mejoras de firmware de terceros, marcada como posiblemente abandonada, pero la página me devolvió 403 y no lo pude verificar. — [magiclantern.fm/about](https://magiclantern.fm/about.html)

### Inferences
- No hay mantenedor activo que agregue modelos a `devices.yml`. Una handycam sin soporte (PJ710VE) lleva años esperando. Cualquier soporte de la CX580V en Sony-PMCA-RE pasaría por un fork o por el código propio de cx580-pro, que ya reutiliza la guarda de la PJ590V.
- La actividad de 2025-2026 se concentra en Alpha recientes con serviceshell y en Backup.bin (idiomas, PAL/NTSC). Las handycam CXD4132 no tienen comunidad activa propia, y un trabajo sobre la UI de `im.elf` sería territorio nuevo.

### Gaps
- No encontré un Discord ni un Telegram de OpenMemories o de hacking de cámaras Sony. Tampoco charlas en CCC, DEF CON o hack.lu sobre BIONZ o cámaras Sony: la búsqueda sólo devolvió una charla sobre Canon DIGIC 8 (hack.lu 2025).
- No pude revisar los hilos de personal-view.com ni de dpreview sobre handycam: la búsqueda no devolvió hilos útiles y personal-view respondió 404 a la URL directa.
- Sin acceso directo a archive.org desde WebFetch, sólo leí los hilos de nex-hack que bajé con curl: el 4, el 68, el 109 y el 273.
