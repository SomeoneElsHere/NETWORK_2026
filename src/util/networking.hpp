#pragma once

// C++ Standard Library
#include <string>

// Unix Specification
#include <sys/types.h>
#include <netdb.h>

using namespace std;

namespace peer_to_peer
{
    /// The maximum number of bytes in a message to be transferred from machine to machine
    static constexpr int16_t MAX_MESSAGE_BUFFER_SIZE = 255;

    /// The maximum number of bytes in a file segment to be transferred from machine to machine
    static constexpr int16_t MAX_CHUNK_SIZE = 1024;
    
    /// The default port used by the tracker server for listening
    static constexpr uint16_t TRACKER_PORT = 10101;

    /**
     * Attempts to receive a string from a networked socket
     * @param sockfd The socket to try receiving a string from
     * @param buffer The raw character buffer to store the string's characters in
     * @param buffer_size The size of the specified buffer
     * @param received_str The memory location to store the received string; pass nullptr to skip creation of the string
     * @return true if bytes were received and a string could be created; false if 0 bytes were received (usually indicates that socket is terminated)
     */
    bool try_receive_string(int sockfd, char buffer[], size_t buffer_size, string* received_str);

    /* Adapted from Jankatay's "util.hpp" */
    
    /**
     * Converts a C++ string into a C-style string and sends it through a socket
     * @param sockfd The file descriptor of the socket to send the string through
     * @param msg The string to be converted and sent
     * @return The number of bytes sent through the socket
     */
    ssize_t send_string(int sockfd, const string& msg);

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
    addrinfo* make_connection_list(uint16_t port);

    /**
     * Check if the specified IP address and port strings follow their respective formats
     * @param ip The string containing the IP address
     * @param port The string containing the port
     * @return true if both the IP address and port strings are correctly formatted; otherwise, false
     */
    bool valid_address(string ip, string port);
}