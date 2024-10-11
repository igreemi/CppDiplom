#include <iostream>
#pragma execution_character_set("utf-8")

#include "spider_read_config.h"
#include "spider_client.h"
#include "server.h"

// Performs an HTTP GET and prints the response
int main()
{
    setlocale(LC_ALL, "Russia");
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    setvbuf(stdout, nullptr, _IOFBF, 1000);

    try
    {
        std::cout << "--- ÑÒÀÐÒ ÏÐÎÃÐÀÌÌÛ ---" << std::endl;
        SpiderReadConfig spider_read_config;
        spider_read_config.LoadConfig("config.ini");
        spider_read_config.PrintConfig();

        std::cout << "--- ÑÒÀÐÒ ÏÀÓÊÀ ---" << std::endl;
        SpiderClient spider_client;
        spider_client.RunSpider(spider_read_config);
        std::cout << "--- ÇÀÂÅÐØÅÍÈÅ ÏÀÓÊÀ ---" << std::endl;

        auto const address = net::ip::make_address("127.0.0.1");
        unsigned short port = static_cast<unsigned short>(std::stoi(spider_read_config.server_port));

        net::io_context ioc{ 1 };

        tcp::acceptor acceptor{ ioc, {address, port} };
        tcp::socket socket{ ioc };
        std::cout << "--- ÑÒÀÐÒ HTTP ÑÅÐÂÅÐÀ ---" << std::endl;
        http_server(acceptor, socket, spider_read_config);

        system("start http://127.0.0.1");

        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}