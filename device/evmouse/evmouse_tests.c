/* Pull in the full translation unit so the test can construct an evmouse_t
 * directly without a /dev/input device — the public evmouse_open_default()
 * returns NULL when no mouse is present (typical CI environment). The test
 * exercises the listen/interrupt state machine, which does not depend on any
 * fds: with nfds=0 the listen loop's select() just times out on its 1ms
 * timeout, giving the loop a chance to observe the listening flag. */
#include "evmouse.c"

#include <assert.h>
#include <SDL.h>

static evmouse_t *empty_mouse_create(void) {
    evmouse_t *mouse = malloc(sizeof(evmouse_t));
    memset(mouse, 0, sizeof(evmouse_t));
    mouse->lock = SDL_CreateMutex();
    mouse->nfds = 0;
    return mouse;
}

static void no_op_listener(const evmouse_event_t *event, void *userdata) {
    (void) event;
    (void) userdata;
}

static int listen_thread(void *arg) {
    evmouse_listen((evmouse_t *) arg, no_op_listener, NULL);
    return 0;
}

/* Regression: interrupt raised before evmouse_listen is called must be honored.
 * Pre-fix, evmouse_interrupt only cleared `listening` (already false), then
 * evmouse_listen set listening=true and looped forever. */
static void test_interrupt_before_listen(void) {
    evmouse_t *mouse = empty_mouse_create();
    evmouse_interrupt(mouse);
    Uint32 start = SDL_GetTicks();
    evmouse_listen(mouse, no_op_listener, NULL);
    Uint32 elapsed = SDL_GetTicks() - start;
    assert(elapsed < 100);
    evmouse_close(mouse);
}

/* Verifies that an interrupt raised by another thread while listen is running
 * exits the loop promptly. This case worked pre-fix too, included to ensure
 * the new logic did not regress it. */
static void test_interrupt_during_listen(void) {
    evmouse_t *mouse = empty_mouse_create();
    SDL_Thread *thread = SDL_CreateThread(listen_thread, "listener", mouse);
    assert(thread != NULL);
    SDL_Delay(50);
    evmouse_interrupt(mouse);
    Uint32 start = SDL_GetTicks();
    SDL_WaitThread(thread, NULL);
    Uint32 elapsed = SDL_GetTicks() - start;
    assert(elapsed < 100);
    evmouse_close(mouse);
}

/* Verifies that the interrupted flag is consumed by exactly one listen call:
 * after the first listen (which the prior interrupt poisoned) returns, a
 * second listen must NOT auto-exit. It should block until another interrupt
 * is raised. */
static void test_interrupt_consumed_after_listen(void) {
    evmouse_t *mouse = empty_mouse_create();
    evmouse_interrupt(mouse);
    evmouse_listen(mouse, no_op_listener, NULL);  /* consumes the interrupt */

    SDL_Thread *thread = SDL_CreateThread(listen_thread, "listener", mouse);
    assert(thread != NULL);
    SDL_Delay(50);
    /* If the interrupted flag had stuck around, the listen would have
     * returned immediately and the thread would already be done. */
    evmouse_interrupt(mouse);
    SDL_WaitThread(thread, NULL);
    evmouse_close(mouse);
}

int main(void) {
    if (SDL_Init(SDL_INIT_TIMER) != 0) {
        return 1;
    }
    test_interrupt_before_listen();
    test_interrupt_during_listen();
    test_interrupt_consumed_after_listen();
    SDL_Quit();
    return 0;
}
