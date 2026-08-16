/*
 * Fixture tests for the sysfs traversal and classification in gamepad_touchpad.c.
 *
 * Both the HID-parent resolution and the decision of which sibling to grab are
 * pure sysfs logic, so they run against synthetic sysfs trees - no real evdev
 * device is needed. Only the EVIOCGRAB itself does.
 *
 * The .c is included directly so the static helpers can be reached, following
 * the pattern used by evmouse_tests.c.
 */
#include "gamepad_touchpad.c"

#include <assert.h>
#include <stdarg.h>

#define DS4_HID "0003:054C:09CC.0021"

#define XBOX_HID "0003:045E:02EA.0030"

static char fixture_root[PATH_MAX];

static void fixture_mkdir(const char *fmt, ...);

static void fixture_symlink(const char *target, const char *fmt, ...);

static void fixture_write(const char *content, const char *fmt, ...);

/**
 * Mirrors a DualShock 4 on webOS: one HID device owning three input children -
 * joystick (has a js node), touchpad (INPUT_PROP_POINTER/BUTTONPAD = 0x5) and
 * motion sensor (INPUT_PROP_ACCELEROMETER = 0x40) - reachable through
 * /sys/dev/char by its hidraw or any event node. Values match the real hardware.
 */
static void build_ds4_fixture(void) {
    fixture_mkdir("%s/dev/char", fixture_root);

    // input14 = joystick: has a js node, so SDL's Linux driver owns it.
    fixture_mkdir("%s/devices/platform/usb/%s/input/input14/event16", fixture_root, DS4_HID);
    fixture_mkdir("%s/devices/platform/usb/%s/input/input14/js7", fixture_root, DS4_HID);
    // input15 = touchpad.
    fixture_mkdir("%s/devices/platform/usb/%s/input/input15/event14", fixture_root, DS4_HID);
    fixture_write("5", "%s/devices/platform/usb/%s/input/input15/properties", fixture_root, DS4_HID);
    // input16 = motion sensor: SDL opens it as a sensor.
    fixture_mkdir("%s/devices/platform/usb/%s/input/input16/event15", fixture_root, DS4_HID);
    fixture_write("40", "%s/devices/platform/usb/%s/input/input16/properties", fixture_root, DS4_HID);
    fixture_mkdir("%s/devices/platform/usb/%s/hidraw/hidraw0", fixture_root, DS4_HID);

    // 13:78 is /dev/input/event14, 13:80 is event16, 234:0 is /dev/hidraw0
    fixture_symlink("../../devices/platform/usb/" DS4_HID "/input/input15/event14", "%s/dev/char/13:78",
                    fixture_root);
    fixture_symlink("../../devices/platform/usb/" DS4_HID "/input/input14/event16", "%s/dev/char/13:80",
                    fixture_root);
    fixture_symlink("../../devices/platform/usb/" DS4_HID "/hidraw/hidraw0", "%s/dev/char/234:0", fixture_root);

    // An Xbox-style pad: joystick only, no touchpad.
    fixture_mkdir("%s/devices/platform/usb/%s/input/input20/event20", fixture_root, XBOX_HID);
    fixture_mkdir("%s/devices/platform/usb/%s/input/input20/js8", fixture_root, XBOX_HID);

    // Virtual input hub: the TV's own remote, IR receiver, etc. all share
    // /sys/devices/virtual, whose input/ owns every one of them. SDL can even
    // report one (a magic remote) as a game controller. The walk must not stop
    // here, or it would grab all of these - including the remote (event0).
    fixture_mkdir("%s/devices/virtual/input/input0/event0", fixture_root);
    fixture_mkdir("%s/devices/virtual/input/input1/event1", fixture_root);
    fixture_mkdir("%s/devices/virtual/input/input11/event11", fixture_root);
    fixture_symlink("../../devices/virtual/input/input11/event11", "%s/dev/char/13:75", fixture_root);
}

/** Whichever node SDL bound, the walk must land on the same HID device. */
static void test_hid_parent_from_any_node(void) {
    char expected[PATH_MAX];
    snprintf(expected, sizeof(expected), "%s/devices/platform/usb/%s", fixture_root, DS4_HID);

    const dev_t nodes[] = {makedev(13, 78), makedev(13, 80), makedev(234, 0)};
    for (size_t i = 0; i < sizeof(nodes) / sizeof(nodes[0]); i++) {
        char out[PATH_MAX];
        assert(hid_parent_for_devnum(fixture_root, nodes[i], out, sizeof(out)));
        assert(strcmp(out, expected) == 0);
    }
}

/**
 * A virtual device (no HID parent) must not resolve - otherwise the walk would
 * stop at /devices/virtual and grab every device sharing that hub, the remote
 * included.
 */
static void test_hid_parent_virtual_device_rejected(void) {
    char out[PATH_MAX];
    assert(!hid_parent_for_devnum(fixture_root, makedev(13, 75), out, sizeof(out)));
}

/** The HID-device-dir predicate accepts real names and rejects everything else. */
static void test_is_hid_device_dirname(void) {
    assert(is_hid_device_dirname("0003:054C:09CC.0021"));
    assert(is_hid_device_dirname("0005:054C:09CC.0041"));
    assert(!is_hid_device_dirname("virtual"));
    assert(!is_hid_device_dirname("input15"));
    assert(!is_hid_device_dirname("0003:054C:09CC"));      // no instance
    assert(!is_hid_device_dirname("0003:054C:09CC.0021x")); // trailing junk
}

