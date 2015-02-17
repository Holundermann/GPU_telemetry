/*
 * Client.cpp
 *
 *  Created on: Feb 23, 2014
 *      Author: clemens
 */

#include "Data/Network/TcpClient.h"

#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

using namespace std;

TcpClient::TcpClient(boost::asio::io_service* io_service, std::string ip, unsigned int port, unsigned int update_intervall) :
     Client(update_intervall), ip_(ip), port_(port), io_service_(io_service), socket_(*io_service)
{
  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip_), port_);
  read_bytes_ = 0;
  header_.amount_of_data_segments = 0;
  header_.base_time = 0;
  header_.size_of_data_segment = 0;
  status_ = "connecting to ";
  status_.append(ip_.c_str());
  status_.append(":");
  status_.append(QString::number(port_));
  socket_.async_connect(endpoint, boost::bind(&TcpClient::connect_handler, this, boost::asio::placeholders::error));
  // create deadline timer for the async connect
//  boost::asio::deadline_timer timer(*io_service_);
//  timer.expires_from_now(boost::posix_time::seconds(10));
//  timer.async_wait(boost::bind(&TcpClient::close, this));
}

TcpClient::~TcpClient()
{
  changeState(COMMAND_EXIT);
  run_ = false;
  socket_.close();
  delete[] gpu_info_.device_name;
  gpu_info_.device_name = 0;

  std::cout << "~TcpClient()" << std::endl;
}

void TcpClient::connect_handler(const boost::system::error_code& error)
{
  if (!error) {
    cout << "[TcpClient::connect_handler] successfully connected to " << ip_ << ":" << port_ << endl;
    status_ = "connected to ";
    status_.append( + ip_.c_str());

    // get bytes to read
    uint64_t bytes_to_read;
    size_t bytes_read = boost::asio::read(socket_, boost::asio::buffer(&bytes_to_read, sizeof(uint64_t)));
    assert(bytes_read == sizeof(uint64_t));

    char* body = new char[bytes_to_read];
    bytes_read = boost::asio::read(socket_, boost::asio::buffer(body, bytes_to_read));
    assert(bytes_read == bytes_to_read);

    size_t device_name_length = bytes_to_read - 4*sizeof(int);
    char* device_name = new char[device_name_length + 1];
    strncpy(device_name, body, device_name_length);
    device_name[device_name_length] = 0;

    size_t pos = device_name_length;
    gpu_info_.device_name = device_name;
    gpu_info_.cc_major = *(static_cast<int*>(static_cast<void*>(body + pos)));
    // this assertion is only for debugging purpose! has to be removed
//    assert(gpu_info_.cc_major == 3);
    pos += sizeof(int);

    gpu_info_.multiprocessor_count = *(static_cast<int*>(static_cast<void*>(body + pos)));
    pos += sizeof(int);
    // this assertion is only for debugging purpose! has to be removed
//    assert(gpu_info_.multiprocessor_count == 8);

    gpu_info_.max_thread_per_mp  = *(static_cast<int*>(static_cast<void*>(body + pos)));
    pos += sizeof(int);
    // this assertion is only for debugging purpose! has to be removed
//    assert(gpu_info_.max_thread_per_mp == 2048);

    gpu_info_.clock_rate = *(static_cast<int*>(static_cast<void*>(body + pos)));
    pos += sizeof(int);
    // this assertion is only for debugging purpose! has to be removed
//    assert(gpu_info_.clock_rate == 1058500);
    MainWindow::instance_->setGpuInfo();
    delete [] body;
  }
  else {
    std::cout << "[TcpClient::connect_handler] error in connection: " << error.message() << std::endl;
    close();
  }
}

void TcpClient::run()
{
  // set gpu info to process data
  process_data_.setGpuInfo(gpu_info_);

  if (socket_.is_open()) {
    try {
      while(!error_ && run_) {
        handle_read_header(error_);
        handle_read_body(error_);
      }
    }
    catch  (std::exception &e) {
      std::cout << "[TcpClient::run] exception occured: " << e.what() << std::endl;
      close(LOST_CONNECTION);
    }
  }
}

