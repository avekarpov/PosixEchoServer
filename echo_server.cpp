#include "echo_server.hpp"
#include "tools.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>
#include <arpa/inet.h>

#define LOG(__LVL__, __MSG__) std::cout << "[" __LVL__ "][EchoServer]: " << __MSG__ << std::endl;
#define ERROR(__MSG__) LOG("e", __MSG__)
#define INFO(__MSG__) LOG("i", __MSG__)
#define DEBUG(__MSG__) LOG("d", __MSG__)

EchoServer::EchoServer(std::string host, const uint16_t port)
    : _host { host == "localhost" ? "127.0.0.1" : std::move(host) }
    , _port { port }
    , _is_running { false }
    , _fd { -1 }
{}

void EchoServer::start() {
    if (_is_running.load()) {
        return;
    }

    INFO("start");

    _fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_THROW_POSIX(_fd);

    _is_running.store(true);

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

    loop();
}

void EchoServer::loop() const {
    while (_is_running.load()) {
        try {
            sockaddr_in client_addr { 0 };
            socklen_t client_addr_len = sizeof(client_addr);
            const auto client_fd = accept(_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
            CHECK_THROW_POSIX(client_fd);

            DEFER {
                close(client_fd);
            };

            DEBUG("next client");

            Buffer buffer;
            auto read_count = readFull(client_fd, buffer.data(), buffer.size());
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

            CHECK_THROW_POSIX(close(client_fd));
        }
        catch (...) {
            ERROR("some error occurred, while handle client");
        }
    }
}

void EchoServer::stop() {
    if (not _is_running.load()) {
        return;
    }

    INFO("stop");

    if (_is_running.exchange(false)) {
        CHECK_THROW_POSIX(close(_fd));
    }
}

EchoServer::~EchoServer() {
    try {
        stop();
    }
    catch (...) {
        ERROR("some error occurred on destryction");
    }
}
