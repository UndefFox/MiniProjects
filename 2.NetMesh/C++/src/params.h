#pragma once

#include "utils/socket.h"



class Params {
public:
    bool helpNeeded;
    Socket::port_t listenPort;
    std::string clientName;

public:
    Params() = default;
    Params(int argc, char* argv[]);
};
