#include "Sender.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;

Sender::Sender(boost::asio::io_service &io_service)
        : stopped_(false),
          socket_(io_service),
          deadline_(io_service),
          heartbeat_timer_(io_service) {
    // Empty
};

Sender::~Sender() {
    stop();
}

// Initiate the connection process.
// The endpoint iterator will have been obtained using a tcp::resolver.
void Sender::start(tcp::resolver::iterator endpoint_iter) {
    // Start the connect actor.
    start_connect(endpoint_iter);

    // Start the deadline actor. You will note that we're not setting any
    // particular deadline here. Instead, the connect and input actors will
    // update the deadline prior to each asynchronous operation.
    deadline_.async_wait(boost::bind(&Sender::check_deadline, this));
}

// Terminate all the actors to shut down the connection.
// It may be called by the user of the client class, or by the class itself in
// response to graceful termination or an unrecoverable error.
void Sender::stop() {
    stopped_ = true;
    boost::system::error_code ignored_ec;
    socket_.close(ignored_ec);
    deadline_.cancel();
    heartbeat_timer_.cancel();
}

void Sender::start_connect(tcp::resolver::iterator endpoint_iter) {
    if (endpoint_iter != tcp::resolver::iterator()) {
        std::cout << "Trying " << endpoint_iter->endpoint() << "...\n";

        // Set a deadline for the connect operation.
        deadline_.expires_from_now(boost::posix_time::seconds(60));

        // Start the asynchronous connect operation.
        socket_.async_connect(endpoint_iter->endpoint(),
                              boost::bind(&Sender::handle_connect, this, _1, endpoint_iter));
    } else { // There are no more endpoints to try. Shut down the client.
        stop();
    }
}

void Sender::check_deadline() {
    if (stopped_) {
        return;
    }

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline_.expires_at() <= deadline_timer::traits_type::now()) {
        // The deadline has passed. The socket is closed so that any outstanding
        // asynchronous operations are cancelled.
        socket_.close();

        // There is no longer an active deadline. The expiry is set to positive
        // infinity so that the actor takes no action until a new deadline is set.
        deadline_.expires_at(boost::posix_time::pos_infin);
    }

    // Put the actor back to sleep.
    deadline_.async_wait(boost::bind(&Sender::check_deadline, this));
}

void Sender::handle_connect(const boost::system::error_code &error, tcp::resolver::iterator endpoint_iter) {
    if (stopped_) {
        return;
    }

    // The async_connect() function automatically opens the socket at the start
    // of the asynchronous operation. If the socket is closed at this time then
    // the timeout handler must have run first.
    if (!socket_.is_open()) {
        std::cout << "Connect timed out\n";

        // Try the next available endpoint.
        start_connect(++endpoint_iter);
    } else if (error) { // Check if the connect operation failed before the deadline expired.
        std::cout << "Connect error: " << error.message() << "\n";

        // We need to close the socket used in the previous connection attempt
        // before starting a new one.
        socket_.close();

        // Try the next available endpoint.
        start_connect(++endpoint_iter);
    } else {  // Otherwise we have successfully established a connection.
        std::cout << "Connected to " << endpoint_iter->endpoint() << "\n";

        // Start the input actor.
        start_read();

        // Start the heartbeat actor.
        start_write();
    }
}

void Sender::start_read() {
    // Set a deadline for the read operation.
    deadline_.expires_from_now(boost::posix_time::seconds(30));

    // Start an asynchronous operation to read a newline-delimited message.
    boost::asio::async_read_until(socket_, input_buffer_, '\n',
                                  [this](boost::system::error_code error, std::size_t /*length*/) {
                                      handle_read(error);
                                  });
}

void Sender::handle_read(const boost::system::error_code &error) {
    if (stopped_) {
        return;
    }

    if (!error) {
        // Extract the newline-delimited message from the buffer.
        std::string line;
        std::istream is(&input_buffer_);
        std::getline(is, line);

        // Empty messages are heartbeats and so ignored.
        if (!line.empty()) {
            std::cout << "Received: " << line << "\n";
        }

        start_read();
    } else {
        std::cout << "Error on receive: " << error.message() << "\n";
        stop();
    }
}

void Sender::start_write() {
    if (stopped_) {
        return;
    }

    // Start an asynchronous operation to send a heartbeat message.
    boost::asio::async_write(socket_, boost::asio::buffer("\n", 1),
                             [this](boost::system::error_code error, std::size_t /*length*/) {
                                 handle_write(error);
                             });
}

void Sender::handle_write(const boost::system::error_code &error) {
    if (stopped_) {
        return;
    }

    if (!error) {
        // Wait 10 seconds before sending the next heartbeat.
        heartbeat_timer_.expires_from_now(boost::posix_time::seconds(10));
        heartbeat_timer_.async_wait(boost::bind(&Sender::start_write, this));
    } else {
        std::cout << "Error on heartbeat: " << error.message() << "\n";
        stop();
    }
}

std::unique_ptr<Sender> Sender::create(std::string host, std::string port) {
    try {
        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        Sender *client = new Sender(io_service);

        client->start(resolver.resolve(tcp::resolver::query(host, port)));

        io_service.run();

        return std::unique_ptr<Sender>(client);
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
        BOOST_THROW_EXCEPTION(e);
    }
}