#pragma once

#include <array>
#include <atomic>

class EchoServer {
private:
    static constexpr size_t BUFFER_SIZE = 256;
    using Buffer = std::array<char, BUFFER_SIZE>;

    static constexpr size_t MAX_CLIENTS = 1;

public:
    EchoServer(std::string host, const uint16_t port);

    void start();
    void stop();

    ~EchoServer();

private:
    const std::string _host;
    const uint16_t _port;

    std::atomic<bool> _is_running;

    int _fd;
};
