#ifndef FILES_HPP
#define FILES_HPP


#include "../includes.hpp"


struct PeerInfo {
  PeerInfo(string _addr, string _port,
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
public:
  // data, public for quick read only.
  string filename;
  size_t filesize;
  string description; // empty if none
  string md5;
  vector<PeerInfo> peers;
  string quickstring; // Of form "file1name file1size file1MD5\n"

  // functions
  bool has_filename(string& compare) { return filename == compare; }
  TrackerFile(string filenam, string filesiz, string desc, string md) {
    //TODO: sanitize the strtoll
    filename = filenam;
    char* endptr = NULL;
    filesize = strtoll(filesiz.c_str(), &endptr, 10);
    description = desc;
    md5 = md;
    quickstring = filenam + " " + filesiz + " " + md + "\n";
  };
  TrackerFile(): filename{}, filesize{}, description{}, md5{} {}
};


// Make a list of all current tracker files by reading the directory
// makes the directory if doesn't exist, returns empty on none
list<TrackerFile> produce_file_list() {
  return list<TrackerFile>();
}


/* implementations */
PeerInfo::PeerInfo(string _addr, string _port,
             size_t _start_byte,
             size_t _end_byte,
             time_t _time_stamp)
  : addr{_addr}, port{_port}, start_byte{_start_byte}
  , end_byte{_end_byte}, time_stamp{_time_stamp} {}

#endif
