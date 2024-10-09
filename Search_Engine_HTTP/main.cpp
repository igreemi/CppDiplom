#include <iostream>
#pragma execution_character_set("utf-8")

#include "spider_read_config.h"
#include "spider_client.h"
#include "server.h"

// Performs an HTTP GET and prints the response
int main()
{
    setlocale(LC_ALL, "Russia");
//    std::locale::global(std::locale("ru_RU.UTF-8"));
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    try
    {
        SpiderReadConfig spider_read_config;
        spider_read_config.LoadConfig("config.ini");
        spider_read_config.PrintConfig();

        SpiderClient spider_client;
        spider_client.RunSpider(spider_read_config);

        auto const address = net::ip::make_address("192.168.31.198");
        unsigned short port = static_cast<unsigned short>(std::stoi(spider_read_config.server_port));

        net::io_context ioc{ 1 };

        tcp::acceptor acceptor{ ioc, {address, port} };
        tcp::socket socket{ ioc };
        http_server(acceptor, socket);

        ioc.run();

    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}