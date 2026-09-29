#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

docker run --rm -v /root/tmp/switch-upd:/root/tmp/switch-upd -w /root/tmp/switch-upd/mame-nx2026/mame/build/projects/sdl/mame/gmake-linux devkitpro-mesa-rust:latest bash -c "
set -e
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=/opt/devkitpro/devkitA64
export PATH=/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin:\$PATH
make config=release CC=aarch64-none-elf-gcc CXX=aarch64-none-elf-g++ AR=aarch64-none-elf-ar ARCH=\"-O3 -fno-strict-aliasing -fomit-frame-pointer -D__SWITCH__ -DSDLMAME_NO64BITIO -I/opt/devkitpro/portlibs/switch/include -isystem /opt/devkitpro/libnx/include -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE\" -j\$(nproc) mame
nacptool --create 'Killer Instinct' 'Thorhax' '1.1.1' ../../../../../kinst.nacp
nacptool --create 'MAME-NX' 'Thorhax' '1.1.1' ../../../../../control.nacp
elf2nro ../../../../../mame ../../../../../kinst.nro --icon=/root/tmp/switch-upd/mame-nx2026/mame/ki-newicon.jpg --nacp=../../../../../kinst.nacp
elf2nro ../../../../../mame ../../../../../mame.nro --icon=/root/tmp/switch-upd/mame-nx2026/mame/ki-newicon.jpg --nacp=../../../../../control.nacp
cp ../../../../../kinst.nro /root/tmp/switch-upd/mame-nx2026/kinst/kinst.nro
"

echo "Build successful:"
echo " - ${SCRIPT_DIR}/kinst.nro (boots directly into Killer Instinct)"
echo " - ${SCRIPT_DIR}/../kinst/kinst.nro"
echo " - ${SCRIPT_DIR}/mame.nro"
