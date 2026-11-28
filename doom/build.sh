#!/bin/sh
# Compila CXDOOM.BIN: doomgeneric (GPL-2, se baja a vendor/doom) con la capa de
# plataforma y la libc mínima de este directorio. No toca la cámara.
#
#   doom/build.sh            deja doom/build/CXDOOM.BIN
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(dirname "$HERE")
DG=$ROOT/vendor/doom/doomgeneric
PIN=e257a09527062bfb986b6e5e0780839b6cbd5b71
if [ ! -d "$DG/.git" ]; then
    mkdir -p "$ROOT/vendor/doom"
    git clone https://github.com/ozkl/doomgeneric.git "$DG"
fi
CHD=$ROOT/vendor/doom/chocolate-doom-2.2
[ -d "$CHD/.git" ] || git clone --depth 1 --branch chocolate-doom-2.2.1 https://github.com/chocolate-doom/chocolate-doom.git "$CHD"
[ "$(git -C "$DG" rev-parse HEAD)" = "$PIN" ] || git -C "$DG" checkout -q "$PIN"
mkdir -p "$HERE/build/obj"
podman run --rm -v "$ROOT:/r:Z" cx580-toolchain sh -c '
set -eu
CC=arm-linux-gnueabi-gcc
CF="-march=armv6 -marm -mfloat-abi=soft -O2 -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
    -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-strict-aliasing -fno-common -w \
    -DCMAP256 -DFEATURE_SOUND -DDOOMGENERIC_RESX=320 -DDOOMGENERIC_RESY=200 -isystem /r/doom/include -I/r/doom -I/r/preload"
O=/r/doom/build/obj
cd /r/vendor/doom/doomgeneric/doomgeneric
for f in $(sed "s/\.c$//" /r/doom/sources.txt); do
    extra=""
    # Sin el bucle del fundido: bloquearía un segundo el hilo de la interfaz.
    [ "$f" = d_main ] && extra="-Dwipe_StartScreen=cxd_wipe_start -Dwipe_EndScreen=cxd_wipe_end -Dwipe_ScreenWipe=cxd_wipe"
    $CC $CF $extra -c $f.c -o $O/$f.o
done
# Música: el emulador OPL y el secuenciador de Chocolate Doom 2.2.1 (GPL-2).
# Las dos de src/ se copian aparte: con #include "..." el compilador busca primero
# junto al archivo, y ahí están los encabezados de Chocolate Doom en vez de los de doomgeneric.
CH=/r/vendor/doom/chocolate-doom-2.2
CS=/r/doom/build/chsrc
mkdir -p $CS
cp $CH/src/midifile.c $CH/src/midifile.h $CH/src/i_oplmusic.c $CS/
for f in dbopl opl opl_queue; do
    $CC $CF -I. -I$CH/opl -c $CH/opl/$f.c -o $O/ch_$f.o
done
$CC $CF -I. -I$CH/opl -include /r/doom/include/SDL.h -c $CS/midifile.c -o $O/ch_midifile.o
$CC $CF -I. -I$CH/opl -include /r/doom/opl_compat.h -Dmusic_opl_module=cxd_opl_music \
    -c $CS/i_oplmusic.c -o $O/ch_i_oplmusic.o
$CC $CF -Wall -I$CH/opl -c /r/doom/opl_cxd.c -o $O/opl_cxd.o
$CC $CF -Wall -I. -c /r/doom/cxd.c -o $O/cxd.o
$CC $CF -Wall -fno-builtin -fno-tree-loop-distribute-patterns -c /r/doom/libc.c -o $O/cxlibc.o
$CC $CF -c /r/doom/start.S -o $O/start.o
arm-linux-gnueabi-ld -q -T /r/doom/blob.ld -o /r/doom/build/cxdoom.elf $O/*.o $($CC $CF -print-libgcc-file-name) -Map /r/doom/build/cxdoom.map
arm-linux-gnueabi-objcopy -O binary /r/doom/build/cxdoom.elf /r/doom/build/CXDOOM.BIN
arm-linux-gnueabi-size /r/doom/build/cxdoom.elf
und=$(arm-linux-gnueabi-nm -u /r/doom/build/cxdoom.elf | wc -l); [ "$und" -eq 0 ] || { echo "símbolos sin definir"; arm-linux-gnueabi-nm -u /r/doom/build/cxdoom.elf; exit 1; }
# Nada de Thumb ni de instrucciones posteriores a ARMv6.
arm-linux-gnueabi-readelf -A /r/doom/build/cxdoom.elf | grep -i "Tag_CPU_arch:\|Tag_THUMB" || true
'
# La tabla de reubicación (sale de los registros que deja «ld -q») va detrás de
# la imagen, y su tamaño y la suma de comprobación, en la cabecera.
python3 -I "$HERE/mkblob.py" "$HERE/build/cxdoom.elf" "$HERE/build/CXDOOM.BIN"
