# Pantallas `view/AGC_LIMIT` y `view/AE_SHIFT` (análisis estático)

Análisis estático de `viewUnified7.so` y `libObj.so` del firmware 1.07, hecho
el 2026-11-26 sin la cámara a mano. Marcas usadas:
**[código]** leído en el desensamblado o en la decompilación,
**[inferido]** deducido sin prueba directa, **[bitácora]** visto en un
`cxreq.log` real. Nada de esto se probó todavía en la cámara.

Direcciones: en `viewUnified7.so` y en `libObj.so`, dirección de Ghidra =
vaddr del archivo + `0x10000`. Abajo van como `G:` (Ghidra) o `v:` (vaddr).
Material en bruto (decompilaciones y desensamblados anotados):
`firmware/ghidra/out/agents/agc-views/`.

## 1. `view/AGC_LIMIT` (`ViewAgc_Limit`)

### Tabla de opciones [código]

`v:0x95e74`, 11 registros de `0x14` bytes `{icono, texto, globo, dB, función INH}`.
La pantalla copia a `this+0x100` los que pasan la consulta de bloqueo (en
orden) y guarda cuántos son en `this+0x22c`. Van 5 por página.

| idx | icono | texto | dB | INH `k101` |
| --- | --- | --- | --- | --- |
| 0 | `0x0220019b` | `0x0204311d` «OFF» | 0 | `0x35e` |
| 1 | `0x02200199` | `0x020430a9` «27dB» | 27 | `0x35b` |
| 2 | `0x02200197` | `0x020430aa` «24dB» | 24 | `0x35a` |
| 3 | `0x02200195` | `0x020430ac` «21dB» | 21 | `0x359` |
| 4 | `0x02200193` | `0x0204306e` «18dB» | 18 | `0x358` |
| 5 | `0x02200191` | `0x02043070` «15dB» | 15 | `0x357` |
| 6 | `0x0220018f` | `0x020430bc` «12dB» | 12 | `0x356` |
| 7 | `0x0220018d` | `0x0204313c` «9dB» | 9 | `0x355` |
| 8 | `0x0220018b` | `0x0204313e` «6dB» | 6 | `0x354` |
| 9 | `0x02200189` | `0x02043152` «3dB» | 3 | `0x353` |
| 10 | `0x02200187` | `0x02043140` «0dB» | 0 | `0x352` |

Los globos son `0x02043042` («Will not set the limit…», sólo para OFF) y
`0x02043002`. Mientras no hay selección, la etiqueta muestra `0x0204318c`.

### Qué lee al abrir [código]

`FUN_0004bf20` (G), al abrir y en los eventos `0x14` y `0x15..0x19`:

```
getModelInfo("model/CAMERA", 0x80)
  k348 != 1            -> idx = 0 (OFF)
  k348 == 1:
    v = k346
    si this[0xfd]:  this[0x244] = k349;  v = min(v, k349)
    v == 0           -> idx = cuenta-1 (la entrada «0dB»)
    si no            -> idx = primera entrada con dB == v (o -1)
```

`this[0xfd] = (getModelInfo("model/STILL_REC", 0xe).k13d != 10)`, con 10 por
omisión. En la CX580V esa consulta da `ret=0` [bitácora], así que
`0xfd = 0` y `k349` no se lee.

Respuesta del modelo a `q=0x80` (`libObj` v:`0x5279a4`, dentro de G:`FUN_00533944`) [código]:
`k345 = obj(+0x1170).get(0)`, `k346 = obj(+0x1170).get(1)`,
`k347 = obj(+0x116c).get(0)`, `k348 = obj(+0x116c).get(1)`,
`k349 = [+0x1d0c]`, `k34a = [+0x1d10]`. Por las cadenas
`PARAMID_AGC_LIMIT_SET/ACT`, `…MODE_SET/ACT`, `…MAX/MIN` [inferido]:
`k345/k346` son el límite SET/ACT, `k347/k348` el modo SET/ACT (1 = encendido),
`k349` el máximo (27) y `k34a` el mínimo (0). La cámara contesta
`k348=0 k346=0 k349=0x1b` [bitácora], o sea que arranca en OFF.