/** An unknown device number must fail rather than resolve to something else. */
static void test_hid_parent_unknown_devnum(void) {
    char out[PATH_MAX];
    assert(!hid_parent_for_devnum(fixture_root, makedev(99, 99), out, sizeof(out)));
}

/** Only the touchpad node is grabbable: the joystick and sensor are SDL's. */
static void test_grabbable_is_touchpad_only(void) {
    char hid_path[PATH_MAX];
    snprintf(hid_path, sizeof(hid_path), "%s/devices/platform/usb/%s", fixture_root, DS4_HID);

    char names[MAX_GRABBED][NODE_NAME_MAX];
    int count = collect_grabbable_nodes(hid_path, names, MAX_GRABBED);
    assert(count == 1);
    assert(strcmp(names[0], "event14") == 0);
}

/** The joystick child (has a js node) is reserved for SDL. */
static void test_reserved_joystick(void) {
    char child[PATH_MAX];
    snprintf(child, sizeof(child), "%s/devices/platform/usb/%s/input/input14", fixture_root, DS4_HID);
    assert(input_child_reserved_by_sdl(child));
}

/** The motion-sensor child (INPUT_PROP_ACCELEROMETER) is reserved for SDL. */
static void test_reserved_accelerometer(void) {
    char child[PATH_MAX];
    snprintf(child, sizeof(child), "%s/devices/platform/usb/%s/input/input16", fixture_root, DS4_HID);
    assert(input_child_reserved_by_sdl(child));
}

/** The touchpad child (pointer, not accelerometer, no js) is not reserved. */
static void test_touchpad_not_reserved(void) {
    char child[PATH_MAX];
    snprintf(child, sizeof(child), "%s/devices/platform/usb/%s/input/input15", fixture_root, DS4_HID);
    assert(!input_child_reserved_by_sdl(child));
}

/** A controller with no touchpad (joystick only) yields nothing to grab. */
static void test_grabbable_none_without_touchpad(void) {
    char hid_path[PATH_MAX];
    snprintf(hid_path, sizeof(hid_path), "%s/devices/platform/usb/%s", fixture_root, XBOX_HID);

    char names[MAX_GRABBED][NODE_NAME_MAX];
    assert(collect_grabbable_nodes(hid_path, names, MAX_GRABBED) == 0);
}

/** A device with no input children yields nothing rather than misbehaving. */
static void test_grabbable_without_input_dir(void) {
    char hid_path[PATH_MAX];
    snprintf(hid_path, sizeof(hid_path), "%s/devices/platform/usb/%s/hidraw", fixture_root, DS4_HID);

    char names[MAX_GRABBED][NODE_NAME_MAX];
    assert(collect_grabbable_nodes(hid_path, names, MAX_GRABBED) == 0);
}

/* --- fixture plumbing --- */

static void fixture_mkdir(const char *fmt, ...) {
    char path[PATH_MAX];
    va_list args;
    va_start(args, fmt);
    vsnprintf(path, sizeof(path), fmt, args);
    va_end(args);

    for (char *p = path + 1; *p != '\0'; p++) {
        if (*p != '/') {
            continue;
        }
        *p = '\0';
        if (mkdir(path, 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "mkdir %s: %s\n", path, strerror(errno));
            exit(1);
        }
        *p = '/';
    }
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "mkdir %s: %s\n", path, strerror(errno));
        exit(1);
    }
}

static void fixture_symlink(const char *target, const char *fmt, ...) {
    char path[PATH_MAX];
    va_list args;
    va_start(args, fmt);
    vsnprintf(path, sizeof(path), fmt, args);
    va_end(args);

    if (symlink(target, path) != 0 && errno != EEXIST) {
        fprintf(stderr, "symlink %s: %s\n", path, strerror(errno));
        exit(1);
    }
}

static void fixture_write(const char *content, const char *fmt, ...) {
    char path[PATH_MAX];
    va_list args;
    va_start(args, fmt);
    vsnprintf(path, sizeof(path), fmt, args);
    va_end(args);

    FILE *f = fopen(path, "w");
    if (f == NULL) {
        fprintf(stderr, "fopen %s: %s\n", path, strerror(errno));
        exit(1);
    }
    fputs(content, f);
    fputc('\n', f);
    fclose(f);
}

static void fixture_rmtree(const char *path) {
    DIR *dir = opendir(path);
    if (dir != NULL) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            char child[PATH_MAX];
            if (snprintf(child, sizeof(child), "%s/%s", path, ent->d_name) < (int) sizeof(child)) {
                fixture_rmtree(child);
            }
        }
        closedir(dir);
    }
    remove(path);
}

int main() {
    char tmpl[] = "/tmp/gamepad_touchpad_tests.XXXXXX";
    if (mkdtemp(tmpl) == NULL) {
        fprintf(stderr, "mkdtemp: %s\n", strerror(errno));
        return 1;
    }
    // hid_parent_for_devnum compares realpath() output, so the root must be
    // canonical too - /tmp is a symlink on some systems.
    if (realpath(tmpl, fixture_root) == NULL) {
        fprintf(stderr, "realpath: %s\n", strerror(errno));
        return 1;
    }
    build_ds4_fixture();

    test_hid_parent_from_any_node();
    test_hid_parent_virtual_device_rejected();
    test_is_hid_device_dirname();
    test_hid_parent_unknown_devnum();
    test_grabbable_is_touchpad_only();
    test_reserved_joystick();
    test_reserved_accelerometer();
    test_touchpad_not_reserved();
    test_grabbable_none_without_touchpad();
    test_grabbable_without_input_dir();

    fixture_rmtree(fixture_root);
    printf("All tests passed\n");
    return 0;
}
