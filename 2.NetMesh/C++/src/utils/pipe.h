#pragma once

#include <cstdint>
#include <optional>
#include <system_error>



struct PipeException : public std::system_error {
    using std::system_error::system_error;
};

struct Pipe {
public:
    typedef std::size_t descriptor_t;


private:
    std::optional<std::pair<descriptor_t, descriptor_t>> m_descriptors;


public:
    Pipe();
    Pipe(Pipe&& other) noexcept;
    Pipe& operator=(Pipe&& other);
    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    ~Pipe();

    int getOutput() const;
    int getInput() const;

    void write(uint8_t byte);
    std::optional<uint8_t> tryReadByte();

    void close() noexcept;
};

