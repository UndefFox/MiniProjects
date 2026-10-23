#pragma once

#include <memory>
#include <openssl/evp.h>
#include <array>
#include <span>
#include <vector>



class Enctryptor {
public:
    static constexpr std::size_t NonceSize = 12;
    typedef std::array<unsigned char, NonceSize> Nonce;
    static constexpr std::size_t TagSize = 16;
    using Tag = std::array<unsigned char, TagSize>;

    typedef std::array<unsigned char, 32> EVP_PKEY_raw;

    struct EncryptedBlock {
        std::vector<std::byte> cipher;
        Tag tag{};
        Nonce nonce;
    };

private:
    static constexpr std::size_t CounterSize = 8;

    struct EVP_PKEY_Deleter {
        void operator()(EVP_PKEY* key) const noexcept { EVP_PKEY_free(key); }
    };
    typedef std::unique_ptr<EVP_PKEY, EVP_PKEY_Deleter> EVP_PKEY_ptr;

    struct EVP_CIPHER_CTX_Deleter {
        void operator()(EVP_CIPHER_CTX* context) const noexcept { EVP_CIPHER_CTX_free(context); }
    };
    typedef std::unique_ptr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_Deleter> EVP_CIPHER_CTX_ptr;


private:
    EVP_PKEY_ptr m_privateKey = nullptr;
    EVP_PKEY_raw m_sharedSecret{};
    bool m_paired = false;

    Nonce m_encryptNonce;


public:
    void initialize(bool isReciever);
    EVP_PKEY_raw getPublickKey();
    void pair(const EVP_PKEY_raw& publicKey);
    bool isPaired() const;

    EncryptedBlock encrypt(std::span<const std::byte> data);
    std::vector<std::byte> decrypt(const EncryptedBlock& block);

private:
    void incrementNonce(Nonce& nonce);
};