### Bloqueos [código]

- Abrir: `INH_WRAPPER q=1 k101=0x82`. Si `k103 == 200` sale un aviso y la
  pantalla no se abre. También dan aviso `MOVIE_REC_DUAL q=1 k101 != 0` y
  `STILL_REC_DUAL q=0xe k13d ∈ {2,3,4,5,7,8,9}`. La cámara pasa todos
  [bitácora].
- Cada opción: `INH_WRAPPER q=1 k101=0x352..0x35e`. Con `k103 == 200` la
  opción desaparece.
- Sólo si `0xfd = 1`: las opciones con dB > `k349` se ven deshabilitadas
  (`setWbBtnState(...,0)`) y no se mandan. En la CX580V no aplica.

### Eventos → acciones (slot 46 G:`0x4c738`, despacho slot 49 G:`0x4d4f8`) [código]

En el estado 0 sólo se atiende el evento `0x11012001`, que pasa al estado 1.
AE_SHIFT hace lo mismo.

| Evento | Acción | Qué hace |
| --- | --- | --- |
| 1, 2, `0x1b`, `0x11004009` | 0 | cierra (`this[0x24c] = ev==2`) |
| 3..7 | `0xe` | selecciona el botón `ev-3`: `0x238 = (pág-1)*5 + botón`, redibuja botones, icono y etiqueta **antes de mandar**, globo con temporizador |
| **8..0xc** | `0xf` | **manda** (`FUN_0004d380` → `FUN_0004d26c(this, [0x238])`) |
| `0xd` / `0xe` | `0x10` / `0x11` | página anterior / siguiente |
| `0xf..0x13` | `0x12` | modo «?» y globo |
| `0x14` | 10 | vuelve a leer `q=0x80` y redibuja |
| `0x15..0x19` | 9 | vuelve a leer `q=0x80` (descarta la selección) |
| `0x1a` | `0x13` | se acabó el tiempo del globo |

### Qué manda (G:`FUN_0004d26c`, v:`0x3d26c`) [código]

```
idx == 0 : requestModelExecute("model/CAMERA", 0x71, {0x344: 0})
idx  > 0 : si (dB <= this[0x244]) o this[0xfd] != 1:
           requestModelExecute("model/CAMERA", 0x71, {0x344: 1, 0x343: dB})
```

`0x343` y `0x344` están en el literal `v:0x3d37c` y en `mov r1,#836`. Los dos
valores son enteros simples de tipo 1. En la CX580V (`0xfd = 0`) la condición
siempre se cumple.

### El modelo de la CX580V sí atiende la acción `0x71` [código]

La acción `0x71` pasa por la tabla de eventos (slot 15, G:`0x537ce4`), que la
manda a la acción interna `0x1a`. De ahí sigue a G:`FUN_00556244`, luego a
G:`FUN_0054b0d4` (`pvt_ActionSetAgcLimit`) y por último a G:`FUN_0056eb60`
(`pvt_SetParamAgcLimit`):

- `0x343` (`PARAMID_DATA_AGC_LIMIT`, literal G:`0x56ed44`): tiene que caber en
  un signed char y ser múltiplo de 3. Si no, `AGCNG1`/`AGCNG2` y no se manda
  nada. Va como `0x96dc` (tipo 3). Si falta, sigue sólo con el modo.
- `0x344` (`…_AGC_LIMIT_MODE`): 0 se vuelve `0x1d1`, 1 se vuelve `0x1d0` y
  cualquier otro valor falla. Va como `0x96dd` (tipo 8).
- Sale `IssueCommandAsync` con `MSGID_SET_AGC_LIMIT_CMD 0x75ba`: el mismo
  mensaje que inyecta `cxobj.c` con `gainshift`, que ya se probó en la
  cámara. El modelo guarda lo pedido en `get/set` índice 2; `k346/k348`
  (índice 1) sólo deberían cambiar cuando la cámara notifique
  `MSGID_NOTIFY_AGC_LIMIT_EVT` [inferido].

