#ifndef FILES_HPP
#define FILES_HPP


#include "../includes.hpp"

const string download_folder = "./data";

enum CREATE {
  SUCCESS,  // successfully made the file
  FAILURE,  // permission errors etc
  EXISTS    // already exists
};

// Create a file and fill with content. Will initialize the folders if they aren't already
// If overwriting=true, instead of overwriting the file it will replace it
// returns false on OS/permission error 
// returns true on success 
// -----------------------------------------------------------
CREATE create_file(string& filename, string& content, bool overwriting=false) {
  // get the current path
  error_code ferr_status = error_code();
  path download_path = path(download_folder);
  fprintf(stderr, "Content is -> %s\n", content.c_str());

  // create the folder if doesn't exist
  if(!exists(download_path, ferr_status)) {
    bool ret = create_directory(download_folder, ferr_status);
    if(!ret) return CREATE::FAILURE;  // couldn't create
    permissions(download_path, perms::owner_all | perms::group_read | perms::others_read, ferr_status);
  } if(ferr_status) return CREATE::FAILURE; // Intentonally NOT "else if"

  // Dowlnoad folder exists now, ensure it's a directory
  if(!is_directory(download_path, ferr_status)) return CREATE::FAILURE;
  if(ferr_status) return CREATE::FAILURE;

  // Create the trackerfile
  path file_path = download_path/filename; // Divide operator is how you concat paths in c++
  if((!overwriting) && exists(file_path, ferr_status)) return CREATE::EXISTS;
  if((!overwriting) && ferr_status) return CREATE::EXISTS;
 
  // write to it and finish
  ofstream tracker_file = ofstream(file_path, ios::out | ios::trunc) << content;
  return tracker_file.good() ? CREATE::SUCCESS : CREATE::FAILURE; 
}


// Get a tracker file. Returns empty if file not found (or empty)
// -----------------------------------------------------------
string get_tracker_file(string& filename) {
  // init
  string ret = ""; 
  constexpr int bufsize = 512;
  char tmp[bufsize] = "";

  // divide operator concats path objects with strings
  path tracker_path = path(download_folder); 
  ifstream tracker_file(tracker_path/filename);
  if(!tracker_file) return ret; // file doesn't exist

  // construct the string and finish
  while(tracker_file) {
    tracker_file.read(tmp, bufsize);
    ret += tmp;
  }
  tracker_file.close();
  return ret;
}


// produce the upper part of a trackerfile string
// sets status false on error, unmodified on success
// ----------------------------------------------------------------
string trackerfile_upper(string& filenam, string& filesiz,
                        string& desc, string& md, bool& status) {
  // check filesize, if this fails we can skip everything else
  status = valid_uint(filesiz);
  if(!status) return "";
  // produce the string
  ostringstream ret = ostringstream("");
  ret << "Filename: " << filenam << '\n'
     << "Filesize: " << filesiz << '\n'
     << "Description: " << desc << '\n'
     << "MD5: " << md << '\n';
  status = ret.good();
  return ret.str();
}


// Delete a peer entry from trackerfile given in string format
// Returns false on any error that prevented it
// ----------------------------------------------------------------
bool delete_peer(string& content, string& address, string& port) {
  // sanitize
  if(content.empty() || address.empty() || port.empty()) return false;
  if(!valid_address(address, port)) return false;

  // One by one add all the lines except for this one into a stringstream
  istringstream iss(content);
  string res = "";
  for(string line; getline(iss, line);) {
    // get the peer info
    vector<string> vals = strsplit(line, ':'); // split
    bool guaranteed_unmatching = false;
    if(vals.size() != 5) {
      res += line + "\n";
      continue;
    }
    string& addr = vals[0], &serv = vals[1], &begin = vals[2], &end = vals[3], &stamp = vals[4];

    // invalid, probably something else, ignore
    if((!valid_address(addr, serv)) || (!valid_uint(begin)) || (!valid_uint(end)) || (!valid_uint(stamp))) {
      res += line + "\n";
      continue;
    }

    // valid, delete the line if matching, add otherwise
    if((addr == address) && (serv == port)) continue;
    res += line + "\n"; 
    break;
  }

  // finish
  content = res;
  return true;
}


/*
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
*/

#endif
