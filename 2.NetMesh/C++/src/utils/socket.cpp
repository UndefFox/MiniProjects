#include "socket.h"

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <utility>



Socket::Socket() :
    m_descriptor(std::nullopt)
{ }

Socket::Socket(descriptor_t descriptor) :
    m_descriptor(descriptor)
{
    if (descriptor == -1) [[unlikely]] {
        throw std::logic_error("Failed to create socket: Tried to bind -1 descriptor");
    }
}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept
    : m_descriptor(std::exchange(other.m_descriptor, std::nullopt))
{ }


Socket& Socket::operator=(Socket&& other) {
    if (m_descriptor) {
        close();
    }

    m_descriptor = std::exchange(other.m_descriptor, std::nullopt);

    return *this;
}

void Socket::init() {
    int descriptor = ::socket(AF_INET, SOCK_STREAM, 0);

    if (descriptor == -1) [[unlikely]]  {
        throw SocketException(errno, std::generic_category(), "Socket::init::socket");
    }

    m_descriptor = descriptor;
}

void Socket::bind(ip_t ip, port_t port) {
    if (!m_descriptor) [[unlikely]] {
        throw std::logic_error("Failed to bind socket: Tried to bind empty socket");
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (::inet_pton(AF_INET, ip.data(), &address.sin_addr.s_addr) == 0) [[unlikely]] {
        throw std::logic_error("Failed to bind socket: Invalid address");
    }

    if (::bind(m_descriptor.value(), (struct sockaddr*)&address, sizeof(address)) == -1) [[unlikely]] {
        throw SocketException(errno, std::generic_category(), "Socket::bind::bind");
    }
}


void Socket::connect(ip_t ip, port_t port) {
    if (!m_descriptor) [[unlikely]] {
        throw std::logic_error("Failed to connect socket: Tried to bind empty socket");
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    std::string ipAddress(ip);
    if (inet_pton(AF_INET, ipAddress.data(), &address.sin_addr.s_addr) == 0) [[unlikely]] {
        throw std::logic_error("Failed to connect socket: Invalid address");
    }

    while (true) {
        if (::connect(m_descriptor.value(), (struct sockaddr*)&address, sizeof(address)) == -1) [[unlikely]] {
            if (std::error_code(errno, std::generic_category()) == std::errc::interrupted) continue;

            throw SocketException(errno, std::generic_category(), "Socket::connect::connect");
        }

        break;
    }
}

Socket Socket::accept() {
    while (true) {
        auto result = ::accept(m_descriptor.value(), nullptr, nullptr);

        if (result == -1) [[unlikely]] {
            if (std::error_code(errno, std::generic_category()) == std::errc::interrupted) continue;

            throw SocketException(errno, std::generic_category(), "Socket::accept::accept");
        }

        return Socket(result);
    }
}

void Socket::listen(int queueSize) {
    if (::listen(m_descriptor.value(), queueSize) == -1) [[unlikely]] {
        throw SocketException(errno, std::generic_category(), "Socket::listen::listen");
    }
}

void Socket::close() {
    while (std::as_const(m_descriptor)) {
        if (::close(m_descriptor.value()) == -1) [[unlikely]] {
            if (std::error_code(errno, std::generic_category()) == std::errc::interrupted) continue;

            throw SocketException(errno, std::generic_category(), "Socket::close::close");
        }

        m_descriptor = std::nullopt;

        break;
    }
}

bool Socket::isValid() const {
    return m_descriptor.has_value();
}

void Socket::shutdown() {
    if (!m_descriptor) [[unlikely]] throw std::logic_error("Failed to shutdown: Tried to use empty socket");

    if (::shutdown(m_descriptor.value(), SHUT_RDWR) == -1) [[unlikely]] {
        throw std::system_error(errno, std::generic_category(), "Socket::shutdown::shutdown");
    }
}

void Socket::setTimeout(std::chrono::seconds timeout) {
    if (!m_descriptor) [[unlikely]] throw std::logic_error("Failed to set timeout: empty socket");

    const auto count = timeout.count();
    timeval value{
        .tv_sec = static_cast<decltype(timeval::tv_sec)>(count),
        .tv_usec = 0
    };

    const descriptor_t fd = m_descriptor.value();

    if (::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Socket::setTimeout::setsockopt");
    }

    if (::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(value)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Socket::setTimeout::setsockopt");
    }
}

void Socket::setKeepAlive(std::chrono::seconds idle, std::chrono::seconds probeInterval, int probeCount) {
    if (!m_descriptor) [[unlikely]] throw std::logic_error("Failed to enable keepalive: empty socket");

    const descriptor_t fd = *m_descriptor;
    const int enabled = 1;
    const int idleSeconds = static_cast<int>(idle.count());
    const int intervalSeconds = static_cast<int>(probeInterval.count());

    if (::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Socket::setKeepAlive::SO_KEEPALIVE");
    }

    if (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idleSeconds, sizeof(idleSeconds)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Socket::setKeepAlive::TCP_KEEPIDLE");
    }

    if (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intervalSeconds, sizeof(intervalSeconds)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Socket::setKeepAlive::TCP_KEEPINTVL");
    }

    if (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &probeCount, sizeof(probeCount)) == -1) {
        throw std::system_error(errno, std::generic_category(), "Socket::setKeepAlive::TCP_KEEPCNT");
    }
}


std::error_code Socket::getError() {
    if (!m_descriptor) [[unlikely]] throw std::logic_error("Failed to get error: Tried to use empty socket");

    int socketError = 0;
    socklen_t length = sizeof(socketError);

    if (::getsockopt(m_descriptor.value(), SOL_SOCKET, SO_ERROR, &socketError, &length) == -1) {
        throw SocketException(errno, std::generic_category(), "Socket::getError::getsockopt");
    }

    return  std::error_code(socketError, std::generic_category());
}

void Socket::read(std::span<std::byte> buffer) {
    if (!m_descriptor) [[unlikely]] throw std::logic_error("Failed to read socket: Tried to use empty socket");
    else if (buffer.empty()) [[unlikely]] throw std::logic_error("Failed to read socket: buffer empty");

    while (!buffer.empty()) {
        auto read = ::recv(m_descriptor.value(), buffer.data(), buffer.size(), 0);

        if (read == -1) {
            if (std::error_code(errno, std::generic_category()) == std::errc::interrupted) continue;

            throw SocketException(errno, std::generic_category(), "Socket::read::recv");
        }
        else if (read == 0) {
            throw SocketException(std::make_error_code(std::errc::connection_reset), "Socket::read::recv");
        }
        else [[likely]] {
            buffer = buffer.subspan(read);
        }
    }
}

void Socket::write(std::span<const std::byte> buffer) {
    if (!m_descriptor) [[unlikely]] throw std::logic_error("Failed to write socket: Tried to use empty socket");
    else if (buffer.empty()) [[unlikely]] throw std::logic_error("Failed to write socket: buffer empty");

    while (!buffer.empty()) {
        auto wrote = ::send(m_descriptor.value(), buffer.data(), buffer.size(), MSG_NOSIGNAL);

        if (wrote == -1) {
            if (std::error_code(errno, std::generic_category()) == std::errc::interrupted) continue;

            throw SocketException(errno, std::generic_category(), "Socket::write::send");
        }
        else [[likely]] {
            buffer = buffer.subspan(wrote);
        }
    }
}

Socket::descriptor_t Socket::descriptor() const {
    if (!m_descriptor) [[unlikely]] {
        throw std::logic_error("Failed to get descriptor: Tried to get empty socket");
    }

    return m_descriptor.value();
}

Socket::operator bool() const {
    return isValid();
}

bool operator==(const Socket &lhs, const Socket &rhs) noexcept {
    return lhs.m_descriptor == rhs.m_descriptor;
}

bool operator!=(const Socket &lhs, const Socket &rhs) noexcept {
    return !(lhs == rhs);
}
