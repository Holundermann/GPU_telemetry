#ifndef PROCESSDATA_H
#define PROCESSDATA_H

#include <atomic>
#include <vector>
#include <map>

#include <boost/thread.hpp>

#include "IODataTypes.h"

class ProcessData
{
  public:
    ProcessData(unsigned int update_intervall);
    std::atomic<bool> run_;

    virtual ~ProcessData();

    /**
    * parsing ds buffer and safes it to data_buffer_ map with the procedure as id
    **/
    void processData(data_segment_buffer& data_to_add);


    /**
    * calculate the y coordinates that the renderarea needs for displaying diagram
    **/
    void calcDataPoints();

    void setGpuInfo(gpu_info& info);

    __inline void setTau(int tau) {tau_ = tau;}

    __inline int getTau() {return tau_;}

    void setPause(bool p);

  private:
    std::atomic<bool> pause_;
    std::map<int, std::vector<parsed_data> > data_buffer_;
    std::map<int, char*> procedure_names_;
    boost::mutex pause_lock_;
    boost::condition_variable_any pause_cond_;
    boost::mutex data_buffer_lock_;
    boost::condition_variable_any data_buffer_cond_;
    unsigned int tau_; ///< time in milliseconds to trigger calculation of data points
    std::map<int, uint64_t> y_coordinates_absolute_; ///< key is procedure id, value is absolute value of used time overall
    std::map<int, long double> y_coordinates_percentage_; ///< key is procedure id, value is auslastung on gpu in %
    boost::thread* process_data_points_;
    boost::system_time timeout_;
    gpu_info* gpu_info_;
    uint64_t overall_cycles_counter_;

    void runDataPointsThread();
    void waitTau();
};

#endif // PROCESSDATA_H
