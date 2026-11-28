#!/bin/sh
# Compila tools/bin/cxloop (ARM, estático, sin libc) con el contenedor del proyecto.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$ROOT/tools/bin"
podman run --rm -v "$ROOT:/r:Z" cx580-toolchain sh -c '
arm-linux-gnueabi-gcc -march=armv6 -marm -mfloat-abi=soft -Os -ffreestanding -fno-pic -fno-pie -fno-stack-protector \
    -fno-unwind-tables -fno-asynchronous-unwind-tables -nostdlib -static -Wall -Wextra -Wl,--build-id=none -Wl,-s \
    -o /r/tools/bin/cxloop /r/tools/cxloop.c && arm-linux-gnueabi-size /r/tools/bin/cxloop'
