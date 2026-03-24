#include "foxpackage.h"

#include <netinet/in.h>
#include <algorithm>


#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))


namespace FoxPackage {


// ---------- Header ----------

Header Header::deserialize(const Raw &raw) {
    Header output;

    std::copy(raw.data(), raw.data() + sizeof(Header), (std::byte*)&output);

    output.size = ntohl(output.size);
    output.receiver = htonl(output.receiver);
    output.sender = htonl(output.sender);

    return output;
}

Raw Header::serialize() const {
    Header copy = *this;

    copy.size = htonl(copy.size);
    copy.receiver = htonl(copy.receiver);
    copy.sender = htonl(copy.sender);

    const auto p = reinterpret_cast<const std::byte*>(&copy);
    return Raw(p, p + sizeof(Header));
}


// ---------- Message ----------

Message::Message() :
    message("")
{ }

Message::Message(const Raw &raw) :
    message(getMessage(raw)),
    Header(Header::deserialize(raw))
{ }

Raw Message::serialize() {
    Raw output;
    const Header::BodySize bodySize = message.size();
    const size_t packageSize = bodySize + sizeof(Header);
    output.resize(packageSize);

    size_t offset = 0;

    Header header{
        .size = bodySize,
        .sender = sender,
        .receiver = receiver,
        .type = Header::Type::Message
    };
    const auto rawHeader = header.serialize();
    std::copy(rawHeader.begin(), rawHeader.end(), (std::byte*)output.data() + offset);
    offset += sizeof(Header);

    std::copy(reinterpret_cast<const std::byte*>(message.begin().base()),
              reinterpret_cast<const std::byte*>(message.end().base()),
              output.data() + offset);
    offset += message.size();

    return output;
}

std::string Message::getMessage(const Raw &raw) {
    return std::string(reinterpret_cast<const char*>(raw.data() + sizeof(Header)), raw.size() - sizeof(Header));
}

// ---------- DiffieHellman ----------

DiffieHellman::DiffieHellman() :
    key(0)
{ }

DiffieHellman::DiffieHellman(const Raw &raw) :
    key(getKey(raw)),
    Header(Header::deserialize(raw))
{ }

Raw DiffieHellman::serialize() {
    Raw output;
    const Header::BodySize bodySize = sizeof(key);
    const size_t packageSize = bodySize + sizeof(Header);
    output.resize(packageSize);

    size_t offset = 0;

    Header header{
        .size = bodySize,
        .sender = sender,
        .receiver = receiver,
        .type = Header::Type::DiffieHellman
    };
    const auto rawHeader = header.serialize();
    std::copy(rawHeader.begin(), rawHeader.end(), (std::byte*)output.data() + offset);
    offset += sizeof(Header);

    uint64_t keyTemp = ntohll(key);

    std::copy(reinterpret_cast<const std::byte*>(&keyTemp),
              reinterpret_cast<const std::byte*>(&keyTemp) + sizeof(keyTemp),
              output.data() + offset);
    offset += sizeof(keyTemp);

    return output;
}

uint64_t DiffieHellman::getKey(const Raw &raw) {
    uint64_t output;
    const auto keyStart = raw.data() + sizeof(Header);

    std::copy(keyStart, keyStart + sizeof(output), reinterpret_cast<std::byte*>(&output));
    output = ntohll(output);

    return output;
}

// ---------- Routing ----------

Routing::Routing() :
    cmd(Command::Reset)
{ }

Routing::Routing(const Raw &raw) :
    cmd(getCmd(raw)),
    Header(Header::deserialize(raw))
{ }

Raw Routing::serialize() {
    Raw output;
    const Header::BodySize bodySize = sizeof(cmd);
    const size_t packageSize = bodySize + sizeof(Header);
    output.resize(packageSize);

    size_t offset = 0;

    Header header{
        .size = bodySize,
        .sender = sender,
        .receiver = receiver,
        .type = Header::Type::Routing
    };
    const auto rawHeader = header.serialize();
    std::copy(rawHeader.begin(), rawHeader.end(), (std::byte*)output.data() + offset);
    offset += sizeof(Header);

    uint8_t cmdTemp = cmd;

    std::copy(reinterpret_cast<const std::byte*>(&cmdTemp),
              reinterpret_cast<const std::byte*>(&cmdTemp) + sizeof(cmdTemp),
              output.data() + offset);
    offset += sizeof(cmdTemp);

    return output;
}

Routing::Command Routing::getCmd(const Raw &raw) {
    Command output;
    const auto cmdStart = raw.data() + sizeof(Header);

    std::copy(cmdStart, cmdStart + sizeof(output), reinterpret_cast<std::byte*>(&output));

    return output;
}

} // namespace FoxPackage
