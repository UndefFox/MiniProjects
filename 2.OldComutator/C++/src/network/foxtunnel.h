#pragma once

#include "socket.h"
#include "foxpackage.h"


/**
 * @brief Class implementing protocol for communication
 * between clients. All packages are automatically
 * encrypted with XOR algorithm.
 *
 * @see FoxPackage to see what messages can be sent.
 */
class FoxTunnel {
public:
    uint64_t key;
    int nodeId;

private:
    Socket socket;


public:
    FoxTunnel();

    /**
     * @brief Reads the next whole package.
     *
     * @return Raw data of the whole package.
     */
    FoxPackage::Raw read();
    /**
     * @brief Writes whole package to the socket.
     *
     * @param package Raw data of the entire package.
     */
    void write(const FoxPackage::Raw& package);

    void setSocket(Socket&& newSocket);
    const Socket& getSocket() const;
    Socket takeSocket();

    /**
     * @brief Starts handshake procedure with connected client.
     *
     * Both clients need to call this function at the same time.
     * No operations can be done in between.
     *
     * @param id Id of the node that calls the handshake.
     */
    void handshake(FoxPackage::Header::NodeId id);

private:
    void applyKey(FoxPackage::Raw& package);
};
