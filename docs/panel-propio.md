# Panel propio en la pantalla de grabación

Escrito y probado en la cámara el 2026-11-27. Con la v24 el dueño confirma,
con foto, que la pantalla de grabación queda limpia: los tres valores abajo,
«+» y «−» a la derecha, y sin los botones W, T y grabar ni los indicadores
nativos de obturación y AE Shift. Las bitácoras de las cinco sesiones no tienen
ninguna orden rechazada. Falta comprobar con un clip que lo grabado coincide
con lo que muestra el panel.

## Qué es

Iris, obturación y ganancia a la vista en la pantalla de grabación de video, y
ajustables sin entrar al menú:

```
                                              +     orilla derecha,
                                              -     donde iban W y T
   F2.6    1/180    [12dB]   (medidor de audio)
```

- **Tocar un valor** lo elige: queda entre corchetes.
- **AUTO** (o tocar otra vez el valor ya elegido) lo pasa de automático a manual
  o al revés. En
  automático lleva una `A` al final (`F2.8A`, `1/60A`); la ganancia automática
  dice `AGC` y los dB que avisa la cámara (`AGC27`).
- **`-` y `+`** mueven el valor elegido. El primer toque sobre un control en
  automático sólo lo fija donde está; el siguiente ya lo mueve. Igual que en las
  pantallas de fábrica.
- La ganancia va de 0 a 24 dB de 3 en 3, con el mismo mecanismo probado de la
  v16 (tope de ganancia más AE Shift fijo).

El panel sólo se ve y sólo atiende sobre la pantalla de grabación de video. Con
un menú u otra pantalla abierta se oculta y los toques van a esa pantalla. En
modo foto y en reproducción no aparece.

## Por qué no debería parpadear

Con las pantallas de fábrica, pasar de IRIS a Shutter Speed cambia la prioridad
de exposición, y en ese cambio la cámara mueve 1/3 de paso el otro control
(medido el 2026-11-27; se corrige en menos de un segundo). El panel no cambia
de prioridad cuando los dos están en manual: deja la que haya y mueve el otro
control con pasos directos, que la cámara obedece desde que se destraban
(`autounlock`). Sin cambio de prioridad no hay desvío que corregir. Falta
comprobarlo en la cámara.

## Cómo está hecho

| Archivo | Qué hace |
| --- | --- |
| `preload/cxpanel.c` | las cinco etiquetas (`GEN_Label` en memoria propia, colgadas de `A_PHYSICAL_LAYER`), los textos, las zonas de toque y qué pantalla está abierta |
| `preload/cxtouch.c` | envolturas de `WidgetSystem::postMouse*`; se traga los toques que caen en el panel |
| `preload/cxobj.c` (`cxobj_panel_*`) | el motor: prioridad, fijar cada control, pasos y ganancia, sobre la lógica ya probada |

La fuente de las etiquetas son los números medidos en la cámara en las
etiquetas de fábrica. Cada paso de la creación de la primera etiqueta se anota
y se escribe al archivo antes de ejecutarse: si la interfaz se cae, la bitácora
dice en qué paso.

## Configuración (`cxmode.txt`)

| Línea | Efecto |
| --- | --- |
| `panel 1` | sólo dibuja |
| `panel 2` | dibuja y atiende el tacto |
| `panelz N X Y W H` | zona N en el espacio de 640×480: 0 «−», 1 iris, 2 obturación, 3 ganancia, 4 «+» |
| `hidebox X Y W H` | oculta lo nativo que quepa entero en esa caja (hasta 4) |
| `hidew HEX` | oculta el widget nativo con ese ID (hasta 12) |
| `paneldump 1` | en el primer toque anota el árbol de widgets de la pantalla (`PW …`) |
| `panelfont N HEX` | N: 0 fuente, 1 tamaño, 2 estilo, 3 alineación, 4 dirección |
| `paneldraw 0` | no llamar a `TextSprite::drawText` a mano |

`panel` enciende por su cuenta `holdmanual`, `autounlock` y `gainlock 4`.

## Lo nativo que estorba

