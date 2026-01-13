#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include <cstring>
#include <memory>
#include <sys/wait.h>

#include "connection.h"

std::atomic<bool> running(true);

void NetworkReader(Conn* conn, bool is_host_role) {
    char buf[BUF_SIZE];
    while (running) {
        memset(buf, 0, BUF_SIZE);
        if (conn->Read(buf, BUF_SIZE)) {
            std::cout << "\n" << (is_host_role ? "[Client says]: " : "[Host says]: ") 
                      << buf << std::endl;
            std::cout << "> " << std::flush;
        } else {
            running = false;
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

    // Ребенок
    if (pid == 0) {
        conn->OnFork(true); 
        
        std::thread reader(NetworkReader, conn.get(), false);
        reader.detach();

        std::string msg;
        while (running && std::getline(std::cin, msg)) {
            conn->Write((void*)msg.c_str(), msg.length() + 1);
        }
    } 
    else {
        conn->OnFork(false); 
        std::cout << "[Host] Child process PID: " << pid << std::endl;

        std::thread reader(NetworkReader, conn.get(), true);
        reader.detach();

        std::cout << "> " << std::flush;
        std::string msg;
        while (running && std::getline(std::cin, msg)) {
            if (msg == "exit") {
                kill(pid, SIGKILL); 
                break;
            }
            conn->Write((void*)msg.c_str(), msg.length() + 1);
        }
    }

    return 0;
}