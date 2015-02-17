#include "Client.h"

Client::Client(unsigned int update_intervall) : process_data_(update_intervall), run_(true)
{
  gpu_info_.device_name = 0;
}

Client::~Client()
{
  std::cout << "Client::~Client()" << std::endl;
}

void Client::processData()
{
  process_data_.processData(ds_buffer_);
}

QString Client::getStatus()
{
  boost::shared_lock<boost::shared_mutex>(state_lock_);
  return status_;
}

void Client::setPause(bool p)
{
  changeState(p ? COMMAND_PAUSE : COMMAND_RUN);
  process_data_.setPause(p);
}

void Client::stopRun()
{
  run_ = false;
  if (state_ != COMMAND_RUN) {
    changeState(COMMAND_RUN);
  }
}

void Client::changeState(short state)
{
  if (state_ == COMMAND_PAUSE && state == COMMAND_EXIT) {
    sendCommand(COMMAND_RUN);
  }
  sendCommand(state);
  state_ = state;
}
