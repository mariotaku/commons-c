#include "gamepad_touchpad.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <linux/input.h>

#include <SDL_stdinc.h>
#include <SDL_version.h>

#include "logging.h"

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define NBITS(x) (((x) - 1) / BITS_PER_LONG + 1)
#define TEST_BIT(bit, array) (((array)[(bit) / BITS_PER_LONG] >> ((bit) % BITS_PER_LONG)) & 1ul)

struct gamepad_touchpad_t {
    int fd;
};

static bool sysfs_hid_parent(const char *dev_path, char *out, size_t out_len);

static bool find_touchpad_sibling(const char *hid_path, char *out, size_t out_len);

static bool is_multitouch_pointer(int fd);

gamepad_touchpad_t *gamepad_touchpad_grab(SDL_GameController *controller) {
    if (controller == NULL) {
        return NULL;
    }
#if SDL_VERSION_ATLEAST(2, 24, 0)
    const char *dev_path = SDL_GameControllerPath(controller);
#else
    const char *dev_path = NULL;
#endif
    if (dev_path == NULL) {
        commons_log_debug("GamepadTouchpad", "No device path for controller");
        return NULL;
    }

    // Whether SDL bound the pad through evdev or hidapi, both nodes live under
    // the same HID device in sysfs, so one walk covers either backend.
    char hid_path[PATH_MAX];
    if (!sysfs_hid_parent(dev_path, hid_path, sizeof(hid_path))) {
        commons_log_debug("GamepadTouchpad", "Can't resolve HID parent of %s", dev_path);
        return NULL;
    }

    char node_path[PATH_MAX];
    if (!find_touchpad_sibling(hid_path, node_path, sizeof(node_path))) {
        return NULL;
    }

    int fd = open(node_path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        commons_log_warn("GamepadTouchpad", "Can't open %s: %s", node_path, strerror(errno));
        return NULL;
    }
    if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        commons_log_warn("GamepadTouchpad", "Can't grab %s: %s", node_path, strerror(errno));
        close(fd);
        return NULL;
    }

    gamepad_touchpad_t *touchpad = SDL_malloc(sizeof(gamepad_touchpad_t));
    if (touchpad == NULL) {
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
        return NULL;
    }
    touchpad->fd = fd;
    commons_log_info("GamepadTouchpad", "Grabbed %s", node_path);
    return touchpad;
}

void gamepad_touchpad_release(gamepad_touchpad_t *touchpad) {
    if (touchpad == NULL) {
        return;
    }
    ioctl(touchpad->fd, EVIOCGRAB, 0);
    close(touchpad->fd);
    SDL_free(touchpad);
}

/**
 * Map the device node SDL bound to the HID device that owns it.
 * Both the input and hidraw children live below the same HID device, so the
 * one that owns the input* children is the common ancestor to search from.
 */
static bool sysfs_hid_parent(const char *dev_path, char *out, size_t out_len) {
    // /sys/class/{input,hidraw} isn't always visible to a sandboxed app, but
    // /sys/dev/char is, so resolve through the device number instead. SDL bound
    // either an event*/js* node (Linux driver) or a hidraw* one (HIDAPI); both
    // live below the same HID device.
    struct stat st;
    if (stat(dev_path, &st) < 0) {
        return false;
    }
    char link[PATH_MAX];
    if (snprintf(link, sizeof(link), "/sys/dev/char/%u:%u", major(st.st_rdev), minor(st.st_rdev)) >=
        (int) sizeof(link)) {
        return false;
    }
    char resolved[PATH_MAX];
    if (realpath(link, resolved) == NULL) {
        return false;
    }

    // Walk up from .../<hid>/input/inputN/eventM or .../<hid>/hidraw/hidrawN
    // until we reach the ancestor that owns the input children.
    while (strncmp(resolved, "/sys/devices/", 13) == 0) {
        char probe[PATH_MAX];
        if (snprintf(probe, sizeof(probe), "%s/input", resolved) < (int) sizeof(probe)) {
            struct stat probe_st;
            if (stat(probe, &probe_st) == 0 && S_ISDIR(probe_st.st_mode)) {
                if (strlen(resolved) >= out_len) {
                    return false;
                }
                strcpy(out, resolved);
                return true;
            }
        }
        char *slash = strrchr(resolved, '/');
        if (slash == NULL) {
            break;
        }
        *slash = '\0';
    }
    return false;
}

/** Look for a multitouch trackpad among the HID device's other input children. */
static bool find_touchpad_sibling(const char *hid_path, char *out, size_t out_len) {
    char inputs_path[PATH_MAX];
    if (snprintf(inputs_path, sizeof(inputs_path), "%s/input", hid_path) >= (int) sizeof(inputs_path)) {
        return false;
    }
    DIR *inputs = opendir(inputs_path);
    if (inputs == NULL) {
        return false;
    }
    bool found = false;
    struct dirent *input_ent;
    while (!found && (input_ent = readdir(inputs)) != NULL) {
        if (strncmp(input_ent->d_name, "input", 5) != 0) {
            continue;
        }
        char input_path[PATH_MAX];
        if (snprintf(input_path, sizeof(input_path), "%s/%s", inputs_path, input_ent->d_name) >=
            (int) sizeof(input_path)) {
            continue;
        }
        DIR *input_dir = opendir(input_path);
        if (input_dir == NULL) {
            continue;
        }
        struct dirent *event_ent;
        while (!found && (event_ent = readdir(input_dir)) != NULL) {
            if (strncmp(event_ent->d_name, "event", 5) != 0) {
                continue;
            }
            char node_path[PATH_MAX];
            if (snprintf(node_path, sizeof(node_path), "/dev/input/%s", event_ent->d_name) >=
                (int) sizeof(node_path)) {
                continue;
            }
            int fd = open(node_path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
            if (fd < 0) {
                continue;
            }
            if (is_multitouch_pointer(fd) && strlen(node_path) < out_len) {
                strcpy(out, node_path);
                found = true;
            }
            close(fd);
        }
        closedir(input_dir);
    }
    closedir(inputs);
    return found;
}

/**
 * The kernel reports a controller touchpad as a pointer device
 * (INPUT_PROP_POINTER, not INPUT_PROP_DIRECT) with multitouch axes. The gamepad
 * node has no ABS_MT axes, so it is never matched.
 */
static bool is_multitouch_pointer(int fd) {
    unsigned long props[NBITS(INPUT_PROP_MAX)];
    memset(props, 0, sizeof(props));
    if (ioctl(fd, EVIOCGPROP(sizeof(props)), props) < 0) {
        return false;
    }
    if (!TEST_BIT(INPUT_PROP_POINTER, props) || TEST_BIT(INPUT_PROP_DIRECT, props)) {
        return false;
    }

    unsigned long absbits[NBITS(ABS_MAX)];
    memset(absbits, 0, sizeof(absbits));
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) < 0) {
        return false;
    }
    return TEST_BIT(ABS_MT_POSITION_X, absbits) && TEST_BIT(ABS_MT_POSITION_Y, absbits);
}
