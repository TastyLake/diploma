#include "http_connection.h"
#include "database.h"
#include "config.h"
#include <sstream>
#include <iomanip>
#include <iostream>

std::string url_decode(const std::string& encoded) {
    std::string res;
    std::istringstream iss(encoded);
    char ch;

    while (iss.get(ch)) {
        if (ch == '%') {
            int hex;
            if (iss >> std::hex >> hex) {
                res += static_cast<char>(hex);
            }
        }
        else if (ch == '+') {
            res += ' ';
        }
        else {
            res += ch;
        }
    }

    return res;
}

std::string convert_to_utf8(const std::string& str) {
    std::string url_decoded = url_decode(str);
    return url_decoded;
}

HttpConnection::HttpConnection(tcp::socket socket)
    : socket_(std::move(socket))
{
}

void HttpConnection::start()
{
    readRequest();
    checkDeadline();
}

void HttpConnection::readRequest()
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
                self->processRequest();
        });
}

void HttpConnection::processRequest()
{
    response_.version(request_.version());
    response_.keep_alive(false);

    switch (request_.method())
    {
    case http::verb::get:
        response_.result(http::status::ok);
        response_.set(http::field::server, "SearchEngine");
        createResponseGet();
        break;
    case http::verb::post:
        response_.result(http::status::ok);
        response_.set(http::field::server, "SearchEngine");
        createResponsePost();
        break;

    default:
        response_.result(http::status::bad_request);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body())
            << "Invalid request-method '"
            << std::string(request_.method_string())
            << "'";
        break;
    }

    writeResponse();
}

void HttpConnection::createResponseGet()
{
    if (request_.target() == "/")
    {
        response_.set(http::field::content_type, "text/html; charset=utf-8");
        beast::ostream(response_.body())
            << "<!DOCTYPE html>"
            << "<html>"
            << "<head>"
            << "<meta charset=\"UTF-8\">"
            << "<title>Search Engine</title>"
            << "<style>"
            << "body { font-family: Arial, sans-serif; margin: 40px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }"
            << ".container { max-width: 800px; margin: 0 auto; background: white; padding: 40px; border-radius: 15px; box-shadow: 0 10px 30px rgba(0,0,0,0.2); }"
            << "h1 { color: #333; text-align: center; margin-bottom: 30px; font-size: 2.5em; }"
            << ".subtitle { text-align: center; color: #666; margin-bottom: 40px; font-size: 1.2em; }"
            << "form { text-align: center; }"
            << "input[type=text] { padding: 15px; width: 500px; font-size: 16px; border: 2px solid #ddd; border-radius: 25px; outline: none; transition: border-color 0.3s; }"
            << "input[type=text]:focus { border-color: #667eea; }"
            << "input[type=submit] { padding: 15px 30px; font-size: 16px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; border-radius: 25px; cursor: pointer; margin-left: 10px; transition: transform 0.2s; }"
            << "input[type=submit]:hover { transform: translateY(-2px); }"
            << ".features { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin: 40px 0; }"
            << ".feature { text-align: center; padding: 20px; background: #f8f9fa; border-radius: 10px; }"
            << ".feature h3 { color: #333; margin-bottom: 10px; }"
            << ".feature p { color: #666; font-size: 0.9em; }"
            << "</style>"
            << "</head>"
            << "<body>"
            << "<div class=\"container\">"
            << "<h1>рџ”Ќ Search Engine</h1>"
            << "<div class=\"subtitle\">Search through indexed web pages with intelligent ranking</div>"
            << "<form action=\"/\" method=\"post\">"
            << "<input type=\"text\" id=\"search\" name=\"search\" placeholder=\"Enter your search query...\">"
            << "<input type=\"submit\" value=\"Search\">"
            << "</form>"
            << "<div class=\"features\">"
            << "<div class=\"feature\">"
            << "<h3>рџ“Љ Real Database</h3>"
            << "<p>Powered by PostgreSQL with full-text indexing</p>"
            << "</div>"
            << "<div class=\"feature\">"
            << "<h3>рџЋЇ Smart Ranking</h3>"
            << "<p>Results sorted by relevance score</p>"
            << "</div>"
            << "<div class=\"feature\">"
            << "<h3>рџЊђ Web Crawling</h3>"
            << "<p>Automatically indexes web pages</p>"
            << "</div>"
            << "</div>"
            << "</div>"
            << "</body>"
            << "</html>";
    }
    else
    {
        response_.result(http::status::not_found);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body()) << "File not found\r\n";
    }
}

