#pragma once

#include <array>
#include <atomic>

class EchoServer {
private:
    static constexpr size_t BUFFER_SIZE = 256;
    static constexpr size_t MAX_CLIENTS = 2;
    static constexpr size_t MAX_EPOLL_EVENTS_COUNT = 2;

public:
    EchoServer(std::string host, const uint16_t port);

    void start();
    void stop();

    ~EchoServer();

private:
    void loop();

    void accept() const;

    static void echo(const int client_fd);

private:
    const std::string _host;
    const uint16_t _port;

    std::atomic<bool> _is_running;

    int _fd;
    int _epoll_fd;
};
