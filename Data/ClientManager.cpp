#include <boost/bind.hpp>

#include "ClientManager.h"

#include "Data/Network/TcpClient.h"

ClientManager* ClientManager::instance_ = 0;
boost::mutex ClientManager::instance_lock_;

ClientManager::ClientManager() : client_(0), io_service_(0), service_thread_(0)
{
}

ClientManager::~ClientManager()
{
  if (client_) {
    boost::unique_lock<boost::shared_mutex> client_lck(client_lock_);
    client_->stopRun();
    service_thread_->join();
    delete client_;
    std::cout << "ClientManager::~ClientManager()" << std::endl;
    client_ = 0;
  }
  if (io_service_) {
    delete io_service_;
    io_service_ = 0;
  }
  if (service_thread_) {
    delete service_thread_;
    service_thread_ = 0;
  }
  {
    boost::unique_lock<boost::mutex> instance_lck(instance_lock_);
    instance_ = 0;
  }
}

void ClientManager::createInstance()
{
  boost::unique_lock<boost::mutex> instance_lck(instance_lock_);
  if(!instance_) {
    instance_ = new ClientManager();
  }
}

void ClientManager::tcpConnect(std::string ip, int port, int update_intervall)
{
  if (client_) {
    boost::unique_lock<boost::shared_mutex> client_lck(client_lock_);
    std::cout << "[ClientManager::tcpConnect] stopping service thread and delete client" << std::endl;
    client_->stopRun();
    service_thread_->join();
    delete client_;
    client_ = 0;
  }
  if (service_thread_) {
    std::cout << "[ClientManager::tcpConnect] deleting service_thread_ and io_service_" << std::endl;
    delete io_service_;
    delete service_thread_;
    io_service_ = 0;
    service_thread_ = 0;
  }
  io_service_ = new boost::asio::io_service;
  client_ = new TcpClient(io_service_, ip, port, update_intervall);
}

void ClientManager::run_service()
{
  try {
    std::cout << "[ClientManager::run_service]" << std::endl;
    if (io_service_) {
      io_service_->run();
    }
    if (client_) {
      client_->run();
    }
  } catch (std::exception &e) {
    std::cerr << "[ClientManager::run_service] an critical error occurred, in - " << e.what() << std::endl;
  }
}

QString ClientManager::getStatus()
{
  boost::shared_lock<boost::shared_mutex> client_lck(client_lock_);
  if (client_)
    return client_->getStatus();
  else
    return QString("not Connected");
}

QString ClientManager::getTransferInfo()
{
  boost::shared_lock<boost::shared_mutex> client_lck(client_lock_);
  if (client_)
    return client_->getTransferInfo();
  else
    return QString("--- kBytes/second");
}

gpu_info* ClientManager::getGpuInfo()
{
  boost::shared_lock<boost::shared_mutex> client_lck(client_lock_);
  if (client_) {
    return client_->getGpuIno()->device_name ? client_->getGpuIno() : 0;
  }
  return 0;
}

void ClientManager::createIOThread()
{
  if(!service_thread_) {
    service_thread_ = new boost::thread(boost::bind(&ClientManager::run_service, this));
  }
}

void ClientManager::stdConnect()
{
  // kilo
  uint64_t size_in_byte = 1024;
  // mega
  size_in_byte *= 1024;
  // #mega
  size_in_byte *= 500;
  MainWindow::instance_->setupRingBuffer(size_in_byte);
  tcpConnect();
}

void ClientManager::setPause(bool p)
{
  boost::shared_lock<boost::shared_mutex> client_lck(client_lock_);
  if (client_) {
    client_->setPause(p);
  }
}

int ClientManager::getUpdateIntervall()
{
  boost::shared_lock<boost::shared_mutex> client_lck(client_lock_);
  if (client_) {
    return client_->getTau();
  }
  return 0;
}
