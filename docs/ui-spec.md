# Interfaz objetivo

Idea del dueño de la cámara, 26 de noviembre de 2026: menús minimalistas,
parecidos a los de una Sony de fotografía, en cuatro pestañas más una vista de
medios. Este documento fija el objetivo de diseño y reparte cada ítem del menú
de fábrica. **Nada de esto está implementado en la cámara.**

Prototipo interactivo en el navegador: `ui/menu-prototype.html`. Usa una escena
sintética y los valores medidos en los clips de prueba. Ahí quedaron aplicadas
las decisiones abiertas de abajo con la opción recomendada.

El inventario sale de la guía de ayuda oficial de la HDR-CX580V. El menú de
fábrica tiene seis categorías: Shooting Mode, Camera/Mic, Image Quality/Size,
Playback Function, Edit/Copy y Setup. [Guía de ayuda](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/contentslist.html) ·
[Estructura del menú](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/01/01/01.html)

Leyenda: **fábrica** = la cámara ya lo ofrece; **nuevo** = no existe en la
unidad y es parte del objetivo del proyecto.

## Pestaña 1 · Exposición

| Control | Origen | Opciones de fábrica |
| --- | --- | --- |
| Iris | nuevo | Hoy sólo se lee en Data Code. Medido f/1.8 a f/4.0 |
| Obturación | nuevo | Hoy sólo se lee en Data Code. Medido 1/50 a 1/250 s |
| Ganancia | nuevo | Hoy sólo se lee en Data Code. Medido 0, 6 y 9 dB |
| Exposure | fábrica | Auto o Manual: un solo ajuste de brillo que mueve iris, obturación y ganancia juntos |
| Low Lux | fábrica | On/Off. En la prueba subió la ganancia de 6 a 9 dB |
| White Balance | fábrica | Auto, Outdoor, Indoor, One Push. Sin Kelvin ni WB Shift |
| Spot Meter | fábrica | Medición por punto tocado en pantalla |
| Zebra | fábrica | Off, 70 IRE, 100 IRE |
| Cinematone | fábrica | On/Off, aspecto «de cine» automático |

Iris, obturación y ganancia se mostrarían juntos, cada uno con su propio
Auto/Manual, como en el modo M de una Sony de foto. Ese es el control
independiente que pide el proyecto.

## Pestaña 2 · AF

| Control | Origen | Opciones de fábrica |
| --- | --- | --- |
| Focus | fábrica | Auto o Manual cerca/lejos por pantalla táctil |
| Spot Focus | fábrica | Enfoque en el punto tocado |
| Spot Meter/Fcs | fábrica | Medición y enfoque en el mismo punto |
| Peaking | fábrica | Off, White, Red, Yellow |
| Tele Macro | fábrica | On/Off |
| Face Detection | fábrica | Prioridad de rostros para foco y exposición |
| SteadyShot (movie) | fábrica | Active, Standard, Off. Cambia el campo de visión |

SteadyShot no es de enfoque, pero no cabe en las otras pestañas. Va aquí como
«lente» salvo que el dueño prefiera otra cosa.

## Pestaña 3 · Formatos de video

| Control | Origen | Opciones de fábrica |
| --- | --- | --- |
| HD/STD | fábrica | AVCHD o MPEG-2. Comprobado en la unidad |
| Cuadros por segundo | fábrica en HD, nuevo en STD | HD: 60i, 60p, 24p. STD: de fábrica solo 60i; se agregan 30p y 24p |
| REC Mode | fábrica | PS 1080/60p 28 Mbps, FX 24 Mbps, FH 17 Mbps, HQ 1440×1080 9 Mbps, LP 5 Mbps |
| x.v.Color | fábrica | On/Off, gama de color ampliada |
| Wide Mode | fábrica | 16:9 o 4:3, sólo en STD |
| Media Select | fábrica | Memoria interna o tarjeta |

