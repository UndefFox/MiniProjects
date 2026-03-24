#include "foxtunnel.h"

#include "diffiehellman.h"


FoxTunnel::FoxTunnel() :
    key(0),
    nodeId(0)
{ }

FoxPackage::Raw FoxTunnel::read() {
    FoxPackage::Raw headerRaw(sizeof(FoxPackage::Header));
    socket.read(headerRaw);

    FoxPackage::Raw headerRawUnencripted(headerRaw);
    applyKey(headerRawUnencripted);

    FoxPackage::Header header = FoxPackage::Header::deserialize(headerRawUnencripted);

    FoxPackage::Raw body(header.size);
    socket.read(body);

    FoxPackage::Raw output(headerRaw);
    output.insert(output.end(), body.begin(), body.end());

    applyKey(output);

    return output;
}

void FoxTunnel::write(const FoxPackage::Raw &package) {
    auto copy = package;

    applyKey(copy);

    socket.write(copy);
}

void FoxTunnel::setSocket(Socket &&newSocket) {
    socket = std::move(newSocket);
}

const Socket &FoxTunnel::getSocket() const {
    return socket;
}

Socket FoxTunnel::takeSocket() {
    return std::move(socket);
}

void FoxTunnel::handshake(FoxPackage::Header::NodeId id) {
    uint64_t newKey = ((uint64_t)std::rand() << 32 | (uint64_t)std::rand()) % 4294965296UL;
    uint64_t newKeyPublic = DiffieHellman::applyExpression(newKey);

    FoxPackage::DiffieHellman first{};
    first.key = newKeyPublic;
    first.sender = id;

    write(first.serialize());

    const auto responseRaw = read();
    FoxPackage::DiffieHellman response(responseRaw);
    nodeId = response.sender;

    key = DiffieHellman::applyExpression(newKey, response.key);
}

void FoxTunnel::applyKey(FoxPackage::Raw& package) {
    for (int i = sizeof(FoxPackage::Header); i < package.size(); i++) {
        package[i] ^= reinterpret_cast<const std::byte*>(&key)[i % sizeof(key)];
    }
}
