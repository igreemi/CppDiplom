#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
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
    std::mutex mt2;
    std::condition_variable cv, cv2, cv3, cv4;

    std::string start_url = "";
    std::string port = "";

    int version = 11;
    int max_depth = 0;

    bool state_th = false;
    bool state_th2 = false;
    bool state_th3 = false;
    bool state_th4 = false;

    bool HandleRedirect(boost::url& target_url, http::response<http::string_body>& res);
    std::string HandleUrl(const std::string& url, std::string& target, std::string& host, std::string port, int depth);
    void SearchAndClearUrl(std::string url_str, std::string host, std::string target, std::queue<std::string>& url_list);

public:
    
    SpiderClient();
    ~SpiderClient();
    void RunSpider(SpiderReadConfig config);
};
