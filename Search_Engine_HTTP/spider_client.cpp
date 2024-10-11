#include "spider_client.h"
//#pragma execution_character_set("utf-8")

void SpiderClient::SearchAndClearUrl(std::string url_str, std::queue<std::string> &url_list)
{
    
    boost::cmatch resRegex;
    boost::regex regular("<a href=\"(/.*?)\"");

    int num_url = 0;
    while (boost::regex_search(url_str.c_str(), resRegex, regular))
    {
        std::string link = resRegex[1].str();
        boost::regex clear_url("/(.+)(/.*)");

        if (!boost::regex_search(link, clear_url))
        {
            if (link.size() > 1)
            {
                std::unique_lock lock(mt);
                bool const was_empty = url_list.empty();
                url_list.push(link);
                //std::cout << "url_list.push(link); " << link << " thread: " << std::this_thread::get_id() << std::endl;
                lock.unlock();

                if (was_empty)
                {
                    cv.notify_all();
                }
            }
        }

        url_str = resRegex.suffix().str();

    }
}

SpiderClient::SpiderClient()
{

}

SpiderClient::~SpiderClient()
{

}

void SpiderClient::RunSpider(SpiderReadConfig& config)
{
    start_url = config.start_url;
    port = config.client_port;
    max_depth = config.max_depth;

    std::queue<std::string> url_list;
    url_list.push(start_url);

    std::mutex url_list_mutex;

    for (int depth = 0; depth < max_depth; depth++)
    {
        std::cout << "DEPTH: " << depth << std::endl;

        std::vector<std::thread> threads;
        std::mutex m;

        int url_list_size = url_list.size();

        for (int th = 0; th < url_list_size; th++)
        {
            threads.emplace_back([&url_list, &depth, config, this]
                {
                    std::cout << "СТАРТ ПОТОКА: " << std::this_thread::get_id() << std::endl;
                    std::string target;

                    std::unique_lock lock(mt);
                    while (url_list.empty())
                    {
                        cv.wait(lock);
                    }
                    
                    if (depth > 0)
                    {
                        target = url_list.front();
                        url_list.pop();
                        //std::cout << "target = url_list.front();" << target << " thread: " << std::this_thread::get_id() << std::endl;
                    }
                    else if (depth == 0)
                    {
                        target = "/";
                        url_list.pop();
                        //std::cout << "url_list.pop();" << std::endl;
                    }
                    lock.unlock();
                    cv.notify_all();

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
                    if (!SSL_set_tlsext_host_name(stream.native_handle(), start_url.c_str()))
                    {
                        beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
                        throw beast::system_error{ ec };
                    }

                    // Look up the domain name
                    auto const results = resolver.resolve(start_url.c_str(), port.c_str());

                    // Make the connection on the IP address we get from a lookup
                    beast::get_lowest_layer(stream).connect(results);

                    stream.handshake(ssl::stream_base::client);

                    // Set up an HTTP GET request message
                    http::request<http::string_body> req{ http::verb::get, target, version };
                    req.set(http::field::host, start_url);
                    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

                    // Send the HTTP request to the remote host
                    http::write(stream, req);

                    // This buffer is used for reading and must be persisted
                    beast::flat_buffer buffer;

                    // Declare a container to hold the response
                    http::response<http::dynamic_body> res;

                    // Receive the HTTP response 
                    http::read(stream, buffer, res);

                    std::string url_str = beast::buffers_to_string(res.body().data());
                    std::string wordStr = url_str;

                    Indexer indexer;
                    indexer.ConnectDB(config);

                    indexer.IndexingPages(start_url + target, wordStr);

                    SearchAndClearUrl(url_str, url_list);

                    beast::error_code ec;
                    stream.shutdown(ec);

                    if (ec != net::ssl::error::stream_truncated)
                    {
                        throw beast::system_error{ ec };
                    }
                    std::cout << "ОСТАНОВКА ПОТОКА: " << std::this_thread::get_id() << std::endl;
                });
        }

        for (auto& thread : threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }
}

