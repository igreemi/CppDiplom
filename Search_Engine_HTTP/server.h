#pragma once
//#pragma execution_character_set("utf-8")

#include <pqxx/pqxx>
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

#include "spider_read_config.h"

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
	http_connection(tcp::socket socket, SpiderReadConfig& config)
		: socket_(std::move(socket))
	{
		connect_db = new pqxx::connection
		("host = " + config.db_host +
			" port = " + config.db_port +
			" dbname = " + config.db_name +
			" user = " + config.db_user +
			" password = " + config.db_password);

		txn = new pqxx::work(*connect_db);

		connect_db->prepare("select_word", "SELECT pages.url FROM pages "
			"JOIN page_words ON pages.id = page_words.page_id "
			"JOIN words ON page_words.word_id = words.id "
			"WHERE words.word IN ($1, $2, $3, $4) "
			"GROUP BY pages.url "
			"ORDER BY SUM(page_words.count) DESC;");
	}

	// Initiate the asynchronous operations associated with the connection.
	void start()
	{
		read_request();
		check_deadline();
	}


private:

	pqxx::connection* connect_db = nullptr;
	pqxx::work* txn = nullptr;

	std::string _host = "";
	std::string _port = "";
	std::string _name_db = "";
	std::string _user = "";
	std::string _pass = "";

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
		std::string words;
		bool word_empty = false;
		std::vector<std::string> words_list = { "", "", "", "" };

		switch (request_.method())
		{
		case http::verb::get:
			response_.result(http::status::ok);
			response_.set(http::field::server, "Beast");
			create_response();
			break;

		case http::verb::post:
			request_.set(http::field::accept_encoding, "UTF-8");
			request_.set(http::field::accept_language, "ru");
			request_.set(http::field::content_type, "text/html;charset=UTF-8");
			words = beast::buffers_to_string(request_.body().data());

			std::cout << words << std::endl;

			std::cout << "--- ЧТО ТО НА КИРИЛЛИЦЕ ---" << std::endl;

			response_.set(http::field::content_type, "text/html;charset=UTF-8");

			parse_words(words, words_list, word_empty);
			if (!word_empty)
			{
				words = search_url_in_db(words_list);
			}

			beast::ostream(response_.body())
				<< "<!DOCTYPE HTML>\n"
				<< "<html lang=\"ru\">\n"
				<< "<head>\n"
				<< "<meta charset=\"utf-8\">\n"
				<< "<title>Search results:</title>\n"
				<< "</head>\n"
				<< "<body>\n"
				<< "<center>\n"
				<< "<h1>ПОИСКОВИК</h1>\n"
				<< "<form action=\"/\" method=\"post\" accept-charset=\"UTF-8\">\n"
				<< "<input type=\"text\" name=\"query\" accept-charset=\"UTF-8\">\n"
				<< "<input type=\"submit\" value=\"ПОИСК\">\n"
				<< "</form>\n"
				<< "<h1>Результаты поиска:</h1>\n"
				<< "<h3>\n"
				<< "<left>\n"
				<< words
				<< "</left>\n"
				<< "</h3>\n"
				<< "</center>\n"
				<< "</body>\n"
				<< "</html>\n";
			break;

		default:
			// We return responses indicating an error if
			// we do not recognize the request method.
			response_.result(http::status::bad_request);
			response_.set(http::field::content_type, "text/plain;charset=UTF-8");
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
			response_.set(http::field::content_type, "text/html;charset=UTF-8");
			beast::ostream(response_.body())
				<< "<!DOCTYPE HTML>\n"
				<< "<html lang=\"ru\">\n"
				<< "<head>\n"
				<< "<meta charset=\"utf-8\">\n"
				<< "<title>SEARCH ENGINE</title>\n"
				<< "</head>\n"
				<< "<body>\n"
				<< "<center>\n"
				<< "<h1>ПОИСКОВИК</h1>\n"
				<< "<form action=\"/\" method=\"post\" accept-charset=\"UTF-8\">\n"
				<< "<input type=\"text\" name=\"query\" accept-charset=\"UTF-8\">\n"
				<< "<input type=\"submit\" value=\"ПОИСК\">\n"
				<< "</form>\n"
				<< "</center>\n"
				<< "</body>\n"
				<< "</html>\n";
		}
		else
		{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/plain;charset=UTF-8");
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

	void parse_words(std::string& words, std::vector<std::string>& words_list, bool& word_empty)
	{
		std::size_t delimiter_pos = words.find('=');
		std::string query = words.substr(0, delimiter_pos);
		std::string word = words.substr(delimiter_pos + 1);

		if (word.size() == 0)
		{
			words = "<h2><p>Запрос должен содержать хотя бы одно слово!</p></h>\n";
			word_empty = true;
			return;
		}

		int count_plus = 0;

		if (std::count(word.begin(), word.end(), '+') < 3)
		{
			count_plus = std::count(word.begin(), word.end(), '+');
		}
		else
		{
			count_plus = 3;
		}

		for (int i = 0; i <= count_plus; i++)
		{
			if (i == 0)
			{
				delimiter_pos = word.find('+');
				words_list[i] = word.substr(0, delimiter_pos);
			}
			else
			{
				word = word.substr(delimiter_pos + 1);
				delimiter_pos = word.find('+');
				words_list[i] = word.substr(0, delimiter_pos);
			}
		}
	}

	std::string search_url_in_db(std::vector<std::string> words)
	{
		std::vector<std::string> tmp_urls;

		pqxx::result result = txn->exec_prepared("select_word", words[0], words[1], words[2], words[3]);

		for (auto const& row : result)
		{
			for (auto const& field : row)
			{
				tmp_urls.push_back(field.c_str());
			}
		}
		return send_url_to_html(tmp_urls);
	}

	std::string send_url_to_html(std::vector<std::string> url_to_html)
	{
		std::string tmp_url_list;

		if (url_to_html.size() == 0)
		{
			return "<h2><p>По Вашему запросу ничего не найдено!</p></h>\n";
		}
		else
		{
			for (int i = 0; i < url_to_html.size(); i++)
			{
				tmp_url_list += "<p><a href =\"https://" + url_to_html[i] + "\" target=\"_blank\">" + std::to_string(i + 1) + ") " + url_to_html[i] + "</a></p>\n";
			}
			return tmp_url_list;
		}
	}
};

// "Loop" forever accepting new connections.
void http_server(tcp::acceptor& acceptor, tcp::socket& socket, SpiderReadConfig& config)
{
	acceptor.async_accept(socket,
		[&](beast::error_code ec)
		{
			if (!ec)
			{
				std::make_shared<http_connection>(std::move(socket), config)->start();
			}
			http_server(acceptor, socket, config);
		});
}
