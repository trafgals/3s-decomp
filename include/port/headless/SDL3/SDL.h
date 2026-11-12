#ifndef HEADLESS_SDL_TYPES_H
#define HEADLESS_SDL_TYPES_H

// The headless build (PLATFORM=headless) has no SDL. The headers of the SDL port
// (include/port/sdl/*.h) only need these type names: the game code never calls SDL
// itself, it calls the port functions, and the headless port implements those
// without SDL (src/anniversary/port/headless/headless_port.c). This directory is
// put first on the include path of the headless build only.

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t Uint8;
typedef int16_t Sint16;
typedef uint16_t Uint16;
typedef int32_t Sint32;
typedef uint32_t Uint32;
typedef uint64_t Uint64;

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_GamepadDeviceEvent SDL_GamepadDeviceEvent;
typedef struct SDL_GamepadButtonEvent SDL_GamepadButtonEvent;
typedef struct SDL_GamepadAxisEvent SDL_GamepadAxisEvent;
typedef struct SDL_KeyboardEvent SDL_KeyboardEvent;

#endif
