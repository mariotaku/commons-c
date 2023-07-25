#include <SDL_rwops.h>
#include <SDL_gamecontroller.h>

#include "../SDL_internal.h"
#include "SDL_joystick_c.h"

#include <dlfcn.h>

#define SDL_CONTROLLER_PLATFORM_FIELD   "platform:"

#if NEED_POLYFILL(2, 0, 2)

POLYFILL int SDL_GameControllerAddMappingsFromRW(SDL_RWops *rw, int freerw)
{
    const char *platform = SDL_GetPlatform();
    int controllers = 0;
    char *buf, *line, *line_end, *tmp, *comma, line_platform[64];
    size_t db_size, platform_len;

    if (rw == NULL) {
        return SDL_SetError("Invalid RWops");
    }
    db_size = (size_t) rw->size(rw);

    buf = (char *) SDL_malloc(db_size + 1);
    if (buf == NULL) {
        if (freerw) {
            rw->close(rw);
        }
        return SDL_SetError("Could not allocate space to read DB into memory");
    }

    if (rw->read(rw, buf, db_size, 1) != 1) {
        if (freerw) {
            rw->close(rw);
        }
        SDL_free(buf);
        return SDL_SetError("Could not read DB");
    }

    if (freerw) {
        rw->close(rw);
    }

    buf[db_size] = '\0';
    line = buf;

    while (line < buf + db_size) {
        line_end = SDL_strchr(line, '\n');
        if (line_end != NULL) {
            *line_end = '\0';
        } else {
            line_end = buf + db_size;
        }

        /* Extract and verify the platform */
        tmp = SDL_strstr(line, SDL_CONTROLLER_PLATFORM_FIELD);
        if (tmp != NULL) {
            tmp += SDL_strlen(SDL_CONTROLLER_PLATFORM_FIELD);
            comma = SDL_strchr(tmp, ',');
            if (comma != NULL) {
                platform_len = comma - tmp + 1;
                if (platform_len + 1 < SDL_arraysize(line_platform)) {
                    SDL_strlcpy(line_platform, tmp, platform_len);
                    if (SDL_strncasecmp(line_platform, platform, platform_len) == 0 &&
                        SDL_GameControllerAddMapping(line) > 0) {
                        controllers++;
                    }
                }
            }
        }

        line = line_end + 1;
    }

    SDL_free(buf);
    return controllers;
}

#endif // NEED_POLYFILL(2, 0, 2)

#if NEED_POLYFILL(2, 0, 12)

// line:2663
POLYFILL SDL_GameControllerType SDL_GameControllerGetType(SDL_GameController *gamecontroller)
{
    static SDL_GameControllerType (*orig)(SDL_GameController *) = (void *) SDL_Polyfill_Unresolved;
    if ((void *) orig == SDL_Polyfill_Unresolved) {
        orig = dlsym(RTLD_NEXT, __func__);
    }
    if (orig != NULL) {
        return orig(gamecontroller);
    }

    SDL_Joystick *joystick = SDL_GameControllerGetJoystick(gamecontroller);

    if (joystick == NULL) {
        return SDL_CONTROLLER_TYPE_UNKNOWN;
    }
    return SDL_GetJoystickGameControllerTypeFromGUID(SDL_JoystickGetGUID(joystick), SDL_JoystickName(joystick));
}

#endif // NEED_POLYFILL(2, 0, 12)

#if NEED_POLYFILL(2, 0, 14)

/**
 * Return whether a game controller has a particular sensor.
 * @see https://github.com/libsdl-org/SDL/blob/release-2.28.1/src/joystick/SDL_gamecontroller.c#L2473
 */
POLYFILL SDL_bool SDL_GameControllerHasSensor(SDL_GameController *gamecontroller, SDL_SensorType type)
{
    static SDL_bool (*orig)(SDL_GameController *, SDL_SensorType) = (void *) SDL_Polyfill_Unresolved;
    if ((void *) orig == SDL_Polyfill_Unresolved) {
        orig = dlsym(RTLD_NEXT, __func__);
    }
    if (orig != NULL) {
        return orig(gamecontroller, type);
    }
    return SDL_FALSE;
}

/**
 * Set whether data reporting for a game controller sensor is enabled
 * @see https://github.com/libsdl-org/SDL/blob/release-2.28.1/src/joystick/SDL_gamecontroller.c#L2498
 */
POLYFILL int
SDL_GameControllerSetSensorEnabled(SDL_GameController *gamecontroller, SDL_SensorType type, SDL_bool enabled)
{
    static int (*orig)(SDL_GameController *, SDL_SensorType, SDL_bool) = (void *) SDL_Polyfill_Unresolved;
    if ((void *) orig == SDL_Polyfill_Unresolved) {
        orig = dlsym(RTLD_NEXT, __func__);
    }
    if (orig != NULL) {
        return orig(gamecontroller, type, enabled);
    }
    return SDL_Unsupported();
}

/**
 * Query whether sensor data reporting is enabled for a game controller
 * @see https://github.com/libsdl-org/SDL/blob/release-2.28.1/src/joystick/SDL_gamecontroller.c#L2547
 */
POLYFILL SDL_bool SDL_GameControllerIsSensorEnabled(SDL_GameController *gamecontroller, SDL_SensorType type)
{
    static SDL_bool (*orig)(SDL_GameController *, SDL_SensorType) = (void *) SDL_Polyfill_Unresolved;
    if ((void *) orig == SDL_Polyfill_Unresolved) {
        orig = dlsym(RTLD_NEXT, __func__);
    }
    if (orig != NULL) {
        return orig(gamecontroller, type);
    }
    return SDL_FALSE;
}

#endif // NEED_POLYFILL(2, 0, 14)