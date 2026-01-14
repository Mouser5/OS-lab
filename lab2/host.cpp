#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include <cstring>
#include <memory>
#include <sys/wait.h>
#include <mutex>
#include <condition_variable>

#include "connection.h"

std::atomic<bool> running{true};
std::atomic<bool> stdin_allowed{false};
std::mutex stdin_mtx;
std::condition_variable stdin_cv;

void NetworkReader(Conn* conn, bool is_host_role) {
    char buf[BUF_SIZE];
    while (running) {
        memset(buf, 0, BUF_SIZE);
        if (conn->Read(buf, BUF_SIZE)) {
            std::cout << "\n" << (is_host_role ? "[Client says]: " : "[Host says]: ")
                      << buf << std::endl;

            stdin_allowed.store(true);
            stdin_cv.notify_one();

            std::cout << "> " << std::flush;
        } else {
            running = false;
            stdin_cv.notify_all();
            kill(getpid(), SIGKILL);
        }
    }
}

int main(int argc, char* argv[]) {
    std::unique_ptr<Conn> conn(CreateConnection(1, true));

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    // РЕБЁНОК
    if (pid == 0) {
        conn->OnFork(true);

        stdin_allowed.store(false);

        std::thread reader(NetworkReader, conn.get(), false);
        reader.detach();

        std::string msg;
        while (running) {
            std::unique_lock<std::mutex> lk(stdin_mtx);
            stdin_cv.wait(lk, [](){ return stdin_allowed.load() || !running; });

            if (!running) break;

            if (!std::getline(std::cin, msg)) {
                running = false;
                stdin_cv.notify_all();
                break;
            }

            conn->Write((void*)msg.c_str(), msg.length() + 1);
            stdin_allowed.store(false);
        }
    }
    else {
        conn->OnFork(false);
        std::cout << "[Host] Child process PID: " << pid << std::endl;

        stdin_allowed.store(true);

        std::thread reader(NetworkReader, conn.get(), true);
        reader.detach();

        std::cout << "> " << std::flush;
        std::string msg;
        while (running) {
            std::unique_lock<std::mutex> lk(stdin_mtx);
            stdin_cv.wait(lk, [](){ return stdin_allowed.load() || !running; });

            if (!running) break;

            if (!std::getline(std::cin, msg)) {
                running=false;
                stdin_cv.notify_all();
                break;
            }

            if (msg == "exit") {
                kill(pid, SIGKILL);
                running=false;
                stdin_cv.notify_all();
                break;
            }

            conn->Write((void*)msg.c_str(), msg.length() + 1);
            stdin_allowed.store(false);
        }
    }

    return 0;
}
