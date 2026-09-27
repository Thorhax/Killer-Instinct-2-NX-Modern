# Killer-Instinct-NX-Modern

[![License](https://img.shields.io/badge/license-BSD--3--Clause%20%2F%20GPL--2.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Nintendo%20Switch-red.svg)](https://switchbrew.org)

Standalone Nintendo Switch arcade port of **Killer Instinct** powered by modern MAME (0.289) and libnx. Built for Atmosphere CFW with native hardware-accelerated rendering, multi-core CPU scheduling, clean hbmenu integration, and full two-player controller support.

---

## Features

- **Direct Boot:** Boots directly into Killer Instinct (`kinst.nro`) without extra frontend menus.
- **Native Switch Pad API:** Low-latency native libnx `pad` input handling.
- **Multi-Core Scheduling:** Thread core mask enabled across all 3 available CPU cores (Cores 0, 1, and 2) for smooth emulation and audio synchronization.
- **Clean Exit Architecture:** Exits cleanly back to `hbmenu` without user break panics.
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
| **Plus + Minus** | Quit to hbmenu | `UI_CANCEL` | Exit | Exit |

---

## Installation

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
