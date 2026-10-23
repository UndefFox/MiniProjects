#include "enctryptor.h"

#include <algorithm>
#include <stdexcept>
#include <memory>



namespace {
struct EVP_PKEY_CTX_Deleter {
    void operator()(EVP_PKEY_CTX* context) const noexcept { EVP_PKEY_CTX_free(context); }
};

typedef std::unique_ptr<EVP_PKEY_CTX, EVP_PKEY_CTX_Deleter> EVP_PKEY_CTX_ptr;
}


void Enctryptor::initialize(bool isReciever) {
    EVP_PKEY_CTX_ptr context{EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr)};
    if (!context) throw std::runtime_error("EVP_PKEY_CTX_new_id failed");

    if (EVP_PKEY_keygen_init(context.get()) <= 0) {
        throw std::runtime_error("EVP_PKEY_keygen_init failed");
    }

    EVP_PKEY* privateKey = nullptr;

    if (EVP_PKEY_keygen(context.get(), &privateKey) <= 0) {
        throw std::runtime_error("EVP_PKEY_keygen failed");
    }

    m_privateKey = EVP_PKEY_ptr{privateKey};

    if (isReciever) {
        m_encryptNonce[NonceSize - 1] = 0x01u;
    }
    else {
        m_encryptNonce[NonceSize - 1] = 0x02u;
    }
}

Enctryptor::EVP_PKEY_raw Enctryptor::getPublickKey() {
    if (!m_privateKey) {
        throw std::logic_error("Failed to get publick key: private key is unitialized.");
    }

    EVP_PKEY_raw result;
    std::size_t length = result.size();

    if (EVP_PKEY_get_raw_public_key(
            m_privateKey.get(),
            result.data(),
            &length) <= 0
    ) {
        throw std::runtime_error("EVP_PKEY_get_raw_public_key failed");
    }

    if (length != result.size()) {
        throw std::runtime_error("unexpected X25519 public key length");
    }

    return result;
}

void Enctryptor::pair(const EVP_PKEY_raw& publicKey) {
    if (!m_privateKey) {
        throw std::logic_error("Failed to get paired key: private key is unitialized.");
    }

    EVP_PKEY_ptr peerPublicKey{
        EVP_PKEY_new_raw_public_key(
            EVP_PKEY_X25519,
            nullptr,
            publicKey.data(),
            publicKey.size())
    };
    if (!peerPublicKey) throw std::runtime_error("EVP_PKEY_new_raw_public_key failed");

    EVP_PKEY_CTX_ptr context{
        EVP_PKEY_CTX_new(m_privateKey.get(), nullptr)
    };
    if (!context) throw std::runtime_error("EVP_PKEY_CTX_new failed");

    if (EVP_PKEY_derive_init(context.get()) <= 0) {
        throw std::runtime_error("EVP_PKEY_derive_init failed");
    }

    if (EVP_PKEY_derive_set_peer(context.get(), peerPublicKey.get()) <= 0) {
        throw std::runtime_error("EVP_PKEY_derive_set_peer failed");
    }

    std::size_t sharedSecretLength = m_sharedSecret.size();

    if (EVP_PKEY_derive(
            context.get(),
            m_sharedSecret.data(),
            &sharedSecretLength) <= 0
    ) {
        throw std::runtime_error("EVP_PKEY_derive failed");
    }

    if (sharedSecretLength != m_sharedSecret.size()) {
        throw std::runtime_error("unexpected shared-secret length");
    }

    const bool allZero = std::all_of(m_sharedSecret.begin(), m_sharedSecret.end(), [](unsigned char byte) {
        return byte == 0;
    });
    if (allZero) throw std::runtime_error("invalid X25519 shared secret");

    m_paired = true;
}

bool Enctryptor::isPaired() const {
    return m_paired;
}

