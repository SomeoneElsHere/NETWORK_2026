#ifndef WORKER_CPP
#define WORKER_CPP

#include "includes.hpp"


// If command is in fact for listing, send list of tracker files to the socket.
// Returns true on success, false if command is different.
// ----------------------------------------------------------------------------
bool proc_list(string& cmd, int sockfd, list<string>& tracker_files, pthread_mutex_t& worker_lock) {
  // init constants
  static const string REP_LIST_HEADER = "REP LIST";
  static const string REP_LIST_FOOTER = "REP LIST END";

  // sanitize command
  if(cmd != "REQ LIST") return false;

  // prepare header
  pthread_mutex_lock(&worker_lock);
  size_t len = tracker_files.size();
  string msg = encapsulate(REP_LIST_HEADER + " " + to_string(len));
  // prepare body, the list of files
  int idx = 0;
  for(string& tracker_file : tracker_files) {
    msg += encapsulate(to_string(idx++) + ' ' + tracker_file);
  }
  pthread_mutex_unlock(&worker_lock);

  // prepare footer, end of protocol, can now send and exit.
  msg += encapsulate(REP_LIST_FOOTER);
  fprintf(stderr, "Server -> Peer %d:\n%s", sockfd, msg.c_str());
  send_string(sockfd, msg);
  return true;
}


// Sear// If command is in fact for get, parse it and send answer through the socket
// Returns true on success or failure (file not found), false if command is different.
// ----------------------------------------------------------------------------
bool proc_get(vector<string>& cmd, int sockfd, pthread_mutex_t& worker_lock) {
  // init constants 
  static const string REP_GET_HEADER = encapsulate("REP GET BEGIN");
  static const string REP_GET_FOOTER = "REP GET END";
  const char REP_GET_ERROR[] = "<REP GET FAIL>\n";

  // sanitize the command
  if(cmd.size() != 2) return false;
  if(cmd.front() != "GET") return false;

  // search for the file
  string filename = cmd.back();
  pthread_mutex_lock(&worker_lock);
  string contents = get_tracker_file(filename);
  pthread_mutex_unlock(&worker_lock);
  if(contents.empty()) { // failure
    fprintf(stderr, "Server -> Peer %d: %s\n", sockfd, REP_GET_ERROR);
    send(sockfd, REP_GET_ERROR, sizeof REP_GET_ERROR, 0);
    return true;
  }

  // Success, get the file md5
  string md5str = "";
  stringstream iss(contents);
  for(string line; getline(iss, line);) {
    bool valid_string = true;
    vector<string> args = str_to_args(line, valid_string);
    if(args.size() != 2) continue;
    if(args.front() != "MD5:") continue;
    md5str = args.back();
    break;
  }

  // construct and send
  string sendstr = REP_GET_HEADER + contents + encapsulate(REP_GET_FOOTER + " " + md5str);
  fprintf(stderr, "Server -> Peer %d:\n%s", sockfd, sendstr.c_str());
  send_string(sockfd, sendstr);
  return true;
}


// Search for a tracker file and send it to the server
// Returns false if tracker file neither found nor sent
// ----------------------------------------------------------------------------
bool proc_create(vector<string>& cmd, int sockfd, list<string>& tracker_files, pthread_mutex_t& worker_lock){
  // init
  constexpr static char REP_CREATE_SUCCESS[] = "<createtracker succ>\n";
  constexpr static char REP_CREATE_EXISTS[] = "<createtracker ferr>\n";
  constexpr static char REP_CREATE_FAIL[] = "<createtracker fail>\n";

  // sanitize
  if(cmd.size() < 1) return false;
  if(cmd.front() != "createtracker") return false;

  // get args
  if(cmd.size() == 6) cmd.emplace(cmd.begin() + 3, string(""));
  if(cmd.size() != 7) { // missing arguments
    fprintf(stderr, "Server -> Peer %d: %s\n", sockfd, REP_CREATE_FAIL);
    send(sockfd, REP_CREATE_FAIL, sizeof REP_CREATE_FAIL, 0);
    return true; // still the command was right, technically
  }; 
  string &name = cmd[1], &size = cmd[2], &desc = cmd[3];
  string &hash = cmd[4], &addr = cmd[5], &port = cmd[6];

  // sanitize some more
  bool valid_file = true;
  string quickstring = (name + ' ') + (size + ' ') + hash;
  string contents = trackerfile_upper(name, size, desc, hash, valid_file);
  if((!valid_file) || (!valid_address(addr, port)) || (!valid_md5(hash)) || contents.empty()) {
    fprintf(stderr, "Server -> Peer %d: %s\n", sockfd, REP_CREATE_FAIL);
    send(sockfd, REP_CREATE_FAIL, sizeof REP_CREATE_FAIL, 0);
    return true; // still the command was right, technically
  }
  contents += (addr + ':') + (port + ":0:") + (size + ':') + to_string(time(NULL)) + "\n";

  // add the file
  pthread_mutex_lock(&worker_lock);
  name.append(".track");
  CREATE create_status = create_file(name, contents);
  if(create_status == CREATE::SUCCESS) tracker_files.push_back(quickstring);
  pthread_mutex_unlock(&worker_lock);

  // send the output, do NOT combine the if(create_status == CREATE:SUCCESS) with if statement above
  if(create_status == CREATE::EXISTS) {
    fprintf(stderr, "Server -> Peer %d:%s\n", sockfd, REP_CREATE_EXISTS);
    send(sockfd, REP_CREATE_EXISTS, sizeof REP_CREATE_EXISTS, 0);
  } else if(create_status == CREATE::FAILURE) {
    fprintf(stderr, "Server -> Peer %d:%s\n", sockfd, REP_CREATE_FAIL);
    send(sockfd, REP_CREATE_FAIL, sizeof REP_CREATE_FAIL, 0);
  } else {
    fprintf(stderr, "Server -> Peer %d:%s\n", sockfd, REP_CREATE_SUCCESS);
    send(sockfd, REP_CREATE_SUCCESS, sizeof REP_CREATE_SUCCESS, 0);
  }

  // finish
  return true;
}


