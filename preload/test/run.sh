#!/bin/sh
# Pruebas de cxg.so en QEMU con el cargador (sonyld.so) y las bibliotecas de
# la cámara. No toca la cámara.
#
#   preload/test/run.sh [SYSROOT]
#
# SYSROOT se arma a partir de firmware/cx580v-1.07/unpacked si no existe.
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
PRELOAD=$(dirname "$HERE")
ROOT=$(dirname "$PRELOAD")
U=$ROOT/firmware/cx580v-1.07/unpacked
SYSROOT=${1:-${TMPDIR:-/tmp}/cx580-sysroot}
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
fail() { echo "FALLA: $*"; exit 1; }
ok() { echo "ok: $*"; }

if [ ! -x "$SYSROOT/lib/sonyld.so" ]; then
    echo "armando sysroot en $SYSROOT"
    rm -rf "$SYSROOT"; mkdir -p "$SYSROOT/usr" "$SYSROOT/setting"
    cp -a "$U/nflasha3_unpacked_unpacked/initrd.img_unpacked/." "$SYSROOT/"
    rm -rf "$SYSROOT/usr"; mkdir -p "$SYSROOT/usr"
    cp -a "$U/nflasha8_unpacked_unpacked/." "$SYSROOT/usr/"
    chmod -R u+x "$SYSROOT/bin" "$SYSROOT/sbin" "$SYSROOT/lib" "$SYSROOT/usr/bin" "$SYSROOT/usr/lib"
    ln -sf sonyld.so "$SYSROOT/lib/libsonyld.so"
    mkdir -p "$SYSROOT/etc" "$SYSROOT/setting"; : > "$SYSROOT/etc/ld.so.cache"
    # La extracción pierde los enlaces simbólicos: se rehacen con el SONAME.
    for f in "$SYSROOT"/lib/*.so* "$SYSROOT"/usr/lib/*.so* "$SYSROOT"/usr/local/lib/*.so*; do
        [ -f "$f" ] || continue
        so=$(readelf -d "$f" 2>/dev/null | sed -n 's/.*(SONAME).*\[\(.*\)\]/\1/p')
        [ -n "$so" ] && [ ! -e "$(dirname "$f")/$so" ] && ln -s "$(basename "$f")" "$(dirname "$f")/$so"
    done
fi
missing=$(for f in "$SYSROOT"/usr/bin/im.elf "$SYSROOT"/usr/lib/libObj.so "$SYSROOT"/usr/lib/libMWF.so; do
    readelf -d "$f" | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p'; done | sort -u | while read -r n; do
    [ -e "$SYSROOT/lib/$n" ] || [ -e "$SYSROOT/usr/lib/$n" ] || [ -e "$SYSROOT/usr/local/lib/$n" ] || echo "$n"; done)
[ -z "$missing" ] || echo "aviso: bibliotecas que no están en el sysroot: $missing"

podman run --rm -v "$PRELOAD:/src:Z" -v "$SYSROOT:$SYSROOT:ro,z" -v "$T:$T:z" cx580-toolchain \
    sh -c "make -s cxg.so cxg-ux.so && rm -f cxg-test.so test/cxtest test/libcxt.so && make -s cxg-test.so TEST_DIR=$T && make -s test/cxtest SYSROOT=$SYSROOT" \
    || fail "compilación"
cp "$PRELOAD/cxg-test.so" "$SYSROOT/setting/cxg-test.so" 2>/dev/null || { chmod u+w "$SYSROOT/setting"; cp "$PRELOAD/cxg-test.so" "$SYSROOT/setting/"; }
cp "$PRELOAD/cxg.so" "$SYSROOT/setting/cxg.so"
cp "$PRELOAD/test/cxtest" "$SYSROOT/usr/bin/cxtest"
cp "$PRELOAD/test/libcxt.so" "$SYSROOT/usr/lib/libcxt.so"

# -cpu: el ARM11 MPCore de la cámara (ARMv6K, sin Thumb-2). Sin esto QEMU acepta
# instrucciones ARMv7 que en la cámara serían SIGILL.
q() { (cd "$SYSROOT" && timeout 30 qemu-arm-static -cpu "${CX_QEMU_CPU:-arm11mpcore}" -L "$SYSROOT" -E LD_LIBRARY_PATH=/lib:/usr/lib "$@"); }

# Símbolos sin resolver: la biblioteca no debe depender de nada.
und=$(podman run --rm -v "$PRELOAD:/src:Z" cx580-toolchain arm-linux-gnueabi-readelf -Ws --dyn-syms cxg.so | awk '$7=="UND" && $8!=""' | wc -l)
[ "$und" -eq 0 ] || fail "cxg.so importa $und símbolos"; ok "cxg.so no importa símbolos"
und=$(podman run --rm -v "$PRELOAD:/src:Z" cx580-toolchain arm-linux-gnueabi-readelf -Ws --dyn-syms cxg-ux.so | awk '$7=="UND" && $8!=""' | wc -l)
[ "$und" -eq 0 ] || fail "cxg-ux.so importa $und símbolos"; ok "cxg-ux.so no importa símbolos"
podman run --rm -v "$PRELOAD:/src:Z" cx580-toolchain arm-linux-gnueabi-readelf -Ws --dyn-syms cxg.so | grep -q 'postMouse\|EventManager' && fail "cxg.so no debe llevar las envolturas de tacto"
ok "cxg.so no lleva las envolturas de tacto y botones"

for b in bin/busybox usr/bin/bootin.elf usr/bin/im.elf; do
    set +e; q -E LD_PRELOAD=/setting/cxg.so "$SYSROOT/$b" >/dev/null 2>&1; a=$?
    q "$SYSROOT/$b" >/dev/null 2>&1; s=$?; set -e
    [ "$a" = "$s" ] || fail "$b sale con $a con la biblioteca y $s sin ella"
    ok "$b se comporta igual con cxg.so (exit $a)"
done

rm -f "$T/mode"
set +e; q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/bootin.elf" >/dev/null 2>&1; set -e
[ -e "$T/mode" ] || fail "bootin.elf no creó la bandera"; ok "bootin.elf crea la bandera"
rm -f "$T/mode"
set +e; q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/bin/busybox" true >/dev/null 2>&1; set -e
[ ! -e "$T/mode" ] || fail "busybox creó la bandera"; ok "busybox no crea la bandera"

# Hilo de copia, sin tabla inicializada: dos copias con estado 5.
rm -f "$T/cxinh.bin"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" 2>&1) || fail "cxtest falló: $out"
echo "$out" | grep -q "cxtest: fin" || fail "cxtest no terminó: $out"
"$ROOT/.venv/bin/python" -I "$ROOT/tools/cxinh.py" "$T/cxinh.bin" > "$T/sum.txt" || fail "no se pudo leer la copia"
cat "$T/sum.txt"
[ "$(grep -c '^snapshot' "$T/sum.txt")" -eq 2 ] || fail "se esperaban dos copias"
grep -q 'not initialised' "$T/sum.txt" || fail "se esperaba tabla sin inicializar"
ok "sin tabla: el hilo escribe dos copias y la app termina normal"

# Con la tabla inicializada por libMWF.
rm -f "$T/cxinh.bin"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" init 2>&1) || fail "cxtest init falló: $out"
"$ROOT/.venv/bin/python" -I "$ROOT/tools/cxinh.py" "$T/cxinh.bin" > "$T/sum.txt" || fail "no se pudo leer la copia"
cat "$T/sum.txt"
grep -q 'snapshot 1 at 1 s: ok' "$T/sum.txt" || fail "la copia con tabla no salió ok"
"$ROOT/.venv/bin/python" -I "$ROOT/tools/cxinh.py" "$T/cxinh.bin" --feature 0x174 --feature 0x402 --feature 0x10 > "$T/f.txt"
cat "$T/f.txt"
grep -q 'feature 0x174: 0x60, 0x7f' "$T/f.txt" || fail "factores de 0x174 mal leídos"
grep -q 'feature 0x402: 0x720' "$T/f.txt" || fail "factores de 0x402 mal leídos"
grep -q 'feature 0x10: not blocked' "$T/f.txt" || fail "0x10 no debía estar bloqueada"
ok "con tabla: copia leída y bits decodificados"

# Una sola copia por instalación.
before=$(sha256sum "$T/cxinh.bin")
q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" init >/dev/null 2>&1 || true
[ "$before" = "$(sha256sum "$T/cxinh.bin")" ] || fail "la copia se sobrescribió"
ok "no sobrescribe una copia existente"

# Interfaz: las cuatro funciones de ViewBase pasan por cxg.so intactas y quedan anotadas.
out=$(q "$SYSROOT/usr/bin/cxtest" view 2>&1) || fail "cxtest view sin biblioteca falló: $out"
echo "$out" | grep -q "view TODO OK" || fail "libcxt.so no se comporta como se espera: $out"
rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" view 2>&1) || fail "cxtest view falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "view TODO OK" || fail "las envolturas alteran argumentos o retornos"
[ -s "$T/cxreq.log" ] || fail "no se escribió la bitácora"
cat "$T/cxreq.log"
grep -q 'HDR base=' "$T/cxreq.log" || fail "falta el encabezado"
grep -q 'OV 1111 "view/IRIS_L" n=7' "$T/cxreq.log" || fail "falta openView"
grep -q 'CV 2222 "view/IRIS_L"' "$T/cxreq.log" || fail "falta closeView"
grep -q 'EX 3333 "model/CAMERA" a=2d k1c4:1=7 ret=e7ec' "$T/cxreq.log" || fail "falta requestModelExecute"
[ "$(grep -c 'GI 4444 "model/CAMERA" q=f' "$T/cxreq.log")" -eq 1 ] || fail "getModelInfo con la misma respuesta debía anotarse 1 vez"
grep -q 'out k12c:1=1234 ret=1' "$T/cxreq.log" || fail "getModelInfo no anotó la salida"
ok "interfaz: argumentos y retornos intactos, bitácora correcta"

# Redirección de prueba: sólo con cxmode.txt, sólo la pantalla Exposure.
rm -f "$T/cxmode.txt" "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" noredirect 2>&1) || fail "cxtest noredirect falló"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "redirect TODO OK" || fail "sin cxmode.txt no debe haber redirección"
printf 'iris\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" redirect 2>&1) || fail "cxtest redirect falló"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "redirect TODO OK" || fail "la redirección no funcionó"
grep -q 'RD 5555 "view/EXPOSURE" -> view/IRIS' "$T/cxreq.log" || fail "la redirección no quedó anotada"
printf 'rm -rf /\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" noredirect 2>&1) || fail "cxtest con modo desconocido falló"
echo "$out" | grep -q "redirect TODO OK" || fail "un modo desconocido no debe redirigir"
rm -f "$T/cxmode.txt"
ok "redirección: sólo con un modo conocido y sólo para Exposure"

# v5: varias redirecciones y reescritura del modo AE.
printf 'redirect view/EXPOSURE view/IRIS\r\nredirect  view/SPOT_METER   view/SHUTTER_SPEED\nredirect view/SPOT_FOCUS view/NO_EXISTE\naemode 1\n' > "$T/cxmode.txt"
rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" v5 2>&1) || fail "cxtest v5 falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "v5 TODO OK" || fail "v5"
grep -q 'RW 6666 "model/CAMERA" aemode 3 -> 1' "$T/cxreq.log" || fail "la reescritura no quedó anotada"
printf 'aemode 9\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" v5 2>&1) || true
echo "$out" | grep -q "ok aemode: 3 se manda como 1" && fail "aemode fuera de rango no debe aplicarse"
rm -f "$T/cxmode.txt"
ok "v5: redirecciones, modo AE reescrito y reportado a la pantalla correcta"

printf 'aeauto 6\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" aeauto 2>&1) || fail "cxtest aeauto falló"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "aeauto TODO OK" || fail "aeauto"
grep -q 'RW 7777 "model/CAMERA" aeauto 5 -> 6' "$T/cxreq.log" || fail "aeauto no quedó anotado"
rm -f "$T/cxmode.txt"
ok "aeauto: el automático de la PJ760V se manda como el de fábrica"

# Mensajes a los objetos de cámara (MWF::ObjIf).
rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" obj 2>&1) || fail "cxtest obj falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "obj TODO OK" || fail "ObjIf"
cat "$T/cxreq.log"
[ "$(grep -c 'OA 8888 c=3100 m=75b4 n=1 p96d3:7=1cb' "$T/cxreq.log")" -eq 1 ] || fail "el asíncrono debía anotarse una vez"
grep -q 'OA 8888 c=3100 m=75b4 n=1 p96d3:7=1ca' "$T/cxreq.log" || fail "el cambio de valor no se anotó"
grep -q 'c=2000' "$T/cxreq.log" && fail "otra categoría no debe anotarse"
grep -q 'OS 9999 c=3100 m=75d0 n=1 p951e:7=0 -> ret=5c out p951e:7=1cb' "$T/cxreq.log" || fail "el síncrono no quedó bien anotado"
ok "ObjIf: mensajes de cámara anotados, argumentos y retornos intactos"

rm -f "$T/cxmode.txt" "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" noobjmanual 2>&1) || fail "cxtest noobjmanual falló"
echo "$out" | grep -q "obj TODO OK" || fail "sin objmanual no debe reescribir: $out"
printf 'objmanual 1\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" objmanual 2>&1) || fail "cxtest objmanual falló"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "obj TODO OK" || fail "objmanual"
grep -q 'RW EXPOSURE_MODE f3 -> f5' "$T/cxreq.log" || fail "la reescritura no quedó anotada"
rm -f "$T/cxmode.txt"
ok "objmanual: prioridad de iris u obturación sale como modo M; automático intacto"

printf 'holdmanual 1\nredirect view/EXPOSURE view/IRIS\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" hold 2>&1) || fail "cxtest hold falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "hold TODO OK" || fail "holdmanual"
grep 'IN ' "$T/cxreq.log"
grep -q 'IN 8888 c=3100 m=75b6 n=2 p0:.*p96d6:7=1cd ret=0' "$T/cxreq.log" || fail "la inyección no quedó anotada"
rm -f "$T/cxmode.txt"
ok "holdmanual: mensajes propios armados con libMWF y enviados al objeto de cámara"

printf 'holdmanual 1\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" fix 2>&1) || fail "cxtest fix falló: $out"
echo "$out" | grep "cxtest:"
grep -E ' (FX|NF|IN|OA) ' "$T/cxreq.log" | head -60
echo "$out" | grep -q "fix TODO OK" || fail "corrección de valores"
grep -q ' FX shutter paso' "$T/cxreq.log" || fail "la corrección no quedó anotada"
rm -f "$T/cxmode.txt"
ok "corrección: lee FNO y obturación por GetParam y da pasos SYNC hasta el valor guardado"

printf 'gainshift 1\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" gain 2>&1) || fail "cxtest gain falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "gain TODO OK" || fail "gainshift"
grep 'IN .*m=75ba' "$T/cxreq.log"
grep -q 'IN 8888 c=3100 m=75ba n=3 p0:.*p96dc:3=12 p96dd:8=1d0' "$T/cxreq.log" || fail "la inyección de AGC no quedó anotada"
rm -f "$T/cxmode.txt"
ok "gainshift: AE Shift controla el límite de ganancia, con tipos entero de 8 bits y sin signo"

printf 'isoshift 1\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" iso 2>&1) || fail "cxtest iso falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "iso TODO OK" || fail "isoshift"
grep 'IN .*m=75d0' "$T/cxreq.log"
grep -q 'IN 8888 c=3100 m=75d0 n=4 p0:.*p951e:7=38 p96fe:7=38 p96ff:7=38' "$T/cxreq.log" || fail "ISO 1600 fijo no quedó anotado"
grep -q 'IN 8888 c=3100 m=75d0 n=4 p0:.*p951e:7=21 p96fe:7=2c p96ff:7=3e' "$T/cxreq.log" || fail "ISO automático no quedó anotado"
rm -f "$T/cxmode.txt"
ok "isoshift: AE Shift manda ISO fijo (o automático) a la capa de cámara"

printf 'expogain 1\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" expo 2>&1) || fail "cxtest expo falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "expo TODO OK" || fail "expogain"
grep -E 'IN .*m=7552| ID ' "$T/cxreq.log"
rm -f "$T/cxmode.txt"
ok "expogain: AE Shift enciende la exposición manual y da sus pasos"

printf 'unlockiso 1\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" unlock 2>&1) || fail "cxtest unlock falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "unlock TODO OK" || fail "unlockiso"
grep -q ' UL ' "$T/cxreq.log" || fail "el desbloqueo no quedó anotado"
rm -f "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" nounlock 2>&1) || fail "cxtest nounlock falló: $out"
echo "$out" | grep -q "unlock TODO OK" || fail "sin unlockiso el ISO no debe desbloquearse"
ok "unlockiso: GetInhInfo contesta libre sólo la función 0x116, sin tocar la tabla"

printf 'expogain 1\nautounlock 1\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" auto 2>&1) || fail "cxtest auto falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "auto TODO OK" || fail "autounlock"
grep -E ' UL | ID ' "$T/cxreq.log"
grep -q 'UL aprendida función 174' "$T/cxreq.log" || fail "el aprendizaje no quedó anotado"
rm -f "$T/cxmode.txt"
ok "autounlock: una orden propia bloqueada aprende la función que la bloquea y se reintenta"

printf 'gainshift 1\ngainlock 4\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" lock 2>&1) || fail "cxtest lock falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "lock TODO OK" || fail "gainlock"
grep -q ' GL AE Shift 2 -> camara +4' "$T/cxreq.log" || fail "gainlock no quedó anotado"
rm -f "$T/cxmode.txt"
ok "gainlock: la cámara recibe AE Shift +N, la pantalla ve su valor y el tope sigue la pantalla"

printf 'label 2614 28b6\nlabel 2bf1 26f0\nlabel zz 1\nlabel 12345 1\ntextlog 1\nimage 24 18f\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" text 2>&1) || fail "cxtest text falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "text TODO OK" || fail "label"
grep -E ' (TX|TS|TI) ' "$T/cxreq.log" | head -12
grep -q 'TI icon 2200024 -> 18f' "$T/cxreq.log" || fail "el cambio de icono no quedó anotado"
grep -q 'TX sprite 2042614 -> 28b6' "$T/cxreq.log" || fail "el cambio de etiqueta no quedó anotado"
grep -q 'TS 2042690' "$T/cxreq.log" || fail "textlog no anotó los números"
rm -f "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" notext 2>&1) || fail "cxtest notext falló: $out"
echo "$out" | grep -q "text TODO OK" || fail "sin label los textos no deben cambiar"
ok "label/image: etiquetas e iconos se cambian por número, a través de las funciones de la cámara"

printf 'uxlog 1\ntext 2690 F2.8_1/60\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" ux 2>&1) || fail "cxtest ux falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "ux TODO OK" || fail "uxlog"
grep -E ' (MP|MR|KE|LB|TX) ' "$T/cxreq.log" | head -12
grep -q 'MP mouse=0 button=0 x=123 y=fffffffb tid=' "$T/cxreq.log" || fail "el toque no quedó anotado"
grep -q 'KE 15020041' "$T/cxreq.log" || fail "el botón no quedó anotado"
grep -q 'LB id=2042614 label=.* font f0f0f000 f0f0f001 f0f0f002 f0f0f003 f0f0f004 f0f0f005' "$T/cxreq.log" || fail "la fuente no quedó anotada"
rm -f "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" noux 2>&1) || fail "cxtest noux falló: $out"
echo "$out" | grep -q "ux TODO OK" || fail "sin uxlog todo debe pasar igual"
[ -e "$T/cxreq.log" ] && grep -qE ' (MP|MR|KE|LB) ' "$T/cxreq.log" && fail "sin uxlog no debe anotarse nada de tacto"
ok "uxlog: tacto y botones pasan intactos por los trampolines y quedan anotados sólo si se pide"

printf 'show c347c 12\nshow c347c 13\nhide c347c 17\nshow c347c 99\nhide c347c 300\nshow 12345 12\n' > "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" menu 2>&1) || fail "cxtest menu falló: $out"
echo "$out" | grep "cxtest:"
echo "$out" | grep -q "menu TODO OK" || fail "show/hide"
grep -E ' MN ' "$T/cxreq.log" | head -8
grep -q 'MN init menu=c347c root id=ff' "$T/cxreq.log" || fail "la tabla de menú no quedó a 0xc347c de la base"
grep -q 'MN show menu=c347c id=c state 0->2 valid 1->1' "$T/cxreq.log" || fail "show no quedó anotado"
grep -q 'MN hide menu=c347c id=11 state 2->0 valid 1->1' "$T/cxreq.log" || fail "hide no quedó anotado"
rm -f "$T/cxmode.txt"; rm -f "$T/cxreq.log"
out=$(q -E LD_PRELOAD=/setting/cxg-test.so "$SYSROOT/usr/bin/cxtest" nomenu 2>&1) || fail "cxtest nomenu falló: $out"
echo "$out" | grep -q "menu TODO OK" || fail "sin reglas el menú no debe cambiar"
ok "show/hide: el estado de los ítems del menú se corrige dentro de initMenuData, sólo en la tabla conocida"

echo "TODO OK"
