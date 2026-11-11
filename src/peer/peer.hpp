#pragma once

#include "config.hpp"

// C++ Standard Library
#include <atomic>
#include <chrono>
#include <thread>

// Unix Specification
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace peer_to_peer::peer;

namespace peer_to_peer
{
    class Peer
    {
        public:
            /// The time between iterations of the tracker listening loop
            static constexpr duration TRACKER_LISTEN_DELAY = 10ms;

        private:
            /// The configuration values for the client thread of this peer
            PeerClientConfig client_config;
    
            /// The socket used for communicating with the tracker server
            int tracker_socket_fd;

            /// The thread used to listen for responses sent by the tracker server
            thread* tracker_listener;

            /// Whether this peer's tracker listening thread is still active
            bool actively_listening;

            /// A kill switch used to terminate the connection to the tracker server and clean up resources
            atomic<bool> killswitch;

            /// Whether the tracker listener thread has completed
            //bool completed;

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

        private:
             /**
             * Receives messages from a tracker server until its connection closes
             */
            void listen_to_tracker();

        public:
            /**
             * Determines whether this peer is still listening for messages from the tracker
             * @return true if the tracker listening thread is still active; otherwise, false
             */
            bool is_listening();

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
             * Closes the connection with the tracker server (if one was established)
             * and closes the socket (if one was opened)
             */
            void close_peer();
    };
}