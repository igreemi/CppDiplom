#pragma once
#pragma execution_character_set("utf-8")

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace my_program_state
{
    std::size_t
        request_count()
    {
        static std::size_t count = 0;
        return ++count;
    }

    std::time_t
        now()
    {
        return std::time(0);
    }
}

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    // Initiate the asynchronous operations associated with the connection.
    void start()
    {
        read_request();
        check_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{ 8192 };

    // The request message.
    http::request<http::dynamic_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        socket_.get_executor(), std::chrono::seconds(60) };

    // Asynchronously receive a complete request message.
    void read_request()
    {
        auto self = shared_from_this();

        http::async_read(
            socket_,
            buffer_,
            request_,
            [self](beast::error_code ec,
                std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);
                if (!ec)
                    self->process_request();
            });
    }

    // Determine what needs to be done with the request message.
    void process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);
        std::string str;

        switch (request_.method())
        {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            create_response();
            break;

        case http::verb::post:
            //request_.set(http::field::accept_encoding, "utf-8");
            request_.set(http::field::content_type, "text/html; charset=utf-8");
            str = beast::buffers_to_string(request_.body().data());
            std::cout << str << std::endl;
            GetWord(str);
            response_.set(http::field::content_type, "text/html; charset=utf-8");
            beast::ostream(response_.body())
                << "<!DOCTYPE HTML>\n"
                << "<html>\n"
                << "<head>\n"
                << "<meta charset=\"utf-8\">\n"
        //        << "<link href=\"https://fonts.googleapis.com/css?family=Roboto:400,700&subset=cyrillic\" rel=\"stylesheet\">\n"
                << "<title>Search results:</title>\n"
                << "</head>\n"
                << "<body>\n"
                << "<center>\n"
                << "<h1>SEARCH</h1>\n"
                << "<form action=\"/\" method=\"post\">\n"
                << "<input type=\"text\" name=\"query\">\n" 
                << "<input type=\"submit\" value=\"ENTER\">\n"
                << "</form>\n"
                <<"<h1>Search results:</h1>\n"
                << "<h1>\n"
                << str
                << "</h1>\n"
                << "</center>\n"
                << "</body>\n"
                << "</html>\n";
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain; charset=utf-8");
            beast::ostream(response_.body())
                << "Invalid request-method '"
                << std::string(request_.method_string())
                << "'";
            break;
        }

        write_response();
    }

    // Construct a response message based on the program state.
    void create_response()
    {
        if (request_.target() == "/")
        {
            response_.set(http::field::content_type, "text/html; charset=utf-8");
            beast::ostream(response_.body())
                << "<!DOCTYPE HTML>\n"
                << "<html>\n"
                << "<head>\n"
                << "<meta charset=\"utf-8\">\n"
        //        << "<link href=\"https://fonts.googleapis.com/css?family=Roboto:400,700&subset=cyrillic\" rel=\"stylesheet\">\n"
                << "<title>SEARCH</title>\n"
                << "</head>\n"
                << "<body>\n"
                << "<center>\n"
                << "<h1>SEARCH</h1>\n"
                << "<form action=\"/\" method=\"post\">\n"
                << "<input type=\"text\" name=\"query\">\n"
                << "<input type=\"submit\" value=\"ENTER\">\n"
                << "</form>\n"
                << "</center>\n"
                << "</body>\n"
                << "</html>\n";
        }
        else
        {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "File not found\r\n";
        }
    }

    // Asynchronously transmit the response message.
    void write_response()
    {
        auto self = shared_from_this();

        response_.content_length(response_.body().size());

        http::async_write(
            socket_,
            response_,
            [self](beast::error_code ec, std::size_t)
            {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                self->deadline_.cancel();
            });
    }

    // Check whether we have spent enough time on this connection.
    void check_deadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
            [self](beast::error_code ec)
            {
                if (!ec)
                {
                    // Close socket to cancel any outstanding operation.
                    self->socket_.close(ec);
                }
            });
    }

    void GetWord(std::string& s)
    {
        std::vector<std::string> word_list;

        std::size_t delimiter_pos = s.find('=');
        std::string query = s.substr(0, delimiter_pos);
        std::string words = s.substr(delimiter_pos + 1);

        int count_plus = 0; 
        if (std::count(words.begin(), words.end(), '+') == 0)
        {
            s = "<h1>Enter your search query! The query must contain at least one word!</h>\n";
        }
        if (std::count(words.begin(), words.end(), '+') < 3)
        {
            count_plus = std::count(words.begin(), words.end(), '+');
        }
        else
        {
            count_plus = 3;
        }
        
        for (int i = 0; i <= count_plus; i++)
        {
            if (i == 0)
            {
                delimiter_pos = words.find('+');
                word_list.push_back(words.substr(0, delimiter_pos));
            }
            else
            {
                words = words.substr(delimiter_pos + 1);
                delimiter_pos = words.find('+');
                word_list.push_back(words.substr(0, delimiter_pos));
            }
        }
        std::cout << "count_plus = " << count_plus << std::endl;
        std::cout << query << " words: ";
        for (int i = 0; i < word_list.size(); i++)
        {
            std::cout << i + 1 << " - " << word_list[i] << " | ";
        }
        std::cout << std::endl;
    }
};

// "Loop" forever accepting new connections.
void http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
    acceptor.async_accept(socket,
        [&](beast::error_code ec)
        {
            if (!ec)
                std::make_shared<http_connection>(std::move(socket))->start();
            http_server(acceptor, socket);
        });
}
