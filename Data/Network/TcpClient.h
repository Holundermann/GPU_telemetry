/*
 * Client.h
 *
 *  Created on: Feb 23, 2014
 *      Author: clemens
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#if WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "winsock2.h"
#endif

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <deque>

#include <string>

#include "Data/Client.h"

#define BUFFSIZE 4096

const std::string LOST_CONNECTION = "lost connection with ";
const std::string COULDNT_CONNECT = "couldn´t connect to ";

class TcpClient : public Client
{
  public:
    TcpClient(boost::asio::io_service* io_service, std::string ip, unsigned int port, unsigned int update_intervall);

    /**
     * closes connection to client
     */
    virtual ~TcpClient();

    virtual QString getTransferInfo();

    virtual void run();

    virtual void sendCommand(short command);


  private:
    std::string ip_;                                           ///< ip to connect to
    unsigned int port_;                                        ///< port to connect to
    boost::asio::io_service* io_service_;                      ///< io service to handle asynchrounos io
    boost::asio::ip::tcp::socket socket_;                      ///< socket where the connection is estaplished
    header header_;                                            ///< current header
    boost::system::error_code error_;                          ///< error for boost io
    size_t read_bytes_;

    void connect_handler(const boost::system::error_code& error_);

    void close(std::string msg = COULDNT_CONNECT);

    void handle_read_header(const boost::system::error_code& error_);

    void handle_read_body(const boost::system::error_code& error_);
};

#endif /* CLIENT_H_ */


