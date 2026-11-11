// The headless port: the functions the game calls in the SDL port
// (include/port/sdl/*.h, include/port/sound/spu.h), without a window, sound or frame
// pacing. Built with PLATFORM=headless instead of port/sdl/*.c and port/sound/spu.c.
//
// The frame structure is the SDL port's (port/sdl/sdl_app.c): main.c calls
// SDLApp_PollEvents() and SDLApp_BeginFrame() before the game frame, which reads the
// pads (flPADGetALL()), and SDLApp_EndFrame() after it. EndFrame must still run the
// PS2 vblank interrupt and ADXPS2_ExecVint(), which the CRI file system needs to make
// progress; only rendering, sound output and pacing are dropped.

#include "port/headless/headless.h"
#include "common.h"
#include "port/sdk_threads.h"
#include "port/sdl/sdl_adx_sound.h"
#include "port/sdl/sdl_app.h"
#include "port/sdl/sdl_game_renderer.h"
#include "port/sdl/sdl_message_renderer.h"
#include "port/sdl/sdl_pad.h"
#include "port/sound/spu.h"
#include "sf33rd/AcrSDK/ps2/foundaps2.h"

#include <stdlib.h>
#include <string.h>

// cri_mw.h conflicts with the port headers, as in sdl_app.c.
int ADXPS2_ExecVint(int mode);

static unsigned long frame_counter = 0;
static SDLPad_ButtonState pads[2];

// --- sdl_app.h ---------------------------------------------------------------

int SDLApp_Init() {
    memset(pads, 0, sizeof(pads));
    harness_init();
    return 0;
}

void SDLApp_Quit() {}

int SDLApp_PollEvents() {
    if (harness_should_quit()) {
        // main.c leaves its loop and returns from AcrMain(); exit with the harness's
        // verdict instead of main()'s 0.
        exit(harness_exit_code());
    }

    return 1;
}

void SDLApp_BeginFrame() {
    frame_counter += 1;
    harness_begin_frame(frame_counter);
}

void SDLApp_EndFrame() {
    begin_interrupt();
    ADXPS2_ExecVint(0);
    end_interrupt();
    harness_end_frame(frame_counter);
}

// --- sdl_pad.h ---------------------------------------------------------------

void headless_set_pad(int id, const SDLPad_ButtonState* state) {
    if (id >= 0 && id < 2) {
        pads[id] = *state;
    }
}

void SDLPad_Init() {}
void SDLPad_HandleGamepadDeviceEvent(SDL_GamepadDeviceEvent* event) {}
void SDLPad_HandleGamepadButtonEvent(SDL_GamepadButtonEvent* event) {}
void SDLPad_HandleGamepadAxisMotionEvent(SDL_GamepadAxisEvent* event) {}
void SDLPad_HandleKeyboardEvent(SDL_KeyboardEvent* event) {}

bool SDLPad_IsGamepadConnected(int id) {
    return id >= 0 && id < 2;
}

void SDLPad_GetButtonState(int id, SDLPad_ButtonState* state) {
    if (id >= 0 && id < 2) {
        *state = pads[id];
    } else {
        memset(state, 0, sizeof(*state));
    }
}

void SDLPad_RumblePad(int id, bool low_freq_enabled, Uint8 high_freq_rumble) {}

// --- sdl_game_renderer.h / sdl_message_renderer.h ------------------------------

SDL_Texture* cps3_canvas = NULL;
SDL_Texture* message_canvas = NULL;

void SDLGameRenderer_Init(SDL_Renderer* renderer) {}
void SDLGameRenderer_BeginFrame() {}
void SDLGameRenderer_RenderFrame() {}
void SDLGameRenderer_EndFrame() {}
void SDLGameRenderer_CreateTexture(unsigned int th) {}
void SDLGameRenderer_DestroyTexture(unsigned int texture_handle) {}
void SDLGameRenderer_UnlockTexture(unsigned int th) {}
void SDLGameRenderer_CreatePalette(unsigned int ph) {}
void SDLGameRenderer_DestroyPalette(unsigned int palette_handle) {}
void SDLGameRenderer_UnlockPalette(unsigned int ph) {}
void SDLGameRenderer_SetTexture(unsigned int th) {}
void SDLGameRenderer_DrawTexturedQuad(const SDLGameRenderer_Vertex* vertices) {}
void SDLGameRenderer_DrawSolidQuad(const SDLGameRenderer_Vertex* vertices) {}
void SDLGameRenderer_DrawSprite(const SDLGameRenderer_Sprite* sprite, unsigned int color) {}
void SDLGameRenderer_DrawSprite2(const SDLGameRenderer_Sprite2* sprite2) {}

void SDLMessageRenderer_Initialize(SDL_Renderer* renderer) {}
void SDLMessageRenderer_BeginFrame() {}
void SDLMessageRenderer_CreateTexture(int width, int height, void* pixels, int format) {}
void SDLMessageRenderer_DrawTexture(int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1,
                                    unsigned int color) {}

// --- sdl_adx_sound.h -----------------------------------------------------------
//
// The music player reports what the SDL one reports for a track that never ends:
// stopped (ADXT_STAT_STOP, 0) until a track starts, then playing (ADXT_STAT_PLAYING,
// 3) until it is stopped. Paused reads as stopped, as in sdl_adx_sound.c.

static int adx_started = 0;
static int adx_paused = 0;

void SDLADXSound_ProcessTracks() {}
void SDLADXSound_Init() {}
void SDLADXSound_Exit() {}

void SDLADXSound_Stop() {
    adx_started = 0;
}

int SDLADXSound_IsPaused() {
    return adx_paused;
}

void SDLADXSound_Pause(int pause) {
    adx_paused = pause;
}

void SDLADXSound_StartSeamless() {
    adx_paused = 0;
}

void SDLADXSound_StartMem(void* buf, size_t size) {
    adx_started = 1;
}

int SDLADXSound_GetNumFiles() {
    return adx_started;
}

void SDLADXSound_EntryAfs(int file_id) {}

void SDLADXSound_StartAfs(int file_id) {
    adx_started = 1;
}

void SDLADXSound_ResetEntry() {}
void SDLADXSound_SetOutVol(int volume) {}

int SDLADXSound_GetStat() {
    if (!adx_started || adx_paused) {
        return 0;
    }

    return 3;
}

// --- spu.h ---------------------------------------------------------------------

void SPU_Init() {}
void SPU_Upload(u32 dst, void* src, u32 size) {}
void SPU_Tick(s16* output) {}
void SPU_StartVoice(int vnum, struct SPUVConf* conf) {}

int SPU_VoiceGetVolume(int vnum) {
    return 0;
}

void SPU_KeyOffVoice(int vnum) {}
void SPU_StopVoice(int vnum) {}
