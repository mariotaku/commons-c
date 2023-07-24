#pragma once

#include "SDL_polyfill_target.h"

#define POLYFILL __attribute__((unused))
#define NEED_POLYFILL(major, minor, patch) (COMMONS_SDL2_POLYFILL_TARGET_MINOR < (minor) || COMMONS_SDL2_POLYFILL_TARGET_PATCH < (patch))

void SDL_Polyfill_Unresolved(void);
