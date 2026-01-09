#include "Daemon.h"
#include <csignal>

void globalSignalHandler(int signum) {
    Daemon::getInstance().handleSignal(signum);
}

int main() {
    signal(SIGHUP, globalSignalHandler);
    signal(SIGTERM, globalSignalHandler);

    Daemon::getInstance().init("config.cfg");
    Daemon::getInstance().run();

    return 0;
}