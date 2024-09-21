#include "echo_server.hpp"
#include "sigterm_handler.hpp"

int main() {
    EchoServer server("127.0.0.1", 8080);
    SigtermHandler::get().add([&server](int) { server.stop(); });

    server.start();
}
