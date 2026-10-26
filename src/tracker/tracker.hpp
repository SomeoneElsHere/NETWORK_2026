#ifndef TRACKER_HPP
#define TRACKER_HPP

#include "../includes.hpp"
constexpr int LISTEN_BACKLOG = 50; // how many connections to listen for at once


class Tracker {
  // variables
  pthread_mutex_t worker_lock = PTHREAD_MUTEX_INITIALIZER; // workers will use this to stay sync'd
  list<TrackerFile> tracker_files = {};                    // list of tracker files.
  AddrinfoExt tracker_info = {};  // Info on self. Address, port, socket, etc.
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

  // Search for a tracker file and return the iterator to it.
  bool send_trackerfile(string& name, int sockfd);

private:
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
  char host[NI_MAXHOST];
  sockaddr_in addr = {};
  socklen_t alen = sizeof addr;

  while(true) {
    int client_sockfd = accept(tracker_info.sockfd, (sockaddr*)&addr, &alen);
    if(client_sockfd < 0) {
      fprintf(stderr, "ERROR - accept %d -> %s:%d\n", errno, __FILE__, __LINE__);
      continue; 
    }
    int name_error = getnameinfo((sockaddr*)&addr, alen, host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    if(!name_error) printf("New connection from %s\n", host);
    pthread_t tid;
    TrackerBox box = { tracker_files, worker_lock, client_sockfd };
    pthread_create(&tid, NULL, proc_tracker_all, &box);
  }
}


#endif

// TODO: modularize the proc_ functions
