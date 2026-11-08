#pragma once

#include "../constants.hpp"
#include "config.hpp"

// C++ Standard Library
#include <atomic>
#include <chrono>
#include <map>
#include <thread>

// Unix Specification
#include <arpa/inet.h>
#include <unistd.h>

using namespace std::chrono;
using namespace std::chrono_literals;

using namespace peer_to_peer::tracker;

namespace peer_to_peer
{
    class Tracker
    {
        public:
            /// The maximum number of peers that can be connected to a tracker server concurrently
            static constexpr int16_t MAX_PEERS = 256;
    
            /// The time between iterations of the peer listening loop
            static constexpr duration PEER_LISTEN_DELAY = 10ms;
    
            /// The time between iterations of the peer thread management loop
            static constexpr duration PEER_MANAGE_DELAY = 10ms;
    
            /// The time between iterations of each peer's work loop
            static constexpr duration PEER_WORK_DELAY = 1ms;

        private:
            /**
             * Information representing a peer connected to a tracker server
             */
            struct peer_connection
            {
                /// The file descriptor for this peer's socket
                int peer_fd;
                /// The string containing this peer's IP address
                const string peer_address;
                /// The last message received from this peer
                optional<string> last_message = nullopt;
                /// The worker thread used to communicate with this peer
                thread* worker_thread = nullptr;
                /// Whether this peer has disconnected and the worker thread has exited
                bool completed = false;
                /// The index this struct occupies in "connected_peers", for debugging purposes.
                int DEBUG_peerarray_index;
            };

            // Tracker server properties

            /// The configuration values for this tracker server
            TrackerServerConfig tracker_config;

            /// The self-appointed IP address used by this tracker server
            sockaddr_in tracker_addr;

            /// The file descriptor for the listening socket of this tracker server
            int tracker_socket_fd;

            // Per-peer management data

            /// A mapping between IP address strings and the slot the respective peer's connection data occupies
            map<string, int> ip_index_mappings;

            /**
             * A set of slots used to store each peer's connection data.
             * If a slot returns nullopt, that slot is empty and can be used to accept a new peer.
             */
            optional<peer_connection*> connected_peers[MAX_PEERS];

            /// The next available slot in "connected_peers" that can be filled with a peer's connection data
            atomic<int>* next_peer_index;

            /// The thread used to listen for and accept/decline peer connections
            thread* peer_listener;

            /// The thread used to dispose of terminated peer connection data and free up slots for new peers
            thread* peer_connection_manager;

            /// A server-wide kill switch used to terminate all existing peer connections and clean up resources
            atomic<bool>* killswitch;

            /**
             * Listens for potential peers until the killswitch is activated
             * @param on_error The function to invoke if/when an error occurs
             */
            void listen_for_peers(NET_ERROR_HANDLER on_error);

            /**
             * Checks for the lowest open peer slot and finishes clearing memory occupied by completed peer connections. Repeats this process until the killswitch is activated.
             */
            void manage_peer_connections();

            /**
             * Initializes a peer connection and starts a worker thread for it
             * @param peer_fd The file descriptor for the peer's socket
             * @param peer_ipv4 The IPv4 address of the peer
             */
            void open_peer_connection(int peer_fd, sockaddr_in peer_ipv4);

            /**
             * Receives messages from a peer until its connection closes
             * @param connection_data The connection data for the peer this worker thread is for
             */
            void peer_worker_task(peer_connection* connection_data);

            /**
             * Common code used in both constructors
             */
            void init();

        public:
            /**
             * Creates a new tracker server with the default configuration
             */
            Tracker();

            /**
             * Creates a new tracker server with a custom configuration
             * @param trackerConfig The configuration values for the tracker server
             */
            Tracker(TrackerServerConfig trackerConfig);

            /**
             * Clean up resources used by this tracker server
             */
            ~Tracker();

            /**
             * Determines whether this tracker server is currently listening for peers
             */
            bool is_listening();

            /**
             * Attempts to initialize the socket this tracker server will use
             * @return true if the tracker server is ready to listen; otherwise, false
             */
            bool try_init();

            /**
             * Starts a thread that listens for peers and creates worker threads when a connection with a peer is established
             * @param on_error The function to call if an error occurs while listening
             * @return true if the thread is started; otherwise, false
             */
            bool try_listen_for_peers(NET_ERROR_HANDLER on_error);

            /**
             * Closes all existing connections with peers and shuts down the tracker server
             */
            void close_server();
    };
}
