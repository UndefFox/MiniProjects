#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <netinet/tcp.h>
#include <span>
#include <string_view>
#include <system_error>



struct SocketException : public std::system_error {
    using std::system_error::system_error;
};

class Socket {
public:
    typedef std::string_view ip_t;
    typedef uint32_t port_t;
    typedef std::size_t descriptor_t;


private:
    std::optional<descriptor_t> m_descriptor;


public:
    Socket();
    Socket(descriptor_t descriptor);
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other);
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    ~Socket();

    void init();
    void bind(ip_t ip, port_t port);
    void connect(ip_t ip, port_t port);
    [[nodiscard]] Socket accept();
    void listen(int queueSize);
    void close();
    bool isValid() const;
    void shutdown();
    void setTimeout(std::chrono::seconds timeout);
    void setKeepAlive(std::chrono::seconds idle, std::chrono::seconds probeInterval, int probeCount);
    std::error_code getError();

    void read(std::span<std::byte> buffer);
    void write(std::span<const std::byte> buffer);

    descriptor_t descriptor() const;

    operator bool() const;
    friend bool operator==(const Socket& lhs, const Socket& rhs) noexcept;
    friend bool operator!=(const Socket& lhs, const Socket& rhs) noexcept;
};
