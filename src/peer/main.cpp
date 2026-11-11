#include "../util/misc.hpp"
#include "config.hpp"
#include "peer.hpp"

#include <iostream>

using namespace std;
using namespace peer_to_peer;
using namespace peer_to_peer::peer;

const string commandstr_exit = "exit";

/// Startup: parse configs -> start server thread (if serverThreadConfig.cfg exists) -> connect to tracker; typed commands are wrapped in < > before sending
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

    // The server thread (serving file chunks to other peers) is configured by
    // serverThreadConfig.cfg; without it the peer runs as a client only
    optional<PeerServerConfig> serverConfig = try_parse_peer_server_config("./serverThreadConfig.cfg");
    if(serverConfig.has_value())
    {
        peer.set_server_config(serverConfig.value());
        if(!peer.try_start_server(&error_code))
        {
            cout << "Could not start this peer's server thread. Error code: " << error_code << endl;
            return 2;
        }
        cout << "Listening for peers on port " << serverConfig -> port
             << "; sharing folder \"" << serverConfig -> shared_directory << "\"." << endl;
    }
    else
        cout << "No serverThreadConfig.cfg found; running without sharing files to other peers." << endl;

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
    cout << "Type a command (e.g. \"REQ LIST\", \"GET file.txt.track\", \"createtracker ...\") or \"exit\" to quit." << endl;

    string message;
    bool sentSuccess;
    do
    {
        message.clear();
        getline(cin, message);
        if(message == commandstr_exit || message.empty())
            break;

        // Commands are typed manually without the angle brackets required
        // by the TCP protocol; wrap them here so the protocol message format
        // ("<command args>\n") is strictly adhered to on the wire
        if(message.front() != '<')
            message = "<" + message + ">\n";

        try
        {
            sentSuccess = peer.try_send_to_tracker(message, message.length(), &error_code);
            if(!sentSuccess)
                cout << "Message failed to send. Error code: " << error_code << endl;
        }
        catch(...)
        {
            sentSuccess = false;
            cout << "An exception was thrown while trying to send message." << endl;
        }
    } while(sentSuccess);

    peer.close_tracker_connection();
    peer.close_server();

    cout << "Connection to tracker closed." << endl;

    return 0;
}