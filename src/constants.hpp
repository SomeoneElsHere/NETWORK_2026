#pragma once

#include <string>
#include <optional>

using namespace std;

namespace peer_to_peer
{
    /// The maximum number of bytes in a segment to be transferred from machine to machine
    static constexpr int16_t MAX_MESSAGE_BUFFER_SIZE = 1024;
    
    /// The default port used by the tracker server for listening
    static constexpr int16_t TRACKER_PORT = 10101;

    /**
     * A function that is called if an error code is returned from a networking function
     * @param error_code The error code that was encountered
     * @param fatal Whether the error resulted in the calling thread/method's termination
     * @param context_str An optional string that describes the context the error occurred in
     */
    typedef void (*NET_ERROR_HANDLER)(int error_code, bool fatal, optional<string> context_str);
}