#include "connection.h"
#include <sys/stat.h>

class ConnFIFO : public Conn {
private:
    std::string fifo_h2c_name;
    std::string fifo_c2h_name;
    int fd_read;
    int fd_write;

public:
    ConnFIFO(int id, bool create) : Conn(id, create) {
        fifo_h2c_name = "/tmp/fifo_h2c_" + std::to_string(id);
        fifo_c2h_name = "/tmp/fifo_c2h_" + std::to_string(id);

        if (create) { 
            mkfifo(fifo_h2c_name.c_str(), 0666);
            mkfifo(fifo_c2h_name.c_str(), 0666);
        }
        Log("FIFO files initialized.");
    }

    void OnFork(bool am_i_child) override {
        Conn::OnFork(am_i_child);

        if (is_host) {
            fd_write = open(fifo_h2c_name.c_str(), O_WRONLY); 
            fd_read = open(fifo_c2h_name.c_str(), O_RDONLY);
        } else {
            fd_read = open(fifo_h2c_name.c_str(), O_RDONLY);
            fd_write = open(fifo_c2h_name.c_str(), O_WRONLY);
        }

        if (fd_read == -1 || fd_write == -1) {
            perror("FIFO open failed");
            exit(1);
        }
        Log("FIFO opened successfully.");
    }

    ~ConnFIFO() {
        close(fd_read);
        close(fd_write);
        if (is_host) {
            unlink(fifo_h2c_name.c_str());
            unlink(fifo_c2h_name.c_str());
            Log("FIFO unlinked.");
        }
    }

    bool Read(void *buf, size_t count) override {
        sem_t* wait_sem = is_host ? &sync_obj->sem_client_has_data : &sync_obj->sem_host_has_data;
        
        if (!WaitSemaphore(wait_sem)) return false;

        ssize_t bytes = read(fd_read, buf, count);
        return (bytes > 0);
    }

    bool Write(void *buf, size_t count) override {
        sem_t* signal_sem = is_host ? &sync_obj->sem_host_has_data : &sync_obj->sem_client_has_data;

        if (write(fd_write, buf, count) == -1) {
            perror("FIFO write error");
            return false;
        }
        PostSemaphore(signal_sem);
        return true;
    }
};

Conn* CreateConnection(int id, bool create) { return new ConnFIFO(id, create); }