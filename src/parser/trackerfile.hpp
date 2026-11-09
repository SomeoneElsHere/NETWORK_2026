#ifndef TRACKERFILE_HPP
#define TRACKERFILE_HPP

#include "../includes.hpp"

struct Peer {
  string addr = "", port = "", start = "", end = "", stamp = "";
};

class TrackerFile {
public:
  // all these functions try reading to the variable if valid
  // if valid, returns true and sets vals, return false otherwise
  // I will replace with a vector based one later
  bool read_generic_uniq(vector<string>& args, string trigger, string& storage);
  bool read_name_uniq(vector<string>& args);
  bool read_size_uniq(vector<string>& args);
  bool read_desc_uniq(vector<string>& args);
  bool read_hash_uniq(vector<string>& args);
  bool read_peer(vector<string>& args);

  // to check if successful parsing invalidates uniqueness rules
  bool is_context_error() { return (desc_count < 2) && uniq; }

  // get the quickstring quickly!
  string get_quickstring() { return (name + ' ') + (size + ' ') + hash; }
  
private:
  string name = "", desc = "", hash = "", size = "";
  vector<Peer> peers;
  // for checking uniqueness
  int desc_count = 0;
  bool uniq = true;
};

// Get a trackerfile from string, sets status false on any error
// will probably make a full on BNF parser for this later
// this is kind of a demo job as is. Don't even have a tokenizer up.
// ------------------------------------------------------
struct TrackerFile parse_file(fstream& fs, bool& status) {
  struct TrackerFile ret;
  for(string line; getline(fs, line);) {
    // valid parses
    if(line.empty()) continue;              // newline
    if(line.front() == '#') continue;       // comment
    vector<string> args = strsplit(line, ':');
    if(ret.read_name_uniq(args)) continue;  // filename
    if(ret.read_size_uniq(args)) continue;  // filesize
    if(ret.read_desc_uniq(args)) continue;  // description
    if(ret.read_hash_uniq(args)) continue;  // hash
    if(ret.read_peer(args)) continue;       // peer
    // invalid parse
    status = false;
    break; 
  }

  // set status with a makeshift `nor` operator and finish
  status &= !ret.is_context_error(); 
  return ret;
}

// Parser function implementations
bool TrackerFile::read_generic_uniq(vector<string>& args, string trigger, string& storage) {
  if(args.size() != 2) return false;      // sanitize
  if(args.front() != trigger) return false; // check key
  if(!storage.empty()) uniq = false;        // ensure unique
  storage = args.back();                     // set 
  return true;                              // finish
}

// filename
bool TrackerFile::read_name_uniq(vector<string>& args) {
  return read_generic_uniq(args, "Filename", name);
}

// filesize
bool TrackerFile::read_size_uniq(vector<string>& args) {
  if(!read_generic_uniq(args, "", size)) return false;
  if(!valid_uint(size)) return false;
  return true;
}

// description
bool TrackerFile::read_desc_uniq(vector<string>& args) {
  bool ret = read_generic_uniq(args, "Description", desc);
  if(ret) desc_count++;
  return ret;
}

// hash
bool TrackerFile::read_hash_uniq(vector<string>& args) {
  if(args.size() != 2) return false;
  if(args.front() != "hash") return false;
  hash = args.back();
  return valid_md5(hash);
}

// peer
bool TrackerFile::read_peer(vector<string>& args) {
  if(args.size() != 5) return false;

  // construct the peer
  Peer peer = {
    .addr   = args[0],
    .port   = args[1],
    .start  = args[2],
    .end    = args[3],
    .stamp  = args[4],
  };

  // sanitize
  if(!valid_address(peer.addr, peer.port)) return false;
  if(!valid_uint(peer.start)) return false;
  if(!valid_uint(peer.end)) return false;
  if(!valid_uint(peer.stamp)) return false;

  // succces 
  peers.push_back(peer);
  return true;
}


// Go through and make a list of all the trackers at a folder
// Meant to be used at the beginning of the program, simply returns empty on error
// -----------------------------------------------------------
list<string> list_trackers(string& target_folder) {
  // init
  list<string> ret;
  error_code ferr_status = error_code();
  directory_iterator files = directory_iterator(download_folder, ferr_status);
  if(ferr_status) return ret;

  // go through and parse
  int idx = 0;
  for(directory_entry file : files) {
    if(!file.is_regular_file()) continue;
    fstream fs(file.path());
    bool is_err = false;
    struct TrackerFile parsed = parse_file(fs, is_err);
    if(is_err) continue;
    ret.push_back(parsed.get_quickstring());
  }

  // finish
  return ret;
}


#endif
