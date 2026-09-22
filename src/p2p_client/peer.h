#pragma once
#include <stdio.h>
#include <thread>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

namespace network2026
{
    /// @brief A peer
    class Peer
    {
        private:
            /* Client */
            uint32_t _trackerIP;
            uint16_t _trackerPort;
            uint32_t _updatetrackerInterval;
            thread _clientThread;

            /* Server */
            uint16_t _serverPort;
            string _sharedPath;
            thread _serverThread;

        public:
            /// @brief Creates a new peer
            Peer();

            /// @brief Cleans up this peer
            ~Peer();

            /// @brief Initializes the client thread
            void openClient();

            /// @brief Initializes the server thread
            void openServer();

            /// @brief Closes the client thread
            void closeClient();

            /// @brief Closes the server thread
            void closeServer();
    };
}