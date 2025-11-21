#include "http_utils.h"
#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

std::string getHtmlContent(const Link& link) {
    std::string result;

    try {
        std::string host = link.hostName;
        std::string port = (link.protocol == ProtocolType::HTTPS) ? "443" : "80";
        std::string target = link.query.empty() ? "/" : link.query;

        net::io_context ioc;

        if (link.protocol == ProtocolType::HTTPS) {
            std::cout << "⚠️  HTTPS not supported in this version, skipping: " << host << target << std::endl;
            return "";
        }

        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::accept, "text/html");

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        result = boost::beast::buffers_to_string(res.body().data());

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (ec && ec != beast::errc::not_connected) {
            throw beast::system_error{ec};
        }

    } catch (const std::exception& e) {
        std::cout << "❌ Error fetching " << link.hostName << link.query << ": " << e.what() << std::endl;
    }

    return result;
}