#include "Daemon.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <thread>
#include <csignal>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>

const std::string Daemon::PID_FILE = "/tmp/my_daemon_task.pid";

Daemon& Daemon::getInstance() {
    static Daemon instance;
    return instance;
}

Daemon::Daemon() : interval(10) {}

void Daemon::init(const std::string& path) {
    try {
        configPath = fs::absolute(path).string();
    } catch (...) {
        std::cerr << "Error resolving config path" << std::endl;
        exit(EXIT_FAILURE);
    }

    checkAndKillOldProcess();
    daemonize();
    setupSystem();
    loadConfig();
    syslog(LOG_INFO, "Start success...");
}

void Daemon::daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);
    chdir("/");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    writePidFile();
}

void Daemon::loadConfig() {
    std::ifstream cfg(configPath);
    if (!cfg.is_open()) {
        syslog(LOG_ERR, "Failed to open config file: %s", configPath.c_str());
        return;
    }

    std::string line;
    while (std::getline(cfg, line)) {
        // Очистка от \r для Windows-файлов
        if (!line.empty() && line.back() == '\r') line.pop_back();

        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            if (key == "sourceDir") sourceDir = value;
            else if (key == "logDir") logDir = value;
            else if (key == "interval") interval = std::stoi(value);
        }
    }
    syslog(LOG_INFO, "Config loaded. Interval: %d", interval);
}

void Daemon::doWork() {
    if (!fs::exists(sourceDir) || !fs::exists(logDir)) {
        syslog(LOG_ERR, "Directories configured do not exist.");
        return;
    }

    std::ofstream logStream(logDir + "/hist.log", std::ios_base::app);
    if (!logStream.is_open()) return;

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    logStream << "Time: " << std::ctime(&now) << "Contents of " << sourceDir << ":\n";

    for (const auto& entry : fs::directory_iterator(sourceDir)) {
        logStream << " - " << entry.path().filename().string() 
                  << (entry.is_directory() ? " [DIR]" : "") << "\n";
    }
    logStream << "-------------------\n";
}

void Daemon::run() {
    while (isRunning) {
        if (isReload) {
            loadConfig();
            isReload = false;
        }
        doWork();
        sleep(interval);
    }
    cleanup();
}

void Daemon::handleSignal(int signum) {
    if (signum == SIGHUP) isReload = true;
    else if (signum == SIGTERM) isRunning = false;
}

void Daemon::setupSystem() {
    openlog("SystemLog", LOG_PID, LOG_DAEMON);
}

void Daemon::checkAndKillOldProcess() {
    std::ifstream pidIn(PID_FILE);
    if (pidIn.is_open()) {
        int oldPid;
        pidIn >> oldPid;
        if (fs::exists("/proc/" + std::to_string(oldPid))) {
            kill(oldPid, SIGTERM);
            sleep(1);
        }
    }
}

void Daemon::writePidFile() {
    std::ofstream pidOut(PID_FILE);
    if (pidOut.is_open()) pidOut << getpid();
}

void Daemon::cleanup() {
    syslog(LOG_INFO, "Daemon stopping.");
    fs::remove(PID_FILE);
    closelog();
    exit(EXIT_SUCCESS);
}