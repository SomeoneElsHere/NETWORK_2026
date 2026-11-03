#pragma once

#include <cstdint>

namespace peer_to_peer
{
    /**
     * The maximum number of bytes in a segment to be transferred from machine to machine
     */
    static constexpr int16_t MAX_MESSAGE_BUFFER_SIZE = 1024;
    
    /**
     * The default port used by the tracker server for listening
     */
    static constexpr int16_t TRACKER_PORT = 10101;
}