#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <string>

#include "http_connection.h"
#include "config.h"

void httpServer(tcp::acceptor& acceptor, tcp::socket& socket)
{
    acceptor.async_accept(socket,
        [&](beast::error_code ec)
        {
            if (!ec)
                std::make_shared<HttpConnection>(std::move(socket))->start();
            httpServer(acceptor, socket);
        });
}

int main(int argc, char* argv[])
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    try
    {
        Config& config = Config::getInstance();
        if (!config.load("../config.ini")) {
            std::cerr << "❌ Failed to load config.ini" << std::endl;
            return EXIT_FAILURE;
        }

        auto const address = net::ip::make_address("0.0.0.0");
        unsigned short port = static_cast<unsigned short>(config.getInt("server", "port", 8080));

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, { address, port }};
        tcp::socket socket{ioc};
        httpServer(acceptor, socket);

        std::cout << "🚀 Search Engine Server Started!" << std::endl;
        std::cout << "📍 http://localhost:" << port << std::endl;
        std::cout << "💡 Press Ctrl+C to stop the server" << std::endl;
        std::cout << std::endl;

        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "💥 Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
