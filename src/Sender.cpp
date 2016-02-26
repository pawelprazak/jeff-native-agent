#include "Sender.hpp"

#include <boost/throw_exception.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "StdSender.hpp"
#include "TcpSender.hpp"

std::unique_ptr<Sender> Sender::create() {
    Sender *ret = new StdSender();
    return std::unique_ptr<Sender>(ret);
}

std::unique_ptr<Sender> Sender::create(std::string host, std::string port) {
    try {
        auto query = boost::asio::ip::tcp::resolver::query(host, port);
        Sender *client = new TcpSender(query);
        return std::unique_ptr<Sender>(client);
    } catch (std::exception &e) {
        BOOST_THROW_EXCEPTION(e);
    }
}