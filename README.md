# Killer-Instinct-NX-Modern

[![License](https://img.shields.io/badge/license-BSD--3--Clause%20%2F%20GPL--2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Nintendo%20Switch-red.svg)](https://switchbrew.org)

Standalone Nintendo Switch arcade port of **Killer Instinct** powered by modern MAME (0.289) and libnx. Built for Atmosphere CFW with native hardware-accelerated rendering, multi-core CPU scheduling, clean hbmenu integration, and full two-player controller support.

---

## Features

- **Direct Boot:** Boots directly into Killer Instinct (`kinst.nro`) without extra frontend menus.
- **Native Switch Pad API:** Low-latency native libnx `pad` input handling.
- **Multi-Core Scheduling:** Thread core mask enabled across all 3 available CPU cores (Cores 0, 1, and 2) for smooth emulation and audio synchronization.
- **Clean Exit Architecture:** `Plus + Minus` saves settings/high scores and closes the game cleanly to the Switch HOME menu.
- **Stutter-Free Disk Access:** The whole hard disk image is decompressed into RAM in the background while the game boots, so FMVs, stage loads and streamed backgrounds never wait on the SD card.
- **Stable Long Sessions:** GPU frame pacing keeps the display from falling behind during long play sessions.
- **Intuitive Combat Controls:** Six-button arcade layout configured with dedicated Punch and Kick rows.
- **Two-Player Parity:** Complete Player 1 and Player 2 support with mirrored bindings.
- **Safe Quit Combo:** `Plus + Minus` combo prevents accidental in-game exits during fast button combos.
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

### 1. Main ROM Archive (`kinst.zip`)
- **SD Path:** `sdmc:/switch/kinst/roms/kinst.zip`
- **MAME Driver Set:** `kinst` (Killer Instinct parent set)
- **Default BIOS Version:** `v1.5d` (`ki-l15d.u98` - CRC `7b65ca3d`, SHA-1 `607394d4ba1713f38c2cb5159303cace9cde991e`)
- **Sound ROMs:** `u10-l1` through `u36-l1`
- *(Alternate BIOS revisions like v1.4, v1.3, and proto v4.7 are also recognized if present in the zip).*

### 2. Hard Disk Image (`kinst.chd`)
- **SD Path:** `sdmc:/switch/kinst/roms/kinst/kinst.chd`
- **Format:** Modern MAME CHD format (CHD v5)
- **SHA-1 Checksum:** `81d833236e994528d1482979261401b198d1ca53`
- **Subfolder Requirement:** MAME requires the hard disk image to be inside a subfolder matching the driver name (`kinst/`) directly inside `roms/`.
- **Use the standard compressed CHD.** Do not convert it to an uncompressed CHD: MAME reads a blank SHA-1 from an uncompressed copy, which breaks its save file (`diff/kinst.dif`, holding settings and high scores) on the next boot. The game decompresses the disk into RAM in the background during its first ~15 seconds, so compression has no effect on gameplay.

---

## Installation

> [!NOTE]
> Launch with **title override** (full RAM mode), not from the album/applet mode. The RAM disk preload and CPU recompiler cache need more memory than applet mode provides.

1. Download the latest release from the [Releases](https://github.com/Thorhax/Killer-Instinct-NX-Modern/releases) page.
2. Extract the archive directly to your SD card. The structure should match:

```text
sdmc:/switch/kinst/
├── kinst.nro
├── ki-newicon.jpg
├── cfg/
│   ├── default.cfg
│   └── kinst.cfg
├── ini/
└── roms/
    ├── kinst.zip
    └── kinst/
        └── kinst.chd
```

> [!IMPORTANT]
> **Game Assets:** Arcade ROMs (`kinst.zip`) and CHD disk images (`kinst.chd`) are copyrighted by Rare / Midway and are not included in this repository. Place your own legally obtained copies into `sdmc:/switch/kinst/roms/`.

---

## Troubleshooting

- `kinst.log` in `sdmc:/switch/kinst/` records each session; the previous four sessions are kept as `kinst.1.log` ... `kinst.4.log` (the newest is always `kinst.log`).
- To record detailed per-second performance data (speed, frame skip, GPU and disk timing) for a bug report, create an empty file `sdmc:/switch/kinst/perf.txt`. Delete it to turn logging off again.
- A `DIFF CHD ERROR` at boot means the save file doesn't match your CHD; delete `sdmc:/switch/kinst/diff/kinst.dif` (this resets settings and high scores).

---

## Building from Source

### Prerequisites
- devkitPro toolchain (`devkitA64`)
- `libnx`
- Switch portlibs (`switch-sdl2`, `switch-mesa`, `switch-glad`, etc.)
- Python 3

### Compiling
Run the Switch build script:

```bash
./build_switch.sh
```

The resulting `kinst.nro` will be output to the build directory with NACP metadata and custom icon attached.

---

## Credits

- **MAME Team:** [mamedev/mame](https://github.com/mamedev/mame)
- **devkitPro & libnx:** [devkitPro](https://devkitpro.org)
- **Port Maintainer:** [Thorhax](https://github.com/Thorhax)