`cxpanel.c` recorre el árbol de widgets de la capa (`getParent`,
`getFirstChild`, `getNextSibling`) con la posición absoluta de cada uno y pone
en modo oculto (`setMode(3)`) lo que pide la configuración. Si la vista lo
vuelve a mostrar, se oculta otra vez en la siguiente llamada. Los campos de
`ux::wgtsys::Widget` salen del desensamblado de `libObj.so` 1.07: modo `+0x19`,
posición local `+0x3c`/`+0x40`, ancho y alto `+0x78`/`+0x7c` (en 16.16) e ID
`+0x104`.

Con W, T y grabar ocultos, el zoom queda en la palanca y la grabación en el
botón START/STOP.

## Pruebas en la cámara

| Versión | Configuración | Resultado |
| --- | --- | --- |
| v20 (`cxg-v20-ux.so`) | `cxmode-v20p.txt`: tira abajo | se dibuja y responde; choca con lo nativo |
| v21 (`cxg-v21.so`) | `cxmode-v21p.txt`: valores abajo, «+» y «−» a la derecha, dos `hidebox` | el panel bien; no se ocultó nada: `Widget::getParent()` de una etiqueta colgada de la raíz devuelve 0 y el recorrido no arrancó |
| v22 | la misma | entra por el nodo del padre (`+8`, y su dueño en `+0`); se fueron W, T y «0EV», quedaron «180» y grabar |
| v23 | `cxmode-v23p.txt`: cajas más anchas, mapa completo | se fue «180»; grabar no, porque mide 54×127 desde (586, 354) y llega a y = 481 |
| v24 (`cxg-v24.so`, md5 `35cf3cc5…`) | `cxmode-v24p.txt`: `hidebox 530 120 110 372` y `hidebox 78 425 356 55` | pantalla limpia, confirmado por el dueño con foto |
| v25 | `cxmode-v25p.txt`: «todo menos» con tres `keepbox` (MENU, indicador de grabación, medidor de audio) y `hidevt 11d1444` (STBY); Audio Mode otra vez en el menú | el dueño confirma que funciona como se esperaba; al cambiar Audio Mode el panel no volvió hasta abrir y cerrar otra pantalla |
| v26 | `cxmode-v26p.txt`: lo mismo, con Demo Mode otra vez en el menú | botón AUTO: funciona. El regreso automático del panel ocultó los botones del menú principal |
| v27 | `cxmode-v26p.txt` | la v26 sin el seguimiento de vistas vivas; el dueño confirma que los menús se ven bien |
| v28 | `cxmode-v28p.txt` (`panelwb 1`) | lectura de balance: AWB, SOL, INT y 1P. La cámara acepta los tres preajustes y avisa el cambio (`NW modo=c8/cf/c3`) |
| v29 | la misma | las listas de opciones (REC_MODE, PB_DEMO_CAM_SET…) cuentan como parte del menú: al cerrarse, el panel sigue oculto |
| v30 (`cxg-v30.so`, md5 `3790c9d2…`) | la misma | One Push desde el panel: con 1P elegido el botón dice SET y fija el blanco con la carta gris. El dueño confirma que funciona. Sin Kelvin ni los experimentos de ISO |

### Lo que enseñó el mapa de la pantalla

`backups/20261127-080417-dumps/cxreq.log` trae el árbol completo de la capa en
la pantalla de grabación (líneas `PW`): 371 widgets. Lo útil:

| Qué | Dónde | Clase (`vt`, dirección de la tabla virtual menos la base de `libObj.so`) |
| --- | --- | --- |
| botones W, T y grabar de la derecha | (586, 120), (586, 237), (586, 354), 54×127 | `11cdc44` |
| los mismos, a la izquierda (ocultos de fábrica) | (0, 120), (0, 237), (0, 354) | `11cdc44` |
| botón de abajo a la derecha | (564, 410) 74×69 | `11b5a10` |
| botón de ver imágenes | (2, 410) 74×69 | `11b5a10` |
| grupo de indicadores de abajo (obturación «180» y otros dos) | (80, 430) 235×40 | `11b9290` |
| indicador de AE Shift («0EV») | (280, 430) 83×40 y (363, 430) | `11d46c0`, `11a9418` |
| medidor de audio | (440, 404) 120×52 | `11bbd38` |

