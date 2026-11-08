#include "../constants.hpp"
#include "../util.hpp"
#include "peer.hpp"

#include <unistd.h>

using namespace std;
using namespace peer_to_peer::peer;

/**
 * Checks if a fail condition passed, and if it did, sets an error code pointer if one was specified
 * @param fail_condition The result of the fail condition
 * @param error_code The memory location to store the error code, if the fail condition passes
 * @return false if fail_condition is true; otherwise, true
 */
static bool check_for_error(bool fail_condition, int* error_code)
{
    if(fail_condition)
    {
        if(error_code != nullptr)
            *error_code = errno;
    
        return false;
    }

    return true;
}

namespace peer_to_peer
{
    Peer::Peer()
    {
        sockaddr_in default_addr;
        
        default_addr.sin_family = AF_INET;
        default_addr.sin_port = htons(TRACKER_PORT);
        default_addr.sin_addr.s_addr = INADDR_ANY;
        
        this -> client_config = PeerClientConfig{
            default_addr,
            1
        };
        this -> tracker_socket_fd = -1;
    }
    
    Peer::Peer(PeerClientConfig clientConfig)
    {
        this -> client_config = clientConfig;
        this -> tracker_socket_fd = -1;
    }

    Peer::~Peer()
    {
        if(this -> tracker_socket_fd >= 0)
            this -> close_socket();
    }

    bool Peer::try_init_socket(int* error_code)
    {
        if(this -> tracker_socket_fd >= 0)
            return false;
    
        tracker_socket_fd = socket(
            AF_INET,
            SOCK_STREAM,
            0
        );
    
        return check_for_error(tracker_socket_fd < 0, error_code);
    }

    bool Peer::try_connect(int* error_code)
    {
        if(this -> tracker_socket_fd < 0)
            return false;
    
        sockaddr* tracker_socket_addr = (sockaddr*)(&this -> client_config.tracker_addr);
    
        int success = connect(
            this -> tracker_socket_fd,
            tracker_socket_addr,
            sizeof(this -> client_config.tracker_addr)
        );
    
        return check_for_error(success == -1, error_code);
    }

    bool Peer::try_send_to_tracker(std::string message, int len, int* error_code)
    {
        if(this -> tracker_socket_fd < 0)
            return false;
    
        std::string content = (std::to_string(len) +":"+ message);
        size_t sentBytes = send_string(
            this -> tracker_socket_fd, 
            content
        );
    
        return check_for_error(sentBytes == -1, error_code);
    }

    void Peer::close_socket()
    {
        if(this -> tracker_socket_fd < 0)
            return;
    
        close(this -> tracker_socket_fd);
        this -> tracker_socket_fd = -1;
    }
}