// Search for a tracker file and send it to the server
// Returns false if tracker file neither found nor sent
// ----------------------------------------------------------------------------
bool proc_update(vector<string>& cmd, int sockfd, pthread_mutex_t& worker_lock){
  static const string REP_GENERIC = "updatetracker";

  // sanitize
  if(cmd.size() != 6) return false;
  if(cmd.front() != "updatetracker") return false;

  // init and sanitize some more
  string &name = cmd[1], &start = cmd[2], &end = cmd[3];
  string &addr = cmd[4], &port = cmd[5];
  if((!valid_uint(start)) || (!valid_uint(end))) return false;

  // make a version of the file without that peer, and then add our peer, and then write back
  CREATE create_status = CREATE::FAILURE;
  pthread_mutex_lock(&worker_lock);
  string contents = get_tracker_file(name);
  if((!contents.empty()) && delete_peer(contents, addr, port)) {
    contents += (addr+ ":") + (port + ":") + (start + ":") + (end + ":") + to_string(time(NULL)) + '\n';
    create_status = create_file(name, contents, true);
  } 
  pthread_mutex_unlock(&worker_lock);

  // error status
  string msg = (REP_GENERIC + ' ') + name + ' ';
  if(create_status == CREATE::SUCCESS) msg += "succ";
  else if(contents.empty()) msg += "ferr";
  else msg += "fail";
  msg = encapsulate(msg);
  fprintf(stderr, "Server -> Peer %d:\n%s", sockfd, msg.c_str());
  send_string(sockfd, msg);

  // finish
  return true;
}

// thread info for the tracker server
struct TrackerBox {
  list<string>& tracker_files;
  pthread_mutex_t& worker_lock;
  int sockfd;
};

// thread function that reads all inputs and handles them
#include <iostream>
void* proc_tracker_all(void* tracker_box) {
  // unbox 
  TrackerBox* unbox = (TrackerBox*)tracker_box;
  list<string>& tracker_files = unbox->tracker_files;
  pthread_mutex_t& worker_lock = unbox->worker_lock;
  int sockfd = unbox->sockfd;

  // init
  size_t bufsiz = 255;
  char bufstr[bufsiz] = "";
  int len = 0;

  // wait for command
  while((len = recv(sockfd, bufstr, bufsiz, 0)) > 0) {
    bufstr[len] = '\0'; // cut
    string str = string(bufstr);
    printf("Peer %d -> Server: %s", sockfd, bufstr);
    fflush(stderr);
    if(!decapsulate(str)) continue; // not a command
    if(proc_list(str, sockfd, tracker_files, worker_lock)) continue;
    //TODO: make valid_string follow the convention
    bool valid_string = true;
    vector<string> strs = str_to_args(str, valid_string);
    if(proc_get(strs, sockfd, worker_lock)) continue;
    if(proc_create(strs, sockfd, tracker_files, worker_lock)) continue;
    if(proc_update(strs, sockfd, worker_lock)) continue;
    fprintf(stderr, "Server -> Peer %d: Invalid Command\n", sockfd);
    send(sockfd, "invalid command\n", sizeof "invalid command\n", 0);
  }
  close(sockfd);
  return NULL;
}


#endif
