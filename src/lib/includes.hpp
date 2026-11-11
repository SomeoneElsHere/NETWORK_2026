// list of libraries we use

#ifndef INCLUDES_HPP
#define INCLUDES_HPP

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
#include "md5.h"  // Distributed with most non-bsd unix-like OS'es but just in case

// our own headerfiles
#include "util.hpp"
#include "files.hpp"
#include "worker.hpp"

#endif
