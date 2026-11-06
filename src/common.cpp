/* Adapted from Jankatay's "util.hpp" */
#include "common.hpp"
#include "global_includes.hpp"

// standard library
#include <cstdio>
#include <ctime>

// unix
#include <stdlib.h>

constexpr int iso8601_buffer_length = 20; // remind me to update it in the year 9999

constexpr int seconds_per_minute = 60;
constexpr int seconds_per_hour = seconds_per_minute * 60;
constexpr int seconds_per_day = seconds_per_hour * 24;

using namespace std;

namespace peer_to_peer
{
    ///
    vector<string> str_split(string& str, char delim)
    {
        // init
        if(str.empty()) return vector<string>{};
        vector<string> ret = {};
        int prev = 0;

        // loop over finding the substr and adding to ret
        for(int tmp = 0; tmp < std::string::npos; prev = tmp+1) {
          tmp = str.find(delim, prev);
          if(tmp >= std::string::npos) break;
          size_t len = tmp - prev;
          ret.push_back(str.substr(prev, len));
        }
    
        // finish return
        ret.push_back(str.substr(prev));
        return ret;
    }

    ///
    string epoch_to_iso8601(time_t epoch) {
      if(epoch < 0) return ""; // sanitize

      struct tm date_utc = {};
      if (!gmtime_r(&epoch, &date_utc)) return "";

      char bufstr[iso8601_buffer_length] = "";

      strftime(bufstr, iso8601_buffer_length, "%FT%T", &date_utc);
      return bufstr; // bufstr already set empty on strftime error
    }

    ///
    string epoch_to_uptime(time_t epoch) {
      // sanitize
      int current_time = time(NULL);
      if ((epoch < 0) || (current_time < 0) || (epoch > current_time)) return "";

      time_t diff = current_time - epoch;

      // split to days, hours, and minutes
      size_t days = diff / seconds_per_day;
      diff -= days * seconds_per_day;
      size_t hours = diff / seconds_per_hour;
      diff -= hours * seconds_per_hour;
      size_t minutes = diff / seconds_per_minute;
      diff -= minutes * seconds_per_minute;

      // success, leftover diff is seconds
      string ret = to_string(days) + " days, ";
      ret += to_string(hours) + " hours, ";
      ret += to_string(minutes) + " minutes, and ";
      ret += to_string(diff) + " seconds.";
      return ret;
    }

    ///
    ssize_t send_string(int sockfd, string& msg) {
        return send(sockfd, msg.c_str(), msg.length(), 0);
    }

    ///
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

    ///
    addrinfo* make_connection_list(int16_t port) {
      // init options
      struct addrinfo addropts = {};
      addropts.ai_family = AF_INET;       // ipv4
      addropts.ai_socktype = SOCK_STREAM; // tcp
      addropts.ai_flags = AI_PASSIVE;     // bind-able
    
      // Convert port from 16-bit integer to C string
      char port_str[6] = "";
      snprintf(port_str, sizeof(port_str), "%d", port);

      // make a list of all configurations with those options
      addrinfo* addrList = nullptr;
      int err_status = getaddrinfo(NULL, port_str, &addropts, &addrList) < 0;
      if(err_status) {
        fprintf(stderr, "ERROR - getaddrinfo -> %s:%d\n", __FILE__, __LINE__);
        return nullptr;
      }
    
      return addrList; // success
    }
}