/*
 * Headless diagnostic for commons-gamepad-touchpad.
 *
 * Opens no window, so it can run alongside a foreground app (e.g. over
 * ares-shell on a webOS TV) without interrupting it. On startup SDL delivers a
 * CONTROLLERDEVICEADDED for every already-connected controller, so the same
 * handler logs both the initial set and later hotplug. Each controller's
 * touchpad node is grabbed; the grab is released on disconnect and, since the
 * kernel drops EVIOCGRAB on close, also on exit.
 *
 * Build: -DCOMMONS_GAMEPAD_TOUCHPAD_EXAMPLE=ON
 */
#include "gamepad_touchpad.h"

#include <SDL.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#define MAX_PADS 16

typedef struct {
    SDL_JoystickID id;
    gamepad_touchpad_t *touchpad;
    bool used;
} pad_entry_t;

static pad_entry_t pads[MAX_PADS];

/* Infer the transport from the device's sysfs path. */
static const char *describe_bus(const char *dev_path) {
    if (dev_path == NULL) {
        return "unknown";
    }
    struct stat st;
    if (stat(dev_path, &st) != 0) {
        return "unknown";
    }
    char link[PATH_MAX], real[PATH_MAX];
    snprintf(link, sizeof(link), "/sys/dev/char/%u:%u", major(st.st_rdev), minor(st.st_rdev));
    if (realpath(link, real) == NULL) {
        return "unknown";
    }
    if (strstr(real, "/usb") != NULL) {
        return "USB";
    }
    if (strstr(real, "/bluetooth") != NULL || strstr(real, "/uhid") != NULL) {
        return "Bluetooth";
    }
    return "other";
}

static void on_added(int device_index) {
    if (!SDL_IsGameController(device_index)) {
        return;
    }
    SDL_GameController *gc = SDL_GameControllerOpen(device_index);
    if (gc == NULL) {
        SDL_Log("open failed: %s", SDL_GetError());
        return;
    }
    SDL_JoystickID id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gc));
    const char *name = SDL_GameControllerName(gc);
#if SDL_VERSION_ATLEAST(2, 24, 0)
    const char *path = SDL_GameControllerPath(gc);
#else
    const char *path = NULL;
#endif
    gamepad_touchpad_t *tp = gamepad_touchpad_grab(gc);

    SDL_Log("connected   id=%d bus=%-9s touchpad=%-7s name=\"%s\"", id, describe_bus(path),
            tp != NULL ? "grabbed" : "none", name != NULL ? name : "?");

    for (int i = 0; i < MAX_PADS; i++) {
        if (!pads[i].used) {
            pads[i] = (pad_entry_t) {.id = id, .touchpad = tp, .used = true};
            return;
        }
    }
    SDL_Log("pad table full, not tracking id=%d", id);
    gamepad_touchpad_release(tp);
    SDL_GameControllerClose(gc);
}

static void on_removed(SDL_JoystickID id) {
    for (int i = 0; i < MAX_PADS; i++) {
        if (pads[i].used && pads[i].id == id) {
            gamepad_touchpad_release(pads[i].touchpad);
            pads[i].used = false;
            SDL_Log("disconnected id=%d", id);
            return;
        }
    }
    SDL_Log("disconnected id=%d (was not tracked)", id);
}

int main(void) {
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Log("watching for controllers (Ctrl-C to quit)");

    SDL_Event ev;
    while (SDL_WaitEvent(&ev)) {
        switch (ev.type) {
            case SDL_CONTROLLERDEVICEADDED:
                on_added(ev.cdevice.which);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                on_removed(ev.cdevice.which);
                break;
            case SDL_QUIT:
                SDL_Quit();
                return 0;
            default:
                break;
        }
    }
    SDL_Quit();
    return 0;
}
