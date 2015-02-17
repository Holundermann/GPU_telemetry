#include "ProcessData.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>
#include <algorithm>

#include <View/MainWindow.h>

ProcessData::ProcessData(unsigned int update_intervall) : run_(true),
                                                          pause_(false),
                                                          tau_(update_intervall),
                                                          gpu_info_(0),
                                                          overall_cycles_counter_(0)
{
  // let the process data thread some time to gather data
  timeout_ = boost::get_system_time() + boost::posix_time::milliseconds(1000);
  process_data_points_ = new boost::thread(&ProcessData::runDataPointsThread, this);
}

ProcessData::~ProcessData()
{
  std::cout << "~ProcessData()" << std::endl;
  run_ = false;
  if (pause_) {
    boost::unique_lock<boost::mutex> lck(pause_lock_);
    pause_cond_.notify_all();
  }

  if (process_data_points_) {
    process_data_points_->join();
    delete process_data_points_;
  }



  for (std::map<int, char*>::iterator it = procedure_names_.begin();
       it != procedure_names_.end();
       it++) {
    delete [] (*it).second;
  }
  procedure_names_.clear();

  MainWindow::instance_->clearProcedures();
}

void ProcessData::runDataPointsThread()
{
  while(run_) {
    waitTau();
//    std::cout << "ProcessData::runDataPointsThread: processing data" << std::endl;
    calcDataPoints();
    if (pause_) {
      boost::unique_lock<boost::mutex> lck(pause_lock_);
      pause_cond_.wait(pause_lock_);
    }
  }
}

void ProcessData::calcDataPoints()
{
  if(!gpu_info_)
    return;

  boost::unique_lock<boost::mutex> lck(data_buffer_lock_);
  float base_time = 0;

  // sum all areas according to clock ticks * used threads
  for (std::map<int, std::vector<parsed_data> >::iterator it = data_buffer_.begin();
       it != data_buffer_.end();
       it++) {
    for (std::vector<parsed_data>::iterator it_data = (*it).second.begin();
         it_data != (*it).second.end();
         it_data++) {
      assert((*it_data).task_end - (*it_data).task_start > 0);
      assert((*it_data).active_thread_mask <= gpu_info_->max_thread_per_mp);
      if (!base_time) {
        base_time = (*it_data).base_time;
      }
      y_coordinates_absolute_[(*it).first] += ((*it_data).task_end - (*it_data).task_start) * (*it_data).active_thread_mask;
      if ((*it_data).active_thread_mask > gpu_info_->max_thread_per_mp) {
        std::cout << "is bigger: " << (*it_data).active_thread_mask << std::endl;
      }
    }
    (*it).second.clear();
  }
  // save overall cycles and reset counter
  uint64_t duration = overall_cycles_counter_;
  overall_cycles_counter_ = 0;
  long double sum = 0;

  for (std::map<int, uint64_t>::iterator it = y_coordinates_absolute_.begin();
       it != y_coordinates_absolute_.end();
       it++) {
    // overall area that could have been produced
    long double dominator = (static_cast<long double>(duration * gpu_info_->multiprocessor_count * gpu_info_->max_thread_per_mp));
    // usage per procedure
    long double tmp = (dominator == 0) ? 0 : static_cast<long double>((*it).second) / dominator;
    (*it).second = 0;
    sum += tmp;
    y_coordinates_percentage_[(*it).first] = tmp;
  }
  MainWindow::instance_->addPerformanceData(y_coordinates_percentage_, procedure_names_, base_time);
  y_coordinates_percentage_.clear();
}

void ProcessData::waitTau()
{
  // thread should sleep till start_time + tau_
  boost::this_thread::sleep(timeout_);
  timeout_ = boost::get_system_time() + boost::posix_time::milliseconds(tau_);
}

