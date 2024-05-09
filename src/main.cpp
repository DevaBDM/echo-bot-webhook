#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;

void handle_request(tcp::socket& socket) {
    std::string message = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello, world!";
    boost::system::error_code ignored_error;
    write(socket, buffer(message), ignored_error);
}

int main() {
    io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 8080));

    while (true) {
        tcp::socket socket(io_service);
        acceptor.accept(socket);
        handle_request(socket);
    }

    return 0;
}

