// Web build stand-in for the ADX music player.
//
// The desktop port decodes the ADX music tracks with ffmpeg. That is a heavy
// dependency to pull into a browser build, so the web build starts without
// music: the calls are accepted and ignored, while the sound effects keep
// working through the emulated SPU (port/sound/spu.c), which is pure SDL.
//
// The ADX streams themselves are still shipped and decoded on the native
// builds.

#include "port/sdl/sdl_adx_sound.h"

#include <stddef.h>

void SDLADXSound_ProcessTracks() {
    // No music to advance.
}

void SDLADXSound_Init() {
    // No music device to open.
}

void SDLADXSound_Exit() {
}

void SDLADXSound_Stop() {
}

int SDLADXSound_IsPaused() {
    return 1;
}

void SDLADXSound_Pause(int pause) {
}

void SDLADXSound_StartSeamless() {
}

void SDLADXSound_StartMem(void* buf, size_t size) {
}

int SDLADXSound_GetNumFiles() {
    return 0;
}

void SDLADXSound_EntryAfs(int file_id) {
}

void SDLADXSound_StartAfs(int file_id) {
}

void SDLADXSound_ResetEntry() {
}

void SDLADXSound_SetOutVol(int volume) {
}

int SDLADXSound_GetStat() {
    return 0; // ADXT_STAT_STOP
}
