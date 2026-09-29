#!/bin/bash
set -e

# Builds kinst2.nro (Killer Instinct 2) inside the devkitPro Docker image.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="1.0.0"

docker run --rm -v "${SCRIPT_DIR}:${SCRIPT_DIR}" -w "${SCRIPT_DIR}/build/projects/sdl/mame/gmake-linux" devkitpro-mesa-rust:latest bash -c "
set -e
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=/opt/devkitpro/devkitA64
export PATH=/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:\$PATH
make config=release CC=aarch64-none-elf-gcc CXX=aarch64-none-elf-g++ AR=aarch64-none-elf-ar ARCH=\"-O3 -fno-strict-aliasing -fomit-frame-pointer -D__SWITCH__ -DSDLMAME_NO64BITIO -I/opt/devkitpro/portlibs/switch/include -isystem /opt/devkitpro/libnx/include -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE\" -j\$(nproc) mame
cd ${SCRIPT_DIR}
nacptool --create 'Killer Instinct 2' 'Thorhax' '${VERSION}' kinst2.nacp
elf2nro mame kinst2.nro --icon=ki2-icon.jpg --nacp=kinst2.nacp
"

echo "Build successful: ${SCRIPT_DIR}/kinst2.nro"
