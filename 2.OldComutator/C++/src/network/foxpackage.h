#pragma once

#include <cstdint>
#include <vector>
#include <string>


/**
 * @brief Contains all the packages supported by
 * FoxTunnel class.
 */
namespace FoxPackage {
typedef std::vector<std::byte> Raw;

#pragma pack(push, 1)
/**
 * @brief POD struct which sits at the start of
 * every package.
 */
struct Header {
public:
    typedef uint32_t BodySize;
    typedef uint32_t NodeId;
    enum Type : uint8_t {
        DiffieHellman = 0,
        Message,
        Routing
    };


public:
    BodySize size;
    NodeId sender;
    NodeId receiver;
    Type type;

public:
    static Header deserialize(const Raw& raw);
    Raw serialize() const;
};
#pragma pack(pop)
static_assert(std::is_trivial_v<Header> && std::is_standard_layout_v<Header>,
              "Header must be POD for binary serialization");


struct DiffieHellman : public Header {
public:
    uint64_t key;

public:
    DiffieHellman();
    DiffieHellman(const Raw& raw);
    Raw serialize();


private:
    static uint64_t getKey(const Raw& raw);
};

struct Message : public Header  {
public:
    std::string message;


public:
    Message();
    Message(const Raw& raw);
    Raw serialize();


private:
    static std::string getMessage(const Raw& raw);
};

struct Routing : public Header  {
public:
    enum Command : uint8_t {
        Reset = 0,
        Trace
    };


public:
    Command cmd;


public:
    Routing();
    Routing(const Raw& raw);
    Raw serialize();


private:
    static Command getCmd(const Raw& raw);
};

}
