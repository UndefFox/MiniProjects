#pragma once

#include <string>


/**
 * @brief Stores all the configurable parameters of the
 * backend.
 */
struct Params {
    /** IP to listen for connections. */
    std::string ip;
    /** Port to listen for connections. */
    int port;
    /** Id of this node instance. */
    int nodeId;
    /** Was the -h flag passed. */
    bool showHelp: 1;


public:
    /**
     * @brief Parses application input flags and stores
     * them in their own fields. Doesn't check the validity
     * of the data.
     *
     * @param argc argc from main call.
     * @param argv argv from main call.
     */
    Params(int argc, char *argv[]);
};


