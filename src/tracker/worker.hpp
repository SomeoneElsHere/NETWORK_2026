#ifndef WORKER_CPP
#define WORKER_CPP


// If command is in fact for listing, send list of tracker files to the socket.
// Returns true on success, false if command is different.
// ----------------------------------------------------------------------------
bool proc_list(string& cmd, int sockfd, list<TrackerFile>& tracker_files, pthread_mutex_t& worker_lock) {
  // init constants
  static const string REP_LIST_HEADER = "REP LIST";
  static const string REP_LIST_FOOTER = "REP LIST END";

  // sanitize command
  if(cmd != "REQ LIST\n") return false;

  // prepare header
  pthread_mutex_lock(&worker_lock);
  size_t len = tracker_files.size();
  string msg = REP_LIST_HEADER + " " + to_string(len) + "\n";

  // prepare body, the list of files
  int idx = 0;
  for(const TrackerFile& tracker_file : tracker_files) {
    msg += to_string(idx++) + " " + tracker_file.quickstring;
  }
  pthread_mutex_unlock(&worker_lock);

  // prepare footer, end of protocol, can now send and exit.
  msg += REP_LIST_FOOTER + "\n";
  send_string(sockfd, msg);
  return true;
}


// Search everywhere for a tracker file and get it's contents
// Returns empty if not found (or file was empty)
// ----------------------------------------------------------------------------
string find_trackerfile(string& name, list<TrackerFile>& tracker_files) {
  for(TrackerFile& file : tracker_files) {
    if(!file.has_filename(name)) continue;
    return "non-empty";
  }
  return "";
}


// If command is in fact for get, parse it and send answer through the socket
// Returns true on success or failure (file not found), false if command is different.
// ----------------------------------------------------------------------------
bool proc_get(vector<string>& cmd, int sockfd, list<TrackerFile>& tracker_files, pthread_mutex_t& worker_lock) {
  // init constants 
  static const string REP_GET_HEADER = "REP GET BEGIN";
  static const string REP_GET_FOOTER = "REP GET END";
  constexpr char REP_GET_ERROR[] = "REP GET FAIL";

  // sanitize the command
  if(cmd.size() != 2) return false;
  if(cmd.front() != "GET") return false;

  // search for the file
  string filename = cmd.back();
  if(filename.back() == '\n') filename.pop_back(); // trim if needed, will probably extend to ".tracker"
  pthread_mutex_lock(&worker_lock);
  string contents = find_trackerfile(filename, tracker_files);
  pthread_mutex_unlock(&worker_lock);
  if(contents.empty()) { // failure
    send(sockfd, REP_GET_ERROR, sizeof REP_GET_ERROR, 0); 
    return true;
  }

  // Success, construct the string and send
  string sendstr = REP_GET_HEADER + '\n' + contents + '\n' + REP_GET_FOOTER + '\n';
  send_string(sockfd, sendstr);
  return true;
}


// Search for a tracker file and send it to the server
// Returns false if tracker file neither found nor sent
// ----------------------------------------------------------------------------
bool proc_create(vector<string>& cmd, int sockfd, list<TrackerFile>& tracker_files, pthread_mutex_t& worker_lock){
  // init
  constexpr static char REP_CREATE_SUCCESS[] = "createtracker succ\n";
  constexpr static char REP_CREATE_FAIL[] = "createtracker fail\n";
  constexpr static char REP_CREATE_EXISTS[] = "createtracker ferr\n";

  // sanitize
  if(cmd.size() < 1) return false;
  if(cmd.front() != "createtracker") return false;

  // Produce the file
  TrackerFile file = {};
  if(cmd.size() == 6) file = TrackerFile(cmd[1], cmd[2], "", cmd[3], cmd[4]);
  else if(cmd.size() == 7) file = TrackerFile(cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
  else { // missing arguments
    // TODO: make it send the usage here instead
    send(sockfd, REP_CREATE_FAIL, sizeof REP_CREATE_FAIL, 0);
    return true; // still the command was right, technically
  }; 

  // add the file
  pthread_mutex_lock(&worker_lock);
  if(find_trackerfile(cmd[1], tracker_files) != "") {
    // File already exists
    send(sockfd, REP_CREATE_EXISTS, sizeof REP_CREATE_EXISTS, 0);
  } else{
    // File is empty, add it
    tracker_files.push_back(file);
    send(sockfd, REP_CREATE_SUCCESS, sizeof REP_CREATE_SUCCESS, 0);
  }
  pthread_mutex_unlock(&worker_lock);
  return true;
}

// thread info for the tracker server
struct TrackerBox {
  list<TrackerFile>& tracker_files;
  pthread_mutex_t& worker_lock;
  int sockfd;
};

// thread function that reads all inputs and handles them
void* proc_tracker_all(void* tracker_box) {
  // unbox 
  TrackerBox* unbox = (TrackerBox*)tracker_box;
  list<TrackerFile>& tracker_files = unbox->tracker_files;
  pthread_mutex_t& worker_lock = unbox->worker_lock;
  int sockfd = unbox->sockfd;

  // init
  size_t bufsiz = 255;
  char bufstr[bufsiz] = "";
  int len = 0;

  // wait for command
  while((len = recv(sockfd, bufstr, bufsiz, 0)) > 0) {
    string str = string(bufstr);
    string trimmed = str.substr(0, len);
    if(proc_list(trimmed, sockfd, tracker_files, worker_lock)) continue;
    //if(proc_update(str)) continue;
    vector<string> strs = strsplit(trimmed);
    if(proc_get(strs, sockfd, tracker_files, worker_lock)) continue;
    if(proc_create(strs, sockfd, tracker_files, worker_lock)) continue;
    send(sockfd, "invalid command\n", sizeof "invalid command\n", 0);
  }
  close(sockfd);
  return NULL;
}

//TODO: add the update command


#endif
