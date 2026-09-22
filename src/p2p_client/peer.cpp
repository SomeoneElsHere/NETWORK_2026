#include <iostream>
#include <stdio.h>
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>

#include "peer.h"

using namespace std;
using namespace network2026;

/* Peer implementations */
Peer::Peer()
{
    this -> _trackerIP = 127 << 24 | 1;
    this -> _trackerPort = 3490;
}

Peer::~Peer()
{
    
}

/* Peer program entry method */
int main(int argc, char* argv[])
{
    unique_ptr<Peer> peer = make_unique<Peer>();

    cout << "Hello, world!" << endl;
    return 0;
}