void HttpConnection::createResponsePost()
{
    if (request_.target() == "/")
    {
        std::string body = beast::buffers_to_string(request_.body().data());
        std::cout << "рџ”Ќ Search request: " << body << std::endl;

        size_t pos = body.find('=');
        if (pos == std::string::npos)
        {
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "Invalid request format\r\n";
            return;
        }

        std::string key = body.substr(0, pos);
        std::string value = body.substr(pos + 1);

        std::string searchQuery = convert_to_utf8(value);

        if (key != "search")
        {
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "Invalid search parameter\r\n";
            return;
        }

        try {
            Config& config = Config::getInstance();
            std::string dbConnection =
                "host=" + config.getString("database", "host") +
                " port=" + std::to_string(config.getInt("database", "port")) +
                " dbname=" + config.getString("database", "name") +
                " user=" + config.getString("database", "username") +
                " password=" + config.getString("database", "password");

            SearchDatabase db(dbConnection);
            auto searchResults = db.search(searchQuery);

            response_.set(http::field::content_type, "text/html; charset=utf-8");

            beast::ostream(response_.body())
                << "<!DOCTYPE html>"
                << "<html>"
                << "<head>"
                << "<meta charset=\"UTF-8\">"
                << "<title>Search Results</title>"
                << "<style>"
                << "body { font-family: Arial, sans-serif; margin: 40px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }"
                << ".container { max-width: 900px; margin: 0 auto; background: white; padding: 40px; border-radius: 15px; box-shadow: 0 10px 30px rgba(0,0,0,0.2); }"
                << "h1 { color: #333; margin-bottom: 10px; }"
                << ".back-link { display: inline-block; margin-bottom: 30px; color: #667eea; text-decoration: none; font-weight: bold; }"
                << ".back-link:hover { text-decoration: underline; }"
                << ".query { color: #666; margin-bottom: 30px; font-size: 1.1em; }"
                << ".results-count { color: #28a745; margin-bottom: 20px; font-weight: bold; }"
                << ".result-item { background: #f8f9fa; padding: 20px; margin: 15px 0; border-radius: 10px; border-left: 4px solid #667eea; transition: transform 0.2s; }"
                << ".result-item:hover { transform: translateX(5px); background: #e9ecef; }"
                << ".result-title { margin: 0 0 8px 0; }"
                << ".result-title a { color: #1a0dab; text-decoration: none; font-size: 1.2em; font-weight: bold; }"
                << ".result-title a:hover { text-decoration: underline; }"
                << ".result-url { color: #006621; font-size: 0.9em; margin: 0 0 8px 0; }"
                << ".result-relevance { color: #70757a; font-size: 0.8em; }"
                << ".no-results { text-align: center; padding: 40px; color: #666; }"
                << "form { margin: 30px 0; }"
                << "input[type=text] { padding: 12px; width: 400px; font-size: 16px; border: 2px solid #ddd; border-radius: 20px; outline: none; }"
                << "input[type=submit] { padding: 12px 25px; font-size: 16px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; border-radius: 20px; cursor: pointer; margin-left: 10px; }"
                << "</style>"
                << "</head>"
                << "<body>"
                << "<div class=\"container\">"
                << "<a href=\"/\" class=\"back-link\">в†ђ Back to search</a>"
                << "<h1>Search Results</h1>"
                << "<div class=\"query\">Query: <strong>" << searchQuery << "</strong></div>";

            if (searchResults.empty()) {
                beast::ostream(response_.body())
                    << "<div class=\"no-results\">"
                    << "<h3>рџ”Ќ No results found</h3>"
                    << "<p>Try different keywords or check the spelling</p>"
                    << "</div>";
            } else {
                beast::ostream(response_.body())
                    << "<div class=\"results-count\">Found " << searchResults.size() << " results</div>";

                for (const auto& result : searchResults) {
                    beast::ostream(response_.body())
                        << "<div class=\"result-item\">"
                        << "<h3 class=\"result-title\"><a href=\"" << result.url << "\" target=\"_blank\">"
                        << (result.title.empty() ? result.url : result.title)
                        << "</a></h3>"
                        << "<div class=\"result-url\">" << result.url << "</div>"
                        << "<div class=\"result-relevance\">Relevance score: " << result.relevance << "</div>"
                        << "</div>";
                }
            }

            beast::ostream(response_.body())
                << "<form action=\"/\" method=\"post\">"
                << "<input type=\"text\" name=\"search\" value=\"" << searchQuery << "\" placeholder=\"Enter your search query...\">"
                << "<input type=\"submit\" value=\"Search Again\">"
                << "</form>"
                << "</div>"
                << "</body>"
                << "</html>";
        } catch (const std::exception& e) {
            response_.result(http::status::internal_server_error);
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                << "<html>"
                << "<head><title>Error</title></head>"
                << "<body>"
                << "<h1>Internal Server Error</h1>"
                << "<p>" << e.what() << "</p>"
                << "<a href=\"/\">Back to search</a>"
                << "</body>"
                << "</html>";
        }
    }
    else
    {
        response_.result(http::status::not_found);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body()) << "File not found\r\n";
    }
}

void HttpConnection::writeResponse()
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

void HttpConnection::checkDeadline()
{
    auto self = shared_from_this();

    deadline_.async_wait(
        [self](beast::error_code ec)
        {
            if (!ec)
            {
                self->socket_.close(ec);
            }
        });
}
