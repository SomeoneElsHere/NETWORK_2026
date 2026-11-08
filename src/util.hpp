#pragma once

// C++ Standard Library
#include <string>
#include <vector>
#include <cstdint>

// Unix Specification
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

namespace peer_to_peer
{
    /* Adapted from Jankatay's "util.hpp" */

    /**
     * Splits a single string into a vector containing multiple substrings
     * @param str The string to be split
     * @param delim The delimiter used to split the string; defaults to a space
     * @return A vector containing the substrings derived from the original string as split by the specified delimiter
     */
    vector<string> str_split(string& str, char delim = ' ');

    /**
     * Converts an epoch from time(NULL) to an ISO-8601 date format string (YYYY/MM/DDTHH:MM:SS)
     * @param epoch The time to be converted into a string
     * @return A string representation of the specified epoch in ISO-8601 format, if conversion was successful; otherwise, an empty string
     */
    string epoch_to_iso8601(time_t epoch);

    /**
     * Converts an epoch from time(NULL) to an uptime string in the form "d..d days, hh hours, mm minutes, and ss seconds."
     * @param epoch The time to be converted into a string
     * @return A string representation of the specified epoch in ISO-8601 format, if conversion was successful.
     * Returns an empty string if an error occurs or NULL info is encountered
     */
    string epoch_to_uptime(time_t epoch);

    /**
     * Converts a C++ string into a C-style string and sends it through a socket
     * @param sockfd The file descriptor of the socket to send the string through
     * @param msg The string to be converted and sent
     * @return The number of bytes sent through the socket
     */
    ssize_t send_string(int sockfd, string& msg);

    struct AddrinfoExt {
      struct addrinfo* addr = {};
      int sockfd = -1;
      pthread_t tid = {}; // thread id
    };

    /**
     * Retrieves a port from an addrinfo
     * @param info The addrinfo to retrieve a port from
     * @return The port stored in the specified addrinfo, if successful; otherwise, -1.
     */
    long int port_from_addrinfo(addrinfo* info);

    /**
     * Creates a list of configurations that an IPv4 TCP server can bind to
     * @param port The port to create a list of connections for
     * @return An addrinfo pointer containing a list of configurations usable with the specified port; otherwise, nullptr
     */
    addrinfo* make_connection_list(int16_t port);
}