### Por qué no manda nada en la cámara (abierto)

En las 7 aperturas de AGC_LIMIT (6 bitácoras de `backups/`) no hay ningún
`EX a=71` [bitácora]. Con
`0xfd = 0`, la acción `0xf` manda siempre, así que los eventos `8..0xc`
nunca llegaron. Los `GI` no sirven para saberlo, porque la bitácora omite
las respuestas repetidas.

Pista [código + inferido]: `viewAgc_Limit.uxc` es copia de
`viewWhiteBalance_L.uxc`, con los mismos hashes y en el mismo orden. Pero
cada botón de opción de WB lleva 5 propiedades de evento (`d96c3d62`,
`547e85a6`, `bcf5801a`, `01ac59fe`, `4a311dea`) y los de AGC sólo 4: les
falta `4a311dea`. White Balance manda en el mismo grupo `8..0xc`
(acción `0x12` → `FUN_0006b180` → `EX a=0x12`) y en la cámara sí funciona
[bitácora]. Hipótesis: `4a311dea` es el evento de «soltar o decidir» que
produce `8..0xc`, y la `.uxc` de la CX580V no lo trae. Sin probar.

Diagnóstico barato: interceptar `_ZNK8ViewBase10GetEventIdEv` (libObj
v:`0x19c560`, que `viewUnified7.so` llama por la PLT) y anotar lo que
devuelve mientras AGC_LIMIT está abierta.

### Receta AGC_LIMIT

1. **Falsear `q=0x80`** en `cxw_info`, con `model/CAMERA` e id `0x80`:
   `k348 = 1` si el límite está encendido (0 = OFF), `k346 = dB` (múltiplo de
   3 entre 0 y 27; 0 cae en «0dB») y dejar `k349 = 0x1b`. `k345/k347/k34a`
   la pantalla no los lee. Hay que contestar **el último valor mandado**,
   porque la pantalla vuelve a leer al abrir y con los eventos `0x14` y
   `0x15..0x19`; si no, la selección regresa a OFF.
2. **Esperar** `EX "model/CAMERA" a=0x71`: `{k344=1, k343=N}` para «N dB» y
   `{k344=0}` para OFF. El modelo de fábrica ya lo convierte en
   `SET_AGC_LIMIT 0x75ba {0x96dc=N, 0x96dd=0x1d0/0x1d1}`, así que no hace
   falta interceptarlo (se puede, si se quiere).
3. **No bloquear** `INH k101 = 0x82` ni `0x352..0x35e` (no contestar `k103=200`).
4. Si `8..0xc` no aparecen, hay dos salidas: (a) en `GetEventId`, convertir
   en `8+i` el evento que llegue al soltar el botón `i` mientras la pantalla
   está abierta; (b) al cerrar la pantalla, leer de `this` (el mismo `self` de
   la bitácora) `idx = [this+0x238]` y `dB = [this+0x100 + idx*0x14 + 0xc]`,
   y mandar el `EX 0x71` desde `cxg.so`. Los desplazamientos están
   verificados en el código. Que `3..7` llegue tampoco está probado.

## 2. `view/AE_SHIFT` (`ViewAeShift_L`)

### Consulta y orden [código]

- `FUN_00037480` (G, v:`0x27480`): `getModelInfo("model/CAMERA", 0x33)`. Con
  `k1ab == 2` pone `this[0xfd] = 1` (encendido); con cualquier otro valor
  (1), apagado. `k1ad` pasa a `this[0x110]` sólo si está entre
  `this[0x104] = -4` y `this[0x100] = 4`. `k1aa` y `k1ae` se leen y se
  descartan.
- `FUN_00038098` (G, v:`0x28098`): `requestModelExecute("model/CAMERA", 0x2e,
  {k1af: this[0x10c] (=1), k1a9: 0xfd ? 2 : 1, k1ac: this[0x110]})`.

