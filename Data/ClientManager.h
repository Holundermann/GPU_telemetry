#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <string>

#include <boost/thread.hpp>

#include "Data/Network/TcpClient.h"

// port and ip where the network client should connect to
//const std::string ip_std("81.16.104.231");
//const int port_std(42420);
//const int update_intervall_std(1000);
const std::string ip_std("10.0.0.90");
const int port_std(4242);
const int update_intervall_std(125);

class ClientManager
{
  private:

    static boost::mutex instance_lock_;

    Client* client_;

    boost::shared_mutex client_lock_;

    boost::asio::io_service* io_service_;

    boost::thread* service_thread_;

    ClientManager();

    ClientManager(const ClientManager&);

  public:
    ~ClientManager();

    static void createInstance();

    static ClientManager* instance_;

    void tcpConnect(std::string ip = ip_std, int port = port_std, int update_intervall = update_intervall_std);

    void run_service();

    QString getStatus();

    QString getTransferInfo();

    gpu_info* getGpuInfo();

    void createIOThread();

    void stdConnect();

    void setPause(bool p);

    int getUpdateIntervall();
};

#endif // CLIENTMANAGER_H
