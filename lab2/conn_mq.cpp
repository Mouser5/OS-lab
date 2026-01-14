#include "connection.h"
#include <mqueue.h>

class ConnMQ : public Conn {
private:
    mqd_t mq_to_client;
    mqd_t mq_to_host;
    std::string name_to_client;
    std::string name_to_host;

public:
    ConnMQ(int id, bool create) : Conn(id, create) {
        name_to_client = "/mq_h2c_" + std::to_string(id);
        name_to_host = "/mq_c2h_" + std::to_string(id);

        struct mq_attr attr;
        attr.mq_flags = 0;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = BUF_SIZE;
        attr.mq_curmsgs = 0;
        
        int flags = O_RDWR | O_CREAT;
        mq_to_client = mq_open(name_to_client.c_str(), flags, 0644, &attr);
        mq_to_host = mq_open(name_to_host.c_str(), flags, 0644, &attr);

        if (mq_to_client == (mqd_t)-1 || mq_to_host == (mqd_t)-1) {
            perror("mq_open failed");
            exit(1);
        }
        Log("MQ queues opened.");
    }

    ~ConnMQ() {
        mq_close(mq_to_client);
        mq_close(mq_to_host);
        if (is_host) { 
            mq_unlink(name_to_client.c_str());
            mq_unlink(name_to_host.c_str());
            Log("MQ queues unlinked.");
        }
    }

    bool Read(void *buf, size_t count) override {
        sem_t* wait_sem = is_host ? &sync_obj->sem_client_has_data : &sync_obj->sem_host_has_data;
        mqd_t read_mq   = is_host ? mq_to_host : mq_to_client;

        if (!WaitSemaphore(wait_sem)) return false; 

        ssize_t bytes = mq_receive(read_mq, (char*)buf, BUF_SIZE, NULL);
        return (bytes >= 0);
    }

    bool Write(void *buf, size_t count) override {
        sem_t* signal_sem = is_host ? &sync_obj->sem_host_has_data : &sync_obj->sem_client_has_data;
        mqd_t write_mq    = is_host ? mq_to_client : mq_to_host;

        if (mq_send(write_mq, (const char*)buf, count, 0) == -1) {
            perror("mq_send error");
            return false;
        }
        
        PostSemaphore(signal_sem);
        return true;
    }
};

Conn* CreateConnection(int id, bool create) { return new ConnMQ(id, create); }