/*
 * Headless diagnostic for commons-gamepad-touchpad.
 *
 * Opens no window, so it can run alongside a foreground app (e.g. over
 * ares-shell on a webOS TV) without interrupting it. On startup SDL delivers a
 * device-added event for everything already connected, so the same handlers log
 * both the initial set and later hotplug: game controllers have their touchpad
 * grabbed, everything else is logged as ignored. The grab is released on
 * disconnect and, since the kernel drops EVIOCGRAB on close, also on exit.
 *
 * Its own lines share the commons-logging format with the library's, tagged
 * "Example" versus "GamepadTouchpad" so the two layers stay distinguishable.
 *
 * Build: -DCOMMONS_GAMEPAD_TOUCHPAD_EXAMPLE=ON
 */
#include "gamepad_touchpad.h"

#include "logging.h"

#include <SDL.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#define TAG "Example"
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

static void on_joystick_added(int device_index) {
    // Game controllers are handled on the controller-added event; anything SDL
    // does not map as a controller (the TV's remote, IR receiver, ...) is only
    // ever a joystick, so log it here as ignored.
    if (SDL_IsGameController(device_index)) {
        return;
    }
    const char *name = SDL_JoystickNameForIndex(device_index);
    commons_log_info(TAG, "ignored     (not a game controller) name=\"%s\"", name != NULL ? name : "?");
}

static void on_controller_added(int device_index) {
    SDL_GameController *gc = SDL_GameControllerOpen(device_index);
    if (gc == NULL) {
        commons_log_warn(TAG, "open failed: %s", SDL_GetError());
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

    commons_log_info(TAG, "connected   id=%d bus=%-9s touchpad=%-7s name=\"%s\"", id, describe_bus(path),
                     tp != NULL ? "grabbed" : "none", name != NULL ? name : "?");

    for (int i = 0; i < MAX_PADS; i++) {
        if (!pads[i].used) {
            pads[i] = (pad_entry_t) {.id = id, .touchpad = tp, .used = true};
            return;
        }
    }
    commons_log_warn(TAG, "pad table full, not tracking id=%d", id);
    gamepad_touchpad_release(tp);
    SDL_GameControllerClose(gc);
}

static void on_controller_removed(SDL_JoystickID id) {
    for (int i = 0; i < MAX_PADS; i++) {
        if (pads[i].used && pads[i].id == id) {
            gamepad_touchpad_release(pads[i].touchpad);
            pads[i].used = false;
            commons_log_info(TAG, "disconnected id=%d", id);
            return;
        }
    }
}

int main(void) {
    commons_logging_init("gamepad-touchpad-example");
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        commons_log_fatal(TAG, "SDL_Init failed: %s", SDL_GetError());
        commons_logging_deinit();
        return 1;
    }
    commons_log_info(TAG, "watching for controllers (Ctrl-C to quit)");

    SDL_Event ev;
    while (SDL_WaitEvent(&ev)) {
        switch (ev.type) {
            case SDL_JOYDEVICEADDED:
                on_joystick_added(ev.jdevice.which);
                break;
            case SDL_CONTROLLERDEVICEADDED:
                on_controller_added(ev.cdevice.which);
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                on_controller_removed(ev.cdevice.which);
                break;
            case SDL_QUIT:
                SDL_Quit();
                commons_logging_deinit();
                return 0;
            default:
                break;
        }
    }
    SDL_Quit();
    commons_logging_deinit();
    return 0;
}
