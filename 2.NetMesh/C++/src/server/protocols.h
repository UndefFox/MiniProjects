#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "enctryptor.h"



namespace Protocols {

typedef std::vector<std::byte> RawPayload_t;
typedef std::span<const std::byte> RawPayloadSpan_t;
typedef std::vector<std::byte> Packet_t;
typedef std::span<const std::byte> PacketSpan_t;
typedef std::uint32_t PacketSize_t;

enum class PacketType : std::uint8_t {
    UserHandshake_Start = 1,
    UserHandshake_Accept,
    TextMessage_Message
};

/** \brief Asembles the package from a raw payload.
 *
 *  First 4 are std::uint32_t sizeOfThePackage.
 *  Next byte is package type.
 *  The rest are raw payload.
 *
 *  Evrything but size of the Package is encrypted.
 */
Packet_t pack(PacketType type, RawPayloadSpan_t buff, Enctryptor& enc);
PacketType unpack(Packet_t& data, Enctryptor& enc);

/** \brief Used for estabilishing connection to a remote server with DiffieHellman key
 *  exchange.
 *
 *  Initiator calls start() to send initial public part of the key. Remote host calls
 *  accept() that applies DiffieHellman to the private key stored inside the State and
 *  return package to send back to the initiator. Initiator calls finish to get the
 *  symetrical key.
 */
namespace UserHandshake {
    struct Data {
        Enctryptor::EVP_PKEY_raw pubKey;
        std::string name;
    };

    RawPayload_t start(Data data);
    RawPayload_t accept(Data data);
    Data retrive(RawPayloadSpan_t data);
}

/** \brief Used for getting text messages across. Sender calls sendMessage()
 *  and reciever can get the text by calling readMessage on the packet.
 */
namespace TextMessage {
    RawPayload_t sendMessage(std::string_view text);
    std::string readMessage(RawPayloadSpan_t data);
}

}
