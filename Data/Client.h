#ifndef CLIENT_H
#define CLIENT_H

#include "ProcessData.h"
#include "View/MainWindow.h"

#include <vector>
#include <atomic>

#include <boost/thread.hpp>
static short COMMAND_RUN = 0;
static short COMMAND_PAUSE = 1;
static short COMMAND_EXIT = 2;

class Client
{
  public:
    Client(unsigned int update_intervall);

    virtual ~Client();

    virtual void run() = 0;

    /**
     * @return last data from buffer vector
     */
    void processData();

    QString getStatus();
    virtual QString getTransferInfo() = 0;

    __inline gpu_info* getGpuIno() {return &gpu_info_;}

    __inline int getTau() {return process_data_.getTau();}

    void stopRun();

    void setPause(bool p);

    void changeState(short state);

  protected:
    data_segment_buffer ds_buffer_;
    gpu_info gpu_info_;
    ProcessData process_data_;

    boost::shared_mutex state_lock_;

    QString status_;
    QString transfer_info_;
    std::atomic<bool> run_;
    short state_;

    virtual void sendCommand(short command) = 0;
};

#endif // CLIENT_H
