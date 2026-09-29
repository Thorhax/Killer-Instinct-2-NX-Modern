// license:BSD-3-Clause
//============================================================
//
//  nxgame.h - which game this standalone Switch build boots
//
//  Everything game-specific in the Switch OSD layer (machine
//  name, SD card folder, log file names, titles) comes from here.
//
//============================================================

#ifndef MAME_OSD_SDL_NXGAME_H
#define MAME_OSD_SDL_NXGAME_H

#pragma once

#define NX_GAME_NAME    "kinst2"                    // MAME machine booted by default
#define NX_GAME_TITLE   "Killer Instinct 2"         // used in log messages
#define NX_SD_DIR       "sdmc:/switch/" NX_GAME_NAME // home folder on the SD card

#endif // MAME_OSD_SDL_NXGAME_H
