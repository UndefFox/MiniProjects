#include "protocols.h"

#include <arpa/inet.h>


#include <algorithm>
#include <cstring>



Protocols::Packet_t Protocols::pack(PacketType type, RawPayloadSpan_t payload, Enctryptor& enc) {
    Enctryptor::EncryptedBlock encrypted;

    if (enc.isPaired()) {
        encrypted = enc.encrypt(payload);
    }
    else {
        encrypted.cipher.resize(payload.size());
        std::copy(
            payload.begin(),
            payload.end(),
            encrypted.cipher.begin()
        );
    }

    const PacketSize_t bodySize =
        sizeof(PacketType) +
        encrypted.nonce.size() +
        encrypted.cipher.size() +
        encrypted.tag.size();

    const auto networkBodySize = htonl(bodySize);

    Packet_t packet;
    packet.resize(sizeof(networkBodySize) + bodySize);

    std::size_t offset = 0;

    std::memcpy(
        packet.data() + offset,
        &networkBodySize,
        sizeof(networkBodySize)
    );
    offset += sizeof(networkBodySize);

    packet[offset] = static_cast<std::byte>(type);
    offset += sizeof(PacketType);

    std::memcpy(
        packet.data() + offset,
        encrypted.nonce.data(),
        encrypted.nonce.size()
    );
    offset += encrypted.nonce.size();

    std::memcpy(
        packet.data() + offset,
        encrypted.tag.data(),
        encrypted.tag.size()
    );
    offset += encrypted.tag.size();

    std::copy(
        encrypted.cipher.begin(),
        encrypted.cipher.end(),
        packet.begin() + offset
    );


    return packet;
}

Protocols::PacketType Protocols::unpack(Protocols::Packet_t& packet, Enctryptor& enc) {
    std::size_t offset = 0;

    PacketSize_t packetSize;

    std::memcpy(
        &packetSize,
        packet.data(),
        sizeof(packetSize)
    );
    packetSize = ntohl(packetSize);
    offset += sizeof(PacketSize_t);

    const auto type = static_cast<PacketType>(std::to_integer<std::uint8_t>(packet[offset]));
    offset += sizeof(PacketType);

    Enctryptor::EncryptedBlock encrypted{};

    std::memcpy(
        encrypted.nonce.data(),
        packet.data() + offset,
        encrypted.nonce.size()
    );
    offset += encrypted.nonce.size();

    std::memcpy(
        encrypted.tag.data(),
        packet.data() + offset,
        encrypted.tag.size()
    );
    offset += encrypted.tag.size();

    encrypted.cipher.resize(packet.size() - offset);
    std::copy(
        packet.begin() + offset,
        packet.end(),
        encrypted.cipher.begin()
    );

    if (enc.isPaired()) {
        auto result = enc.decrypt(encrypted);
        packet.swap(result);
    }
    else {
        packet.swap(encrypted.cipher);
    }

    return static_cast<PacketType>(type);
}

Protocols::RawPayload_t Protocols::UserHandshake::start(Data data) {
    RawPayload_t result(data.pubKey.size() + data.name.size());

    std::memcpy(
        result.data(),
        data.pubKey.data(),
        data.pubKey.size()
    );

    std::memcpy(
        result.data() + data.pubKey.size(),
        data.name.data(),
        data.name.size()
    );

    return result;
}

Protocols::RawPayload_t Protocols::UserHandshake::accept(Data data) {
    return Protocols::UserHandshake::start(data);
}

Protocols::UserHandshake::Data Protocols::UserHandshake::retrive(RawPayloadSpan_t data) {
    Data result;

    std::memcpy(
        result.pubKey.data(),
        data.data(),
        result.pubKey.size()
    );

    result.name.resize(data.size() - result.pubKey.size());

    std::memcpy(
        result.name.data(),
        data.data() + result.pubKey.size(),
        result.name.size()
    );

    return result;
}


Protocols::RawPayload_t Protocols::TextMessage::sendMessage(std::string_view text) {
    RawPayload_t result(text.size());

    std::memcpy(
        result.data(),
        text.data(),
        result.size()
    );

    return result;
}

std::string Protocols::TextMessage::readMessage(RawPayloadSpan_t data) {
    std::string result(data.size(), ' ');

    std::memcpy(
        result.data(),
        data.data(),
        result.size()
    );

    return result;
}


