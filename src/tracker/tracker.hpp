#ifndef TRACKER_HPP
#define TRACKER_HPP

#include "../includes.hpp"
constexpr int LISTEN_BACKLOG = 50; // how many connections to listen for at once

class Tracker {
  AddrinfoExt tracker_info = {}; // Info on self. Address, port, socket, etc.
  list<int> connections = {};    // lists have faster insertion/deletion than vector
  time_t start_time = -1;        // when the server was first started
  struct addrinfo* make_connection_list(const char* port); // helper function

public: 
  // set true on any error, unset manually to ignore
  bool err_status = false; 
  void print_status();                // print server address, port, uptime, and error status
  Tracker(const char* port = "9600"); // constructor

  // destructor to free everything
  ~Tracker() {
    if(tracker_info.addr) freeaddrinfo(tracker_info.addr);
    if(tracker_info.sockfd < 0) close(tracker_info.sockfd);
  } 
};


// returns list of configurations that can start an ipv4 tcp server to bind to
// returns NULL on error
// ---------------------------------------------------------------------
struct addrinfo* Tracker::make_connection_list(const char* port) {
  // init options
  struct addrinfo addropts = {};
  addropts.ai_family = AF_INET;       // ipv4
  addropts.ai_socktype = SOCK_STREAM; // tcp
  addropts.ai_flags = AI_PASSIVE;     // bind-able

  // make a list of all configurations with those options
  struct addrinfo* addrList = NULL;
  err_status = getaddrinfo(NULL, port, &addropts, &addrList) < 0;
  if(err_status) {
    fprintf(stderr, "ERROR - getaddrinfo -> %s:%d\n", __FILE__, __LINE__);
    return NULL;
  }
 
  return addrList; // success
}


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

  // success, record what time server started at
  start_time = time(NULL);
} 


// Print information about the server
// What it is, when it started, where it started, and how long it has been running, any errors?
// ---------------------------------------------------------------------
void Tracker::print_status() {
  fprintf(stderr, "server: tracker\n"); 

  // get date, or set skip that if start_time is negative
  string start_date = epoch_to_iso8601(start_time); 
  if(start_date.empty()) fprintf(stderr, "started: unkown\n");
  else fprintf(stderr, "\tstarted: %s\n", start_date.c_str());

  // get address, will already be empty on NULL
  string addr_string = addrinfo_to_string(tracker_info.addr);
  if(addr_string.empty()) fprintf(stderr, "address: unkown\n");
  else fprintf(stderr, "\taddress: %s\n", addr_string.c_str());

  // get uptime, you get the gist
  string uptime = epoch_to_uptime(start_time);
  if(uptime.empty()) fprintf(stderr, "uptime: unkown\n");
  else fprintf(stderr, "\tuptime: %s\n", uptime.c_str());

  // print error status
  fprintf(stderr, "\terror: %s\n", err_status ? "true" : "false");
}

#endif
