#ifndef UTIL_H
#define UTIL_H

// Utility functions and wrappers for general usage 
#include "../includes.hpp"


// Wrapper to calculate md5 easily
// Returns empty string on error
// ---------------------------------------------------------------------
std::string md5(const char* file) {
  char buf[MD5_DIGEST_STRING_LENGTH];
  MD5_CTX context = {};
  MD5Init(&context);
  MD5File(file, buf);
  MD5End(&context, NULL);
  return string(buf);
}


// For calling with c++ types
// ---------------------------------------------------------------------
string md5(string file) { return md5(file.c_str()); }
string md5(path file) { return md5(file.string()); }


// extended addrinfo struct to manage a network connection
// ---------------------------------------------------------------------
struct AddrinfoExt {
  struct addrinfo* addr = {};
  int sockfd = -1;
  pthread_t tid = {}; // thread id
};


// convert epoch from time(NULL) to iso-8601 date format string (YYYY/MM/DDTHH:MM:SS)
// string is empty on error
// ---------------------------------------------------------------------
string epoch_to_iso8601(time_t epoch) {
  if(epoch < 0) return ""; // sanitize
  struct tm date_utc = {};
  if (!gmtime_r(&epoch, &date_utc)) return "";
  constexpr size_t buflen = 20; // remind me to update it in the year 9999
  char bufstr[buflen] = "";
  strftime(bufstr, buflen, "%FT%T", &date_utc);
  return bufstr; // bufstr already set empty on strftime error
}


// convert epoch from time(NULL) to uptime in form "d..d days, hh hours, mm minutes, and ss seconds."
// returns empty on error (or NULL info)
// ---------------------------------------------------------------------
string epoch_to_uptime(time_t epoch) {
  // sanitize
  int current_time = time(NULL);
  if ((epoch < 0) || (current_time < 0) || (epoch > current_time)) return "";

  time_t diff = current_time - epoch;

  // split to days, hours, and minutes
  size_t days = diff / 24*60*60;
  diff -= days*24*60*60;
  size_t hours = diff / 60*60;
  diff -= hours*60*60;
  size_t minutes = diff / 60;
  diff -= minutes*60;

  // success, leftover diff is seconds
  string ret = to_string(days) + " days, ";
  ret += to_string(hours) + " hours, ";
  ret += to_string(minutes) + " minutes, and ";
  ret += to_string(diff) + " seconds.";
  return ret;
}


// convert binary address to string of form "127.0.0.1:9999" (as example)
// returns empty on error (or NULL info)
// ---------------------------------------------------------------------
string addrinfo_to_string(addrinfo* info) {
  if(!info) return NULL; // sanitize

  // get address and port
  char host[NI_MAXHOST], serv[NI_MAXSERV];
  int err_status = getnameinfo(info->ai_addr, info->ai_addrlen,
                        host, NI_MAXHOST, serv, NI_MAXSERV,
                        NI_NUMERICHOST | NI_NUMERICSERV);

  // error
  if(err_status) return ""; 

  // success, concat and return
  return string(host) + ":" + serv;
}



#endif
