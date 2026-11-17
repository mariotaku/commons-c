/* Use pipes for evdev input so disconnect (EOF) and event delivery exercise
 * the real select/read loop. Only discovery and device ioctls are simulated. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <SDL.h>

static FILE *input_inventory(const char *path, const char *mode);
static int open_device(const char *path, int flags);
static int device_ioctl(int fd, unsigned long request, ...);
#define fopen input_inventory
#define open open_device
#define ioctl device_ioctl
#include "evmouse.c"
#undef fopen
#undef open
#undef ioctl

static evmouse_t *mouse;
static int channel[2];
static int node, replacement_node, opens, grabs, motions;
static SDL_bool grab_replacement;

static FILE *input_inventory(const char *path, const char *mode) {
    assert(strcmp(path, "/proc/bus/input/devices") == 0);
    (void)mode;
    FILE *file = tmpfile();
    assert(file);
    fputs("H: Handlers=kbd event2\nB: KEY=1\n\n", file);
    fputs("H: Handlers=event3\nB: REL=1\n\n", file);
    if (node >= 0) fprintf(file, "H: Handlers=mouse0 event%d\nB: REL=143\n", node);
    rewind(file);
    return file;
}

static int open_device(const char *path, int flags) {
    char expected[32];
    snprintf(expected, sizeof(expected), "/dev/input/event%d", node);
    assert(strcmp(path, expected) == 0);
    assert(flags & O_NONBLOCK);
    opens++;
    return dup(channel[0]);
}

static int device_ioctl(int fd, unsigned long request, ...) {
    (void)fd;
    va_list args;
    va_start(args, request);
    if (request == EVIOCGRAB) {
        if (va_arg(args, int)) grabs++;
    } else {
        uint8_t *bits = va_arg(args, uint8_t *);
        memset(bits, 0, _IOC_SIZE(request));
        if (_IOC_NR(request) == 0x20 + EV_KEY) bits[BTN_MOUSE / 8] |= 1 << (BTN_MOUSE % 8);
        else {
            assert(_IOC_NR(request) == 0x20 + EV_REL);
            bits[0] = (1 << REL_X) | (1 << REL_Y);
        }
    }
    va_end(args);
    return 0;
}

static void connect_mouse(int number, int delta) {
    node = number;
    assert(pipe(channel) == 0);
    struct input_event event = {.type = EV_REL, .code = REL_X, .value = delta};
    assert(write(channel[1], &event, sizeof(event)) == sizeof(event));
}

static void on_motion(const evmouse_event_t *event, void *unused) {
    (void)unused;
    assert(event->type == SDL_MOUSEMOTION);
    if (++motions == 1) {
        assert(event->motion.xrel == 7);
        assert(grabs == 1);
        evmouse_set_grab(mouse, grab_replacement);
        close(channel[1]);
        close(channel[0]);
        connect_mouse(replacement_node, -14);
    } else {
        assert(event->motion.xrel == -14);
        evmouse_interrupt(mouse);
    }
}

static void test_reconnect(int replacement, SDL_bool grab) {
    node = -1;
    replacement_node = replacement;
    grab_replacement = grab;
    opens = grabs = motions = 0;
    mouse = evmouse_open_default();
    assert(mouse && opens == 0);
    evmouse_set_grab(mouse, SDL_TRUE);
    connect_mouse(16, 7);
    evmouse_listen(mouse, on_motion, NULL);
    assert(motions == 2 && opens == 2);
    assert(grabs == (grab ? 2 : 1));
    discover_mice(mouse);
    assert(opens == 2); /* Steady-state discovery must not reopen any node. */
    evmouse_close(mouse);
    close(channel[1]);
    close(channel[0]);
}

int main(void) {
    if (SDL_Init(SDL_INIT_TIMER) != 0) return 1;
    test_reconnect(17, SDL_TRUE);
    test_reconnect(16, SDL_FALSE);
    SDL_Quit();
    return 0;
}
