#include "evmouse.h"

#include <SDL_timer.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <linux/input.h>

#include "logging.h"

typedef struct dev_fd_t {
    int fd;
    unsigned int event_number;
    SDL_bool grab;
} dev_fd_t;

struct evmouse_t {
    SDL_mutex *lock;
    /* `listening` is set true on entry to evmouse_listen and false when the
     * loop should exit. `interrupted` records that an interrupt was requested:
     * unlike `listening`, it is observable BEFORE evmouse_listen has started,
     * so an interrupt that races ahead of the listener is not lost. The flag
     * is consumed (cleared) by the next evmouse_listen call, on entry if
     * interrupted-before-listen or at exit if interrupted-during-listen. */
    SDL_bool listening;
    SDL_bool interrupted;
    SDL_bool requested_grab;
    dev_fd_t fds[EVMOUSE_MAX_FDS];
    int nfds;
};

static SDL_bool discover_mice(evmouse_t *mouse);
static void discover_from_directory(evmouse_t *mouse);
static SDL_bool is_mouse(int fd);

static inline SDL_bool has_bit(const uint8_t *bits, uint32_t bit);

static void dispatch_motion(const struct input_event *raw, evmouse_listener_t listener, void *userdata);

static void dispatch_wheel(const struct input_event *raw, evmouse_listener_t listener, void *userdata);

static void dispatch_button(const struct input_event *raw, evmouse_listener_t listener, void *userdata);


evmouse_t *evmouse_open_default() {
    evmouse_t *mouse = calloc(1, sizeof(*mouse));
    if (mouse == NULL) return NULL;
    mouse->lock = SDL_CreateMutex();
    if (mouse->lock == NULL) {
        free(mouse);
        return NULL;
    }
    if (!discover_mice(mouse)) {
        /* Some environments expose evdev without procfs. Retain startup discovery there. */
        discover_from_directory(mouse);
        if (mouse->nfds == 0) {
            evmouse_close(mouse);
            return NULL;
        }
    }
    return mouse;
}

void evmouse_close(evmouse_t *mouse) {
    assert(mouse != NULL);
    int ret = SDL_LockMutex(mouse->lock);
    assert(ret == 0);
    for (int i = 0; i < mouse->nfds; i++) {
        if (mouse->fds[i].grab) {
            if (ioctl(mouse->fds[i].fd, EVIOCGRAB, 0) < 0) {
                commons_log_warn("EvMouse", "Failed to ungrab fd %d: %d (%s)", mouse->fds[i].fd, errno,
                                 strerror(errno));
            }
        }
        close(mouse->fds[i].fd);
    }
    SDL_UnlockMutex(mouse->lock);
    SDL_DestroyMutex(mouse->lock);
    free(mouse);
}

void evmouse_listen(evmouse_t *mouse, evmouse_listener_t listener, void *userdata) {
    if (SDL_LockMutex(mouse->lock) != 0) {
        return;
    }
    if (mouse->listening) {
        SDL_UnlockMutex(mouse->lock);
        return;
    }
    if (mouse->interrupted) {
        /* An interrupt was requested before we could start listening. Consume
         * the signal and return without entering the loop, so the caller is
         * not stuck waiting for an event that will never come. */
        mouse->interrupted = SDL_FALSE;
        SDL_UnlockMutex(mouse->lock);
        return;
    }
    mouse->listening = SDL_TRUE;
    SDL_UnlockMutex(mouse->lock);
    Uint32 last_scan = SDL_GetTicks();
    while (evmouse_is_interrupted(mouse)) {
        if ((Uint32)(SDL_GetTicks() - last_scan) >= 250) {
            SDL_LockMutex(mouse->lock);
            discover_mice(mouse);
            SDL_UnlockMutex(mouse->lock);
            last_scan = SDL_GetTicks();
        }
        fd_set fds;
        FD_ZERO(&fds);
        for (int i = 0; i < mouse->nfds; i++) {
            FD_SET(mouse->fds[i].fd, &fds);
        }
        struct timeval timeout = {.tv_sec = 0, .tv_usec = 1000};
        if (select(FD_SETSIZE, &fds, NULL, NULL, &timeout) <= 0) {
            continue;
        }

        for (int i = 0; i < mouse->nfds; i++) {
            int fd = mouse->fds[i].fd;
            if (!FD_ISSET(fd, &fds)) {
                continue;
            }
            struct input_event raw_ev;
            ssize_t count = read(fd, &raw_ev, sizeof(raw_ev));
            if (count != (ssize_t)sizeof(raw_ev)) {
                if (count == 0 || (count < 0 && (errno == ENODEV || errno == EIO))) {
                    SDL_LockMutex(mouse->lock);
                    close(fd);
                    mouse->fds[i] = mouse->fds[--mouse->nfds];
                    SDL_UnlockMutex(mouse->lock);
                    i--; /* Check the descriptor moved into this slot. */
                }
                continue;
            }
            switch (raw_ev.type) {
                case EV_REL: {
                    switch (raw_ev.code) {
                        case REL_X:
                        case REL_Y:
                            dispatch_motion(&raw_ev, listener, userdata);
                            break;
                        case REL_HWHEEL:
                        case REL_WHEEL:
                            dispatch_wheel(&raw_ev, listener, userdata);
                            break;
                    }
                    break;
                }
                case EV_KEY: {
                    if (raw_ev.code >= BTN_LEFT && raw_ev.code <= BTN_TASK) {
                        dispatch_button(&raw_ev, listener, userdata);
                    }
                    break;
                }
            }
        }
    }
    if (SDL_LockMutex(mouse->lock) == 0) {
        mouse->interrupted = SDL_FALSE;
        SDL_UnlockMutex(mouse->lock);
    }
}

