# Se compila dentro del contenedor cx580-toolchain (ver toolchain/Containerfile):
#   podman run --rm -v $PWD:/src:Z cx580-toolchain make
# Las pruebas en QEMU están en test/run.sh.
CC      = arm-linux-gnueabi-gcc
ARCH    = -march=armv6 -marm -mfloat-abi=soft
CFLAGS  = $(ARCH) -Os -fPIC -Wall -Wextra -ffreestanding -fno-stack-protector -fno-builtin
LDFLAGS = -shared -nostdlib -nostartfiles -Wl,--hash-style=both -Wl,-s -Wl,-z,norelro -Wl,-T,cxg.ld \
          -Wl,-soname,cxg.so -Wl,--build-id=none
SRC     = cxguard.c cxinh.c cxdl.c cxlog.c cxview.c cxobj.c cxtext.c cxmenu.c cxtouch.c cxpanel.c
HDR     = cx.h cxg.ld
# Rutas del host para la variante de prueba (QEMU no traduce archivos nuevos).
TEST_DIR ?= /tmp/cxtest

all: cxg.so

cxg.so: $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

# Variante con la bitácora de tacto y botones (cxux.c). Aparte, para no meter
# envolturas nuevas en la versión de uso diario.
cxg-ux.so: $(SRC) cxux.c $(HDR)
	$(CC) $(CFLAGS) $(LDFLAGS) -DCX_UX -o $@ $(SRC) cxux.c

# Variante para QEMU: rutas del host, la app es cxtest, la interfaz es libcxt.so
# y los tiempos son cortos.
cxg-test.so: $(SRC) cxux.c $(HDR)
	$(CC) $(CFLAGS) $(LDFLAGS) -DCX_FLAG_PATH='"$(TEST_DIR)/mode"' \
	  -DCX_INH_PATH='"$(TEST_DIR)/cxinh.bin"' -DCX_LOG_PATH='"$(TEST_DIR)/cxreq.log"' \
	  -DCX_MODE_PATH='"$(TEST_DIR)/cxmode.txt"' \
	  -DCX_APP_NAME='"cxtest"' -DCX_VIEW_LIB='"/libcxt.so"' -DCX_OBJ_LIB='"/libcxt.so"' -DCX_INH_LIB='"/libcxt.so"' -DCX_TEXT_LIB='"/libcxt.so"' \
	  -DCX_T1=1 -DCX_T2=2 -DCX_FLUSH_SECS=1 \
	  -DCX_PEND_MS=0 -DCX_FIX_SETTLE_MS=0 -DCX_FIX_WAIT_MS=1000 \
	  -DCX_UX -DCX_UX_LIB='"/libcxt.so"' -DCX_TOUCH_LIB='"/libcxt.so"' -DCX_PANEL_LIB='"/libcxt.so"' -DCX_VIEWBASE_LIB='"/libcxt.so"' \
	  -DCX_MENU_LIB='"/libcxt.so"' -DCX_MENU_VIEW_LIB='"/cxtest"' -o $@ $(SRC) cxux.c

# Hace de libObj.so en la prueba.
test/libcxt.so: test/libcxt.c
	$(CC) $(ARCH) -Os -fPIC -nostdlib -shared -Wl,-soname,libcxt.so -Wl,--hash-style=both -o $@ $<

# Programa de prueba: enlaza la libMWF.so de la cámara (SYSROOT/usr/lib) y libcxt.so.
# Su tabla de menú falsa va en .cxmenu, a 0xc347c de la base, como la de Camera/Mic
# en viewUnified7.so.
test/cxtest: test/cxtest.c $(HDR) test/libcxt.so
	$(CC) $(ARCH) -Os -nostdlib -fno-builtin \
	  -Wl,--dynamic-linker=/lib/libsonyld.so -Wl,--hash-style=both \
	  -Wl,-Ttext-segment=0x10000 -Wl,--section-start=.cxmenu=0xd347c \
	  -Wl,--allow-shlib-undefined -o $@ $< -Ltest -l:libcxt.so -L$(SYSROOT)/usr/lib -l:libMWF.so

clean:
	rm -f cxg.so cxg-ux.so cxg-test.so test/cxtest test/libcxt.so cxguard.so cxguard-test.so