void TcpClient::handle_read_header(const boost::system::error_code& error)
{
  if (!error) {
    size_t size_to_read = sizeof(int) +  // size of one data segment
                          sizeof(int) +  // how many data segments
                          sizeof(float) + // base time
                          sizeof(int);   // lenght of all names + all integer which determine the size of the strings

    char* buffer = new char[size_to_read];
//    std::cout << "TcpClient::handle_read_header: size to read is: " << size_to_read << std::endl;

    assert(buffer);

//    std::cout << "TcpClient::handle_read_header: trying to read header from server" << std::endl;

    {
      boost::unique_lock<boost::shared_mutex>(state_lock_);
      read_bytes_ += boost::asio::read(socket_, boost::asio::buffer(buffer, size_to_read));
    }

    // read amount of data segment from buffer
    int curr_pos = 0;
    header_.amount_of_data_segments = *(static_cast<int*>(static_cast<void*>(&buffer[curr_pos])));
//    cout << "TcpClient::handle_read_header: # data segments: " << header_.amount_of_data_segments << endl;

    // read size of data segment
    curr_pos += sizeof(int);
    header_.size_of_data_segment = *(static_cast<int*>(static_cast<void*>(&buffer[curr_pos])));
//    cout << "TcpClient::handle_read_header: size of data segments: " << header_.size_of_data_segment << endl;

    // read base time
    curr_pos += sizeof(int);
    header_.base_time = *(static_cast<float*>(static_cast<void*>(&buffer[curr_pos])));
//    cout << "TcpClient::handle_read_header: base time: " << header_.base_time << endl;

    // read lenght of all names
    curr_pos += sizeof(float);
    header_.procedures_name_length = *(static_cast<int*>(static_cast<void*>(&buffer[curr_pos])));
//    cout << "TcpClient::handle_read_header: procedures_name_length: " << header_.procedures_name_length << endl;

    delete [] buffer;
  } else {
    close(LOST_CONNECTION);
  }
}

void TcpClient::handle_read_body(const boost::system::error_code& error)
{
  if (!error) {
    data_segment_buffer seg_buffer;
    seg_buffer.header_data = header_;
//    cout << "# data seg: " << header_.amount_of_data_segments << endl;
//    cout << "size of data seg: " << header_.size_of_data_segment << endl;
//    cout << "base time: " << header_.base_time << endl;
    size_t size_to_read = header_.size_of_data_segment * header_.amount_of_data_segments + header_.procedures_name_length;
//    std::cout << "TcpClient::handle_read_body: trying to alloc " << size_to_read << std::endl;
    //char* buffer = static_cast<char*>(malloc(size_to_read));
    char* buffer = new char[size_to_read];

    size_t read = boost::asio::read(socket_, boost::asio::buffer(buffer, size_to_read));
    assert(read == size_to_read);
    {
      boost::shared_lock<boost::shared_mutex>(state_lock_);
      read_bytes_ += read;
    }
    seg_buffer.parsed_data = buffer;
    ds_buffer_ = seg_buffer;
    processData();
//    std::cout << "TcpClient::handle_read_body " << std::endl;
    delete [] buffer;
  } else {
    close(LOST_CONNECTION);
  }
}

QString TcpClient::getTransferInfo()
{
  size_t bytes_transfered;
  {
    boost::shared_lock<boost::shared_mutex>(state_lock_);
    bytes_transfered = read_bytes_;
    read_bytes_ = 0;
  }

  // status gets updated every STATUSBAR_UPDATE_INTERVALL in ms
  int counter = 0;
  while(bytes_transfered > 1024) {
    ++counter;
    bytes_transfered /= 1024;
  }

  QString postfix;
  switch(counter) {
    case 0:
      // Byte
      postfix = " Byte";
      break;
    case 1:
      //  KByte
      postfix = " KByte";
      break;
    case 2:
      // MByte
      postfix = " MByte";
      break;
    case 3:
      // GByte
      postfix = " GByte";
      break;
    default:
      postfix = " too fast";
      break;
  }

  postfix.append("/second");

  return QString(QString::number(bytes_transfered, 'f', 2) + postfix);
}

void TcpClient::close(std::string msg)
{
  std::cout << "[TcpClient::close]" << std::endl;
  socket_.close();
  status_.clear();
  status_.append(msg.c_str());
  status_.append(ip_.c_str());
  status_.append(":");
  status_.append(QString::number(port_));
}


void TcpClient::sendCommand(short command)
{
  std::cout << "[TcpClient::sendCommand] sending command " << command << std::endl;
  if (socket_.is_open()) {
    try {
      socket_.send(boost::asio::buffer(&command, sizeof(short)));
    } catch (std::exception e) {
      std::cout << "[TcpClient::sendCommand] error while trying to send data over socket: " << e.what() << std::endl;
    }
  }
}
