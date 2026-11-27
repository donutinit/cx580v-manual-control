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
- **Tocar el valor ya elegido** lo pasa de automático a manual o al revés. En
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

## Lo que no hace todavía

- No se guarda nada al apagar.
- No hay repetición al dejar el dedo apoyado: un toque, un paso.
- Foco y balance de blancos siguen en el menú.
- El modelo de la interfaz de fábrica no se entera de lo que hace el panel. Si
  después se abre la pantalla IRIS de fábrica, muestra el valor real pero su
  botón de automático puede no coincidir.
