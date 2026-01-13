#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <atomic>

const size_t BUF_SIZE = 1024;

struct SharedSync {
    sem_t sem_host_has_data;   
    sem_t sem_client_has_data; 
};

class Conn {
protected:
    int id;
    bool is_host;
    SharedSync* sync_obj; 

    void Log(const std::string& msg) {
        time_t now = time(0);
        char* dt = ctime(&now);
        dt[strlen(dt) - 1] = 0; 
        std::cerr << "[" << dt << "] [" << (is_host ? "HOST" : "CLIENT") << "] " << msg << std::endl;
    }

    void InitSharedSync() {
        sync_obj = (SharedSync*)mmap(NULL, sizeof(SharedSync), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (sync_obj == MAP_FAILED) {
            perror("mmap failed");
            exit(1);
        }
        
        sem_init(&sync_obj->sem_host_has_data, 1, 0);
        sem_init(&sync_obj->sem_client_has_data, 1, 0);
    }

    bool WaitSemaphore(sem_t* sem) {
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 10; 

        if (sem_timedwait(sem, &timeout) == -1) {
            Log("TIMEOUT.");
            return false; 
        }
        return true;
    }

    void PostSemaphore(sem_t* sem) {
        sem_post(sem);
    }

public:
    Conn(int id, bool create) : id(id), is_host(create), sync_obj(nullptr) {
        InitSharedSync();
    }

    virtual ~Conn() {
        if (is_host && sync_obj) {
            sem_destroy(&sync_obj->sem_host_has_data);
            sem_destroy(&sync_obj->sem_client_has_data);
            munmap(sync_obj, sizeof(SharedSync));
        }
    }

    virtual void OnFork(bool am_i_child) {
        is_host = !am_i_child;
    }

    virtual bool Read(void *buf, size_t count) = 0;
    virtual bool Write(void *buf, size_t count) = 0;
};

extern Conn* CreateConnection(int id, bool create);