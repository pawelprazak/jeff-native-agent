#ifndef JEFF_NATIVE_AGENT_SENDER_H
#define JEFF_NATIVE_AGENT_SENDER_H

#include <queue>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/thread/thread.hpp>

//
// This class manages socket timeouts by applying the concept of a deadline.
// Some asynchronous operations are given deadlines by which they must complete.
// Deadlines are enforced by an "actor" that persists for the lifetime of the
// client object:
//
//  +----------------+
//  |                |
//  | check_deadline |<---+
//  |                |    |
//  +----------------+    | async_wait()
//              |         |
//              +---------+
//
// If the deadline actor determines that the deadline has expired, the socket
// is closed and any outstanding operations are consequently cancelled.
//
// Connection establishment involves trying each endpoint in turn until a
// connection is successful, or the available endpoints are exhausted. If the
// deadline actor closes the socket, the connect actor is woken up and moves to
// the next endpoint.
//
//  +---------------+
//  |               |
//  | start_connect |<---+
//  |               |    |
//  +---------------+    |
//           |           |
//  async_-  |    +----------------+
// connect() |    |                |
//           +--->| handle_connect |
//                |                |
//                +----------------+
//                          :
// Once a connection is     :
// made, the connect        :
// actor forks in two -     :
//                          :
// an actor for reading     :       and an actor for
// inbound messages:        :       sending heartbeats:
//                          :
//  +------------+          :          +-------------+
//  |            |<- - - - -+- - - - ->|             |
//  | start_read |                     | start_write |<---+
//  |            |<---+                |             |    |
//  +------------+    |                +-------------+    | async_wait()
//          |         |                        |          |
//  async_- |    +-------------+       async_- |    +--------------+
//   read_- |    |             |       write() |    |              |
//  until() +--->| handle_read |               +--->| handle_write |
//               |             |                    |              |
//               +-------------+                    +--------------+
//
// The input actor reads messages from the socket, where messages are delimited
// by the newline character. The deadline for a complete message is 30 seconds.
//
// The heartbeat actor sends a heartbeat (a message that consists of a single
// newline character) every 10 seconds. In this example, no deadline is applied
// message sending.
//
class Sender {
public:
    Sender();

    virtual ~Sender();

    void start(boost::asio::ip::tcp::resolver::query query);

    // Initiate the connection process.
    // The endpoint iterator will have been obtained using a tcp::resolver.
    void start(boost::asio::ip::tcp::resolver::iterator endpoint_iter);

    // Terminate all the actors to shut down the connection.
    // It may be called by the user of the client class, or by the class itself in
    // response to graceful termination or an unrecoverable error.
    void stop();

    // Queues a message to be send
    void send(std::string value);

    // Flushes all unsent messages
    void flush();

    // Create the client and run the event loop
    static std::unique_ptr<Sender> create(std::string host, std::string port);

private:
    void start_connect(boost::asio::ip::tcp::resolver::iterator endpoint_iter);

    void check_deadline();

    void handle_connect(const boost::system::error_code &error, boost::asio::ip::tcp::resolver::iterator endpoint_iter);

    void start_read();

    void start_write();

    void handle_read(const boost::system::error_code &error);

    void handle_write(const boost::system::error_code &error, long delay_in_seconds = 10);

private:
    bool stopped_;
    std::queue<std::string, std::list<std::string>> queue;

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket;
    boost::asio::streambuf input_buffer;
    boost::asio::deadline_timer deadline;
    boost::asio::deadline_timer heartbeat_timer;

    boost::thread_group worker_threads;
};


#endif //JEFF_NATIVE_AGENT_SENDER_H
