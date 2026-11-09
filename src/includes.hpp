// list of libraries we use

#ifndef INCLUDES_H
#define INCLUDES_H

// standard
#include <string>
#include <list>
#include <map>
#include <vector>
#include <fstream>
#include <filesystem>
#include <cinttypes> 
using namespace std;
using namespace filesystem;

// system
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

// external libraries, check references
#include "./libmd/md5.h"  // Distributed with most non-bsd unix-like OS'es but just in case

// our own headerfiles
#include "./mix/util.hpp"
// #include "./parser/trackerfile.hpp" not in use yet, still experimenting
#include "./tracker/files.hpp"
#include "./tracker/worker.hpp"
#include "./tracker/tracker.hpp"


#endif