Los ID de los widgets (`+0x104`) son números corridos según el orden de
creación (`0x5b` en adelante). Coincidieron en dos arranques, pero no son
nombres: para ocultar se usa la posición. La vista vuelve a mostrar sus
indicadores varias veces por sesión (al tocar la pantalla, al cambiar un
valor); `cxpanel.c` recorre el árbol una vez por segundo y entre recorridos
vuelve a ocultar lo que reapareció.

Otras medidas de esas sesiones: `ViewBase::getModelInfo` se llama unas 80 veces
por segundo en el hilo de la interfaz, así que sirve de reloj para refrescar
los textos; `GraphicsEngine::updateResource` corre en otro hilo y no sirve.

### Lo que enseñó la sesión de la v25

- **Elegir una opción en un menú cierra todo sin `closeView`**, y el panel se
  queda oculto hasta abrir y cerrar otra pantalla. La v26 intentó resolverlo
  contando las vistas vivas con el constructor y el destructor de `ViewBase`
  (`_ZN8ViewBaseC2EP11ViewManager`, `_ZN8ViewBaseD2Ev`). **No sirve y rompió el
  menú**: el menú principal sigue vivo después de cerrarse, se aprendió como
  parte de la pantalla de grabación y, con el menú abierto, el «todo menos» le
  ocultó sus botones. La v27 quita ese mecanismo. Sigue pendiente una señal
  fiable de qué pantalla está a la vista.
- **Cambiar Audio Mode hace que la interfaz reenvíe sus ajustes**, entre ellos
  `SET_EXPOSURE_MODE` en automático: lo que se había fijado desde el panel se
  pierde, porque el modelo de la interfaz no lo conoce. El panel lo muestra
  bien, pero hay que volver a fijarlo.
- **Los ID de los widgets cambian de un arranque a otro** (corridos en uno en
  esa sesión): confirma que no sirven para identificar nada.
- La interfaz recibió `ret=a000` seis veces en `0x6500` (el zoom) con la
  obturación en 1/15. Causa sin determinar.

### Tamaño de la biblioteca

`cxg.so` v30 mide 48 068 bytes y el instalador acepta hasta 49 152 (tres
bloques de `/setting`). Quedan 1 084 bytes. Se enlaza sin tabla de símbolos y
sin relro; además el enlazador mete hasta 4 KiB de relleno entre el código y
los datos cuando eso le ahorra una página de memoria; `preload/cxg.ld` fija la
forma sin relleno. Antes de agregar más código hay que hacer
sitio: quitar los experimentos de ISO que no sirvieron o hacer que el
instalador cuente los bloques que ya ocupa la biblioteca instalada.

### Balance de blancos y One Push

Órdenes copiadas de lo que manda la pantalla de fábrica
(`backups/20261127-092723-guard-install/cxreq.log`):

| Qué | Mensaje | Parámetros |
| --- | --- | --- |
| preajuste | `SET_WB_MODE` (`0x7546`) | `WB_MODE` (`0x958d`, tipo 7): `c7` automático, `c8` sol, `cf` interior, `c3` One Push |
| tomar el blanco | `CUSTOM_WB_CMD` (`0x7542`) | `CUSTOM_WB_OUT_RANGE_ADAPT` (`0x9586`) = 0 y `CUSTOM_WB_AUTO_REVIEW` (`0x9587`) = 0, los dos bool |
| guardarlo | `CUSTOM_WB_SAVE_CMD` (`0x7544`) | `CUSTOM_WB_SAVE_CH` (`0x958c`) = `c3` |

La pantalla de fábrica pide después los datos (`SYNC_REQ_CUSTOM_WB_DATA_CMD`,
`0x650c`, con `CUSTOM_WB_CH` = `c3`) y se los pasa a su modelo (acciones `0x14`
y `0x15` de `model/CAMERA`). El panel no hace ese paso: falta saber si un
ajuste tomado desde el panel sobrevive al apagado.

## Lo que no hace todavía

- No se guarda nada al apagar.
- No hay repetición al dejar el dedo apoyado: un toque, un paso.
- El foco sigue en el menú.
- El modelo de la interfaz de fábrica no se entera de lo que hace el panel. Si
  después se abre la pantalla IRIS de fábrica, muestra el valor real pero su
  botón de automático puede no coincidir.
