#include "util/misc.hpp"
#include "config.hpp"
#include "peer.hpp"

#include <iostream>

using namespace std;
using namespace peer_to_peer;
using namespace peer_to_peer::peer;

const string commandstr_exit = "exit";

int main()
{
    optional<PeerClientConfig> clientConfig = try_parse_peer_client_config("./clientThreadConfig.cfg");
    if(!clientConfig.has_value())
    {
        cout << "The specified clientThreadConfig file could not be read. Does it exist, and is it formatted properly?" << endl;
        return 1;
    }

    int error_code = 0;
    Peer peer(clientConfig.value());

    cout << "Attempting to open the socket..." << endl;
    if(!peer.try_init_socket(&error_code))
    {
        cout << "Could not open a socket for connection. Error code: " << error_code << endl;
        return 2;
    }
    
    cout << "Attempting to establish a connection..." << endl;
    if(!peer.try_connect(&error_code))
    {
        cout << "Connection could not be successfully established. Error code: " << error_code << endl;
        return 3;
    }

    cout << "Connection successfully established." << endl;
    
    string message;
    bool sentSuccess;
    do {
        message.clear();
        getline(cin, message);
        if(message == commandstr_exit)
            break;

        try
        {
            sentSuccess = peer.try_send_to_tracker(message, message.length() - 1, &error_code);
            if(!sentSuccess)
                cout << "Message failed to send. Error code: " << error_code << endl;
        }
        catch (...) {
            sentSuccess = false;
            cout << "An exception was thrown while trying to send message." << endl;
        }
    }
    while(sentSuccess);

    peer.close_peer();

    return 0;
}