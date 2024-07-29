#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <cstdlib>

#include <vector>
#include <string>
#include <regex>

#include "root_certificates.h"
#include "indexer.h"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

// Performs an HTTP GET and prints the response
int main()
{
    setlocale(LC_ALL, "");
//    std::locale::global(std::locale("ru_RU.UTF-8"));
//    SetConsoleCP(1251);
//    SetConsoleOutputCP(1251);
//    auto const host = "openssl.org";
    auto const host = "greem.narod.ru";
    auto const port = "443";
    auto const target = "/";
    int version = 11;

    Indexer indexer;

    std::vector<std::string> urlList;

    try
    {
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx(ssl::context::tlsv12_client);

        // This holds the root certificate used for verification
        load_root_certificates(ctx);

        // Verify the remote server's certificate
        ctx.set_verify_mode(ssl::verify_peer);

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        ssl::stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host))
        {
            beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
            throw beast::system_error{ ec };
        }

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{ http::verb::get, target, version };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        std::string urlStr = beast::buffers_to_string(res.body().data());
        std::string wordStr = urlStr;

        boost::cmatch resRegex;

        boost::regex regular("<a href=\"(.*?)\"");

        while (boost::regex_search(urlStr.c_str(), resRegex, regular))
        {
            std::string link = resRegex[1].str();
            urlList.push_back(link);
            urlStr = resRegex.suffix().str();
        }

        std::cout << "--------------------------URL-LIST--------------------------" << std::endl;
        for (auto res : urlList)
        {
            std::cout << res << std::endl;
        }
        std::cout << "------------------------------------------------------------" << std::endl;
        indexer.PrintVocabulary(indexer.AnalyzeWords(wordStr));

          // Gracefully close the stream
        beast::error_code ec;
        stream.shutdown(ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if (ec != net::ssl::error::stream_truncated)
        {
            throw beast::system_error{ ec };
        }
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}