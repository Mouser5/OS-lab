#include "connection.h"
#include <sys/socket.h>

class ConnSock : public Conn {
private:
    int sv[2]; 
    int my_sock;

public:
    ConnSock(int id, bool create) : Conn(id, create) {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
            perror("socketpair failed");
            exit(1);
        }
        Log("Socketpair created.");
    }

    void OnFork(bool am_i_child) override {
        Conn::OnFork(am_i_child);
        if (am_i_child) {
            close(sv[0]); 
            my_sock = sv[1];
        } else {
            close(sv[1]); 
            my_sock = sv[0];
        }
    }

    ~ConnSock() {
        close(my_sock);
    }

    bool Read(void *buf, size_t count) override {
        sem_t* wait_sem = is_host ? &sync_obj->sem_client_has_data : &sync_obj->sem_host_has_data;
        
        if (!WaitSemaphore(wait_sem)) return false;

        ssize_t bytes = recv(my_sock, buf, count, 0);
        return (bytes > 0);
    }

    bool Write(void *buf, size_t count) override {
        sem_t* signal_sem = is_host ? &sync_obj->sem_host_has_data : &sync_obj->sem_client_has_data;

        if (send(my_sock, buf, count, 0) == -1) {
            perror("socket send error");
            return false;
        }
        PostSemaphore(signal_sem);
        return true;
    }
};

Conn* CreateConnection(int id, bool create) { return new ConnSock(id, create); }