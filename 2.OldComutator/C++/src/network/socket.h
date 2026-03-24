#pragma once

#include <arpa/inet.h>
#include <span>
#include <exception>
#include <string_view>


/**
 * @brief Owning wrapper for a socket file descriptor.
 */
class Socket {
public:
    enum class Shutdown {
        Read = SHUT_RD,
        Write = SHUT_WR,
        Both = SHUT_RDWR
    };

public:
    /**
     * @brief Gets instance with no associated socket.
     */
    Socket() noexcept;
    /**
     * @brief Creates instance and takes ownership
     * of the descriptor.
     *
     * @param descriptor Socket file descriptor.
     */
    Socket(int descriptor) noexcept;
    Socket(Socket&& s) noexcept;
    ~Socket();
    Socket(const Socket&) = delete;

    Socket& operator=(Socket&& other);
    Socket& operator=(Socket& other) = delete;

    /**
     * @brief Acquires a new socket, closing the old one.
     */
    void init();
    void bind(std::string_view ip, uint16_t port);
    void connect(std::string_view ip, uint16_t port);
    void listen(int maxQueue);
    Socket accept();
    void shutdown(Shutdown direction);
    void close();

    /**
     * @brief Sets global operation timeout on all operations.
     *
     * @param timeout Timeout in seconds.
     */
    void setOperationTimeout(int timeout);
    /**
     * @brief Sets keep alive flag with dedicated timeout. Checks
     * connection every 60 seconds and fails after single attempt.
     *
     * @param timeout Timeout in seconds.
     */
    void setKeepAlive(bool flag, int timeout);

    /**
     * @brief Reads data from the socket until given span is filled.
     *
     * @param buffer Buffer to fill with read data.
     */
    void read(std::span<std::byte> buffer);

    /**
     * @brief Writes data to the socket until all data was sent.
     *
     * @param buffer Bytes to send.
     */
    void write(std::span<const std::byte> buffer);

    /**
     * @brief Checks if socket is closed by trying to read a byte.
     * If no data is present, the thread is blocked.
     *
     * @return true if socket didn't return any error, false otherwise.
     */
    bool isClosed() const;

    /**
     * @brief Returns file descriptor of the owned socket.
     *
     * @return file descriptor.
     */
    int get() const;


private:
    int descriptor;
};


/**
 * @brief Exception class for socket errors.
 */
class SocketException : public std::exception {
private:
    int rawError;


public:
    /**
     * @brief Creates new erorr instance by grabbing errorcode from
     * socket options of the passed descriptor.
     *
     * @param descriptor File descriptor to grab the error from.
     */
    SocketException(int descriptor);
    const char* what() const noexcept override;
    SocketException() = delete;
};