void ProcessData::processData(data_segment_buffer& ds_buffer)
{
  uint curr_pos = 0;
  uint64_t tmp_end_time = 0;
  uint64_t tmp_start_time = 0;
  boost::unique_lock<boost::mutex> lck(data_buffer_lock_);
  // std::cout << "processing data" << std::endl;
  // parse buffer
  for (int i = 0; i < ds_buffer.header_data.amount_of_data_segments; i++) {
    parsed_data tmp;
    tmp.task_start = *(static_cast<uint64_t*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos)));
    curr_pos += sizeof(uint64_t);
    tmp.task_end = *(static_cast<uint64_t*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos)));
    curr_pos += sizeof(uint64_t);
    int id = *(static_cast<int*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos)));
    tmp.procedure_id = id;
    curr_pos += sizeof(int);
    tmp.multiprocessor_id = *(static_cast<int*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos)));
    curr_pos += sizeof(int);
    tmp.active_thread_mask = *(static_cast<int*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos)));
    curr_pos += sizeof(int);
//    tmp.not_sure = *(static_cast<int*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos)));
//    curr_pos += sizeof(int);
    tmp_end_time = tmp_end_time < tmp.task_end ? tmp.task_end : tmp_end_time;
    tmp_start_time = tmp_start_time > tmp.task_start ? tmp.task_start : tmp_start_time;
    tmp.base_time = ds_buffer.header_data.base_time;
//    if(tmp.task_start == 0) {
//      std::cout << "tmp.task_start is zero, produced by proc: " << id << std::endl;
//    }
    if (tmp.multiprocessor_id > gpu_info_->multiprocessor_count - 1) {
      std::cout << "ProcessData::processData: OHOH tmp.multiprocessor_id: " << tmp.multiprocessor_id << std::endl;
      assert(tmp.multiprocessor_id < gpu_info_->multiprocessor_count - 1);
    }
    data_buffer_[id].push_back(tmp);
    MainWindow::instance_->addElement2RB(tmp);
  }
  // get and set procedure names
  // length of string| name |length of string| name | ...
  uint offset = curr_pos + sizeof(int);
  uint j = 0;
  uint abort = curr_pos + ds_buffer.header_data.procedures_name_length;
//  std::cout << "[ProcessData::processData] " << "curr_pos: " << curr_pos << " name lenght: " << *(static_cast<int*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos))) << std::endl;
//  std::cout << "[ProcessData::processData] procedure name length " << *(static_cast<int*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos))) << " offset is: " << offset << " abort position is: "<< abort << std::endl;
  for (uint i = offset + *(static_cast<int*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos)));
       curr_pos + sizeof(uint) < abort;
       i += *(static_cast<int*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos))) + sizeof(uint)) {
    curr_pos += sizeof(int); // curr pos is start pos of procedure name, i is end pos of procedure name
//    std::cout << "[ProcessData::processData] i: " << i << " curr_pos: " << curr_pos << std::endl;
    char* name = new char[i - curr_pos + sizeof(char)];
    if (name) {
      strncpy(name, ds_buffer.parsed_data + curr_pos, i - curr_pos);
      name[i - curr_pos] = '\0';
    }
    else {
      assert(false);
    }
    curr_pos = i;
//    std::cout << "[ProcessData::processData] " << " curr_pos: " << curr_pos << " name lenght: " << *(static_cast<int*>(static_cast<void*>(ds_buffer.parsed_data + curr_pos))) << std::endl;
    // set name if no name is set
    std::map<int, char*>::iterator it_names = procedure_names_.find(j);
    if (it_names != procedure_names_.end() && strcmp((*it_names).second, name)) {
      // TODO inform procedure of new name
      std::cout << "[ProcessData::processData] setting new name " << name << ", old name was: " << (*it_names).second << " to position " << j << std::endl;
      delete [] (*it_names).second;
      procedure_names_[j] = name;
    } else if (it_names == procedure_names_.end()){
      std::cout << "[ProcessData::processData] Workitem found for new procedure with name: " << name << " to position " << j << std::endl;
      procedure_names_[j] = name;
    } else {
      delete [] name;
    }
    ++j;
  }
  // add temp end time to overall cycles counter - it represents the overall time the gpu ran on the task
  // over all procedures!
  overall_cycles_counter_ += tmp_end_time - tmp_start_time;
}

void ProcessData::setGpuInfo(gpu_info& info)
{
  gpu_info_ = &info;
}

void ProcessData::setPause(bool p)
{
  if (!p) {
    boost::unique_lock<boost::mutex> lck(pause_lock_);
    pause_cond_.notify_all();
  }
  pause_ = p;
}
