#include "networking.hpp"
#include "strings.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

namespace peer_to_peer
{
    /* Adapted from Jankatay's "util.hpp" */

    ///
    ssize_t send_string(int sockfd, const string& msg) {
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

    ///
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
}