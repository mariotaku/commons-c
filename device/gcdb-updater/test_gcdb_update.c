#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "logging.h"
#include "gamecontrollerdb_updater.h"

int main() {
    commons_logging_init("test");
    commons_gcdb_updater_t updater = {
            .platform = "Linux",
            .path = "gdcb_test.txt",
    };

    unlink(updater.path);

    commons_gcdb_updater_init(&updater);
    assert(commons_gcdb_updater_update(&updater));
    assert(!commons_gcdb_updater_update(&updater));
    while (access(updater.path, F_OK) != 0 && errno == ENOENT) {
        sleep(1);
    }
    assert(commons_gcdb_updater_update(&updater));
    commons_gcdb_updater_deinit(&updater);
    commons_logging_deinit();
}