[REC Mode](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/04/01/01.html) ·
[Frame Rate](https://helpguide.sony.net/gbmig/44162931/v1/us/contents/07/04/02/02.html)

Cuadros por segundo en STD, a pedido del dueño. Los clips STD de prueba salieron
a 720×480 entrelazado, 60i. MPEG-2 a esa resolución admite 30p progresivo y 24p
con pulldown dentro del perfil de los DVD (MP@ML). 60p a 720×480 excede ese
perfil y no se ofrece. Falta comprobar que el codificador de la cámara acepte
30p y 24p.

## Pestaña 4 · Audio

| Control | Origen | Opciones de fábrica |
| --- | --- | --- |
| Formato de audio | fábrica, fijo | Siempre 2ch estéreo, a pedido del dueño. Basta el Audio Mode de fábrica en 2ch con el ítem fuera del menú; en el menú queda sólo como dato |
| Micref Level | fábrica | Normal o Low |
| Auto Wind NR | fábrica | On/Off |
| Blt-in Zoom Mic | fábrica | On/Off, el audio sigue al zoom |
| Audio Level Display | fábrica | On/Off, medidor en pantalla |
| Nivel de grabación manual | nuevo | No existe: Micref Level sólo tiene dos posiciones |

## Media view

| Función | Origen |
| --- | --- |
| Event View (galería por fecha) | fábrica |
| Data Code → Camera Data, con iris, obturación, ganancia y WB | fábrica, hoy en Setup |
| Delete, Protect, Copy, Direct Copy | fábrica |

La propuesta es mostrar Camera Data siempre en esta vista, sin ir a Setup.

## Ajustes (sin pestaña asignada)

Estos ítems no caben en las cuatro pestañas, pero hacen falta: sin `Format`
no se limpia una tarjeta, y `USB Connect` y `USB LUN Setting` los usa este
proyecto para conectar la cámara.

Format, Media Info, File Number, Repair Img. DB F., TV Type, HDMI Resolution,
CTRL FOR HDMI, USB Connect, USB Connect Setting, USB LUN Setting, Beep, LCD
Brightness, REC Lamp, Power On By LCD, Language Setting, Calibration, Battery
Info, Eco Mode, Date & Time Setting, Area Setting, Display Setting, Guide Frame.

**Recomendación:** una quinta pestaña «Ajustes», como el menú de herramientas
de las Sony de foto. Decisión del dueño.

## Se oculta

Funciones de consumo que no entran en ninguna pestaña.

| Grupo | Ítems |
| --- | --- |
| Modos automáticos | Intelligent Auto, Scene Selection, Auto Back Light |
| Disparo de foto | Smile Shutter, Smile Sensitivity, Self-Timer, Flash, Flash Level, Red Eye Reduction, Image Size |
| Efectos | Fader, Golf Shot |
| Audio | Closer Voice, Audio Mode 5.1ch |
| Accesorios y zoom | Light Bright (luz de video), Digital Zoom, Conversion Lens |
| Reproducción de consumo | Map View, Highlight Movie, Scenario, Highlight Playback |
| Música y discos | Download Music, Empty Music, Disc Burn |
| GPS | Your Location, GPS Setting, Auto Clock ADJ, Auto Area ADJ |
| Otros | Demo Mode, My Button (las pestañas lo sustituyen) |

**Por decidir:** Smth Slw REC (cámara lenta) y Cinematone pueden servir en
trabajo profesional. Quedan fuera del recorte hasta que el dueño diga.

## Pendiente para llegar aquí

1. Confirmar en la unidad que el inventario coincide con su menú real.
2. Ubicar en el firmware dónde se definen los menús y dónde se deciden iris,
   obturación y ganancia. El respaldo está extraído en
   `firmware/cx580v-1.07/unpacked/`. Ese análisis no está hecho.
3. Decidir el camino: ocultar ítems y cambiar valores por ajustes reversibles,
   o modificar el firmware. Lo segundo exige un mecanismo de recuperación
   comprobado antes de escribir nada en la flash.
