#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <thread>
#include <queue>
#include <condition_variable>
#include <Windows.h>

#include "root_certificates.h"
#include "indexer.h"
#include "spider_read_config.h"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class SpiderClient
{
private:

    std::mutex mt;
    std::mutex url_list_pull;
    std::mutex url_indexing;
    std::condition_variable cv;
    std::string start_url = "";
    std::string port = "";
    std::string tmp_url = "";
    int version = 11;
    int max_depth = 0;

    void SearchAndClearUrl(std::string url_str, std::queue<std::string>& url_list);

public:
    
    SpiderClient();
    ~SpiderClient();
    void RunSpider(SpiderReadConfig& config);
};