void evmouse_set_grab(evmouse_t *mouse, SDL_bool grab) {
    if (SDL_LockMutex(mouse->lock) != 0) {
        return;
    }
    mouse->requested_grab = grab;
    for (int i = 0; i < mouse->nfds; i++) {
        if (mouse->fds[i].grab != grab) {
            if (ioctl(mouse->fds[i].fd, EVIOCGRAB, grab ? 1 : 0) < 0) {
                commons_log_warn("EvMouse", "Failed to set grab=%d on fd %d: %d (%s)", grab, mouse->fds[i].fd,
                                 errno, strerror(errno));
            } else {
                mouse->fds[i].grab = grab;
            }
        }
    }
    SDL_UnlockMutex(mouse->lock);
}

void evmouse_interrupt(evmouse_t *mouse) {
    if (SDL_LockMutex(mouse->lock) != 0) {
        return;
    }
    /* Set both flags: listening breaks the in-flight loop, interrupted records
     * the request so an evmouse_listen call that has not yet entered the loop
     * (or has not been called at all) honors it. */
    mouse->interrupted = SDL_TRUE;
    mouse->listening = SDL_FALSE;
    SDL_UnlockMutex(mouse->lock);
}

SDL_bool evmouse_is_interrupted(evmouse_t *mouse) {
    if (SDL_LockMutex(mouse->lock) != 0) {
        return SDL_FALSE;
    }
    SDL_bool interrupted;
    interrupted = mouse->listening;
    SDL_UnlockMutex(mouse->lock);
    return interrupted;
}

static void open_mouse(evmouse_t *mouse, unsigned int number) {
    if (mouse->nfds == EVMOUSE_MAX_FDS) return;
    for (int i = 0; i < mouse->nfds; i++) {
        if (mouse->fds[i].event_number == number) return;
    }

    char path[32];
    snprintf(path, sizeof(path), "/dev/input/event%u", number);
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) return;
    if (fd >= FD_SETSIZE || !is_mouse(fd)) {
        close(fd);
        return;
    }
    dev_fd_t device = {.fd = fd, .event_number = number};
    if (mouse->requested_grab) {
        if (ioctl(fd, EVIOCGRAB, 1) == 0) device.grab = SDL_TRUE;
        else commons_log_warn("EvMouse", "Failed to grab %s: %s", path, strerror(errno));
    }
    mouse->fds[mouse->nfds++] = device;
}

static SDL_bool event_number(const char *name, unsigned int *number) {
    char extra;
    return sscanf(name, "event%u%c", number, &extra) == 1;
}

static void discover_from_directory(evmouse_t *mouse) {
    DIR *dir = opendir("/dev/input");
    if (dir == NULL) return;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        unsigned int number;
        if (event_number(entry->d_name, &number)) open_mouse(mouse, number);
    }
    closedir(dir);
}

