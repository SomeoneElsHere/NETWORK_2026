#pragma once

#include "config.hpp"

using namespace std;
using namespace peer_to_peer::peer;

namespace peer_to_peer
{
    class Peer
    {
        private:
            /// The configuration values for the client thread of this peer
            PeerClientConfig client_config;
    
            /// The socket used for communicating with the tracker server
            int tracker_socket_fd;

        public:
            /**
             * Creates a new peer with the default client thread configuration
             */
            Peer();

            /**
             * Creates a new peer with a custom client thread configuration
             * @param clientConfig The configuration values for the client thread
             */
            Peer(PeerClientConfig clientConfig);

            /**
             * Clean up resources used by this peer
             */
            ~Peer();

            /**
             * Attempts to initialize the socket and tracker address this peer will use
             * @param error_code The memory location to store the error code, if creating the socket fails
             * @return true if creation of the socket was successful; otherwise, false
             */
            bool try_init_socket(int* error_code);

            /**
             * Attempts to connect to the tracker server using the created socket
             * @param error_code The memory location to store the error code, if the connection attempt fails
             * @return true if connection was successful; otherwise, false
             */
            bool try_connect(int* error_code);

            /**
             * Attempts to send a string message to the tracker
             * @param message The message to be sent
             * @param len The length of the message
             * @param error_code The memory location to store the error code, if the send attempt fails
             * @return true if the message was successfully sent; otherwise, false
             */
            bool try_send_to_tracker(std::string message, int len, int* error_code);

            /**
             * Closes the file descriptor for the socket connected to the tracker server, if its open
             */
            void close_socket();
    };
}