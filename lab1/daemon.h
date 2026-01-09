#ifndef DAEMON_H
#define DAEMON_H

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class Daemon {
public:
    static Daemon& getInstance();

    Daemon(const Daemon&) = delete;
    void operator=(const Daemon&) = delete;

    void init(const std::string& path);
    void run();
    void handleSignal(int signum);

private:
    Daemon();

    std::string configPath;
    std::string sourceDir;
    std::string logDir;
    int interval;

    volatile bool isRunning = true;
    volatile bool isReload = false;

    void setupSystem();
    void daemonize();
    void checkAndKillOldProcess();
    void writePidFile();
    void loadConfig();
    void doWork();
    void cleanup();

    static const std::string PID_FILE;
};

#endif