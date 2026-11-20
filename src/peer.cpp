#include "lib/includes.hpp"
#include <cstring>
#include <iostream>
#include <map>

#ifndef TRACKER_ADDR
#define TRACKER_ADDR "127.0.0.1"
#endif

#ifndef TRACKER_PORT
#define TRACKER_PORT 10101
#endif

#ifndef PEER_PORT
#define PEER_PORT 10102
#endif

#define LISTEN_BACKLOG 50

int tracker_sockfd; //global server sockfd, dont override  

//TODO (in order, if done with one part debug it before going onto the next.):
//1. Finish Upload and Download methods
//    a. download and upload from and to the Files map in the "this_box"
//2. Interface with Baran's file parsing
//    a. Make method to do file parsing and saving
//    b. Get input from peer via files or whatever way they want us to get input. Make this a method.
//3. Multithreading
//    a. Have a thread maker method using the given mutexes.
//    b. implement mutex with parsing loop.
//4. Memory Mangement
//    a. Make a method for deleting thread resources.
//    b. Make a method for deleting IOBox resources.


struct IOBox {
  
  int thissockfd_Tracker;
  //const char* is ip
  //int is peerfd
  map<const char*,int> Peers;
  //const char* is name
  //FILE* is file
  map<const char*,FILE*> Files;
  //const char* is ip
  //int* is emergency stop, set value to 0 to stop.
  map<const char*,int*> Peers;

};

struct DownloadInfo {
  
  const char* FileName; //filename of thing to send or recieve
  const char* Ip; // ip of other peer

};

struct ThreadMutex {
  
  DownloadInfo info; //info for thing to download or recieve
  int* isWorking; //if the thread is currently working
  int* shouldbeWorking; // if the thread should be working

};

IOBox this_box; //this peer's box




// starts listen for this peer for SENDING TO SERVER
// -----------------------------------------------------------
void listen_from_server()
{
  // connect to tracker
  tracker_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {};
  socklen_t addrlen = sizeof addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(TRACKER_PORT);
  addr.sin_addr.s_addr = inet_addr(TRACKER_ADDR);
  int connect_status = connect(tracker_sockfd, (struct sockaddr*)&addr, addrlen);
  if(connect_status < 0) {
    fprintf(stderr, "ERROR_TRACKER -> connect\n");
  }
}

// starts listen for this peer for SENDING TO PEER, returns peer sock
// -----------------------------------------------------------
int listen_from_sock(const char* PEER_ADDR)
{
  
  // connect to sock
  int peer_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {};
  socklen_t addrlen = sizeof addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PEER_PORT);
  addr.sin_addr.s_addr = inet_addr(PEER_ADDR);
  int connect_status = connect(peer_sockfd, (struct sockaddr*)&addr, addrlen);
  if(connect_status < 0) {
    fprintf(stderr, "ERROR_PEER -> connect\n");
  }
  return peer_sockfd;
}

// start a thread where you upload some given file to a socket WIP
// -----------------------------------------------------------
void* upload(void* info)
{
  DownloadInfo* fil = (DownloadInfo*)(info);
  const ssize_t strlen = 1024;
  char pkt[strlen] = "";
  
}

// start a thread where you download a file from some socket WIP
// -----------------------------------------------------------
void* download(void* info)
{
  DownloadInfo* fil = (DownloadInfo*)(info);
  const ssize_t strlen = 1024;
  char pkt[strlen] = "";

  
}



void* IOBoxMaker(FILE* fp, const char* thisIp )
{
  //make this peer's identity for recieving from tracker
  int this_sockfd_tracker = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr_tracker = {};
  socklen_t addrlen_tracker = sizeof addr_tracker;
  addr_tracker.sin_family = AF_INET;
  addr_tracker.sin_port = htons(TRACKER_PORT);
  addr_tracker.sin_addr.s_addr = inet_addr(thisIp);

  
  
  // make that boxxxxx
  IOBox* bx = (IOBox*)(malloc(sizeof(IOBox)));
  bx->Peers = {}; //empty peer map.
  bx->thissockfd_Tracker = this_sockfd_tracker;
  bx->Files = {}; //empty file map

  // box...
  return ((void*)bx);
  
}


int main() {

 


  

}