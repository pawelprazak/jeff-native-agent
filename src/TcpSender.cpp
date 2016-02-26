#include "TcpSender.hpp"

#include <boost/asio.hpp>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;

TcpSender::TcpSender(boost::asio::ip::tcp::resolver::query endpoint)
        : Sender(),
          stopped_(false),
          endpoint(endpoint),
          socket(io_service),
          deadline(io_service),
          heartbeat_timer(io_service) {
    // Empty
};

TcpSender::~TcpSender() {
    stop();
}

// Initiate the connection process.
// The endpoint iterator will have been obtained using a tcp::resolver.
void TcpSender::start() {
    tcp::resolver resolver(io_service);
    tcp::resolver::iterator endpoint_iter = resolver.resolve(endpoint);

    // Start the connect actor.
    start_connect(endpoint_iter);

    // Start the deadline actor. You will note that we're not setting any
    // particular deadline here. Instead, the connect and input actors will
    // update the deadline prior to each asynchronous operation.
    deadline.async_wait(boost::bind(&TcpSender::check_deadline, this));

    // Run the service loop on a thread pool
    worker_threads.create_thread(
            boost::bind(&boost::asio::io_service::run, &io_service)
    );
}

// Terminate all the actors to shut down the connection.
// It may be called by the user of the client class, or by the class itself in
// response to graceful termination or an unrecoverable error.
void TcpSender::stop() {
    stopped_ = true;
    boost::system::error_code ignored_ec;
    socket.close(ignored_ec);
    deadline.cancel();
    heartbeat_timer.cancel();
    worker_threads.join_all();
}

void TcpSender::start_connect(tcp::resolver::iterator endpoint) {
    if (endpoint != tcp::resolver::iterator()) {
        std::cout << "Trying " << endpoint->endpoint() << "...\n";

        // Set a deadline for the connect operation.
        deadline.expires_from_now(boost::posix_time::seconds(60));

        // Start the asynchronous connect operation.
        socket.async_connect(endpoint->endpoint(),
                             boost::bind(&TcpSender::handle_connect, this, _1, endpoint));
    } else { // There are no more endpoints to try. Shut down the client.
        stop();
    }
}

void TcpSender::check_deadline() {
    if (stopped_) {
        return;
    }

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline.expires_at() <= deadline_timer::traits_type::now()) {
        // The deadline has passed. The socket is closed so that any outstanding
        // asynchronous operations are cancelled.
        socket.close();

        // There is no longer an active deadline. The expiry is set to positive
        // infinity so that the actor takes no action until a new deadline is set.
        deadline.expires_at(boost::posix_time::pos_infin);
    }

    // Put the actor back to sleep.
    deadline.async_wait(boost::bind(&TcpSender::check_deadline, this));
}

void TcpSender::handle_connect(const boost::system::error_code &error, tcp::resolver::iterator endpoint_iter) {
    if (stopped_) {
        return;
    }

    // The async_connect() function automatically opens the socket at the start
    // of the asynchronous operation. If the socket is closed at this time then
    // the timeout handler must have run first.
    if (!socket.is_open()) {
        std::cout << "Connect timed out\n";

        // Try the next available endpoint.
        start_connect(++endpoint_iter);
    } else if (error) { // Check if the connect operation failed before the deadline expired.
        std::cout << "Connect error: " << error.message() << "\n";

        // We need to close the socket used in the previous connection attempt
        // before starting a new one.
        socket.close();

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

void TcpSender::start_read() {
    // Set a deadline for the read operation.
    deadline.expires_from_now(boost::posix_time::seconds(30));

    // Start an asynchronous operation to read a newline-delimited message.
    boost::asio::async_read_until(socket, input_buffer, '\n',
                                  [this](boost::system::error_code error, std::size_t /*length*/) {
                                      handle_read(error);
                                  });
}

void TcpSender::handle_read(const boost::system::error_code &error) {
    if (stopped_) {
        return;
    }

    if (!error) {
        // Extract the newline-delimited message from the buffer.
        std::string line;
        std::istream is(&input_buffer);
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

void TcpSender::start_write() {
    if (stopped_) {
        return;
    }

    std::string &message = queue.front();
    queue.pop();

    // Start an asynchronous operation to send a heartbeat message.
    boost::asio::async_write(socket, boost::asio::buffer(message.c_str(), message.size()),
                             [this](boost::system::error_code error, std::size_t /*length*/) {
                                 handle_write(error);
                             });

    std::cout << "Message was sent\n";
}

void TcpSender::handle_write(const boost::system::error_code &error, long delay_in_seconds) {
    if (stopped_) {
        return;
    }

    if (!queue.empty()) {
        start_write();
    } else {
        std::cout << "Message queue is empty\n";
    }

    if (!error) {
        // Wait 10 seconds before sending the next messages.
        heartbeat_timer.expires_from_now(boost::posix_time::seconds(delay_in_seconds));
        heartbeat_timer.async_wait(boost::bind(&TcpSender::start_write, this));
    } else {
        std::cout << "Error on send: " << error.message() << "\n";
        stop();
    }
}

void TcpSender::send(std::string value) {
    queue.push(value);
    std::cout << "Message added, queue has " << queue.size() << " messages\n";
}

void TcpSender::flush() {
    const boost::system::error_code error;
    while (!queue.empty()) {
        handle_write(error, 0);
    }
    std::cout << "Messages flushed, queue has " << queue.size() << " messages\n";
    if (error) {
        std::cerr << "could not flush: " << boost::system::system_error(error).what() << std::endl;
    }
}