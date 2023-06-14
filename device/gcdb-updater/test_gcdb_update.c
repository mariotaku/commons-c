#include <unistd.h>
#include <assert.h>

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
    commons_gcdb_updater_deinit(&updater);
    commons_logging_deinit();
}