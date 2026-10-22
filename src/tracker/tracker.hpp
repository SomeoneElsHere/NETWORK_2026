#ifndef TRACKER_HPP
#define TRACKER_HPP

#include "../includes.hpp"
constexpr int LISTEN_BACKLOG = 50; // how many connections to listen for at once

class Tracker {
  // variables
  pthread_mutex_t worker_lock = PTHREAD_MUTEX_INITIALIZER; // workers will use this to stay sync'd
  list<TrackerFile> tracker_files = {};                    // list of tracker files.
  AddrinfoExt tracker_info = {};  // Info on self. Address, port, socket, etc.
  //list<int> connections = {};     // lists have faster insertion/deletion than vector
  time_t start_time = -1;         // when the server was first started, read-only.

public: 
  // set true on any error, unset manually to ignore
  bool err_status = false; 
  void print_status();                // print server address, port, uptime, and error status
  Tracker(const char* port = "9600"); // constructor
  void run();                         // easy to use api

  // destructor to free everything
  ~Tracker() {
    if(tracker_info.addr) freeaddrinfo(tracker_info.addr);
    if(tracker_info.sockfd < 0) close(tracker_info.sockfd);
    pthread_mutex_destroy(&worker_lock);
  } 

  // constants with scope basically
  const string REP_LIST_HEADER = "REP LIST\n";
  constexpr static char REP_LIST_FOOTER[] = "REP LIST END\n";
  constexpr static char REP_GET_HEADER[] = "REP GET BEGIN\n";
  const string REP_GET_FOOTER = "REP GET END";

  // Search for a tracker file and return the iterator to it.
  TrackerFile search_trackerfile(string& name);

private:
  // helper functions to handle commands
  bool proc_list(string& cmd, int sockfd);
  bool proc_get(vector<string>& cmd, int sockfd);
  bool proc_create(string& cmd){return true;}
  bool proc_update(string& cmd){return true;}


  // find a tracker file by string name
  TrackerFile search_file(string& name);
};


// create a tracker and bind it to `port`
// tries bunch of different ipv4 connection options until one succeeds
// sets err_status true on error or if all failed
// ---------------------------------------------------------------------
Tracker::Tracker(const char* port /* = 9999 */) {
  // get a list of options
  struct addrinfo* addrList = make_connection_list(port); 
  err_status = addrList == NULL;
  if(err_status) return;

  // while going through the list
  for(struct addrinfo* addr = addrList; addr; addr = addrList->ai_next) {
    // try the socket
    int sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if(sockfd < 0) continue;

    // get around the "bind address already in use"
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // try the bind
    int bind_status = bind(sockfd, addr->ai_addr, addr->ai_addrlen);
    if (bind_status >= 0) { // success, save and stop going through the list
      tracker_info.sockfd = sockfd;
      tracker_info.addr = addr;
      break; 
    }
    close(sockfd); // failure, clean the socket and continue
  }

  // can now start listening
  err_status = listen(tracker_info.sockfd, LISTEN_BACKLOG) < 0;
  if(err_status) {
    fprintf(stderr, "ERROR - listen -> %s:%d\n", __FILE__, __LINE__);
    return;
  }

  // success, record what time server started at.
  start_time = time(NULL);
} 


// Print information about the server
// What it is, when it started, where it started, and how long it has been running, any errors?
// ---------------------------------------------------------------------
void Tracker::print_status() {
  fprintf(stderr, "server: tracker\n"); 

  // get date, or set skip that if start_time is negative
  string start_date = epoch_to_iso8601(start_time); 
  if(start_date.empty()) fprintf(stderr, "\tstarted: unkown\n");
  else fprintf(stderr, "\tstarted: %s\n", start_date.c_str());

  // get address, will already be empty on NULL
  long int port = port_from_addrinfo(tracker_info.addr);
  if(port < 0) fprintf(stderr, "\tport: unkown\n");
  else fprintf(stderr, "\tport: %ld\n", port);

  // get uptime, you get the gist
  string uptime = epoch_to_uptime(start_time);
  if(uptime.empty()) fprintf(stderr, "\tuptime: unkown\n");
  else fprintf(stderr, "\tuptime: %s\n", uptime.c_str());

  // print error status
  fprintf(stderr, "\terror: %s\n", err_status ? "true" : "false");
}


// easy to use api, imagine python
// for now just accepts all connections, adds them to the list, and starts a thread for them
// ---------------------------------------------------------------------
void Tracker::run() {
  char host[NI_MAXHOST], serv[NI_MAXSERV];
  sockaddr_in addr = {};
  socklen_t alen = -1;

  while(true) {
    int s = accept(tracker_info.sockfd, (sockaddr*)&addr, &alen);
    if(s < 0) continue; // ignore errors
    s = getnameinfo((sockaddr*)&addr, alen, host, NI_MAXHOST, serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
    if(s != 0) printf("New connection from %s:%s\n", host, serv);
    //TODO: ptrhreads
  }
}


// If command is in fact for listing, send list of tracker files to the socket.
// Returns true on success, false if command is different.
// Sets err_status true on error and returns false.
// ----------------------------------------------------------------------------
bool Tracker::proc_list(string& cmd, int sockfd) {
  if(err_status) return false;
  if(cmd != "REQ LIST\n") return false;
  size_t len = tracker_files.size();

  // header
  string msg = REP_LIST_HEADER + " " + to_string(len) + "\n";
  send_string(sockfd, msg);

  // body
  int idx = 0;
  for(const TrackerFile& tracker_file : tracker_files) {
    msg = to_string(idx++) + " " + tracker_file.quickstring;
    send_string(sockfd, msg);
  }

  // footer
  send(sockfd, REP_LIST_FOOTER, sizeof REP_LIST_FOOTER, 0);
  return true;
}


// If command is in fact for get, parse it and send answer through the socket
// Returns true on success, false if command is different.
// Sets err_status true on error and returns false.
// ----------------------------------------------------------------------------
bool Tracker::proc_get(vector<string>& cmd, int sockfd) {
  if(err_status) return false;
  if(cmd.size() != 2) return false;
  return true;
}


// Search for a tracker file and return the iterator to it.
// Returns tracker_files.end() on error
// ----------------------------------------------------------------------------
TrackerFile Tracker::search_trackerfile(string& name) {
  for(TrackerFile& file : tracker_files) {
    if(!file.has_filename(name)) continue;
    return file;
  }
  return {};
}

#endif
