#include "lib/includes.hpp"
#include <cstring>
#include <iostream>

// I hate this but easiest way is to make address/ports compile-time defined for the demo??
#ifndef TRACKER_ADDR
#define TRACKER_ADDR "127.0.0.1"
#endif

#ifndef TRACKER_PORT
#define TRACKER_PORT 10101
#endif

#define LISTEN_BACKLOG 50

struct IOBox {
  FILE* fp;
  int sockfd;
};

// start a server to listen on some port (should've done this on util.hpp)
// returns the socket
// -----------------------------------------------------------
int listen_from(const char* port);

// start a thread where you upload some given file to a socket
// -----------------------------------------------------------
void* upload(void* upload_box);

// start a thread where you download a file from some socket
// -----------------------------------------------------------
void* download(void* download_box);

// I hate this
// -----------------------------------------------------------
void handle_potential_get(stringstream& ss);


// Thinking a bit, I am not the one carrying its argc, argv from the shell
// main is not really the start point of a program when you think about it
// Thinking a bit, I am not the one carrying its return value to the shell
// main is not really the endin point of a program when you think about it
// So what is main? Just a quirky function like any other?? It's due today
int main() {

  // connect to tracker
  int tracker_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {};
  socklen_t addrlen = sizeof addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(TRACKER_PORT);
  addr.sin_addr.s_addr = inet_addr(TRACKER_ADDR);
  int connect_status = connect(tracker_sockfd, (struct sockaddr*)&addr, addrlen);
  if(connect_status < 0) {
    fprintf(stderr, "ERROR -> connect\n");
    return -1;
  }

  // init for I/O
  const ssize_t strlen = 1024;
  char strbuf[strlen] = "";

  // while getting user input
  for(string line = ""; getline(cin, line);) {
    // for later
    bool trashbin = true; // ignore
    vector<string> args = str_to_args(line, trashbin);

    // send
    line = encapsulate(line);
    send_string(tracker_sockfd, line);

    // get a reply
    int len = recv(tracker_sockfd, strbuf, strlen, 0);
    strbuf[len] = '\0';
    printf("%s", strbuf); // I know unsafe, due today

    // I know these should be their own functions, but due today
    // <createtracker succ>   -> we created a tracker rn, start hosting the file
    string rep = string(strbuf);
    if(rep.empty()) continue; // ????
    if(rep == "<createtracker succ>\n") { // I am hardcoding these atp
      FILE* fp = fopen(args[1].c_str(), "r");
      if(!fp) {
        fprintf(stderr, "ERROR -> fopen(\"./hello_world.txt\", \"r\")\n");
        continue;
      }
      // init peer 
      int peer_sockfd = listen_from(args.back().c_str());
      if(peer_sockfd < 0) {
        fclose(fp);
        continue;
      }
      // can now start the thread
      IOBox* upload_box = new IOBox {
        .fp = fp,
        .sockfd = peer_sockfd
      };
      pthread_t todo; // Literally named todo because holy shit is this a TODO
      pthread_create(&todo, NULL, upload, (void*)upload_box);
      continue;
    }

    // <GET ...>  -> we got a trackerfile, start downloading the file
    stringstream ss(rep); // more c++ yipeee
    handle_potential_get(ss);
  }
}


// start a thread where you allow people to download a file from
// --------------------------------------------------------------------------------
void* upload(void* upload_box) {
  constexpr char SUCCESS[] = "<SUCC>\n"; // If did not get this, other peer knows an error hapened
  // sanitize and init
  if(!upload_box) pthread_exit(NULL);
  IOBox* unbox = (IOBox*)upload_box;
  const int listen_sockfd = unbox->sockfd;
  FILE* fp = unbox->fp;
  int sockfd = -1;
  while((sockfd = accept(listen_sockfd, NULL, NULL)) >= 0) {
    // seek to start of the file
    fseek(fp, 0, SEEK_SET);
    // Tell them to start listening
    const size_t strlen = 1024;
    char strbuf[strlen] = "";
    send(sockfd, SUCCESS, sizeof SUCCESS, 0);
    recv(sockfd, strbuf, strlen, 0); // get ack
    while(fgets(strbuf, strlen, fp)) {
      if(send(sockfd, strbuf, strnlen(strbuf, strlen), 0)) continue;
      fprintf(stderr, "ERROR -> Socket closed while still sending the file!!\n");
      break;
    }

    // finish
    close(sockfd);
  }
  fclose(fp);
  close(listen_sockfd);
  pthread_exit(NULL); // would probably never reach here but meh
}

