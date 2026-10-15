#ifndef FFORMAT_HPP
#define FFORMAT_HPP


#include "../includes.hpp"


struct PeerMetadata {
  PeerMetadata(string _addr, string _port,
               size_t _start_byte,
               size_t _end_byte,
               time_t _time_stamp);
private:
  string addr;
  string port; // size_t just in case
  size_t start_byte;
  size_t end_byte;
  time_t time_stamp;
};


// Library to make, read, and store the tracker file format 
class TrackerFile {
private:
  string filename;
  size_t filesize;
  string description; // empty if none
  string md5;
public:
  string quickstring; // Of form "file1name file1MD5\n"
};


// Make a list of all current tracker files by reading the directory
// makes the directory if doesn't exist, returns empty on none
list<TrackerFile> produce_file_list() {
  return list<TrackerFile>();
}


/* implementations */
PeerMetadata::PeerMetadata(string _addr, string _port,
             size_t _start_byte,
             size_t _end_byte,
             time_t _time_stamp)
  : addr{_addr}, port{_port}, start_byte{_start_byte}
  , end_byte{_end_byte}, time_stamp{_time_stamp} {}

#endif