Enctryptor::EncryptedBlock Enctryptor::encrypt(std::span<const std::byte> data) {
    EVP_CIPHER_CTX_ptr context{
        EVP_CIPHER_CTX_new()
    };
    if (!context) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    if (EVP_EncryptInit_ex(
            context.get(),
            EVP_aes_256_gcm(),
            nullptr,
            nullptr,
            nullptr) != 1
        ) {
        throw std::runtime_error("EVP_EncryptInit_ex cipher selection failed");
    }

    if (EVP_EncryptInit_ex(
            context.get(),
            nullptr,
            nullptr,
            reinterpret_cast<const unsigned char*>(m_sharedSecret.data()),
            reinterpret_cast<const unsigned char*>(m_encryptNonce.data())) != 1
        ) {
        throw std::runtime_error("EVP_EncryptInit_ex key and nonce failed");
    }

    EncryptedBlock result;
    result.cipher.resize(data.size());

    int ciphertextLength = 0;

    if (EVP_EncryptUpdate(
            context.get(),
            reinterpret_cast<unsigned char*>(result.cipher.data()),
            &ciphertextLength,
            reinterpret_cast<const unsigned char*>(data.data()),
            static_cast<int>(data.size())) != 1
        ) {
        throw std::runtime_error("EVP_EncryptUpdate failed");
    }

    int finalLength = 0;

    if (EVP_EncryptFinal_ex(
            context.get(),
            reinterpret_cast<unsigned char*>(result.cipher.data()) + ciphertextLength,
            &finalLength) != 1
        ) {
        throw std::runtime_error("EVP_EncryptFinal_ex failed");
    }

    result.cipher.resize(static_cast<std::size_t>(ciphertextLength + finalLength));

    if (EVP_CIPHER_CTX_ctrl(
            context.get(),
            EVP_CTRL_GCM_GET_TAG,
            static_cast<int>(result.tag.size()),
            result.tag.data()) != 1
        ) {
        throw std::runtime_error("EVP_CTRL_GCM_GET_TAG failed");
    }

    result.nonce = m_encryptNonce;
    incrementNonce(m_encryptNonce);

    return result;
}

std::vector<std::byte> Enctryptor::decrypt(const EncryptedBlock &block) {
    EVP_CIPHER_CTX_ptr context{
        EVP_CIPHER_CTX_new()
    };
    if (!context) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    if (EVP_DecryptInit_ex(
            context.get(),
            EVP_aes_256_gcm(),
            nullptr,
            nullptr,
            nullptr) != 1
    ) {
        throw std::runtime_error("EVP_DecryptInit_ex cipher selection failed");
    }

    if (EVP_DecryptInit_ex(
            context.get(),
            nullptr,
            nullptr,
            reinterpret_cast<const unsigned char*>(m_sharedSecret.data()),
            reinterpret_cast<const unsigned char*>(block.nonce.data())) != 1
    ) {
        throw std::runtime_error("EVP_DecryptInit_ex key and nonce failed");
    }

    std::vector<std::byte> plaintext;
    plaintext.resize(block.cipher.size());

    int plaintextLength = 0;

    if (EVP_DecryptUpdate(
            context.get(),
            reinterpret_cast<unsigned char*>(plaintext.data()),
            &plaintextLength,
            reinterpret_cast<const unsigned char*>(block.cipher.data()),
            static_cast<int>(block.cipher.size())) != 1
    ) {
        throw std::runtime_error("EVP_DecryptUpdate failed");
    }

    if (EVP_CIPHER_CTX_ctrl(
            context.get(),
            EVP_CTRL_GCM_SET_TAG,
            static_cast<int>(block.tag.size()),
            const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(block.tag.data()))) != 1
    ) {
        throw std::runtime_error("EVP_CTRL_GCM_SET_TAG failed");
    }

    int finalLength = 0;

    const int authenticated = EVP_DecryptFinal_ex(
        context.get(),
        reinterpret_cast<unsigned char*>(plaintext.data()) + plaintextLength,
        &finalLength
    );

    if (authenticated != 1) {
        throw std::runtime_error("authentication failed");
    }

    return plaintext;
}

void Enctryptor::incrementNonce(Nonce &nonce) {
    for (std::size_t i = nonce.size(); i-- > 0;) {
        m_encryptNonce[i] += 1u;

        if (m_encryptNonce[i] != 0xffu) {
            return;
        }
    }

    throw std::runtime_error("AES-GCM nonce space exhausted");
}


