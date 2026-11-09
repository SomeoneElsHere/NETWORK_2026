#ifndef UTIL_HPP
#define UTIL_HPP

// Utility functions and wrappers for general usage 
#include "../includes.hpp"

// returns list of configurations that can start an ipv4 tcp server to bind to
// returns NULL on error
// ---------------------------------------------------------------------
struct addrinfo* make_connection_list(const char* port) {
  // init options
  struct addrinfo addropts = {};
  addropts.ai_family = AF_INET;       // ipv4
  addropts.ai_socktype = SOCK_STREAM; // tcp
  addropts.ai_flags = AI_PASSIVE;     // bind-able

  // make a list of all configurations with those options
  struct addrinfo* addrList = NULL;
  int err_status = getaddrinfo(NULL, port, &addropts, &addrList) < 0;
  if(err_status) {
    fprintf(stderr, "ERROR - getaddrinfo -> %s:%d\n", __FILE__, __LINE__);
    return NULL;
  }
 
  return addrList; // success
}


// split a string into vectors around quotation marks.
// If double quotes used, string will accept single quotes and vice versa
// Also allows backslashes to use double quotes in double quotes.
// After the quotations, will split around whitespace.
// Allows commans to be used as seperators outside of quotations too
// Sets err_status=true on errors like unclosed quotations or unknown characters
// ---------------------------------------------------------------------
vector<string> str_to_args(string& str, bool& err_status) {
  // sanitize and init
  if(str.empty()) return vector<string>{};
  bool dquotes = false, quotes = false, escape = false;
  vector<string> ret = {};
  string current = "";

  // while looping over the string
  for(unsigned char c : str) {
    // special characters
    if((c == '"') && (!quotes) && (!escape)) dquotes = !dquotes;      // handle double quotations
    else if((c == '\'') && (!dquotes) && (!escape)) quotes = !quotes; // handle single quotations
    else if(c == '\\' && !escape) {escape = true; continue;}          // the escape/backslash 
    else if(isspace(c) && (!quotes) && (!dquotes) && (!escape)) {     // whitespace
      ret.push_back(current);
      current.clear();
    } else if(isprint(c)) current.push_back(c);                       // normal characters
    else {                                                            // unkown, unprintable characters
      err_status = true;
      fprintf(stderr, "WARNING - Invalid character \"%du\"-> %s:%d\n", c, __FILE__, __LINE__);
      return ret;
    } 
    escape = false;
  }

  // sanitize and push last string if there is any left
  if(quotes || dquotes || escape) {
    fprintf(stderr, "WARNING - Unclosed quotations or stray backslash -> %s:%d\n", __FILE__, __LINE__);
    err_status = true;
    return ret;
  }
  if(!current.empty()) ret.push_back(current);
  return ret;
}

// Wrapper to calculate md5 easily
// Returns empty string on error
// ---------------------------------------------------------------------
string md5(const char* file) {
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
string md5(path& file) { return md5(file.string()); }


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
  return "";
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


// get the port from addrinfo
// returns -1 on error
// ---------------------------------------------------------------------
long int port_from_addrinfo(addrinfo* info) {
  if(!info) return 0; // sanitize

  // get address and port
  char serv[NI_MAXSERV] = "";
  int err_status = getnameinfo(info->ai_addr, info->ai_addrlen,
                               NULL, 0, serv, NI_MAXSERV, NI_NUMERICSERV);
  if(err_status) return -1; // error

  // convert to base-10 int
  // TODO: sanitize this strtol
  char* endptr = NULL;
  long int ret = strtol(serv, &endptr, 10);

  // sanitize and return
  if(!endptr) return -1;
  if(*endptr) return -1;
  return ret;
}


// send string to socket
ssize_t send_string(int sockfd, string& msg) {
  return send(sockfd, msg.c_str(), msg.length(), 0);
}

// Turn MSG -> <MSG>\n
// ---------------------------------------------
string encapsulate(string msg) {
  const static string begin_mark = "<";
  msg = begin_mark + msg + ">\n";
  return msg;
}

// Turn <MSG>\n -> MSG
// Sets status false if wrong format, true otherwise.
// Accepts "<>\n" as correct empty string
// ---------------------------------------------
bool decapsulate(string& msg) {
  // '<'
  if(msg.empty()) return false;
  if(msg.front() != '<') return false;
  msg.erase(0, 1);

  // '\n'
  if(msg.empty()) return false;
  if(msg.back() != '\n') return false;
  msg.pop_back();

  // '>'
  if(msg.empty()) return false;
  if(msg.back() != '>') return false;
  msg.pop_back();

  // success
  return true;
}

// check if a given string is a valid number fast
// ---------------------------------------------
bool valid_uint(string& alleged_number) {
  // sanitize 
  if(alleged_number.empty()) return false;

  // convert and see
  errno = 0;
  char* endptr = NULL;
  strtoumax(alleged_number.c_str(), &endptr, 10); 
  return ((*endptr) == '\0') && (endptr != alleged_number.c_str()) && (!errno);
}


// check if a given string is a valid md5 fast
// ---------------------------------------------
bool valid_md5(string& alleged_md5) {
  // sanitize  and split md5 string into pieces
  if(alleged_md5.length() != 32) return false;
  for(char c : alleged_md5) {
    bool valid_letter = (c >= 'a') || (c >= 'A');
    valid_letter &= (c <= 'f') || (c <= 'F');
    valid_letter |= isdigit(c);
    if(!valid_letter) return false;
  }
  // success
  return true;
}


// Check if following are valid ip and port number
// ---------------------------------------------
bool valid_address(string ip, string port) {
  // sanitize and init
  if(!valid_uint(port)) return false; // fastest one
  struct sockaddr_in bin_ipv4;
  struct sockaddr_in6 bin_ipv6;

  // convert and see
  if(!inet_pton(AF_INET, ip.c_str(), &bin_ipv4)) return false;
  if(!inet_pton(AF_INET, ip.c_str(), &bin_ipv6)) return false;
  return true;
}

// split a string around some given char delim
// will assume multiple delims in a rows as empty strings
// -----------------------------------------------------
vector<string> strsplit(string& str, char delim = ' ') {
  //init
  vector<string> ret;
  string tmp = "";

  // process 
  for(char c : str) {
    if(c == delim) {
      ret.push_back(tmp);
      tmp.clear();
    } else tmp.push_back(c);
  }
  if(!tmp.empty()) ret.push_back(tmp);

  // finish
  return ret;
}


#endif
