#include "echo_server.hpp"
#include "tools.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <thread>

// TODO: not thread safe
#define LOG(__LVL__, __MSG__) std::cout << "[" __LVL__ "][EchoServer]: " << __MSG__ << std::endl;
#define ERROR(__MSG__) LOG("e", __MSG__)
#define INFO(__MSG__) LOG("i", __MSG__)
#define DEBUG(__MSG__) LOG("d", __MSG__)
#define TRACE(__MSG__) // LOG("t", __MSG__)

EchoServer::EchoServer(std::string host, const uint16_t port)
    : _host { host == "localhost" ? "127.0.0.1" : std::move(host) }
    , _port { port }
    , _is_running { false }
    , _fd { -1 }
    , _epoll_fd { -1 }
{}

void EchoServer::start() {
    if (_is_running.load()) {
        return;
    }

    INFO("start");

    _is_running.store(true);

    loop();

    INFO("end");
}

void EchoServer::loop() {
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_THROW_POSIX(_fd);

    const int flags = fcntl(_fd, F_GETFL);
    CHECK_THROW_POSIX(flags);
    CHECK_THROW_POSIX(fcntl(_fd, F_SETFL, flags | O_NONBLOCK));

    _epoll_fd = epoll_create1(0); // TODO: flags?
    CHECK_THROW_POSIX(_epoll_fd);

    DEFER {
        if (-1 == close(_epoll_fd)) {
            ERROR(ERRNO);
        }
        if (-1 == close(_fd)) {
            ERROR(ERRNO);
        }
        INFO("free fds");
    };

    sockaddr_in addr { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    if (const auto res = inet_pton(AF_INET, _host.data(), &addr.sin_addr.s_addr); 0 == res) {
        throw std::runtime_error { "invalid string for address family" };
    }
    else {
        CHECK_THROW_POSIX(res);
    }
    CHECK_THROW_POSIX(bind(_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)));

    CHECK_THROW_POSIX(listen(_fd, MAX_CLIENTS));

    epoll_event event;
    event.data.fd = _fd;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    CHECK_THROW_POSIX(epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _fd, &event));

    using EpollEvents = std::array<epoll_event, MAX_EPOLL_EVENTS_COUNT>;
    EpollEvents events;

    while (_is_running.load()) {
        try {
            TRACE("wait epoll events");
            const auto events_count = epoll_wait(
                _epoll_fd,
                events.data(),
                events.size(),
                100
            );

            if (events_count == -1) {
                if (errno == EINTR) {
                    continue;
                }
                THROW_ERRNO;
            }

            TRACE("process epoll events");
            for (size_t i = 0; i < events_count; ++i) {
                if (0 == events[i].events) {
                    continue;
                }

                if (events[i].data.fd == _fd) {
                    accept();
                }
                else {
                    const auto client_fd = events[i].data.fd;

                    if ((EPOLLHUP | EPOLLERR) & events[i].events == (EPOLLHUP | EPOLLERR)) {
                        INFO("client error, close");
                        CHECK_THROW_POSIX(close(client_fd));

                        continue;
                    }

                    if ((EPOLLIN | EPOLLOUT) & events[i].events == (EPOLLIN | EPOLLOUT)) {
                        echo(client_fd);
                    }

                    if (events[i].events & ~(EPOLLIN | EPOLLOUT | EPOLLHUP | EPOLLERR)) {
                        ERROR("unexpected client event: " << events[i].events);
                    }
                }
            }
        }
        catch (const std::exception &error) {
            ERROR("error, while process epoll events: " << error.what());
        }
        catch (...) {
            ERROR("some error occurred, while process epoll events");
        }
    }
}

void EchoServer::accept() const {
    sockaddr_in client_addr { 0 };
    socklen_t client_addr_len = sizeof(client_addr);
    const auto client_fd = ::accept(_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
    CHECK_THROW_POSIX(client_fd);

    const int client_flags = fcntl(client_fd, F_GETFL);
    CHECK_THROW_POSIX(client_flags);
    CHECK_THROW_POSIX(fcntl(client_fd, F_SETFL, client_flags | O_NONBLOCK));

    epoll_event event;
    event.data.fd = client_fd;
    event.events = EPOLLIN | EPOLLOUT;
    DEBUG("add client to epoll");
    CHECK_THROW_POSIX(epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, client_fd, &event));
}

void EchoServer::echo(const int client_fd) {
    INFO("echo client");

    using Buffer = std::array<char, BUFFER_SIZE>;
    Buffer buffer;

    auto read_count = readFull(client_fd, buffer.data(), buffer.size());

    // TODO: fix, when client send empty message got infinity loop
    if (0 == read_count) {
        INFO("empty client message, close");
        CHECK_THROW_POSIX(close(client_fd));

        return;
    }

    if (read_count == buffer.size()) {
        std::string big_buffer;
        big_buffer.reserve(big_buffer.size() * 2);

        while (read_count == buffer.size()) {
            big_buffer.append(buffer.data(), buffer.size());

            read_count = readFull(client_fd, buffer.data(), buffer.size());
        }
        big_buffer.append(buffer.data(), read_count);

        DEBUG("receive from client (big):\n" << big_buffer);

        toupper(big_buffer.data(), big_buffer.size());

        DEBUG("send to client (big):\n" << big_buffer);
        readFull(client_fd, big_buffer.data(), big_buffer.size());
    }
    else {

        DEBUG("receive from client:\n" << std::string_view(buffer.data(), read_count));

        toupper(buffer.data(), read_count);

        DEBUG("send to client:\n" << std::string_view(buffer.data(), read_count));
        writeFull(client_fd, buffer.data(), read_count);
    }
}

void EchoServer::stop() {
    if (not _is_running.load()) {
        return;
    }

    INFO("stop");

    _is_running.exchange(false);
}

EchoServer::~EchoServer() {
    try {
        stop();
    }
    catch (...) {
        ERROR("some error occurred on destruction");
    }
}
