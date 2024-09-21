#pragma once

#include <functional>
#include <list>
#include <csignal>

void sigterm_handler(const int signal);

struct SigtermHandler {
public:
    static SigtermHandler& get() {
        static SigtermHandler signal_handler;
        return signal_handler;
    }

    void add(std::function<void(int)> handler) {
        _handlers.push_back(std::move(handler));
    }

    void handle(const int signal) const {
        for (const auto& handler : _handlers) {
            handler(signal);
        }
    }

private:
    SigtermHandler() {
        std::signal(SIGTERM, &sigterm_handler);
    }

private:
    std::list<std::function<void(int)>> _handlers;
};

void sigterm_handler(const int signal) {
    SigtermHandler::get().handle(signal);
}