static SDL_bool discover_mice(evmouse_t *mouse) {
    /* webOS pre-creates evdev nodes. Opening every slot, even with O_NONBLOCK,
     * can stall the input thread for hundreds of milliseconds. */
    FILE *inventory = fopen("/proc/bus/input/devices", "r");
    if (inventory == NULL) return SDL_FALSE;
    char line[1024];
    unsigned int number = 0;
    SDL_bool has_event = SDL_FALSE, relative_xy = SDL_FALSE;
    while (fgets(line, sizeof(line), inventory)) {
        if (line[0] == '\n') {
            if (has_event && relative_xy) open_mouse(mouse, number);
            has_event = relative_xy = SDL_FALSE;
        } else if (strncmp(line, "H: Handlers=", 12) == 0) {
            char *save;
            for (char *token = strtok_r(line + 12, " \t\r\n", &save); token;
                 token = strtok_r(NULL, " \t\r\n", &save)) {
                if (event_number(token, &number)) {
                    has_event = SDL_TRUE;
                    break;
                }
            }
        } else if (strncmp(line, "B: REL=", 7) == 0) {
            const char *last = strrchr(line + 7, ' ');
            unsigned long bits = strtoul(last ? last + 1 : line + 7, NULL, 16);
            relative_xy = (bits & 3) == 3; /* REL_X and REL_Y */
        }
    }
    if (has_event && relative_xy) open_mouse(mouse, number);
    fclose(inventory);
    return SDL_TRUE;
}

static SDL_bool is_mouse(int fd) {
    uint8_t keycaps[(KEY_MAX / 8) + 1] = {0};
    uint8_t relcaps[(REL_MAX / 8) + 1] = {0};

    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keycaps)), keycaps) < 0) {
        return SDL_FALSE;
    }

    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relcaps)), relcaps) < 0) {
        return SDL_FALSE;
    }
    if (has_bit(keycaps, BTN_MOUSE) && has_bit(relcaps, REL_X) && has_bit(relcaps, REL_Y)) {
        return SDL_TRUE;
    }

    return SDL_FALSE;
}

static inline SDL_bool has_bit(const uint8_t *bits, uint32_t bit) {
    return (bits[bit / 8] & 1 << (bit % 8)) != 0;
}

static void dispatch_motion(const struct input_event *raw, evmouse_listener_t listener, void *userdata) {
    evmouse_event_t event = {.motion = {.type=SDL_MOUSEMOTION}};
    switch (raw->code) {
        case REL_X:
            event.motion.xrel = raw->value;
            break;
        case REL_Y:
            event.motion.yrel = raw->value;
            break;
    }
    listener(&event, userdata);
}

static void dispatch_wheel(const struct input_event *raw, evmouse_listener_t listener, void *userdata) {
    evmouse_event_t event = {.wheel = {.type=SDL_MOUSEWHEEL}};
    switch (raw->code) {
        case REL_WHEEL:
            event.wheel.y = raw->value;
            break;
        case REL_HWHEEL:
            event.wheel.x = raw->value;
            break;
    }
    listener(&event, userdata);
}

static void dispatch_button(const struct input_event *raw, evmouse_listener_t listener, void *userdata) {
    evmouse_event_t event = {
        .button = {
            .type=raw->value ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP,
            .state = raw->value ? SDL_PRESSED : SDL_RELEASED
        }
    };
    switch (raw->code) {
        case BTN_LEFT:
            event.button.button = SDL_BUTTON_LEFT;
            break;
        case BTN_RIGHT:
            event.button.button = SDL_BUTTON_RIGHT;
            break;
        case BTN_MIDDLE:
            event.button.button = SDL_BUTTON_MIDDLE;
            break;
        case BTN_SIDE:
            event.button.button = SDL_BUTTON_X1;
            break;
        case BTN_EXTRA:
            event.button.button = SDL_BUTTON_X2;
            break;
        case BTN_FORWARD:
            event.button.button = SDL_BUTTON_X2 + 1;
            break;
        case BTN_BACK:
            event.button.button = SDL_BUTTON_X2 + 2;
            break;
        case BTN_TASK:
            event.button.button = SDL_BUTTON_X2 + 3;
            break;
        default:
            commons_log_warn("EvMouse", "Unhandled button event: %d", raw->code);
            return;
    }
    listener(&event, userdata);
}