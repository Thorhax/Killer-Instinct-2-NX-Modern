# Killer-Instinct-2-NX-Modern

[![License](https://img.shields.io/badge/license-BSD--3--Clause%20%2F%20GPL--2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Nintendo%20Switch-red.svg)](https://switchbrew.org)

Standalone Nintendo Switch arcade port of **Killer Instinct 2** powered by modern MAME (0.289) and libnx. Built for Atmosphère CFW with hardware-accelerated rendering, multi-core CPU scheduling, stutter-free disk streaming and full two-player controller support.

Companion to [Killer-Instinct-NX-Modern](https://github.com/Thorhax/Killer-Instinct-NX-Modern) (the original Killer Instinct), sharing the same engine and fixes.

---

## Features

- **Direct Boot:** Boots directly into Killer Instinct 2 (`kinst2.nro`) without extra frontend menus.
- **Standard ROMs:** Works with the standard MAME `kinst2.zip` and compressed `kinst2.chd` — no conversion needed.
- **Stutter-Free Disk Access:** The whole ~440 MB hard disk image is decompressed into RAM in the background by two threads while the game boots, reading ahead of whatever the game is loading. FMVs, stage loads and backgrounds never wait on the SD card.
- **Stable Long Sessions:** GPU frame pacing keeps the display smooth for as long as you play.
- **Native Switch Pad API:** Low-latency native libnx `pad` input handling.
- **Intuitive Combat Controls:** Six-button arcade layout configured with dedicated Punch and Kick rows.
- **Two-Player Parity:** Complete Player 1 and Player 2 support with mirrored bindings.
- **Clean Exit:** `Plus + Minus` saves settings and high scores and closes the game cleanly to the Switch HOME menu.
- **Persistent Configuration:** Fully working `.cfg` saving and loading on the SD card.

---

## Control Layout

Controls are configured with dedicated **Punch (top)** and **Kick (bottom)** rows:

| Physical Button | Arcade Function | Port ID | Player 1 | Player 2 |
| :--- | :--- | :--- | :--- | :--- |
| **Y** | High Attack - Quick (Quick Punch) | `BUTTON1` | `JOYCODE_1_BUTTON4` | `JOYCODE_2_BUTTON4` |
| **L** / **ZR** | High Attack - Medium (Medium Punch) | `BUTTON2` | `JOYCODE_1_BUTTON2` / `8` | `JOYCODE_2_BUTTON2` / `8` |
| **X** | High Attack - Fierce (Fierce Punch) | `BUTTON3` | `JOYCODE_1_BUTTON3` | `JOYCODE_2_BUTTON3` |
| **B** | Low Attack - Quick (Quick Kick) | `BUTTON4` | `JOYCODE_1_BUTTON1` | `JOYCODE_2_BUTTON1` |
| **R** / **ZL** | Low Attack - Medium (Medium Kick) | `BUTTON5` | `JOYCODE_1_BUTTON5` / `7` | `JOYCODE_2_BUTTON5` / `7` |
| **A** | Low Attack - Fierce (Fierce Kick) | `BUTTON6` | `JOYCODE_1_BUTTON6` | `JOYCODE_2_BUTTON6` |
| **D-Pad / L-Stick** | Movement | `JOYSTICK_*` | Up / Down / Left / Right | Up / Down / Left / Right |
| **Plus (+)** | Start | `START1 / START2` | `JOYCODE_1_START` | `JOYCODE_2_START` |
| **Minus (-)** | Insert Coin | `COIN1 / COIN2` | `JOYCODE_1_SELECT` | `JOYCODE_2_SELECT` |
| **Stick L Click (L3)**| Pause Game | `UI_PAUSE` | Pause | Pause |
| **Stick R Click (R3)**| MAME Options / Cheats | `UI_MENU` | Menu | Menu |
| **Plus + Minus** | Quit to HOME menu | `UI_CANCEL` | Exit | Exit |

---

## Required ROM & CHD Details

This standalone port is based on **modern MAME (0.289)**. Ensure you are using the modern MAME dump of the game ROM archive and hard disk image:

### 1. Main ROM Archive (`kinst2.zip`)
- **SD Path:** `sdmc:/switch/kinst2/roms/kinst2.zip`
- **MAME Driver Set:** `kinst2` (Killer Instinct 2 parent set)
- **Default BIOS Version:** `v1.4` (`ki2-l14.u98` - CRC `27d0285e`, SHA-1 `aa7a2a9d72a47dd0ea2ee7b2776b79288060b179`)
- **Sound ROMs:** `ki2_l1.u10` through `ki2_l1.u36`
- *(Alternate BIOS revisions v1.3, v1.1, v1.0 and v1.4 AnyIDE are also recognized if present in the zip.)*

### 2. Hard Disk Image (`kinst2.chd`)
- **SD Path:** `sdmc:/switch/kinst2/roms/kinst2/kinst2.chd`
- **Format:** Modern MAME CHD format (CHD v5)
- **SHA-1 Checksum:** `e7c9291b4648eae0012ea0cc230731ed4987d1d5`
- **Subfolder Requirement:** MAME requires the hard disk image to be inside a subfolder matching the driver name (`kinst2/`) directly inside `roms/`.
- **Use the standard compressed CHD.** Do not convert it to an uncompressed CHD: MAME reads a blank SHA-1 from an uncompressed copy, which breaks its save file (`diff/kinst2.dif`, holding settings and high scores) on the next boot. The game decompresses the disk into RAM in the background during its first ~25 seconds, so compression has no effect on gameplay.

---

## Installation

> [!NOTE]
> Launch with **title override** (full RAM mode), not from the album/applet mode. The RAM disk preload (~440 MB) and CPU recompiler cache need more memory than applet mode provides.

1. Download the latest release from the [Releases](https://github.com/Thorhax/Killer-Instinct-2-NX-Modern/releases) page.
2. Extract the archive directly to your SD card. The structure should match:

```text
sdmc:/switch/kinst2/
├── kinst2.nro
├── ki2-icon.jpg
├── cfg/
│   ├── default.cfg
│   └── kinst2.cfg
├── ini/
└── roms/
    ├── kinst2.zip
    └── kinst2/
        └── kinst2.chd
```

> [!IMPORTANT]
> **Game Assets:** Arcade ROMs (`kinst2.zip`) and CHD disk images (`kinst2.chd`) are copyrighted by Rare / Nintendo / Midway and are not included in this repository. Place your own legally obtained copies into `sdmc:/switch/kinst2/roms/`.

---

## Troubleshooting

- `kinst2.log` in `sdmc:/switch/kinst2/` records each session; the previous four sessions are kept as `kinst2.1.log` ... `kinst2.4.log` (the newest is always `kinst2.log`).
- To record detailed per-second performance data (speed, frame skip, GPU and disk timing) for a bug report, create an empty file `sdmc:/switch/kinst2/perf.txt`. Delete it to turn logging off again.
- A `DIFF CHD ERROR` at boot means the save file doesn't match your CHD; delete `sdmc:/switch/kinst2/diff/kinst2.dif` (this resets settings and high scores).
- A brief slowdown during the first seconds of the startup sequence is expected while the disk is being loaded into RAM.

---

## Building from Source

### Prerequisites
- devkitPro toolchain (`devkitA64`), `libnx` and Switch portlibs (`switch-sdl2`, `switch-mesa`, `switch-glad`, etc.) — the build script uses the `devkitpro-mesa-rust` Docker image
- Python 3

### Compiling
Run the Switch build script from the repository root:

```bash
./build_switch.sh
```

It produces `kinst2.nro` in the repository root with NACP metadata and icon attached. The game this build boots (machine name, SD folder, titles) is set in `src/osd/sdl/nxgame.h`.

---

## Credits

- **MAME Team:** [mamedev/mame](https://github.com/mamedev/mame)
- **devkitPro & libnx:** [devkitPro](https://devkitpro.org)
- **Port Maintainer:** [Thorhax](https://github.com/Thorhax)
