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

#ifndef INPUT_PROP_ACCELEROMETER
#define INPUT_PROP_ACCELEROMETER 0x06
#endif

#define SYSFS_ROOT "/sys"
#define DEV_INPUT_ROOT "/dev/input"

#define MAX_GRABBED 8
#define NODE_NAME_MAX 32

struct gamepad_touchpad_t {
    int fds[MAX_GRABBED];
    int count;
};

static bool hid_parent_for_devnum(const char *sysfs_root, dev_t rdev, char *out, size_t out_len);

static bool is_hid_device_dirname(const char *name);

static int collect_grabbable_nodes(const char *hid_path, char names[][NODE_NAME_MAX], int max_names);

static bool input_child_reserved_by_sdl(const char *child_path);

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

    struct stat st;
    if (stat(dev_path, &st) != 0) {
        commons_log_debug("GamepadTouchpad", "Can't stat %s: %s", dev_path, strerror(errno));
        return NULL;
    }

    char hid_path[PATH_MAX];
    if (!hid_parent_for_devnum(SYSFS_ROOT, st.st_rdev, hid_path, sizeof(hid_path))) {
        commons_log_debug("GamepadTouchpad", "Can't resolve HID parent of %s", dev_path);
        return NULL;
    }

    char names[MAX_GRABBED][NODE_NAME_MAX];
    int num_names = collect_grabbable_nodes(hid_path, names, MAX_GRABBED);
    if (num_names == 0) {
        return NULL;
    }

    gamepad_touchpad_t *touchpad = SDL_calloc(1, sizeof(gamepad_touchpad_t));
    if (touchpad == NULL) {
        return NULL;
    }
    for (int i = 0; i < num_names; i++) {
        char node_path[PATH_MAX];
        if (snprintf(node_path, sizeof(node_path), "%s/%s", DEV_INPUT_ROOT, names[i]) >= (int) sizeof(node_path)) {
            continue;
        }
        int fd = open(node_path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }
        if (ioctl(fd, EVIOCGRAB, 1) < 0) {
            // Best-effort: a failed grab just means the platform may still see the
            // touchpad. Nothing the caller can act on, so info rather than warn.
            commons_log_info("GamepadTouchpad", "Can't grab %s: %s", node_path, strerror(errno));
            close(fd);
            continue;
        }
        commons_log_info("GamepadTouchpad", "Grabbed %s", node_path);
        touchpad->fds[touchpad->count++] = fd;
    }
    if (touchpad->count == 0) {
        SDL_free(touchpad);
        return NULL;
    }
    return touchpad;
}

void gamepad_touchpad_release(gamepad_touchpad_t *touchpad) {
    if (touchpad == NULL) {
        return;
    }
    for (int i = 0; i < touchpad->count; i++) {
        ioctl(touchpad->fds[i], EVIOCGRAB, 0);
        close(touchpad->fds[i]);
    }
    SDL_free(touchpad);
}

/**
 * Find the device that owns the input children of whatever node SDL bound.
 *
 * SDL reports an "event" or "js" node with the Linux driver, or a "hidraw" one
 * with HIDAPI; both live below the same HID device. /sys/class is not always
 * visible to a sandboxed app, so resolve through /sys/dev/char instead and walk
 * up to the enclosing HID device.
 *
 * The ancestor must be a real HID device (dir named BUS:VENDOR:PRODUCT.INSTANCE),
 * not merely something with an input/ subdirectory: virtual input devices - the
 * TV's own remote, IR receiver, etc. - all sit under /sys/devices/virtual, whose
 * input/ owns every one of them. Stopping there would grab the lot, including the
 * remote. A genuine controller always has its own HID device dir.
 *
 * @param sysfs_root Mount point of sysfs, parameterised for tests.
 */
static bool hid_parent_for_devnum(const char *sysfs_root, dev_t rdev, char *out, size_t out_len) {
    char link[PATH_MAX];
    if (snprintf(link, sizeof(link), "%s/dev/char/%u:%u", sysfs_root, major(rdev), minor(rdev)) >=
        (int) sizeof(link)) {
        return false;
    }
    char resolved[PATH_MAX];
    if (realpath(link, resolved) == NULL) {
        return false;
    }

    // Never walk above <sysfs_root>/devices.
    char devices_root[PATH_MAX];
    if (snprintf(link, sizeof(link), "%s/devices", sysfs_root) >= (int) sizeof(link) ||
        realpath(link, devices_root) == NULL) {
        return false;
    }
    size_t devices_root_len = strlen(devices_root);
    if (strncmp(resolved, devices_root, devices_root_len) != 0) {
        return false;
    }

    while (strlen(resolved) > devices_root_len) {
        char *slash = strrchr(resolved, '/');
        const char *base = slash != NULL ? slash + 1 : resolved;
        if (is_hid_device_dirname(base)) {
            if (strlen(resolved) >= out_len) {
                return false;
            }
            strcpy(out, resolved);
            return true;
        }
        if (slash == NULL) {
            break;
        }
        *slash = '\0';
    }
    return false;
}

