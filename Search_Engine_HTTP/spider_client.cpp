#include "spider_client.h"

bool SpiderClient::HandleRedirect(boost::url& target_url, http::response<http::string_body>& res)
{
    auto location_header = res.find(http::field::location);
    
    if (location_header == res.end())
    {
        return false;
    }
    target_url = boost::url(location_header->value());

    return true;
}

std::string SpiderClient::HandleUrl(const std::string& url, std::string& target, 
    std::string& host, std::string port, int depth)
{
    boost::url parse_url{ url };

    std::string scheme = parse_url.scheme();

    bool use_ssl = false;

    if (port == "443")
    {
        use_ssl = true;
    }
    else if (scheme == "https")
    {
        port = "443";
        use_ssl = true;
    }

    while (true)
    {
        if (parse_url.host().empty())
        {
            std::string tmp_url = parse_url.path();
            size_t delimiter_pos = tmp_url.find('/');
            host = tmp_url.substr(0, delimiter_pos);
            target = tmp_url.substr(delimiter_pos);
        }
        else if (!parse_url.host().empty())
        {
            host = parse_url.host();
            target = parse_url.path();
        }

        scheme = parse_url.scheme();

        net::io_context ioc;

        ssl::context ctx{ ssl::context::tlsv12_client };

        if (use_ssl)
        {
            load_root_certificates(ctx);
            ctx.set_verify_mode(ssl::verify_peer);
        }

        tcp::resolver resolver{ ioc };

        tcp::socket socket{ ioc };
        ssl::stream<beast::tcp_stream> stream{ ioc, ctx };
        
        if (use_ssl)
        {
            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                std::cout << "--- !SSL_set_tlsext_host_name ---" << std::endl;
                beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
                throw beast::system_error{ ec };
            }
        }

        auto results = resolver.resolve(host.c_str(), port.c_str());

        if (!use_ssl)
        {
            boost::asio::connect(socket, results.begin(), results.end());
        }
        else
        {
            beast::get_lowest_layer(stream).connect(results);
            stream.handshake(ssl::stream_base::client);
        }

        http::request<http::empty_body> req{ http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        if (!use_ssl)
        {
            http::write(socket, req);
        }
        else
        {
            http::write(stream, req);
        }

        beast::flat_buffer buffer;

        http::response<http::string_body> res;

        if (!use_ssl)
        {
            http::read(socket, buffer, res);
        }
        else
        {
            http::read(stream, buffer, res);
        }

        if (res.result() == http::status::found || res.result() == http::status::moved_permanently || res.result() == http::status::temporary_redirect)
        {
            if (!HandleRedirect(parse_url, res))
            {
                return "";
            }

            if (res.result() == http::status::moved_permanently
                || res.result() == http::status::found
                || res.result() == http::status::temporary_redirect)
            {
                if (scheme == "https")
                {
                    use_ssl = true;
                    port = "443";
                }
                else if (scheme == "http")
                {
                    use_ssl = false;
                    port = "80";
                }
            }
            continue;
        }
        else if (res.result_int() == 200)
        {
            return res.body();
        }
        else if (res.result_int() == 404)
        {
            std::cerr << "Страница не найдена: " << res.result() << std::endl;

            return "";
        }
        else
        {
            return "";
        }
    }
}

void SpiderClient::SearchAndClearUrl(std::string url_str, std::string host, 
    std::string target, std::queue<std::string>& url_list)
{
    boost::cmatch resRegex;
    boost::regex regular("<a href=\"(.*?)\"");

    while (boost::regex_search(url_str.c_str(), resRegex, regular))
    {
        std::string link = resRegex[1].str();

        if (link.find('#') != std::string::npos)
        {
            if (link[0] == '#')
            {

            }
            else
            {
                size_t delimetr_pos = link.find('#');
                std::string tmp_link = host + target + link.substr(0, delimetr_pos);
                url_list.push(tmp_link);
            }
        }
        else if(link[0] == '/')
        {
            std::string tmp_link = host + link;
            url_list.push(tmp_link);
        }
        else if(link.find("http://") == 0 || link.find("https://") == 0)
        {
            url_list.push(link);
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

void SpiderClient::RunSpider(SpiderReadConfig config)
{
    start_url = config.start_url;
    port = config.client_port;
    max_depth = config.max_depth;

    std::queue<std::string> url_list;
    url_list.push(start_url);
    std::string host = "";

    for (int depth = 0; depth < max_depth; depth++)
    {
        unsigned int num_threads = 1;

        if(depth > 0)
        {
            num_threads = std::thread::hardware_concurrency();
        }

        size_t url_list_size = url_list.size();

        for (int th = 0; th < url_list_size; th += num_threads)
        {
            std::vector<std::thread> threads;

            for(unsigned int i = 0; i < num_threads; i++)
            {
                if (th + i >= url_list_size)
                {
                    break;
                }

                threads.emplace_back([&url_list, depth, &host, config, this]
                    {
                        std::string target = "";

                        std::string url = "";

                        std::unique_lock lock(mt);
                        cv.wait(lock, [&] { return state_th = true; });

                        url = url_list.front();
                        url_list.pop();

                        state_th = false;
                        lock.unlock();
                        cv.notify_all();

                        std::unique_lock<std::mutex> lock2(mt2);
                        cv2.wait(lock2, [&] { return state_th2 = true; });

                        std::string url_str = HandleUrl(url, target, host, port, depth);

                        state_th2 = false;
                        lock2.unlock();
                        cv2.notify_all();

                        if (!url_str.empty())
                        {
                            std::unique_lock<std::mutex> lock3(mt3);
                            cv3.wait(lock3, [&] { return state_th3 = true; });

                            std::string wordStr = url_str;

                            Indexer indexer;
                            indexer.ConnectDB(config);
                            indexer.IndexingPages(url, wordStr);

                            state_th3 = false;
                            lock3.unlock();
                            cv3.notify_all();

                            std::unique_lock<std::mutex> lock4(mt4);
                            cv4.wait(lock4, [&] { return state_th4 = true; });
                            
                            SearchAndClearUrl(url_str, host, target, url_list);

                            state_th4 = false;
                            lock4.unlock();
                            cv4.notify_all();
                        }
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
}