| Evento | Acción | Qué hace |
| --- | --- | --- |
| 3 (barra) | 1 | `0x110 = GetEventParameter(1).valor − 4` y **manda**, sin redibujar |
| 4 | 2 | vuelve a leer `q=0x33` y redibuja; cierra si la exposición cambió a automática |
| 5 / 6 | 3 / 4 | `0xfd = 1/0`, globo y temporizador de 10, **manda y luego redibuja** (optimista) |
| 9 / 10 | 7 / 8 | ocultan el globo |

En la barra, el icono sale del modelo, de `k1ad`, y no de lo que se mandó.
En la bitácora, después de cada `EX a=2e` vienen `GI q=10` y `GI q=33`, lo
que encaja con que el evento 4 sea el aviso de cambio del modelo [inferido].
Con `gainlock`, `cxg.so` ya le devuelve a la pantalla su propio valor, así
que todo queda coherente.

### Cómo se ve el valor [código]

- **Es una imagen, no texto.** `FUN_00037dd0` toma `getWidget(0x840d98cd)` y
  lo pasa por `Check_PAS_Icon`; después llama al virtual `+0x138` (setImage)
  con `tabla[k+4]`.
  Tabla en v:`0x92c88`: −4 `0x02200023`, −3 `…22`, −2 `…21`, −1 `…20`,
  0 `…24`, +1 `…1c`, +2 `…1d`, +3 `…1e`, +4 `…1f`. Apagado: `0x02200025`
  (literal v:`0x281c8`).
- El virtual `+0x138` de `PAS_Icon` (libObj v:`0x85e294`, no exportado)
  compara el tamaño de la imagen con el del widget y llama **por la PLT** a
  `GEN_Icon::setImage(unsigned int, bool)`
  (`_ZN8GEN_Icon8setImageEjb`, v:`0x169ef0`, ARM) con
  `(this = PAS_Icon+0x170, id, 0)`. Hay `JUMP_SLOT` en `0x10e2c54` y
  `ABS32` en la vtable (`0x11d4cf4`): se puede interceptar.
- Etiqueta (`PAS_LabelCustom`, virtual `+0x13c`): `0x02042312` mientras no se
  ha tocado nada, `0x02042314` apagado y `0x02042315` encendido.
- Las plantillas `${v1}${v2}.${v3}EV` (`0x02042263`) y `${v1}${v2}dB`
  (`0x02042265`) no aparecen en `viewUnified7.so`. Sólo están en `libObj.so`,
  junto a `Check_ACT_LabelPbDatacode` (código de datos en reproducción)
  [inferido por la ubicación].

### Receta AE_SHIFT → dB

Interceptar `GEN_Icon::setImage(this, id, b)` y, sólo con `view/AE_SHIFT`
abierta (`cxview.c` ya sabe cuándo se abre y se cierra), cambiar el id. Se
usan los iconos de la propia tabla de AGC, para `(k+4)·3` dB:

| AE | id AE | id nuevo |
| --- | --- | --- |
| −4…−1 | `0x02200023`, `…22`, `…21`, `…20` | `0x02200187` (0 dB), `…189`, `…18b`, `…18d` (9 dB) |
| 0 | `0x02200024` | `0x0220018f` (12 dB) |
| +1…+4 | `0x0220001c`, `…1d`, `…1e`, `…1f` | `0x02200191` (15 dB), `…193`, `…195`, `0x02200197` (24 dB) |
| apagado | `0x02200025` | `0x0220019b` (AGC OFF) |

Textos, con el `label` que ya existe: `2312→318c`, `2314→3042`, `2315→3002`.
El título «AE Shift» (`0x2311`, va en la `.uxc`) podría pasar a `0x2322`
«AGC Limit» [inferido].

Riesgos sin probar: que esos iconos estén en los recursos de la CX580V (la
tabla de menús de `libObj` v:`0x1119170..0x1119300` los nombra, pero no los
encontré en `share/app`), y su tamaño, porque `PAS_Icon` revisa el tamaño de
la imagen original antes de llamar. La otra opción es reescribir en memoria
los 9 valores de v:`0x92c88`, que caen en un segmento de sólo lectura y
necesitan `mprotect`.