// start a thread where you download a file 
// --------------------------------------------------------------------------------
void* download(void* download_box) {
  constexpr char SUCCESS[] = "<SUCC>\n"; // If did not get this, other peer knows an error hapened
  // sanitize and init
  if(!download_box) pthread_exit(NULL);
  IOBox* unbox = (IOBox*)download_box;
  const int sockfd = unbox->sockfd;
  FILE* fp = unbox->fp;

  // Establish connection
  ssize_t len = -1;
  const size_t strlen = 1024;
  char strbuf[len] = "";
  len = recv(sockfd, strbuf, strlen, 0);
  strbuf[len] = '\0';
  send(sockfd, "ACK", sizeof "ACK", 0);
  bool err_status = (len < 0);
  if(!err_status) err_status = strncmp(strbuf, SUCCESS, sizeof SUCCESS) != 0;
  if(err_status) {
    fprintf(stderr, "ERROR -> recv\n");
    close(sockfd);
    fclose(fp);
    pthread_exit(NULL);
  }
  
  // Start listening and write to the file
  while((len = recv(sockfd, strbuf, strlen, 0)) > 0) {
    fwrite(strbuf, sizeof strbuf[0], len, fp);
  }

  // finish
  close(sockfd);
  fclose(fp);
  pthread_exit(NULL);
}


// start a server to listen on some port (should've done this on util.hpp)
// returns the socket
// --------------------------------------------------------------------------------
int listen_from(const char* port) {
  struct addrinfo* addrList = make_connection_list(port); 
  int sockfd = -1;

  // while going through the list
  for(struct addrinfo* addr = addrList; addr; addr = addrList->ai_next) {
    // try the socket
    sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if(sockfd < 0) continue;

    // get around the "bind address already in use"
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // try the bind
    int bind_status = bind(sockfd, addr->ai_addr, addr->ai_addrlen);
    if (bind_status >= 0) break; 
    close(sockfd); // failure, clean the socket and continue
  }

  // can now start listening
  int err_status = (sockfd >= 0) ? (listen(sockfd, LISTEN_BACKLOG) < 0) : 0;
  if(err_status) {
    fprintf(stderr, "ERROR - listen -> %s:%d\n", __FILE__, __LINE__);
    return -1;
  }

  // success, return it
  return sockfd;
}


// I hate this
// -----------------------------------------------------------
void handle_potential_get(stringstream& ss) {
  // I am not even commenting anymore
  string line = "";
  getline(ss, line);
  if(line != "<REP GET BEGIN>") return;
  int idx;
  string filename = "", addr = "", port = "";
  while(getline(ss, line)) {
    if(line.empty()) continue;
    if(line.front() == '#') continue;
    if(line.find("<REP GET END") != string::npos) break; 
    vector<string> args = strsplit(line, ' ');
    vector<string> addrinfo = strsplit(line, ':');
    if(args.size() == 2) {
      if(args.front() == "Filename:") filename = args.back();
      continue;
    }
    if(addrinfo.size() != 5) continue;
    addr = addrinfo[0];
    port = addrinfo[1];
    break;
  }
  if(filename.empty() || addr.empty() || port.empty() || (!valid_address(addr, port))) {
    fprintf(stderr, "Peer could not parse the trackerfile\n");
    return;
  }
  uintmax_t binary_port = strtoumax(port.c_str(), NULL, 10);
  cout << "File " << filename << " @ " << addr << ':' << port << endl;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addrin = {};
  socklen_t addrlen = sizeof addrin;
  addrin.sin_family = AF_INET;
  addrin.sin_port = htons(binary_port);
  addrin.sin_addr.s_addr = inet_addr(addr.c_str());
  int connect_status = connect(sockfd, (struct sockaddr*)&addrin, addrlen);
  if(connect_status < 0) {
    fprintf(stderr, "ERROR -> Could not connect to the peer at %s:%s\n", addr.c_str(), port.c_str());
    return;
  }
  FILE* fp = fopen(filename.c_str(), "w");
  IOBox *download_box = new IOBox {
    .fp = fp,
    .sockfd = sockfd
  };
  pthread_t todo; // Literally named todo because holy shit is this a TODO
  pthread_create(&todo, NULL, download, (void*)download_box);
  pthread_join(todo, NULL);
}
