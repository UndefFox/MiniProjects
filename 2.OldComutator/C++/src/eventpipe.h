#pragma once


/**
 * @brief Provides easy way to notify blocked thread
 * of an event from the other thread.
 */
struct EventPipe {
private:
    int output;
    int input;


public:
    EventPipe();
    ~EventPipe();

    /**
     * @brief Get output file descriptor of the pipe.
     *
     * @return File descriptor.
     */
    int getOutput() const;
    /**
     * @brief Get input file descriptor of the pipe.
     *
     * @return File descriptor.
     */
    int getInput() const;

    /**
     * @brief Writes one byte to the pipe with value 1.
     */
    void write();
    /**
     * @brief Reads and discards single byte from the
     * pipe. If no data is pressent, blocks.
     */
    void reset();
};
