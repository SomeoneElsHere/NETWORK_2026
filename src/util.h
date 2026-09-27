#ifndef UTIL_H
#define UTIL_H

// Utility functions and wrappers for general usage 
#include "libmd/md5.h"
#include <filesystem>
#include <string>

using namespace std;
using namespace filesystem;

// Wrapper to calculate md5 easily
// Returns empty string on error
// -----------------------------------
string md5(const char* file) {
  char buf[MD5_DIGEST_STRING_LENGTH];
  MD5_CTX context = {};
  MD5Init(&context);
  MD5File(file, buf);
  MD5End(&context, NULL);
  return string(buf);
}

// For calling with c++ types
string md5(string file) { return md5(file.c_str()); }
string md5(path file) { return md5(file.string()); }


#endif
