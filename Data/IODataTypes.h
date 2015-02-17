#ifndef IODATATYPES_H
#define IODATATYPES_H

#include <cstdlib>
#include <stdint.h>
#include <vector>

/**
 * contains header information
 */
typedef struct {
    int   size_of_data_segment;       ///< sizeof data segments
    int   amount_of_data_segments;    ///< # of data segments
    float base_time;                  ///< base time
    int   procedures_name_length;     ///< lenght of all names + all integer which determine the size of the strings
} header;

/**
 * contains header information + data segments to parse
 */
typedef struct {
    header header_data;                    ///< header informations
    char* parsed_data;                    ///< data segments to parse
} data_segment_buffer;

typedef struct {
  char* device_name;
  int cc_major;
  int multiprocessor_count;
  int max_thread_per_mp;
  int clock_rate;
} gpu_info;

struct parsed_data {
  unsigned int procedure_id;
  float base_time;
  uint64_t task_start;
  uint64_t task_end;
  int multiprocessor_id;
  int active_thread_mask;
  int not_sure;
  } ;

#endif /* IODATATYPES_H */