/** A HID device sysfs dir is named "BUS:VENDOR:PRODUCT.INSTANCE", all hex. */
static bool is_hid_device_dirname(const char *name) {
    unsigned int bus, vendor, product, instance;
    int consumed = 0;
    return sscanf(name, "%x:%x:%x.%x%n", &bus, &vendor, &product, &instance, &consumed) == 4 &&
           name[consumed] == '\0';
}

/**
 * Collect the event nodes under a controller's HID device that are safe to grab.
 *
 * Everything below the HID device belongs to the controller, so provenance -
 * not the touchpad's own capabilities - decides. Grab every input child except
 * the two SDL reads itself: the joystick (identified by a js* node) and the
 * motion sensor (INPUT_PROP_ACCELEROMETER). What remains is the touchpad (and
 * any future controller-owned pointer device), which the platform would
 * otherwise consume as touchscreen input. A controller with no touchpad (e.g. a
 * plain Xbox pad) yields nothing.
 *
 * This keeps classification entirely in sysfs, so it needs no open evdev device.
 *
 * @return Number of event node names written to @p names.
 */
static int collect_grabbable_nodes(const char *hid_path, char names[][NODE_NAME_MAX], int max_names) {
    char inputs_path[PATH_MAX];
    if (snprintf(inputs_path, sizeof(inputs_path), "%s/input", hid_path) >= (int) sizeof(inputs_path)) {
        return 0;
    }
    DIR *inputs = opendir(inputs_path);
    if (inputs == NULL) {
        return 0;
    }
    int num_names = 0;
    struct dirent *input_ent;
    while (num_names < max_names && (input_ent = readdir(inputs)) != NULL) {
        if (strncmp(input_ent->d_name, "input", 5) != 0) {
            continue;
        }
        char input_path[PATH_MAX];
        if (snprintf(input_path, sizeof(input_path), "%s/%s", inputs_path, input_ent->d_name) >=
            (int) sizeof(input_path)) {
            continue;
        }
        if (input_child_reserved_by_sdl(input_path)) {
            continue;
        }
        DIR *input_dir = opendir(input_path);
        if (input_dir == NULL) {
            continue;
        }
        struct dirent *event_ent;
        while (num_names < max_names && (event_ent = readdir(input_dir)) != NULL) {
            if (strncmp(event_ent->d_name, "event", 5) != 0 || strlen(event_ent->d_name) >= NODE_NAME_MAX) {
                continue;
            }
            strcpy(names[num_names++], event_ent->d_name);
        }
        closedir(input_dir);
    }
    closedir(inputs);
    return num_names;
}

/**
 * True if an input child is one SDL consumes directly and must be left alone:
 * the joystick (has a js* node) or the motion sensor (INPUT_PROP_ACCELEROMETER).
 *
 * Despite the name, INPUT_PROP_ACCELEROMETER marks the whole inertial node - the
 * kernel reports accelerometer data on ABS_X/Y/Z and gyroscope data on
 * ABS_RX/RY/RZ under this one property, and there is no INPUT_PROP_GYROSCOPE. So
 * a DualShock 4 / DualSense combined motion node (which SDL reads as both
 * SDL_SENSOR_ACCEL and SDL_SENSOR_GYRO) is matched here and left to SDL.
 */
static bool input_child_reserved_by_sdl(const char *child_path) {
    DIR *dir = opendir(child_path);
    if (dir != NULL) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, "js", 2) == 0) {
                closedir(dir);
                return true;
            }
        }
        closedir(dir);
    }

    char props_path[PATH_MAX];
    if (snprintf(props_path, sizeof(props_path), "%s/properties", child_path) >= (int) sizeof(props_path)) {
        return false;
    }
    FILE *f = fopen(props_path, "r");
    if (f == NULL) {
        return false;
    }
    unsigned long props = 0;
    bool is_accel = fscanf(f, "%lx", &props) == 1 && ((props >> INPUT_PROP_ACCELEROMETER) & 1ul) != 0;
    fclose(f);
    return is_accel;
}
